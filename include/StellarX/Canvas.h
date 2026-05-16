/*******************************************************************************
 * @类: Canvas
 * @摘要: 画布容器控件，用于分组和管理子控件
 * @描述:
 *      作为其他控件的父容器，提供统一的背景和边框样式。
 *      负责将事件传递给子控件并管理它们的绘制顺序。
 *      在托管重绘模式下，Canvas 通常作为一组子控件的安全重绘 root。
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
	COLORREF              canvasBkClor = RGB(255, 255, 255);    //背景颜色

	// 清除所有子控件
	void clearAllControls();
public:
	// 默认构造：创建一个基础矩形容器
	Canvas();
	// 指定初始位置和尺寸构造容器
	Canvas(int x, int y, int width, int height);
	~Canvas() {}

	// 修改容器运行态 X，并同步重算全部受管理子控件的世界坐标
	void setX(int x)override;
	// 修改容器运行态 Y，并同步重算全部受管理子控件的世界坐标
	void setY(int y)override;
	// 修改容器运行态宽度，并同步重算全部受管理子控件的世界坐标
	void setWidth(int width) override;
	// 修改容器运行态高度，并同步重算全部受管理子控件的世界坐标
	void setHeight(int height) override;

	//绘制容器及其子控件
	void draw() override;
	bool handleEvent(const ExMessage& msg) override;
	// 递归清理子树中的 hover / tooltip / 临时按下态，不把 Canvas 自己升级为整块重绘源。
	bool clearTransientMouseState() override;
	// 添加一个由 Canvas 托管的子控件
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
	// 托管模式下登记为 root；非托管模式下走局部或根级重绘
	void requestRepaint(Control* parent)override;    
	bool hasManagedDirtySubtree() const override;
	RECT getManagedRepaintCoverageRect() const override;
	RECT getManagedRepaintPersistentCoverageRect() const override;
	// 判断当前 Canvas 是否可安全做局部提交
	bool canCommitManagedPartialRepaint() const override; 
	// 托管收口阶段执行 Canvas 的真正重绘
	void commitManagedRepaint() override;                 
	// 获取直接子控件列表
	std::vector<std::unique_ptr<Control>>& getControls() { return controls; }
protected:
	// 统一解算后，按当前运行态矩形把所有受管理子控件重新映射到新的世界坐标。
	void applyResolvedLayoutRect(const StellarX::ResolvedLayoutRect& rect) override;
private:
	// 容器自己的几何变化后，需要统一刷新所有子控件的运行态矩形。
	void relayoutManagedChildren();
	//用来检查对话框是否模态,此控件不做实现
	bool model() const override { return false; };
};
