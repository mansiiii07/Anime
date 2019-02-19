//
//  main.cpp
//  Pixel 2
//
//  Created by Indi Kernick on 3/2/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include <iostream>
#include <QtGui/qevent.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlayout.h>
#include <QtWidgets/qmenubar.h>
#include <QtWidgets/qscrollarea.h>
#include <QtWidgets/qdockwidget.h>
#include <QtWidgets/qfiledialog.h>
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qstylepainter.h>
#include <QtWidgets/qdesktopwidget.h>

/*
In case I decide that we need KC filters

#include <pixman-1/pixman.h>

#define d2f pixman_double_to_fixed
    const double frac = 1.0/5.0;
    pixman_fixed_t filterParams[] = {
      d2f(3), d2f(3),
      d2f(-1), d2f(-1), d2f(0),
      d2f(-1), d2f(0), d2f(1),
      d2f(0), d2f(1), d2f(1)
 
      //d2f(5), d2f(5),
      //d2f(1), d2f(0), d2f(0), d2f(0), d2f(0),
      //d2f(0), d2f(1), d2f(0), d2f(0), d2f(0),
      //d2f(0), d2f(0), d2f(0), d2f(0), d2f(0),
      //d2f(0), d2f(0), d2f(0), d2f(-1), d2f(0),
      //d2f(0), d2f(0), d2f(0), d2f(0), d2f(-1),
 
      //d2f(3), d2f(3),
      //d2f(frac), d2f(frac), d2f(frac),
      //d2f(frac), d2f(frac), d2f(frac),
      //d2f(frac), d2f(frac), d2f(frac),
    };
 
    pixman_image_set_filter(
      pixman_img,
      PIXMAN_FILTER_CONVOLUTION,
      filterParams,
      std::size(filterParams)
    );
    pixman_image_composite32(
      PIXMAN_OP_SRC,
      pixman_img,
      nullptr,
      pixman_filtered,
      0, 0, 0, 0, 0, 0,
      w, h
    );
    QImage filtered(filteredDat, w, h, QImage::Format_ARGB32);
 
    pixman_image *pixman_img = pixman_image_create_bits(
      PIXMAN_a8r8g8b8,
      w, h,
      reinterpret_cast<uint32_t *>(img.bits()),
      img.bytesPerLine()
    );
    uint8_t *filteredDat = new uint8_t[w * h * 4];
    pixman_image *pixman_filtered = pixman_image_create_bits(
      PIXMAN_a8r8g8b8,
      w, h,
      reinterpret_cast<uint32_t *>(filteredDat),
      w * 4
    );
*/


