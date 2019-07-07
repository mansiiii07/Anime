//
//  sprite.cpp
//  Pixel 2
//
//  Created by Indi Kernick on 7/7/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "sprite.hpp"

#include "serial.hpp"
#include <QtCore/qfile.h>

namespace {

// @TODO maybe this could be improved?
// Look at PNG
constexpr char const magic_number[] = {'P', 'I', 'X', '2'};

}

void Sprite::newFile(const Format newFormat, const QSize newSize) {
  format = newFormat;
  size = newSize;
  Q_EMIT canvasInitialized(format, size);
  timeline.initCanvas(format, size);
  timeline.initDefault();
}

void Sprite::saveFile(const QString &path) const {
  QFile file{path};
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    // @TODO handle this properly
    throw std::exception{};
  }
  file.write(magic_number, sizeof(magic_number));
  serializeBytes(&file, format);
  //if (format == Format::palette) {
  //  serialize(&file, *palette);
  //}
 
  serializeBytes(&file, static_cast<uint16_t>(size.width()));
  serializeBytes(&file, static_cast<uint16_t>(size.height()));
  timeline.serialize(&file);
}

void Sprite::openFile(const QString &path) {
  // @TODO handle corrupted or invalid files properly
  QFile file{path};
  if (!file.open(QIODevice::ReadOnly)) {
    throw std::exception{};
  }
  char header[sizeof(magic_number)];
  file.read(header, sizeof(magic_number));
  if (std::memcmp(header, magic_number, sizeof(magic_number)) != 0) {
    throw std::exception{};
  }
  deserializeBytes(&file, format);
  //if (format == Format::palette) {
  //  deserialize(&file, *palette);
  //}
 
  uint16_t width;
  uint16_t height;
  deserializeBytes(&file, width);
  deserializeBytes(&file, height);
  size = {width, height};
  Q_EMIT canvasInitialized(format, size);
  timeline.initCanvas(format, size);
  timeline.deserialize(&file);
}

#include "sprite.moc"
