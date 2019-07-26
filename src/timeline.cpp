//
//  timeline.cpp
//  Pixel 2
//
//  Created by Indi Kernick on 6/7/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "timeline.hpp"

#include "serial.hpp"
#include "cell span.hpp"
#include "composite.hpp"
#include <QtCore/qdir.h>
#include "export pattern.hpp"

Timeline::Timeline()
  : currPos{0, 0}, frameCount{0} {}

void Timeline::initDefault() {
  frameCount = 1;
  changeFrameCount();
  Layer layer;
  layer.spans.push_back({makeCell(), 1});
  layer.name = "Layer 0";
  layers.push_back(std::move(layer));
  selection = {0, 0, -1, -1};
  changeLayerCount();
  changeFrame();
  changePos();
  Q_EMIT selectionChanged(selection);
  changeLayers(0, 1);
}

namespace {

void serializeImage(QIODevice *dev, const QImage &image) {
  assert(dev);
  assert(!image.isNull());
  image.save(dev, "png");
}

void deserializeImage(QIODevice *dev, QImage &image) {
  assert(dev);
  image.load(dev, "png");
}

}

void Timeline::serialize(QIODevice *dev) const {
  assert(dev);
  serializeBytes(dev, static_cast<uint16_t>(layers.size()));
  serializeBytes(dev, static_cast<uint16_t>(frameCount));
  for (const Layer &layer : layers) {
    serializeBytes(dev, layer.visible);
    serializeBytes(dev, static_cast<uint16_t>(layer.name.size()));
    dev->write(layer.name.data(), layer.name.size());
    serializeBytes(dev, static_cast<uint16_t>(layer.spans.size()));
    for (const CellSpan &span : layer.spans) {
      serializeBytes(dev, static_cast<uint16_t>(span.len));
      serializeBytes(dev, static_cast<bool>(span.cell));
      if (span.cell) {
        serializeImage(dev, span.cell->image);
      }
    }
  }
}

void Timeline::deserialize(QIODevice *dev) {
  assert(dev);
  layers.resize(deserializeBytesAs<uint16_t>(dev));
  frameCount = deserializeBytesAs<uint16_t>(dev);
  for (Layer &layer : layers) {
    deserializeBytes(dev, layer.visible);
    layer.name.resize(deserializeBytesAs<uint16_t>(dev));
    dev->read(layer.name.data(), layer.name.size());
    layer.spans.resize(deserializeBytesAs<uint16_t>(dev));
    for (CellSpan &span : layer.spans) {
      span.len = deserializeBytesAs<uint16_t>(dev);
      if (deserializeBytesAs<bool>(dev)) {
        span.cell = makeCell();
        deserializeImage(dev, span.cell->image);
      }
    }
  }
  selection = {0, 0, -1, -1};
  changeFrameCount();
  changeLayerCount();
  changeFrame();
  changePos();
  Q_EMIT selectionChanged(selection);
  changeLayers(0, layerCount());
}

CellRect Timeline::selectCells(const ExportOptions &options) const {
  CellRect rect;
  switch (options.layerSelect) {
    case LayerSelect::all_composited:
    case LayerSelect::all:
      rect.minL = 0;
      rect.maxL = layerCount() - 1;
      break;
    case LayerSelect::current:
      rect.minL = rect.maxL = currPos.l;
      break;
    default: Q_UNREACHABLE();
  }
  switch (options.frameSelect) {
    case FrameSelect::all:
      rect.minF = 0;
      rect.maxF = frameCount - 1;
      break;
    case FrameSelect::current:
      rect.minF = rect.maxF = currPos.f;
      break;
    default: Q_UNREACHABLE();
  }
  return rect;
}