/*
 
 Performing basic graphics operations on QImages and rendering them
 
    setWindowTitle("Pixel 2");
    setupMenubar();
 
    int w = 256;
    int h = 256;
 
    uint8_t *imgDat = new uint8_t[4 * w * h];
    QImage img{imgDat, w, h, QImage::Format_ARGB32};
    std::memset(imgDat, 0, 4 * w * h);

    int x = 50;
    int y = 40;

    {
      QPainter painter{&img};
      painter.setRenderHint(QPainter::Antialiasing, false);
      painter.setCompositionMode(QPainter::CompositionMode_Source);
      painter.fillRect(x, y, 101, 101, QColor{0, 0, 255, 127});
 
      painter.setBrush(QBrush{QColor{255, 0, 0}});
      painter.setPen(QColor{0, 255, 0});
      painter.drawEllipse({x + 50, y + 50}, 50, 50);
 
      painter.setPen(QColor{255, 0, 255});
      painter.drawLine(x + 20, y + 30, x + 68, y + 85);
    }

    int index = ((y + 0) * w + (x + 0)) * 4;
    std::cout << int(imgDat[index]) << ' ' << int(imgDat[index + 1]) << ' ' << int(imgDat[index + 2]) << ' ' << int(imgDat[index + 3]) << '\n';

    union {
      uint32_t rgba;
      uint8_t comp[4];
    };

    rgba = (127 << 24) | (255 << 0);
    std::cout << int(comp[0]) << ' ' << int(comp[1]) << ' ' << int(comp[2]) << ' ' << int(comp[3]) << '\n';

    uint8_t *maskDat = new uint8_t[w * h];
    START_TIMER(MemsetMask);
    std::memset(maskDat, 255, w * h);
    STOP_TIMER(MemsetMask);
    QImage mask{maskDat, w, h, QImage::Format_Alpha8}; // for drawing and masking
    QImage realMask{maskDat, w, h, QImage::Format_Grayscale8}; // for rendering

    START_TIMER(RenderMask);
    {
      QPainter painter{&mask};
      painter.setRenderHint(QPainter::Antialiasing, false);
      painter.setCompositionMode(QPainter::CompositionMode_Source);
      painter.fillRect(x, y, 70, 70, QColor{0, 0, 0, 0});
    }
    STOP_TIMER(RenderMask);

    QImage maskAnd = mask;
    maskAnd.detach();

    START_TIMER(AndMask);
    andMask(maskAnd, mask);
    STOP_TIMER(AndMask);

    START_TIMER(CopyMask);
    QImage copy = mask;
    copy.detach();
    STOP_TIMER(CopyMask);

    START_TIMER(NotMask);
    notMask(mask);
    notMask(mask);
    STOP_TIMER(NotMask);

    START_TIMER(ApplyMask);
    applyMask(img, mask);
    STOP_TIMER(ApplyMask);

    START_TIMER(ToTexture);
    pixmap = QPixmap::fromImage(img, Qt::NoFormatConversion);
    STOP_TIMER(ToTexture);
    //pixmap = QPixmap::fromImage(realMask, Qt::NoFormatConversion);

    //pixmap = QPixmap::fromImage(realMask);
    //pixmap.load("/Users/indikernick/Library/Developer/Xcode/DerivedData/Pixel_2-gqoblrlhvynmicgniivandqktune/Build/Products/Debug/Pixel 2.app/Contents/Resources/icon.png");

    label.setPixmap(pixmap);
    label.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    label.setMinimumSize(0, 0);
    label.resize(w, h);
    label.show();

    layout.addWidget(&button, 0, 0);
    layout.addWidget(&label, 1, 0);
    setLayout(&layout);

    show();

*/

#include <cmath>

struct Color {
  uint8_t r, g, b, a;
};

struct ColorF {
  float r, g, b, a;
};

float toFloat(const uint8_t component) {
  return static_cast<float>(component) / 255.0f;
}

uint8_t fromFloat(const float component) {
  return static_cast<uint8_t>(std::clamp(std::round(component * 255.0f), 0.0f, 255.0f));
}

ColorF toFloat(const Color color) {
  return {toFloat(color.r), toFloat(color.g), toFloat(color.b), toFloat(color.a)};
}

Color fromFloat(const ColorF color) {
  return {fromFloat(color.r), fromFloat(color.g), fromFloat(color.b), fromFloat(color.a)};
}

// straight alpha with float precision
Color compositeF(const Color aInt, const Color bInt, const uint8_t afInt, const uint8_t bfInt) {
  const float aF = toFloat(afInt);
  const float bF = toFloat(bfInt);
  const ColorF a = toFloat(aInt);
  const ColorF b = toFloat(bInt);
  
  const float cA = a.a*aF + b.a*bF;
  if (cA == 0.0f) {
    return {0, 0, 0, 0};
  } else {
    const float cR = (a.a*aF*a.r + b.a*bF*b.r) / cA;
    const float cG = (a.a*aF*a.g + b.a*bF*b.g) / cA;
    const float cB = (a.a*aF*a.b + b.a*bF*b.b) / cA;
    return fromFloat({cR, cG, cB, cA});
  }
}

// straight alpha with uint8 precision
Color compositeI(const Color a, const Color b, const uint8_t aF, const uint8_t bF) {
  const uint32_t cA = a.a*aF + b.a*bF;
  if (cA == 0) {
    return {0, 0, 0, 0};
  } else {
    const uint8_t cR = (a.a*aF*a.r + b.a*bF*b.r) / cA;
    const uint8_t cG = (a.a*aF*a.g + b.a*bF*b.g) / cA;
    const uint8_t cB = (a.a*aF*a.b + b.a*bF*b.b) / cA;
    return {cR, cG, cB, static_cast<uint8_t>(cA / 255)};
  }
}

// premultiplied alpha with uint8 precision
Color compositeM(const Color a, const Color b, const uint8_t aF, const uint8_t bF) {
  const uint8_t cR = (aF*a.r + bF*b.r) / 255;
  const uint8_t cG = (aF*a.g + bF*b.g) / 255;
  const uint8_t cB = (aF*a.b + bF*b.b) / 255;
  const uint8_t cA = (aF*a.a + bF*b.a) / 255;
  return {cR, cG, cB, cA};
}

