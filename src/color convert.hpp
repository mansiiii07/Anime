//
//  color convert.hpp
//  Pixel 2
//
//  Created by Indi Kernick on 29/4/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef color_convert_hpp
#define color_convert_hpp

#include <QtGui/qcolor.h>

struct HSV {
  int h, s, v;
};

struct RGB {
  int r, g, b;
};

QRgb hsv2rgb(qreal, qreal, qreal);
HSV color2hsv(QColor);
RGB color2rgb(QColor);
RGB hsv2rgb(HSV);
HSV rgb2hsv(RGB);

#endif