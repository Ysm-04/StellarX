/*******************************************************************************
 * @类: Control
 * @摘要: 所有控件的抽象基类，定义通用接口和基础功能
 * @描述:
 *     提供控件的基本属性和方法，包括位置、尺寸、重绘标记等。
 *     实现绘图状态保存和恢复机制，确保控件绘制不影响全局状态。
 *     同时提供“事件阶段登记、收口阶段统一提交”的托管重绘基础接口。
 *
 * @特性:
 *     - 定义控件基本属性（坐标、尺寸、脏标记）
 *     - 提供绘图状态管理（saveStyle/restoreStyle）
 *     - 声明纯虚接口（draw、handleEvent等）
 *     - 禁止移动语义，禁止拷贝语义
 *
 * @使用场景: 作为所有具体控件类的基类，不直接实例化
 * @所属框架: 星垣(StellarX) GUI框架
 * @作者: 我在人间做废物
 ******************************************************************************/
#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#ifndef WINVER
#define WINVER _WIN32_WINNT
#endif
#include <windows.h>
#include <vector>
#include <memory>
#include <easyx.h>
#undef MessageBox
#include <iostream>
#include <string>
#include <functional>
#include "CoreTypes.h"

class Window;

class Control
{
protected:
	std::string id;     // 控件ID
	int localx, x, localy, y;           // 左上角坐标
	int localWidth, width, localHeight, height;  // 控件尺寸
	Control* parent = nullptr;    // 父控件
	Window* hostWindow = nullptr; // 宿主窗口（顶层由 Window 注入，子控件可沿 parent 回溯）
	bool dirty = true; // 是否重绘
	bool show = true; // 是否显示
	bool eventVisualChanged = false; // 最近一次 handleEvent 是否真的引发了视觉变化（用于上层判断是否需要登记重绘）

	/* == 布局模式 == */
	StellarX::LayoutMode layoutMode = StellarX::LayoutMode::Fixed;  // 布局模式
	StellarX::Anchor anchor_1 = StellarX::Anchor::Top; // 锚点
	StellarX::Anchor anchor_2 = StellarX::Anchor::Right; // 锚点

	/* == 背景快照 ==  */
	std::unique_ptr<IMAGE> saveBkImage;
	int saveBkX = 0, saveBkY = 0;      // 快照保存起始坐标
	int saveWidth = 0, saveHeight = 0; // 快照保存尺寸
	bool hasSnap = false;     //  当前是否持有有效快照

	StellarX::RouRectangle rouRectangleSize; // 圆角矩形椭圆宽度和高度

	LOGFONT currentFont{};       // 保存当前字体样式和颜色
	COLORREF currentColor{};
	COLORREF currentBkColor{};  // 保存当前填充色
	COLORREF currentBorderColor{}; // 边框颜色
	LINESTYLE currentLineStyle{}; // 保存当前线型

	Control(const Control&) = delete;
	Control& operator=(const Control&) = delete;
	Control(Control&&) = delete;
	Control& operator=(Control&&) = delete;

	Control() : localx(0), x(0), localy(0), y(0), localWidth(100), width(100), height(100), localHeight(100) {}
	Control(int x, int y, int width, int height)
		: localx(x), x(x), localy(y), y(y), localWidth(width), width(width), height(height), localHeight(height) {
	}
public:

	virtual ~Control()
	{
		discardBackground();
	}
protected:
	// 向上请求重绘：普通路径交给父容器，托管路径则登记到 Window
	virtual void requestRepaint(Control* parent);
	// 根控件/无父时触发重绘
	virtual void onRequestRepaintAsRoot();
	// 当前是否处于 Window 托管分发阶段；若为真，则不应立即画
	bool shouldDeferManagedRepaint() const;
protected:
	//保存背景快照
	virtual void saveBackground(int x, int y, int w, int h);
	// putimage 回屏
	virtual void restBackground();
	// 回贴旧背景并释放快照
	void discardBackground();
public:
	// 仅作废快照，不回贴旧背景
	void invalidateBackgroundSnapshot();
	//“纯作废快照 + 标脏”，不再在 resize 路径里回贴旧背景
	virtual void onWindowResize();
	// 获取位置和尺寸
	int getX() const { return x; }
	int getY() const { return y; }
	int getWidth() const { return width; }
	int getHeight() const { return height; }
	int getRight() const { return x + width; }
	int getBottom() const { return y + height; }

	int getLocalX() const { return localx; }
	int getLocalY() const { return localy; }
	int getLocalWidth() const { return localWidth; }
	int getLocalHeight() const { return localHeight; }
	int getLocalRight() const { return localx + localWidth; }
	int getLocalBottom() const { return localy + localHeight; }

	virtual void setX(int x) { this->x = x; dirty = true; }
	virtual void setY(int y) { this->y = y; dirty = true; }
	virtual void setWidth(int width) { this->width = width; dirty = true; }
	virtual void setHeight(int height) { this->height = height; dirty = true; }
public:

	virtual void draw() = 0;
	virtual bool handleEvent(const ExMessage& msg) = 0;//返回true代表事件已消费
	//设置是否显示
	virtual void setIsVisible(bool show);
	//设置父容器指针
	void setParent(Control* parent) { this->parent = parent; }
	//设置宿主窗口（通常仅由顶层 Window/对话框注入）
	virtual void setHostWindow(Window* host) { this->hostWindow = host; }
	Window* getHostWindow() const;                         // 获取宿主 Window；子控件可沿 parent 向上回溯
	RECT getBoundsRect() const;                           // 获取当前控件外接矩形，用于覆盖/相交判断
	Control* getManagedRepaintRoot();                     // 找到本控件对应的托管重绘 root
	bool hasValidBackgroundSnapshot() const { return hasSnap && saveBkImage != nullptr; } // 当前是否持有可用于局部恢复的快照
	virtual bool canCommitManagedPartialRepaint() const; // 当前 root 是否可安全做“局部提交”而非整 root 重画
	virtual void commitManagedRepaint();                  // 托管收口阶段真正执行绘制的入口
	//设置是否重绘
	virtual void setDirty(bool dirty) { this->dirty = dirty; }
	//检查控件是否可见
	bool IsVisible() const { return show; };
	//获取控件id
	std::string getId() const { return id; }
	//检查是否为脏
	bool isDirty() { return dirty; }
	//获取控件最近一次事件处理是否引发了视觉变化
	bool didEventAffectVisual() const { return eventVisualChanged; }
	//用来检查对话框是否模态，其他控件不用实现
	virtual bool model()const = 0;
	//布局
	void setLayoutMode(StellarX::LayoutMode layoutMode_);
	void setAnchor(StellarX::Anchor anchor_1, StellarX::Anchor anchor_2);
	StellarX::Anchor getAnchor_1() const;
	StellarX::Anchor getAnchor_2() const;
	StellarX::LayoutMode getLayoutMode() const;
protected:
	void saveStyle();
	void restoreStyle();
	void resetEventVisualChanged() { eventVisualChanged = false; }
	void markEventVisualChanged(bool changed = true) { eventVisualChanged = changed; }
};