Color mulAlpha(const Color color) {
  const uint8_t r = (color.r * color.a) / 255;
  const uint8_t g = (color.g * color.a) / 255;
  const uint8_t b = (color.b * color.a) / 255;
  return {r, g, b, color.a};
}

Color divAlpha(const Color color) {
  if (color.a == 0) {
    return {0, 0, 0, 0};
  } else {
    const uint8_t r = (color.r * 255) / color.a;
    const uint8_t g = (color.g * 255) / color.a;
    const uint8_t b = (color.b * 255) / color.a;
    return {r, g, b, color.a};
  }
}

std::ostream &operator<<(std::ostream &stream, const Color color) {
  stream.width(3);
  stream << int(color.r) << ' ';
  stream.width(3);
  stream << int(color.g) << ' ';
  stream.width(3);
  stream << int(color.b) << ' ';
  stream.width(3);
  stream << int(color.a);
  return stream;
}

std::ostream &operator<<(std::ostream &stream, const ColorF color) {
  stream.width(3);
  stream << color.r << ' ';
  stream.width(3);
  stream << color.g << ' ';
  stream.width(3);
  stream << color.b << ' ';
  stream.width(3);
  stream << color.a;
  return stream;
}

bool operator==(const Color a, const Color b) {
  return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

bool operator!=(const Color a, const Color b) {
  return !(a == b);
}

constexpr int difference_threshold = 0;

bool different(const uint8_t a, const uint8_t b) {
  const int aI = a;
  const int bI = b;
  return aI + difference_threshold < bI || bI + difference_threshold < aI;
}

bool different(const Color a, const Color b) {
  return different(a.r, b.r) ||
         different(a.g, b.g) ||
         different(a.b, b.b) ||
         different(a.a, b.a);
}

void testComposite() {
  std::cout << "A                   B                   Float               Int                 Premul\n";

  uint8_t vals[] = {0, 63, 127, 191, 255};
  for (int cA = 0; cA != sizeof(vals); ++cA) {
    for (int aA = 0; aA != sizeof(vals); ++aA) {
      for (int cB = 0; cB != sizeof(vals); ++cB) {
        for (int aB = 0; aB != sizeof(vals); ++aB) {
          const Color a = {vals[cA], 0, 0, vals[aA]};
          const Color b = {vals[cB], 0, 0, vals[aB]};
          const uint8_t aF = 255;
          const uint8_t bF = 255 - a.a;
          const Color cF = compositeF(a, b, aF, bF);
          const Color cI = compositeI(a, b, aF, bF);
          //const Color cM = divAlpha(compositeM(mulAlpha(a), mulAlpha(b), aF, bF));
          //if (different(cF, cI) || different(cI, cM) || different(cF, cM)) {
          //  std::cout << a << " \t" << b << " \t" << cF << " \t" << cI << " \t" << cM << '\n';
          //}
          if (different(cF, cI)) {
            std::cout << a << " \t" << b << " \t" << cF << " \t" << cI << '\n';
          }
        }
      }
    }
  }
}

void applyMask(QImage &image, const QImage &mask) {
  assert(image.size() == mask.size());
  assert(image.format() == QImage::Format_ARGB32);
  assert(mask.format() == QImage::Format_Alpha8);
  
  image.detach();
  
  const ptrdiff_t dstPitch = image.bytesPerLine();
  uchar *dstData = image.bits();
  const ptrdiff_t maskPitch = mask.bytesPerLine();
  const uchar *maskData = mask.bits();
  const ptrdiff_t width = image.width();
  const ptrdiff_t height = image.height();
  
  for (ptrdiff_t y = 0; y != height; ++y) {
    for (ptrdiff_t x = 0; x != width; ++x) {
      uchar *const dstPixel = dstData + x * 4;
      const uchar maskPixel = maskData[x];
      dstPixel[0] &= maskPixel;
      dstPixel[1] &= maskPixel;
      dstPixel[2] &= maskPixel;
      dstPixel[3] &= maskPixel;
    }
    dstData += dstPitch;
    maskData += maskPitch;
  }
}

// each scanline is 32-bit aligned

void notMask(QImage &dst) {
  assert(dst.format() == QImage::Format_Alpha8);
  
  dst.detach();
  
  const ptrdiff_t pitch = dst.bytesPerLine();
  uchar *data = dst.bits();
  const ptrdiff_t width = dst.width() / 4;
  const ptrdiff_t height = dst.height();
  
  for (ptrdiff_t y = 0; y != height; ++y) {
    for (ptrdiff_t x = 0; x != width; ++x) {
      uint32_t *const pixel = reinterpret_cast<uint32_t *>(data) + x;
      *pixel ^= 0xFFFFFFFF;
    }
    data += pitch;
  }
}

template <typename Op>
void binaryMaskOp(QImage &dst, const QImage &src, Op &&op) {
  assert(dst.size() == src.size());
  assert(dst.format() == QImage::Format_Alpha8);
  assert(src.format() == QImage::Format_Alpha8);
  assert(dst.bytesPerLine() == src.bytesPerLine());
  
  dst.detach();
  
  const ptrdiff_t pitch = dst.bytesPerLine();
  uchar *dstData = dst.bits();
  const uchar *srcData = src.bits();
  const ptrdiff_t width = dst.width() / 4;
  const ptrdiff_t height = dst.height();
  
  for (ptrdiff_t y = 0; y != height; ++y) {
    for (ptrdiff_t x = 0; x != width; ++x) {
      uint32_t *const dstPixel = reinterpret_cast<uint32_t *>(dstData) + x;
      const uint32_t srcPixel = reinterpret_cast<const uint32_t *>(srcData)[x];
      op(dstPixel, srcPixel);
    }
    dstData += pitch;
    srcData += pitch;
  }
}

void andMask(QImage &dst, const QImage &src) {
  binaryMaskOp(dst, src, [](uint32_t *const dstPixel, const uint32_t srcPixel) {
    *dstPixel &= srcPixel;
  });
}

void orMask(QImage &dst, const QImage &src) {
  binaryMaskOp(dst, src, [](uint32_t *const dstPixel, const uint32_t srcPixel) {
    *dstPixel |= srcPixel;
  });
}

void xorMask(QImage &dst, const QImage &src) {
  binaryMaskOp(dst, src, [](uint32_t *const dstPixel, const uint32_t srcPixel) {
    *dstPixel ^= srcPixel;
  });
}

uint32_t composeRGBA(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 255) {
  return (uint32_t{a} << 24) | (uint32_t{r} << 16) | (uint32_t{g} << 8) | uint32_t{b};
}

class Timer {
public:
  using Clock = std::chrono::high_resolution_clock;
  using OutputDuration = std::chrono::duration<double, std::milli>;
  
  void start(const char *timerName) {
    assert(timerName);
    assert(!name);
    name = timerName;
    startTime = Clock::now();
  }
  void stop() {
    assert(name);
    const auto endTime = Clock::now();
    const auto diff = std::chrono::duration_cast<OutputDuration>(
      endTime - startTime
    );
    std::cout.width(16);
    std::cout << std::left << name << ' ';
    std::cout.precision(3);
    std::cout << diff.count() << "ms\n";
    name = nullptr;
  }
  void stopAndStart(const char *timerName) {
    stop();
    start(timerName);
  }

private:
  const char *name = nullptr;
  Clock::time_point startTime;
};

#include <QtCore/qtimer.h>

class StatusBar : public QWidget {
public:
  explicit StatusBar(QWidget *parent)
    : QWidget{parent}, label{this} {
    timer.setInterval(5000);
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, &StatusBar::hideTemp);
    label.setMinimumWidth(400);
  }
  
  void showTemp(const QString &text) {
    tempText = text;
    timer.start();
    setText();
  }
  void showPerm(const QString &text) {
    permText = text;
    setText();
  }
  
private:
  QLabel label;
  QString permText;
  QString tempText;
  QTimer timer;
  
  void setText() {
    if (tempText.isEmpty()) {
      label.setText(permText);
    } else {
      label.setText(permText + " | " + tempText);
    }
  }
  void hideTemp() {
    tempText = "";
    setText();
  }
};

