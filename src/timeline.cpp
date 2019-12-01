//
//  timeline.cpp
//  Animera
//
//  Created by Indi Kernick on 6/7/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "timeline.hpp"

#include "file io.hpp"
#include "composite.hpp"
#include "export png.hpp"
#include "sprite file.hpp"
#include "sprite export.hpp"

Timeline::Timeline()
  : currPos{LayerIdx{0}, FrameIdx{0}}, frameCount{0} {}

void Timeline::initDefault() {
  frameCount = FrameIdx{1};
  Layer layer;
  layer.spans.pushNull(frameCount);
  layer.name = "Layer 0";
  layers.push_back(std::move(layer));
  selection = empty_rect;
  delay = ctrl_delay.def;
  change();
}

void Timeline::optimize() {
  LayerIdx idx{};
  for (Layer &layer : layers) {
    for (CellSpan &span : layer.spans) {
      optimizeCell(*span.cell);
    }
    layer.spans.optimize();
    changeSpan(idx);
    ++idx;
  }
  changeFrame();
  changePos();
}

void Timeline::change() {
  changeFrameCount();
  changeLayerCount();
  changeFrame();
  changePos();
  Q_EMIT selectionChanged(selection);
  changeLayers(LayerIdx{0}, layerCount());
  Q_EMIT delayChanged(delay);
}

Error Timeline::openImage(
  const QString &path,
  const PaletteSpan palette,
  Format &format,
  QSize &size
) {
  QImage image;
  FileReader reader;
  TRY(reader.open(path));
  TRY(importPng(reader.dev(), palette, image, format));
  TRY(reader.flush());
  canvasFormat = format;
  canvasSize = size = image.size();
  
  frameCount = FrameIdx{1};
  Layer layer;
  layer.spans.pushNull(frameCount);
  layer.spans.begin()->cell->img = std::move(image);
  layer.name = "Layer 0";
  layers.push_back(std::move(layer));
  selection = empty_rect;
  delay = ctrl_delay.def;
  return {};
}

Error Timeline::serializeHead(QIODevice &dev) const {
  SpriteInfo info;
  info.width = canvasSize.width();
  info.height = canvasSize.height();
  info.layers = layerCount();
  info.frames = frameCount;
  info.delay = delay;
  info.format = canvasFormat;
  return writeAHDR(dev, info);
}

Error Timeline::serializeBody(QIODevice &dev) const {
  for (const Layer &layer : layers) {
    TRY(writeLHDR(dev, layer));
    for (const CellSpan &span : layer.spans) {
      TRY(writeCHDR(dev, span));
      if (*span.cell) TRY(writeCDAT(dev, span.cell->img, canvasFormat));
    }
  }
  return {};
}

Error Timeline::serializeTail(QIODevice &dev) const {
  return writeAEND(dev);
}

Error Timeline::deserializeHead(QIODevice &dev, Format &format, QSize &size) {
  SpriteInfo info;
  TRY(readAHDR(dev, info));
  canvasSize = size = {info.width, info.height};
  layers.resize(+info.layers);
  frameCount = info.frames;
  canvasFormat = format = info.format;
  delay = info.delay;
  return {};
}

Error Timeline::deserializeBody(QIODevice &dev) {
  for (Layer &layer : layers) {
    TRY(readLHDR(dev, layer));
    for (CellSpan &span : layer.spans) {
      TRY(readCHDR(dev, span, canvasFormat));
      if (*span.cell) TRY(readCDAT(dev, span.cell->img, canvasFormat));
    }
  }
  return {};
}

Error Timeline::deserializeTail(QIODevice &dev) {
  TRY(readAEND(dev));
  selection = empty_rect;
  change();
  return {};
}

Error Timeline::exportTimeline(const ExportOptions &options, const PaletteCSpan palette) const {
  Exporter exporter{options, palette, canvasFormat, canvasSize};
  return exporter.exportSprite(layers);
}

LayerIdx Timeline::getLayers() const {
  return layerCount();
}

FrameIdx Timeline::getFrames() const {
  return frameCount;
}

CellPos Timeline::getCurrent() const {
  return currPos;
}

CellRect Timeline::getSelection() const {
  return selection;
}

void Timeline::initCanvas(const Format format, const QSize size) {
  canvasFormat = format;
  canvasSize = size;
}

void Timeline::nextFrame() {
  if (locked) return;
  currPos.f = (currPos.f + FrameIdx{1}) % frameCount;
  changeFrame();
  changePos();
}

void Timeline::prevFrame() {
  if (locked) return;
  currPos.f = (currPos.f - FrameIdx{1} + frameCount) % frameCount;
  changeFrame();
  changePos();
}

void Timeline::layerBelow() {
  if (locked) return;
  currPos.l = std::min(currPos.l + LayerIdx{1}, layerCount() - LayerIdx{1});
  changePos();
}

