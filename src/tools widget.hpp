//
//  tools widget.hpp
//  Pixel 2
//
//  Created by Indi Kernick on 10/3/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef tools_widget_hpp
#define tools_widget_hpp

#include "current tool.hpp"
#include <QtWidgets/qgroupbox.h>
#include <QtWidgets/qboxlayout.h>

class ToolWidget;

class ToolsWidget final : public QGroupBox {
  Q_OBJECT

  friend class ToolWidget;

public:
  explicit ToolsWidget(QWidget *);

Q_SIGNALS:
  void cellModified();
  void overlayModified();

public Q_SLOTS:
  void mouseDown(QPoint, ButtonType, QImage *);
  void mouseMove(QPoint, QImage *);
  void mouseUp(QPoint, ButtonType, QImage *);
  void keyPress(Qt::Key, QImage *);
  void changeCell(Cell *);
  
private:
  CurrentTool currTool;
  ToolColors colors;
  std::vector<ToolWidget *> tools;
  
  void changeTool(Tool *);
  template <typename ToolClass>
  ToolWidget *makeToolWidget(const QString &);
  
  void emitModified(ToolChanges);
  
  void paintEvent(QPaintEvent *) override;
};

#endif