class ToolsWidget : public QWidget {
public:
  ToolsWidget(QWidget *parent)
    : QWidget{parent} {
    layout = new QVBoxLayout{this};
    setLayout(layout);
    setFixedSize(24*2+2, (24*2+2) * 11);
  }

private:
  QVBoxLayout *layout = nullptr;
};

class TimelineWidget : public QWidget {
public:
  explicit TimelineWidget(QWidget *parent)
    : QWidget{parent}, status{this} {
    setMinimumHeight(128);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    status.showPerm("Permanent message");
    status.showTemp("Temporary message");
  }

private:
  StatusBar status;
};

class RenderWidget : public QScrollArea {
public:
  RenderWidget(QWidget *parent)
    : QScrollArea{parent} {
    setMinimumSize(128, 128);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setAlignment(Qt::AlignCenter);
    view = new QLabel{this};
    img.load("/Users/indikernick/Library/Developer/Xcode/DerivedData/Pixel_2-gqoblrlhvynmicgniivandqktune/Build/Products/Debug/Pixel 2.app/Contents/Resources/icon.png");
    view->setPixmap(img.scaled(64, 64));
    view->resize(64, 64);
    setWidget(view);
  }
  
private:
  QLabel *view = nullptr;
  QPixmap img;
};

class EditorWidget : public QScrollArea {
public:
  EditorWidget(QWidget *parent)
    : QScrollArea{parent},
      cursor{Qt::CrossCursor} {
    setMinimumSize(128, 128);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setAlignment(Qt::AlignCenter);
    view = new QLabel{this};
    img.load("/Users/indikernick/Library/Developer/Xcode/DerivedData/Pixel_2-gqoblrlhvynmicgniivandqktune/Build/Products/Debug/Pixel 2.app/Contents/Resources/icon.png");
    view->setPixmap(img.scaled(1024, 1024));
    view->resize(1024, 1024);
    setWidget(view);
    setCursor(cursor);
  }
  
private:
  QLabel *view = nullptr;
  QPixmap img;
  QCursor cursor;
};

