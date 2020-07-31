﻿//
//  export texture atlas.cpp
//  Animera
//
//  Created by Indiana Kernick on 15/7/20.
//  Copyright © 2020 Indiana Kernick. All rights reserved.
//

#include "export texture atlas.hpp"

#include "animation.hpp"
#include "composite.hpp"
#include "atlas generator.hpp"
#include "surface factory.hpp"
#include "graphics convert.hpp"
#include <Graphics/transform.hpp>

namespace {

bool includeLayer(const LayerVis vis, const bool visible) {
  switch (vis) {
    case LayerVis::visible: return visible;
    case LayerVis::hidden: return !visible;
    case LayerVis::all: return true;
  }
}

template <typename Idx>
void checkNegative(Idx &idx, const Idx count) {
  if (idx < Idx{0}) idx += count;
}

template <typename Func>
Error eachFrame(const AnimExportParams &params, const Animation &anim, Func func) {
  const tcb::span<const Layer> layers = anim.timeline.getLayerArray();
  LayerRange layerRange = params.layers;
  FrameRange frameRange = params.frames;
  
  checkNegative(layerRange.min, anim.timeline.getLayers());
  checkNegative(layerRange.max, anim.timeline.getLayers());
  checkNegative(frameRange.min, anim.timeline.getFrames());
  checkNegative(frameRange.max, anim.timeline.getFrames());
  
  const LayerIdx layerCount = layerRange.max - layerRange.min + LayerIdx{1};
  
  Frame frame;
  frame.reserve(+layerCount);
  
  std::vector<CelIterator> celIters;
  celIters.reserve(+layerCount);
  for (LayerIdx l = layerRange.min; l <= layerRange.max; ++l) {
    celIters.emplace_back(layers[+l].cels, frameRange.min);
  }
  GroupIterator groupIter{anim.timeline.getGroupArray(), frameRange.min};
  bool changedGroup = true;
  
  SpriteNameState state;
  state.layer = layerRange.min;
  state.layerCount = LayerIdx{1};
  state.groupCount = anim.timeline.getGroups();
  state.layerName = layers[+layerRange.min].name;
  
  for (FrameIdx f = frameRange.min; f <= frameRange.max; ++f) {
    frame.clear();
    for (LayerIdx l = {}; l != layerCount; ++l) {
      if (!includeLayer(layerRange.vis, layers[+(l + layerRange.min)].visible)) continue;
      if (const CelImage *cel = celIters[+l].img(); *cel) {
        frame.push_back(cel);
      }
      celIters[+l].incr();
    }
    
    if (changedGroup) {
      const GroupInfo info = groupIter.info();
      state.group = info.group;
      state.frameCount = info.end - info.begin;
      state.groupBegin = info.begin;
      state.groupName = groupIter.name();
    }
    state.frame = f;
    changedGroup = groupIter.incr();
    
    TRY(func(frame, state));
  }
  
  return {};
}

template <typename Func>
Error eachCel(const AnimExportParams &params, const Animation &anim, Func func) {
  const tcb::span<const Layer> layers = anim.timeline.getLayerArray();
  LayerRange layerRange = params.layers;
  FrameRange frameRange = params.frames;
  
  SpriteNameState state;
  state.layerCount = anim.timeline.getLayers();
  state.groupCount = anim.timeline.getGroups();
  
  checkNegative(layerRange.min, anim.timeline.getLayers());
  checkNegative(layerRange.max, anim.timeline.getLayers());
  checkNegative(frameRange.min, anim.timeline.getFrames());
  checkNegative(frameRange.max, anim.timeline.getFrames());
  
  for (LayerIdx l = layerRange.min; l <= layerRange.max; ++l) {
    const Layer &layer = layers[+l];
    if (!includeLayer(layerRange.vis, layer.visible)) continue;
    CelIterator celIter{layer.cels, frameRange.min};
    GroupIterator groupIter{anim.timeline.getGroupArray(), frameRange.min};
    bool changedGroup = true;
    
    state.layer = l;
    state.layerName = layer.name;
    
    for (FrameIdx f = frameRange.min; f <= frameRange.max; ++f) {
      if (changedGroup) {
        const GroupInfo info = groupIter.info();
        state.group = info.group;
        state.frameCount = info.end - info.begin;
        state.groupBegin = info.begin;
        state.groupName = groupIter.name();
      }
      
      state.frame = f;
      TRY(func(celIter.img(), state));
      
      celIter.incr();
      changedGroup = groupIter.incr();
    }
  }
  
  return {};
}

struct Images {
  QImage canvas;
  QImage xformed;
};

bool isIdentity(const SpriteTransform &transform) {
  return transform.scaleX == 1 && transform.scaleY == 1 && transform.angle == 0;
}

QSize getTransformedSize(const QSize canvasSize, const SpriteTransform &transform) {
  QSize size;
  size.setWidth(canvasSize.width() * std::abs(transform.scaleX));
  size.setHeight(canvasSize.height() * std::abs(transform.scaleY));
  return convert(gfx::rotateSize(convert(size), transform.angle));
}

void initImages(Images &images, const AnimExportParams &params, const Animation &anim) {
  Format imageFormat = anim.getFormat();
  if (params.composite && anim.getFormat() != Format::gray) {
    imageFormat = Format::rgba;
  }
  images.canvas = {anim.getSize(), qimageFormat(imageFormat)};
  if (isIdentity(params.transform)) {
    images.xformed = {};
  } else {
    const QSize xformedSize = getTransformedSize(images.canvas.size(), params.transform);
    images.xformed = {xformedSize, images.canvas.format()};
  }
}

void applyTransform(Images &images, const SpriteTransform &transform) {
  visitSurface(images.xformed, [&](const auto dst) {
    const auto src = makeCSurface<typename decltype(dst)::Pixel>(images.canvas);
    gfx::spatialTransform(dst, src, [&](const gfx::Point dstPos) {
      gfx::Point srcPos = gfx::rotate(transform.angle, dst.size(), dstPos);
      srcPos = transform.scaleX < 0 ? gfx::flipHori(dst.size(), srcPos) : srcPos;
      srcPos = transform.scaleY < 0 ? gfx::flipVert(dst.size(), srcPos) : srcPos;
      return gfx::scale({std::abs(transform.scaleX), std::abs(transform.scaleY)}, srcPos);
    });
  });
}

Error addImage(
  const std::size_t index,
  const ExportParams &params,
  const AnimExportParams &animParams,
  Images &images
) {
  if (images.xformed.isNull()) {
    return params.generator->copyImage(index, images.canvas);
  } else {
    applyTransform(images, animParams.transform);
    return params.generator->copyImage(index, images.xformed);
  }
}

void addFrameNames(
  std::size_t &index,
  const ExportParams &params,
  const AnimExportParams &animParams,
  const Animation &anim
) {
  const QSize size = getTransformedSize(anim.getSize(), animParams.transform);
  auto iterate = [&](const Frame &frame, const SpriteNameState &state) {
    NameInfo info = {animParams.name, state, frame.empty() ? QSize{} : size};
    params.generator->appendName(index++, info);
    return Error{};
  };
  static_cast<void>(eachFrame(animParams, anim, iterate));
}

void addCelNames(
  std::size_t &index,
  const ExportParams &params,
  const AnimExportParams &animParams,
  const Animation &anim
) {
  const QSize size = getTransformedSize(anim.getSize(), animParams.transform);
  auto iterate = [&](const CelImage *img, const SpriteNameState &state) {
    NameInfo info = {animParams.name, state, *img ? size : QSize{}};
    params.generator->appendName(index++, info);
    return Error{};
  };
  static_cast<void>(eachCel(animParams, anim, iterate));
}

Error addFrameImages(
  std::size_t &index,
  const ExportParams &params,
  const AnimExportParams &animParams,
  const Animation &anim
) {
  Images images;
  initImages(images, animParams, anim);
  const Format format = anim.getFormat();
  const PaletteCSpan palette = anim.palette.getPalette();
  
  auto iterate = [&](const Frame &frame, const SpriteNameState &) {
    if (!frame.empty()) {
      if (format == Format::gray) {
        compositeFrame<FmtGray>(images.canvas, palette, frame, format, images.canvas.rect());
      } else {
        compositeFrame<FmtRgba>(images.canvas, palette, frame, format, images.canvas.rect());
      }
      return addImage(index++, params, animParams, images);
    } else {
      return params.generator->copyImage(index++, {});
    }
  };
  
  return eachFrame(animParams, anim, iterate);
}

Error addCelImages(
  std::size_t &index,
  const ExportParams &params,
  const AnimExportParams &animParams,
  const Animation &anim
) {
  Images images;
  initImages(images, animParams, anim);
  
  auto iterate = [&](const CelImage *cel, const SpriteNameState &) {
    if (*cel) {
      clearImage(images.canvas);
      blitImage(images.canvas, cel->img, cel->pos);
      return addImage(index++, params, animParams, images);
    } else {
      return params.generator->copyImage(index++, {});
    }
  };
  
  return eachCel(animParams, anim, iterate);
}

Format compositedFormat(const Format format, const bool composite) {
  if (format == Format::index && composite) {
    return Format::rgba;
  } else {
    return format;
  }
}

using AnimPtr = std::unique_ptr<const Animation, void(*)(const Animation *)>;
using AnimArray = std::vector<AnimPtr>;

Error exportTextureAtlas(const ExportParams &params, const AnimArray &anims) {
  assert(params.generator);
  assert(params.anims.size() == anims.size());
  assert(!anims.empty());
  
  for (std::size_t s = 0; s != anims.size(); ++s) {
    const Format format = compositedFormat(anims[s]->getFormat(), params.anims[s].composite);
    if (!params.generator->supported(params.pixelFormat, format)) {
      return "Combination of pixel format and animation format is not supported by atlas generator";
    }
  }
  
  const AtlasInfo info = {params.name, params.directory, params.pixelFormat};
  TRY(params.generator->beginAtlas(info));
  
  std::size_t spriteIndex = 0;
  for (std::size_t s = 0; s != anims.size(); ++s) {
    if (params.anims[s].composite) {
      addFrameNames(spriteIndex, params, params.anims[s], *anims[s]);
    } else {
      addCelNames(spriteIndex, params, params.anims[s], *anims[s]);
    }
  }
  
  if (params.whitepixel) {
    params.generator->appendWhiteName(spriteIndex);
  }
  
  if (QString name = params.generator->endNames(); !name.isNull()) {
    return "Sprite name collision \"" + name + "\"";
  }
  
  TRY(params.generator->beginImages());
  
  spriteIndex = 0;
  for (std::size_t s = 0; s != anims.size(); ++s) {
    const Format format = compositedFormat(anims[s]->getFormat(), params.anims[s].composite);
    TRY(params.generator->setImageFormat(format, anims[s]->palette.getPalette()));
    if (params.anims[s].composite) {
      TRY(addFrameImages(spriteIndex, params, params.anims[s], *anims[s]));
    } else {
      TRY(addCelImages(spriteIndex, params, params.anims[s], *anims[s]));
    }
  }
  
  if (params.whitepixel) {
    TRY(params.generator->copyWhiteImage(spriteIndex));
  }
  
  return params.generator->endAtlas();
}

}

Error exportTextureAtlas(const ExportParams &params, const std::vector<QString> &paths) {
  // TODO: This is not very efficient
  // We only need to load the selected portion of the file
  // Although the common case is to export the whole thing so
  // we'd be optimizing an uncommon situation
  
  // Also, I have to do all this nonsense instead of using std::vector<Animation>
  // because QObject doesn't have a move constructor. Might be better off using
  // a simpler data structure here. We don't need the full functionality of
  // Animation.
  AnimArray anims;
  for (const QString &path : paths) {
    auto *anim = new Animation;
    if (Error err = anim->openFile(path)) {
      delete anim;
      return err;
    }
    anims.push_back(AnimPtr{anim, [](const Animation *anim) {
      delete anim;
    }});
  }
  
  return exportTextureAtlas(params, anims);
}

Error exportTextureAtlas(const ExportParams &params, const Animation &anim) {
  AnimArray anims;
  anims.push_back(AnimPtr{&anim, [](const Animation *) {}});
  return exportTextureAtlas(params, anims);
}
