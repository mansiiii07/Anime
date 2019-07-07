//
//  timeline cells widget.hpp
//  Pixel 2
//
//  Created by Indi Kernick on 24/6/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef timeline_cells_widget_hpp
#define timeline_cells_widget_hpp

#include "cell.hpp"
#include "scroll bar widget.hpp"

class CellsWidget final : public QWidget {
  Q_OBJECT
  
public:
  explicit CellsWidget(QWidget *);

Q_SIGNALS:
  void resized();
  void ensureVisible(QPoint);
  
public Q_SLOTS:
  void setCurrPos(CellPos);
  void setLayer(LayerIdx, const Spans &);
  void setFrameCount(FrameIdx);
  void setLayerCount(LayerIdx);

private:
  QPixmap cellPix;
  QPixmap beginLinkPix;
  QPixmap endLinkPix;
  QImage currPosImg;
  QImage layersImg;

  void paintEvent(QPaintEvent *) override;
  void focusOutEvent(QFocusEvent *) override;
};

class CellScrollWidget final : public ScrollAreaWidget {
  Q_OBJECT
  
public:
  explicit CellScrollWidget(QWidget *);

  CellsWidget *getChild();

Q_SIGNALS:
  void rightMarginChanged(int);
  void bottomMarginChanged(int);

public Q_SLOTS:
  void contentResized();
  void ensureVisible(QPoint);

private:
  QWidget *rect = nullptr;

  void resizeEvent(QResizeEvent *) override;
};

#endif