class CentralWidget : public QWidget {
public:
  CentralWidget(QWidget *parent)
    : QWidget{parent} {
    layout = new QHBoxLayout{this};
    layout->addWidget(new EditorWidget{this});
    layout->addWidget(new RenderWidget{this});
    setLayout(layout);
  }

private:
  QHBoxLayout *layout = nullptr;
};

class Window : public QMainWindow {
public:
  Window() {
    setupMenubar();
    setupUI();
    show();
  }

  void fileOpen(QFileOpenEvent *event) {
    //button.setText(static_cast<QFileOpenEvent *>(event)->file());
  }

private:
  QMenuBar *menubar = nullptr;
  QDockWidget *toolDock = nullptr;
  QDockWidget *timelineDock = nullptr;
  
  void setupUI() {
    toolDock = new QDockWidget{this};
    toolDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    toolDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    toolDock->setWidget(new ToolsWidget{this});
    toolDock->setTitleBarWidget(new QWidget{});
    addDockWidget(Qt::LeftDockWidgetArea, toolDock);
    
    timelineDock = new QDockWidget{this};
    timelineDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    timelineDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    timelineDock->setWidget(new TimelineWidget{this});
    timelineDock->setTitleBarWidget(new QWidget{});
    addDockWidget(Qt::BottomDockWidgetArea, timelineDock);
    
    setCentralWidget(new CentralWidget{this});
  }
  
  void setupMenubar() {
    menubar = new QMenuBar{this};
    QMenu *file = menubar->addMenu("File");
    QAction *open = file->addAction("Open");
    QAction *save = file->addAction("Save");
    file->addSeparator();
    file->addAction("Export");
    connect(open, &QAction::triggered, this, &Window::openDoc);
    connect(save, &QAction::triggered, this, &Window::saveDoc);
  }

  void openDoc() {
    std::cout << "Open document\n";
    QString path = QFileDialog::getOpenFileName(
      this,
      "Open Image",
      QDir::homePath(),
      "PNG Files (*.png)",
      nullptr,
      QFileDialog::ReadOnly | QFileDialog::HideNameFilterDetails
    );
    if (!path.isEmpty()) {
      //pixmap.load(path);
      //label.setPixmap(pixmap);
      //label.resize(pixmap.size());
    }
  }
  void saveDoc() {
    std::cout << "Save document\n";
    QString path = QFileDialog::getSaveFileName(
      this,
      "Save Image",
      QDir::homePath() + "/Frame 0.png",
      "",
      nullptr,
      QFileDialog::HideNameFilterDetails
    );
    if (!path.isEmpty()) {
      //pixmap.save(path);
    }
  }
  
  void resizeEvent(QResizeEvent *event) override {
    
  }
};