namespace {

inline void assertEval([[maybe_unused]] const bool cond) noexcept {
  assert(cond);
}

QImage grayToIndexed(const PaletteCSpan palette, QImage image) {
  assert(image.format() == QImage::Format_Grayscale8);
  assertEval(image.reinterpretAsFormat(QImage::Format_Indexed8));
  QVector<QRgb> table(static_cast<int>(palette.size()));
  std::copy(palette.cbegin(), palette.cend(), table.begin());
  image.setColorTable(table);
  return image;
}

}

QImage Timeline::convertImage(
  const ExportFormat format,
  const PaletteCSpan palette,
  QImage image
) const {
  // @TODO libpng
  switch (canvasFormat) {
    case Format::rgba:
      assert(format == ExportFormat::rgba);
      return image;
    case Format::palette:
      if (format == ExportFormat::rgba) {
        assert(image.format() == qimageFormat(Format::rgba));
        return image;
      } else if (format == ExportFormat::indexed) {
        return grayToIndexed(palette, image);
      } else if (format == ExportFormat::grayscale) {
        assert(image.format() == qimageFormat(Format::gray));
        return image;
      } else Q_UNREACHABLE();
    case Format::gray:
      if (format == ExportFormat::grayscale) {
        return image;
      } else if (format == ExportFormat::monochrome) {
        return grayToMono(image);
      } else Q_UNREACHABLE();
    default: Q_UNREACHABLE();
  }
}

namespace {

int apply(const Line line, const int value) {
  return value * line.stride + line.offset;
}

}

void Timeline::exportFile(
  const ExportOptions &options,
  const PaletteCSpan palette,
  QImage image,
  CellPos pos
) const {
  QString path = options.directory;
  if (path.back() != QDir::separator()) {
    path.push_back(QDir::separator());
  }
  pos.l = apply(options.layerLine, pos.l);
  pos.f = apply(options.frameLine, pos.f);
  path += evalExportPattern(options.name, pos.l, pos.f);
  path += ".png";
  assertEval(convertImage(options.format, palette, image).save(path));
}

void Timeline::exportCompRect(
  const ExportOptions &options,
  const PaletteCSpan palette,
  const CellRect rect
) const {
  const LayerIdx rectLayers = rect.maxL - rect.minL + 1;
  Frame frame;
  frame.reserve(rectLayers);
  std::vector<ConstCellIter> iterators;
  iterators.reserve(rectLayers);
  for (LayerIdx l = rect.minL; l <= rect.maxL; ++l) {
    iterators.push_back(find(layers[l].spans, rect.minF));
  }
  for (FrameIdx f = rect.minF; f <= rect.maxF; ++f) {
    frame.clear();
    for (LayerIdx l = 0; l != rectLayers; ++l) {
      if (!layers[l + rect.minL].visible) continue;
      if (const Cell *cell = iterators[l].get(); cell) {
        frame.push_back(cell);
      }
      iterators[l].incr();
    }
    QImage result = compositeFrame(palette, frame, canvasSize, canvasFormat);
    exportFile(options, palette, result, {rect.minL, f});
  }
}

void Timeline::exportRect(
  const ExportOptions &options,
  const PaletteCSpan palette,
  const CellRect rect
) const {
  for (LayerIdx l = rect.minL; l <= rect.maxL; ++l) {
    const Layer &layer = layers[l];
    // @TODO does the user want to skip invisible layers?
    if (!layer.visible) continue;
    ConstCellIter iter = find(layer.spans, rect.minF);
    for (FrameIdx f = rect.minF; f <= rect.maxF; ++f) {
      if (const Cell *cell = iter.get(); cell) {
        exportFile(options, palette, cell->image, {l, f});
      }
      iter.incr();
    }
  }
}

void Timeline::exportTimeline(const ExportOptions &options, const PaletteCSpan palette) const {
  const CellRect rect = selectCells(options);
  if (composited(options.layerSelect)) {
    exportCompRect(options, palette, rect);
  } else {
    exportRect(options, palette, rect);
  }
}

void Timeline::initCanvas(const Format format, const QSize size) {
  canvasFormat = format;
  canvasSize = size;
}

