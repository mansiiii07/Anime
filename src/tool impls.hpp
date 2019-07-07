//
//  tool impls.hpp
//  Pixel 2
//
//  Created by Indi Kernick on 19/2/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef tool_impls_hpp
#define tool_impls_hpp

#include "tool.hpp"
#include "polygon.hpp"
#include "paint params.hpp"

class BrushTool final : public Tool {
public:
  ToolChanges mouseDown(const ToolMouseEvent &) override;
  ToolChanges mouseMove(const ToolMouseEvent &) override;
  ToolChanges mouseUp(const ToolMouseEvent &) override;

  void setRadius(int);
  void setMode(SymmetryMode);

private:
  QPoint lastPos = no_point;
  // @TODO restore these to 0 and none
  int radius = 0;
  SymmetryMode mode = SymmetryMode::none;
  QRgb color = 0;
  
  void symPoint(StatusMsg &, QPoint);
  bool symPoint(QImage &, QRgb, QPoint);
  bool symLine(QImage &, QRgb, QLine);
};

class FloodFillTool final : public Tool {
public:
  ToolChanges mouseDown(const ToolMouseEvent &) override;
  ToolChanges mouseMove(const ToolMouseEvent &) override;
};

class RectangleSelectTool final : public Tool {
public:
  ToolChanges mouseDown(const ToolMouseEvent &) override;
  ToolChanges mouseMove(const ToolMouseEvent &) override;
  ToolChanges mouseUp(const ToolMouseEvent &) override;
  
private:
  QPoint startPos = no_point;
  QImage selection;
  QImage overlay;
  QPoint offset;
  SelectMode mode = SelectMode::copy;
};

class PolygonSelectTool final : public Tool {
public:
  ToolChanges mouseDown(const ToolMouseEvent &) override;
  ToolChanges mouseMove(const ToolMouseEvent &) override;
  ToolChanges mouseUp(const ToolMouseEvent &) override;
  
private:
  Polygon polygon;
  QImage selection;
  QImage mask;
  QImage overlay;
  QPoint offset;
  SelectMode mode = SelectMode::copy;
};

// @TODO What if you could remove from the selection by pressing undo?

class WandSelectTool final : public Tool {
public:
  void attachCell(Cell *) override;
  void detachCell() override;
  ToolChanges mouseLeave(const ToolLeaveEvent &) override;
  ToolChanges mouseDown(const ToolMouseEvent &) override;
  ToolChanges mouseMove(const ToolMouseEvent &) override;
  ToolChanges mouseUp(const ToolMouseEvent &) override;

private:
  QImage selection;
  QImage mask;
  QImage overlay;
  QPoint offset;
  SelectMode mode = SelectMode::copy;
  
  void toggleMode(const ToolMouseEvent &);
  void addToSelection(const ToolMouseEvent &);
};

template <typename Derived>
class DragPaintTool : public Tool {
public:
  ~DragPaintTool();
  
  void attachCell(Cell *) override final;
  void detachCell() override final;
  ToolChanges mouseDown(const ToolMouseEvent &) override final;
  ToolChanges mouseMove(const ToolMouseEvent &) override final;
  ToolChanges mouseUp(const ToolMouseEvent &) override final;

protected:
  bool isDragging() const;
  QRgb getColor() const;

private:
  QPoint startPos = no_point;
  QImage cleanImage;
  QRgb color = 0;
  
  Derived *that();
};

class LineTool final : public DragPaintTool<LineTool> {
public:
  friend class DragPaintTool;

  ~LineTool();
  
  void setRadius(int);

private:
  // @TODO restore this to 0
  int radius = 2;
 
  bool drawPoint(QImage &, QPoint);
  bool drawDrag(QImage &, QPoint, QPoint);
  void drawOverlay(QImage &, QPoint);
  void updateStatus(StatusMsg &, QPoint, QPoint);
};

class StrokedCircleTool final : public DragPaintTool<StrokedCircleTool> {
public:
  friend class DragPaintTool;

  ~StrokedCircleTool();

  void setShape(CircleShape);
  void setThick(int);
  
private:
  CircleShape shape = CircleShape::c1x1;
  // @TODO restore this to 1
  int thickness = 8;

  bool drawPoint(QImage &, QPoint);
  bool drawDrag(QImage &, QPoint, QPoint);
  void drawOverlay(QImage &, QPoint);
  void updateStatus(StatusMsg &, QPoint, QPoint);
};

class FilledCircleTool final : public DragPaintTool<FilledCircleTool> {
public:
  friend class DragPaintTool;
  
  ~FilledCircleTool();
  
  void setShape(CircleShape);

private:
  CircleShape shape = CircleShape::c1x1;
  
  bool drawPoint(QImage &, QPoint);
  bool drawDrag(QImage &, QPoint, QPoint);
  void drawOverlay(QImage &, QPoint);
  void updateStatus(StatusMsg &, QPoint, QPoint);
};

class StrokedRectangleTool final : public DragPaintTool<StrokedRectangleTool> {
public:
  friend class DragPaintTool;
  
  ~StrokedRectangleTool();
  
  void setThick(int);

private:
  // @TODO restore this to 1
  int thickness = 4;

  bool drawPoint(QImage &, QPoint);
  bool drawDrag(QImage &, QPoint, QPoint);
  void drawOverlay(QImage &, QPoint);
  void updateStatus(StatusMsg &, QPoint, QPoint);
};

class FilledRectangleTool final : public DragPaintTool<FilledRectangleTool> {
public:
  friend class DragPaintTool;
  
  ~FilledRectangleTool();

private:
  bool drawPoint(QImage &, QPoint);
  bool drawDrag(QImage &, QPoint, QPoint);
  void drawOverlay(QImage &, QPoint);
  void updateStatus(StatusMsg &, QPoint, QPoint);
};

class TranslateTool final : public Tool {
public:
  void attachCell(Cell *) override;
  ToolChanges mouseDown(const ToolMouseEvent &) override;
  ToolChanges mouseMove(const ToolMouseEvent &) override;
  ToolChanges mouseUp(const ToolMouseEvent &) override;
  ToolChanges keyPress(const ToolKeyEvent &) override;

private:
  QImage cleanImage;
  QPoint lastPos = no_point;
  QPoint pos = no_point;
  bool drag = false;
  
  void translate(QPoint, QRgb);
  void updateSourceImage(QRgb);
  void updateStatus(StatusMsg &);
};

class FlipTool final : public Tool {
public:
  void attachCell(Cell *) override;
  ToolChanges mouseMove(const ToolMouseEvent &) override;
  ToolChanges keyPress(const ToolKeyEvent &) override;
  
private:
  bool flipX = false;
  bool flipY = false;
  
  void updateStatus(StatusMsg &);
};

class RotateTool final : public Tool {
public:
  void attachCell(Cell *) override;
  ToolChanges mouseMove(const ToolMouseEvent &) override;
  ToolChanges keyPress(const ToolKeyEvent &) override;

private:
  int angle = 0;
  bool square;
  
  void updateStatus(StatusMsg &);
};

#endif