class Application : public QApplication {
public:
  Application(int &argc, char **argv)
    : QApplication{argc, argv} {
    window.setMinimumSize(640, 360);
    window.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    window.setGeometry(
      QStyle::alignedRect(
        Qt::LeftToRight,
        Qt::AlignCenter,
        window.size(),
        desktop()->availableGeometry()
      )
    );
  }
  
private:
  Window window;
  
  bool event(QEvent *event) override {
    if (event->type() == QEvent::FileOpen) {
      window.fileOpen(static_cast<QFileOpenEvent *>(event));
      return true;
    } else {
      return QApplication::event(event);
    }
  }
};

#include "animation.hpp"
#include "cell impls.hpp"

template <typename CellClass>
auto getCell(const Animation &anim, const LayerIdx l, const FrameIdx f) {
  Cell *cell = anim.getCell(l, f);
  assert(cell);
  auto *derived = dynamic_cast<CellClass *>(cell);
  assert(derived);
  return derived;
};

QImage dupImage(const QImage &img) {
  QImage duplicate{img.size(), img.format()};
  std::memcpy(duplicate.bits(), img.constBits(), img.sizeInBytes());
  return duplicate;
}

#include "paint tool impls.hpp"
#include "composite.hpp"

int main(int argc, char **argv) {
  /*Image img;
  img.data.load("/Users/indikernick/Library/Developer/Xcode/DerivedData/Pixel_2-gqoblrlhvynmicgniivandqktune/Build/Products/Debug/Pixel 2.app/Contents/Resources/icon.png");
  img.xform.angle = 1;
  img.xform.posX = 3;
  img.xform.posY = 5;
  img.xform.flipX = true;
  QImage xformed = img.transformed();
  xformed.save("/Users/indikernick/Desktop/test.png");*/
  
  QImage idxImg{2, 2, QImage::Format_Indexed8};
  idxImg.detach();
  idxImg.bits()[0] = 0;
  idxImg.bits()[1] = 20;
  idxImg.bits()[4] = 40;
  idxImg.bits()[5] = 60;
  idxImg.reinterpretAsFormat(QImage::Format_Grayscale8);
  idxImg.save("/Users/indikernick/Desktop/idx_test.png");
  
  QImage loaded("/Users/indikernick/Desktop/idx_test.png");
  std::cout << (loaded.format() == QImage::Format_Grayscale8) << '\n';
  std::cout << (loaded.format() == QImage::Format_Indexed8) << '\n';
  std::cout << (loaded.format() == QImage::Format_Alpha8) << '\n';
  std::cout << loaded.format() << '\n';
  std::cout << '\n';
  
  QImage colImg(1, 1, QImage::Format_ARGB32);
  colImg.detach();
  colImg.bits()[0] = 63;
  colImg.bits()[1] = 127;
  colImg.bits()[2] = 0;
  colImg.bits()[3] = 0;
  colImg.save("/Users/indikernick/Desktop/col_test.png");
  
  const int multiplied = ((127 * 63) / 255);
  std::cout << multiplied << '\n';
  std::cout << ((multiplied * 255) / 63) << '\n';
  std::cout << '\n';
  
  QImage loadedCol("/Users/indikernick/Desktop/col_test.png");
  std::cout << (loadedCol.format() == QImage::Format_ARGB32) << '\n';
  std::cout << (loadedCol.format() == QImage::Format_ARGB32_Premultiplied) << '\n';
  std::cout << loadedCol.format() << '\n';
  std::cout << int(loadedCol.constBits()[0]) << ' '
            << int(loadedCol.constBits()[1]) << ' '
            << int(loadedCol.constBits()[2]) << ' '
            << int(loadedCol.constBits()[3]) << '\n';
  
  std::cout << "\n\n";
  
  Timer timer;
  
  timer.start("Alloc");
  QImage image{256, 256, QImage::Format_ARGB32};
  timer.stop();
  
  timer.start("Clear");
  image.fill(0);
  timer.stop();
  
  timer.start("Memset");
  std::memset(image.bits(), 0, image.sizeInBytes());
  timer.stop();
  
  timer.start("Refer");
  QImage copy = image;
  timer.stop();
  
  timer.start("Copy");
  copy.detach();
  timer.stop();
  
  timer.start("Memcpy");
  std::memcpy(copy.bits(), image.constBits(), image.sizeInBytes());
  timer.stop();
  
  timer.start("Duplicate");
  QImage dup = dupImage(image);
  timer.stop();
  
  //testComposite();
  
  SourceCell source({64, 64}, Format::color);
  source.image.xform.angle = 0;
  StrokedCircleTool tool;
  tool.setThickness(4);
  [[maybe_unused]] const bool ok = tool.attachCell(&source);
  assert(ok);
  QImage overlay({64, 64}, getImageFormat(Format::color));
  overlay.fill(0);
  
  ToolEvent event;
  event.type = ButtonType::primary;
  event.pos = QPoint{32, 32};
  event.colors.primary = qRgba(191, 63, 127, 191);
  event.overlay = &overlay;
  
  timer.start("MouseDown");
  tool.mouseDown(event);
  timer.stop();
  QImage drawing = source.image.data;
  compositeOverlay(drawing, overlay);
  drawing.save("/Users/indikernick/Desktop/overlay_0.png");
  
  event.pos = QPoint{32, 36};
  timer.start("MouseMove");
  tool.mouseMove(event);
  timer.stop();
  drawing = source.image.data;
  compositeOverlay(drawing, overlay);
  drawing.save("/Users/indikernick/Desktop/overlay_1.png");
  
  event.pos = QPoint{32, 40};
  timer.start("MouseMove");
  tool.mouseMove(event);
  timer.stop();
  drawing = source.image.data;
  compositeOverlay(drawing, overlay);
  drawing.save("/Users/indikernick/Desktop/overlay_2.png");
  
  event.pos = QPoint{32, 44};
  timer.start("MouseUp");
  tool.mouseUp(event);
  timer.stop();
  drawing = source.image.data;
  compositeOverlay(drawing, overlay);
  drawing.save("/Users/indikernick/Desktop/overlay_3.png");
  
  source.image.data.save("/Users/indikernick/Desktop/brush.png");
  
  /*QFile file{"/Users/indikernick/Desktop/project.px2"};
  
  file.open(QIODevice::WriteOnly | QIODevice::Truncate);
  Animation anim({1, 1}, Format::color);
  anim.appendTransform(0);
  anim.appendLayer();
  anim.appendDuplicate(1);
  anim.appendDuplicate(1);
  anim.appendLayer();
  anim.appendSource(2);
  auto *src = getCell<SourceCell>(anim, 2, 0);
  src->image.xform.posX = 123;
  src->image.xform.posY = 456;
  src->image.xform.angle = 1;
  src->image.xform.flipY = true;
  src->image.data.bits()[0] = 191;
  src->image.data.bits()[1] = 160;
  src->image.data.bits()[2] = 63;
  src->image.data.bits()[3] = 2;
  
  anim.serialize(&file);
  file.close();
  
  file.open(QIODevice::ReadOnly);
  Animation newAnim{&file};
  file.close();
  
  assert(newAnim.size.width() == anim.size.width());
  assert(newAnim.size.height() == anim.size.height());
  assert(newAnim.format == anim.format);
  assert(newAnim.layers.size() == anim.layers.size());
  for (LayerIdx l = 0; l != newAnim.layers.size(); ++l) {
    assert(newAnim.layers[l].size() == anim.layers[l].size());
  }
  if (newAnim.format == Format::palette) {
    assert(newAnim.palette.size() == anim.palette.size());
    assert(newAnim.palette == anim.palette);
  }
  
  auto *trans_0_0 = getCell<TransformCell>(newAnim, 0, 0);
  auto *dup_1_0 = getCell<DuplicateCell>(newAnim, 1, 0);
  auto *dup_1_1 = getCell<DuplicateCell>(newAnim, 1, 1);
  auto *src_2_0 = getCell<SourceCell>(newAnim, 2, 0);
  assert(src_2_0->image.xform.posX == src->image.xform.posX);
  assert(src_2_0->image.xform.posY == src->image.xform.posY);
  assert(src_2_0->image.xform.angle == src->image.xform.angle);
  assert(src_2_0->image.xform.flipX == src->image.xform.flipX);
  assert(src_2_0->image.xform.flipY == src->image.xform.flipY);
  assert(src_2_0->image.data.bits()[0] == src->image.data.bits()[0]);
  assert(src_2_0->image.data.bits()[1] == src->image.data.bits()[1]);
  assert(src_2_0->image.data.bits()[2] == src->image.data.bits()[2]);
  assert(src_2_0->image.data.bits()[3] == src->image.data.bits()[3]);
  */
  return 0;
  
  Application app{argc, argv};
  return app.exec();
}
