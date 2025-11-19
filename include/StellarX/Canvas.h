/*******************************************************************************
 * @类: Canvas
 * @摘要: 画布容器控件，用于分组和管理子控件
 * @描述:
 *      作为其他控件的父容器，提供统一的背景和边框样式。
 *      负责将事件传递给子控件并管理它们的绘制顺序。
 *
 * @特性:
 *     - 支持四种矩形形状（普通、圆角，各有边框和无边框版本）
 *     - 可自定义填充模式、边框颜色和背景颜色
 *     - 自动管理子控件的生命周期和事件传递
 *     - 支持嵌套容器结构
 *
 * @使用场景: 用于分组相关控件、实现复杂布局或作为对话框基础
 * @所属框架: 星垣(StellarX) GUI框架
 * @作者: 我在人间做废物
 *******************************************************************************/

#pragma once
#include "Control.h"
#include"Table.h"

class Canvas : public Control
{
protected:
    std::vector<std::unique_ptr<Control>> controls;
    
    StellarX::ControlShape               shape = StellarX::ControlShape::RECTANGLE;   //容器形状
    StellarX::FillMode          canvasFillMode = StellarX::FillMode::Solid;           //容器填充模式
    StellarX::LineStyle        canvasLineStyle = StellarX::LineStyle::Solid;          //线型
    int                        canvaslinewidth = 1;                                   //线宽
   
    COLORREF              canvasBorderClor = RGB(0, 0, 0);    //边框颜色
    COLORREF              canvasBkClor = RGB(255,255,255);    //背景颜色


    // 清除所有子控件
	void clearAllControls();
public:
    Canvas();
    Canvas(int x, int y, int width, int height);
    ~Canvas() {}

    //绘制容器及其子控件
    void draw() override;
    bool handleEvent(const ExMessage& msg) override;
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
    //设置不可见后传递给子控件重写
    void setIsVisible(bool visible) override;
    void setDirty(bool dirty) override;
    void onWindowResize() override;
    void requestRepaint(Control* parent)override;
    //获取子控件列表
    std::vector<std::unique_ptr<Control>>& getControls() { return controls; }
private:
    //用来检查对话框是否模态,此控件不做实现
    bool model() const override { return false; };
};