void Timeline::layerAbove() {
  if (locked) return;
  currPos.l = std::max(currPos.l - LayerIdx{1}, LayerIdx{0});
  changePos();
}

namespace {

CellRect normalize(const CellRect rect) {
  return {
    std::min(rect.minL, rect.maxL),
    std::min(rect.minF, rect.maxF),
    std::max(rect.minL, rect.maxL),
    std::max(rect.minF, rect.maxF)
  };
}

}

void Timeline::beginSelection() {
  if (locked) return;
  selection.minL = currPos.l;
  selection.minF = currPos.f;
  selection.maxL = currPos.l;
  selection.maxF = currPos.f;
  Q_EMIT selectionChanged(selection);
}

void Timeline::continueSelection() {
  if (locked) return;
  selection.maxL = currPos.l;
  selection.maxF = currPos.f;
  Q_EMIT selectionChanged(normalize(selection));
}

void Timeline::endSelection() {
  if (locked) return;
  selection.maxL = currPos.l;
  selection.maxF = currPos.f;
  selection = normalize(selection);
  Q_EMIT selectionChanged(selection);
}

void Timeline::clearSelection() {
  if (locked) return;
  selection = empty_rect;
  Q_EMIT selectionChanged(selection);
}

void Timeline::insertLayer() {
  if (locked) return;
  Layer layer;
  layer.spans.pushNull(frameCount);
  layer.name = "Layer " + std::to_string(layers.size());
  layers.insert(layers.begin() + +currPos.l, std::move(layer));
  changeLayerCount();
  Q_EMIT selectionChanged(selection);
  changeLayers(currPos.l, layerCount());
  changeFrame();
  changePos();
  Q_EMIT modified();
}

void Timeline::removeLayer() {
  if (locked) return;
  if (layers.size() == 1) {
    layers.front().spans.clear(frameCount);
    layers.front().name = "Layer 0";
    layers.front().visible = true;
    changeLayers(LayerIdx{0}, LayerIdx{1});
  } else {
    layers.erase(layers.begin() + +currPos.l);
    changeLayerCount();
    Q_EMIT selectionChanged(selection);
    currPos.l = std::min(currPos.l, layerCount() - LayerIdx{1});
    changeLayers(currPos.l, layerCount());
  }
  changeFrame();
  changePos();
  Q_EMIT modified();
}

void Timeline::moveLayerUp() {
  if (locked) return;
  if (currPos.l == LayerIdx{0}) return;
  std::swap(layers[+(currPos.l - LayerIdx{1})], layers[+currPos.l]);
  changeLayers(currPos.l - LayerIdx{1}, currPos.l + LayerIdx{1});
  changeFrame();
  layerAbove();
  Q_EMIT modified();
}

void Timeline::moveLayerDown() {
  if (locked) return;
  if (currPos.l == layerCount() - LayerIdx{1}) return;
  std::swap(layers[+currPos.l], layers[+(currPos.l + LayerIdx{1})]);
  changeLayers(currPos.l, currPos.l + LayerIdx{2});
  changeFrame();
  layerBelow();
  Q_EMIT modified();
}

void Timeline::insertFrame() {
  if (locked) return;
  ++frameCount;
  changeFrameCount();
  for (LayerIdx l = {}; l != layerCount(); ++l) {
    layers[+l].spans.insert(currPos.f);
    changeSpan(l);
  }
  Q_EMIT selectionChanged(selection);
  nextFrame();
  Q_EMIT modified();
}

void Timeline::removeFrame() {
  if (locked) return;
  if (frameCount == FrameIdx{1}) {
    for (LayerIdx l = {}; l != layerCount(); ++l) {
      layers[+l].spans.clear(FrameIdx{1});
      changeSpan(l);
    }
  } else {
    --frameCount;
    changeFrameCount();
    for (LayerIdx l = {}; l != layerCount(); ++l) {
      layers[+l].spans.remove(currPos.f);
      changeSpan(l);
    }
    Q_EMIT selectionChanged(selection);
  }
  currPos.f = std::max(currPos.f - FrameIdx{1}, FrameIdx{0});
  changeFrame();
  changePos();
  Q_EMIT modified();
}

void Timeline::clearCell() {
  if (locked) return;
  layers[+currPos.l].spans.replace(currPos.f, false);
  changeSpan(currPos.l);
  changeFrame();
  changePos();
  Q_EMIT modified();
}

void Timeline::extendCell() {
  if (locked) return;
  layers[+currPos.l].spans.extend(currPos.f);
  changeSpan(currPos.l);
  nextFrame();
  Q_EMIT modified();
}

void Timeline::splitCell() {
  if (locked) return;
  layers[+currPos.l].spans.split(currPos.f);
  changeSpan(currPos.l);
  changeFrame();
  changePos();
  Q_EMIT modified();
}