void Timeline::nextFrame() {
  currPos.f = (currPos.f + 1) % frameCount;
  changeFrame();
  changePos();
}

void Timeline::prevFrame() {
  currPos.f = (currPos.f - 1 + frameCount) % frameCount;
  changeFrame();
  changePos();
}

void Timeline::layerBelow() {
  currPos.l = std::min(currPos.l + 1, layerCount() - 1);
  changePos();
}

void Timeline::layerAbove() {
  currPos.l = std::max(currPos.l - 1, 0);
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
  selection.minL = currPos.l;
  selection.minF = currPos.f;
  selection.maxL = currPos.l;
  selection.maxF = currPos.f;
  Q_EMIT selectionChanged(selection);
}

void Timeline::continueSelection() {
  selection.maxL = currPos.l;
  selection.maxF = currPos.f;
  Q_EMIT selectionChanged(normalize(selection));
}

void Timeline::endSelection() {
  selection.maxL = currPos.l;
  selection.maxF = currPos.f;
  selection = normalize(selection);
  Q_EMIT selectionChanged(selection);
}

void Timeline::clearSelection() {
  selection = {0, 0, -1, -1};
  Q_EMIT selectionChanged(selection);
}

void Timeline::insertLayer() {
  Layer layer;
  layer.spans.push_back({nullptr, frameCount});
  layer.name = "Layer " + std::to_string(layers.size());
  layers.insert(layers.begin() + currPos.l, std::move(layer));
  changeLayerCount();
  Q_EMIT selectionChanged(selection);
  changeLayers(currPos.l, layerCount());
  changeFrame();
  changePos();
}

void Timeline::removeLayer() {
  if (layers.size() == 1) {
    clear(layers.front().spans, frameCount);
    layers.front().name = "Layer 0";
    layers.front().visible = true;
    changeLayers(0, 1);
  } else {
    layers.erase(layers.begin() + currPos.l);
    changeLayerCount();
    Q_EMIT selectionChanged(selection);
    currPos.l = std::min(currPos.l, layerCount() - 1);
    changeLayers(currPos.l, layerCount());
  }
  changeFrame();
  changePos();
}

void Timeline::moveLayerUp() {
  if (currPos.l == 0) return;
  std::swap(layers[currPos.l - 1], layers[currPos.l]);
  changeLayers(currPos.l - 1, currPos.l + 1);
  changeFrame();
  layerAbove();
}

void Timeline::moveLayerDown() {
  if (currPos.l == layerCount() - 1) return;
  std::swap(layers[currPos.l], layers[currPos.l + 1]);
  changeLayers(currPos.l, currPos.l + 2);
  changeFrame();
  layerBelow();
}

void Timeline::insertFrame() {
  ++frameCount;
  changeFrameCount();
  for (LayerIdx l = 0; l != layerCount(); ++l) {
    insertCopy(layers[l].spans, currPos.f);
    changeSpan(l);
  }
  Q_EMIT selectionChanged(selection);
  nextFrame();
}

void Timeline::insertNullFrame() {
  ++frameCount;
  changeFrameCount();
  for (LayerIdx l = 0; l != layerCount(); ++l) {
    insertNew(layers[l].spans, currPos.f, nullptr);
    changeSpan(l);
  }
  Q_EMIT selectionChanged(selection);
  nextFrame();
}

void Timeline::removeFrame() {
  if (frameCount == 1) {
    for (LayerIdx l = 0; l != layerCount(); ++l) {
      clear(layers[l].spans, 1);
      changeSpan(l);
    }
  } else {
    --frameCount;
    changeFrameCount();
    for (LayerIdx l = 0; l != layerCount(); ++l) {
      remove(layers[l].spans, currPos.f);
      changeSpan(l);
    }
    Q_EMIT selectionChanged(selection);
  }
  currPos.f = std::max(currPos.f - 1, 0);
  changeFrame();
  changePos();
}

