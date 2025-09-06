#pragma once
#include "Control.h"
// 画布容器控件，可以作为其他控件的父容器
// 功能:
//   - 包含和管理子控件
//   - 将事件传递给子控件
//   - 提供统一的背景和边框
// 使用场景: 用于分组相关控件或实现复杂布局

class Canvas : public Control
{
private:
    std::vector<std::unique_ptr<Control>> controls;
    
    StellarX::ControlShape          shape = StellarX::ControlShape::RECTANGLE;   //容器形状
    
    StellarX::FillMode                   canvasFillMode = StellarX::FillMode::Solid;   //容器填充模式
    StellarX::LineStyle                  canvasLineStyle = StellarX::LineStyle::Solid;   //线型
    int                        canvaslinewidth = 1;             //线宽
   
    COLORREF              canvasBorderClor = RGB(0, 0, 0);//边框颜色
    COLORREF              canvasBkClor = RGB(255,255,255);    //背景颜色

public:
    Canvas(int x, int y, int width, int height);
    ~Canvas() {}
    //绘制容器及其子控件
    void draw() override;
    void handleEvent(const ExMessage& msg) override;
    //添加控件
    void addControl(std::unique_ptr<Control> control);

    //设置容器样式
    void setShape(StellarX::ControlShape shape);
    //设置容器填充模式
    void setCanvasfillMode(StellarX::FillMode mode);
    //设置容器边框颜色
    void setBorderColor(COLORREF color);
    //设置填充颜色
    void setCanvasBkColor(COLORREF color);
    //设置线形
    void setCanvasLineStyle(StellarX::LineStyle style);
    //设置线段宽度
    void setLinewidth(int width);

};