void Timeline::growCell(const QRect rect) {
  Cell &cell = *getCell(currPos);
  if (cell) {
    ::growCell(cell, canvasFormat, rect);
    return;
  }
  if (locked) return;
  layers[+currPos.l].spans.replace(currPos.f, true);
  ::growCell(*getCell(currPos), canvasFormat, rect);
  changeSpan(currPos.l);
  changeFrame();
  changePos();
  Q_EMIT modified();
}

void Timeline::setCurrPos(const CellPos pos) {
  assert(LayerIdx{0} <= pos.l);
  assert(pos.l < layerCount());
  assert(FrameIdx{0} <= pos.f);
  assert(pos.f < frameCount);
  if (locked) return;
  if (currPos.f != pos.f) {
    currPos = pos;
    changeFrame();
    changePos();
  } else if (currPos.l != pos.l) {
    currPos.l = pos.l;
    changePos();
  }
}

void Timeline::setVisibility(const LayerIdx idx, const bool visible) {
  assert(LayerIdx{0} <= idx);
  assert(idx < layerCount());
  bool &layerVis = layers[+idx].visible;
  // TODO: Emit signal when layer visibility changed?
  if (layerVis != visible) {
    layerVis = visible;
    // Q_EMIT visibilityChanged(idx, visible);
    changeFrame();
  }
  Q_EMIT modified();
}

void Timeline::setName(const LayerIdx idx, const std::string_view name) {
  assert(LayerIdx{0} <= idx);
  assert(idx < layerCount());
  layers[+idx].name = name;
  Q_EMIT modified();
}

void Timeline::setDelay(const int newDelay) {
  assert(ctrl_delay.min <= newDelay);
  assert(newDelay <= ctrl_delay.max);
  delay = newDelay;
  Q_EMIT modified();
}

void Timeline::clearSelected() {
  if (locked) return;
  LayerCells nullSpans;
  nullSpans.pushNull(selection.maxF - selection.minF + FrameIdx{1});
  for (LayerIdx l = selection.minL; l <= selection.maxL; ++l) {
    layers[+l].spans.replaceSpan(selection.minF, nullSpans);
    changeSpan(l);
  }
  changeFrame();
  changePos();
  Q_EMIT modified();
}

void Timeline::copySelected() {
  if (locked) return;
  clipboard.clear();
  for (LayerIdx l = selection.minL; l <= selection.maxL; ++l) {
    const FrameIdx idx = selection.minF;
    const FrameIdx len = selection.maxF - selection.minF + FrameIdx{1};
    clipboard.push_back(layers[+l].spans.extract(idx, len));
  }
}

void Timeline::pasteSelected() {
  if (locked) return;
  if (selection.minL > selection.maxL) return;
  const LayerIdx endLayer = std::min(
    layerCount(), selection.minL + static_cast<LayerIdx>(clipboard.size())
  );
  const FrameIdx frames = frameCount - selection.minF;
  for (LayerIdx l = selection.minL; l < endLayer; ++l) {
    LayerCells spans = clipboard[+(l - selection.minL)].truncateCopy(frames);
    layers[+l].spans.replaceSpan(selection.minF, spans);
    changeSpan(l);
  }
  changeFrame();
  changePos();
  Q_EMIT modified();
}

void Timeline::lock() {
  assert(!locked);
  locked = true;
}

void Timeline::unlock() {
  assert(locked);
  locked = false;
}

Cell *Timeline::getCell(const CellPos pos) {
  return layers[+pos.l].spans.get(pos.f);
}

Frame Timeline::getFrame(const FrameIdx pos) const {
  Frame frame;
  frame.reserve(layers.size());
  for (const Layer &layer : layers) {
    if (layer.visible) {
      frame.push_back(layer.spans.get(pos));
    }
  }
  return frame;
}

LayerIdx Timeline::layerCount() const {
  return static_cast<LayerIdx>(layers.size());
}

void Timeline::changePos() {
  Q_EMIT currPosChanged(currPos);
  Q_EMIT currCellChanged(getCell(currPos));
}

void Timeline::changeFrame() {
  Q_EMIT frameChanged(getFrame(currPos.f));
}

void Timeline::changeSpan(const LayerIdx idx) {
  Q_EMIT layerChanged(idx, layers[+idx].spans);
}

void Timeline::changeLayers(const LayerIdx begin, const LayerIdx end) {
  assert(begin < end);
  for (LayerIdx l = begin; l != end; ++l) {
    Q_EMIT layerChanged(l, layers[+l].spans);
    Q_EMIT visibilityChanged(l, layers[+l].visible);
    Q_EMIT nameChanged(l, layers[+l].name);
  }
}

void Timeline::changeFrameCount() {
  Q_EMIT frameCountChanged(frameCount);
}

void Timeline::changeLayerCount() {
  Q_EMIT layerCountChanged(layerCount());
}

#include "timeline.moc"