void Timeline::clearCell() {
  replaceNew(layers[currPos.l].spans, currPos.f, nullptr);
  changeSpan(currPos.l);
  changeFrame();
  changePos();
}

void Timeline::extendCell() {
  extend(layers[currPos.l].spans, currPos.f);
  changeSpan(currPos.l);
  nextFrame();
}

void Timeline::splitCell() {
  split(layers[currPos.l].spans, currPos.f);
  changeSpan(currPos.l);
  changeFrame();
  changePos();
}

void Timeline::requestCell() {
  replaceNew(layers[currPos.l].spans, currPos.f, makeCell());
  changeSpan(currPos.l);
  changeFrame();
  changePos();
}

void Timeline::setCurrPos(const CellPos pos) {
  assert(0 <= pos.l);
  assert(pos.l < layerCount());
  assert(0 <= pos.f);
  assert(pos.f < frameCount);
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
  assert(0 <= idx);
  assert(idx < layerCount());
  bool &layerVis = layers[idx].visible;
  // @TODO Emit signal when layer visibility changed?
  // if (layerVis != visible) {
    layerVis = visible;
  //   Q_EMIT visibilityChanged(idx, visible);
  // }
  changeFrame();
}

void Timeline::setName(const LayerIdx idx, const std::string_view name) {
  assert(0 <= idx);
  assert(idx < layerCount());
  layers[idx].name = name;
  // @TODO Emit signal when layer name changed?
  // Q_EMIT nameChanged(idx, name);
}

void Timeline::clearSelected() {
  Spans nullSpans;
  nullSpans.push_back({nullptr, selection.maxF - selection.minF + 1});
  for (LayerIdx l = selection.minL; l <= selection.maxL; ++l) {
    replaceSpan(layers[l].spans, selection.minF, nullSpans);
    changeSpan(l);
  }
  changeFrame();
  changePos();
}

void Timeline::copySelected() {
  clipboard.clear();
  for (LayerIdx l = selection.minL; l <= selection.maxL; ++l) {
    const FrameIdx idx = selection.minF;
    const FrameIdx len = selection.maxF - selection.minF + 1;
    clipboard.push_back(extract(layers[l].spans, idx, len));
  }
}

void Timeline::pasteSelected() {
  if (selection.minL > selection.maxL) return;
  const LayerIdx endLayer = std::min(
    layerCount(), selection.minL + static_cast<LayerIdx>(clipboard.size())
  );
  const FrameIdx frames = frameCount - selection.minF;
  for (LayerIdx l = selection.minL; l < endLayer; ++l) {
    Spans spans = truncateCopy(clipboard[l - selection.minL], frames);
    replaceSpan(layers[l].spans, selection.minF, spans);
    changeSpan(l);
  }
  changeFrame();
  changePos();
}

CellPtr Timeline::makeCell() const {
  return std::make_unique<Cell>(canvasSize, canvasFormat);
}

Cell *Timeline::getCell(const CellPos pos) {
  return get(layers[pos.l].spans, pos.f);
}

Frame Timeline::getFrame(const FrameIdx pos) const {
  Frame frame;
  frame.reserve(layers.size());
  for (const Layer &layer : layers) {
    if (layer.visible) {
      if (const Cell *cell = get(layer.spans, pos); cell) {
        frame.push_back(cell);
      }
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
  Q_EMIT layerChanged(idx, layers[idx].spans);
}

void Timeline::changeLayers(const LayerIdx begin, const LayerIdx end) {
  assert(begin < end);
  for (LayerIdx l = begin; l != end; ++l) {
    Q_EMIT layerChanged(l, layers[l].spans);
    Q_EMIT visibilityChanged(l, layers[l].visible);
    Q_EMIT nameChanged(l, layers[l].name);
  }
}

void Timeline::changeFrameCount() {
  Q_EMIT frameCountChanged(frameCount);
}

void Timeline::changeLayerCount() {
  Q_EMIT layerCountChanged(layerCount());
}

#include "timeline.moc"