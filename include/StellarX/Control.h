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
	friend class Window;
	friend class Canvas;
protected:
	std::string id;     // 控件ID
	int localx, x, localy, y;           // local* 为设计态父局部坐标，x/y 为运行态世界坐标
	int localWidth, width, localHeight, height;  // local* 为设计态尺寸，width/height 为运行态尺寸
	Control* parent = nullptr;    // 父控件
	Window* hostWindow = nullptr; // 宿主窗口（顶层由 Window 注入，子控件可沿 parent 回溯）
	bool dirty = true; // 是否重绘
	bool show = true; // 是否显示
	bool eventVisualChanged = false; // 最近一次 handleEvent 是否真的引发了视觉变化（用于上层判断是否需要登记重绘）

	/* == 布局模式 == */
	StellarX::LayoutMode layoutMode = StellarX::LayoutMode::Fixed;  // 布局模式
	StellarX::Anchor anchor_1 = StellarX::Anchor::Top; // 旧版兼容锚点 1
	StellarX::Anchor anchor_2 = StellarX::Anchor::Right; // 旧版兼容锚点 2
	// 新布局模型：内部统一使用“水平轴 + 垂直轴”的规格描述，不再直接依赖旧锚点求解。
	StellarX::LayoutSpec layoutSpec{
		{ false, true, StellarX::AxisSizePolicy::FixedSize, StellarX::AxisAlignPolicy::End },
		{ true, false, StellarX::AxisSizePolicy::FixedSize, StellarX::AxisAlignPolicy::Start }
	};
	// 控件能力边界：用于限制某些控件在特定轴上是否允许 Stretch。
	StellarX::LayoutCapability layoutCapability{};

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

	// 公开几何 setter：
	// - 面向用户使用
	// - 默认只修改运行态矩形
	// - 不允许自动回写设计基线 local*
	// 若某个复合控件需要在 setter 后维护自己拥有的运行态子树一致性，
	// 由该控件在 override 中自行处理，但仍不得越权承担统一布局器职责。
	virtual void setX(int x);
	virtual void setY(int y);
	virtual void setWidth(int width);
	virtual void setHeight(int height);
public:

	virtual void draw() = 0;
	virtual bool handleEvent(const ExMessage& msg) = 0;//返回true代表事件已消费
	// 轻量清理接口：仅用于“没有机会收到本次 WM_MOUSEMOVE”的兄弟分支，
	// 清掉 hover / tooltip / 临时按下态等鼠标瞬时状态。
	// 该接口不是事件分发入口，不参与新的命中判断，也不允许回写设计基线。
	virtual bool clearTransientMouseState();
	//设置是否显示
	virtual void setIsVisible(bool show);
	//设置父容器指针
	void setParent(Control* parent) { this->parent = parent; }
	//设置宿主窗口（通常仅由顶层 Window/对话框注入）
	virtual void setHostWindow(Window* host) { this->hostWindow = host; }
	Window* getHostWindow() const;                         // 获取宿主 Window；子控件可沿 parent 向上回溯
	RECT getBoundsRect() const;                           // 获取当前控件外接矩形，用于覆盖/相交判断
	// 获取本控件“本轮实际可能写到屏幕上的覆盖范围”。
	// 默认等于控件本体矩形；像 Button 这类会额外绘制 Tooltip 的控件，可 override 后扩大范围。
	// 托管重绘 coverage、overlay 相交判断统一走这个接口，而不再默认使用控件本体 bounds。
	virtual RECT getManagedRepaintCoverageRect() const;
	// 获取会影响后续控件背景快照的“持久绘制范围”。
	// Tooltip 等临时浮层不应进入该范围，避免上层兄弟补画时把临时浮层抓进背景快照。
	virtual RECT getManagedRepaintPersistentCoverageRect() const;
	Control* getManagedRepaintRoot();                     // 找到本控件对应的托管重绘 root
	Control* getManagedRepaintDirectBranch(Control* root); // 找到“root 下面承接本次脏变化的直接子分支”
	bool hasValidBackgroundSnapshot() const { return hasSnap && saveBkImage != nullptr; } // 当前是否持有可用于局部恢复的快照
	virtual bool hasManagedDirtySubtree() const;          // 当前控件或其受管理子树中是否存在待提交的脏内容
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
	// 设置旧版布局模式（兼容入口）
	void setLayoutMode(StellarX::LayoutMode layoutMode_);
	// 设置旧版双锚点输入，并映射到内部统一 LayoutSpec
	void setAnchor(StellarX::Anchor anchor_1, StellarX::Anchor anchor_2);
	// 直接设置水平轴布局规格。
	// 调用该接口后会自动切换到 AnchorToEdges 布局模式；
	// 这是新布局模型的公开入口，后设置者会覆盖旧 setAnchor() 对水平轴的映射结果。
	void setHorizontalLayoutSpec(const StellarX::AxisLayoutSpec& spec);
	// 直接设置垂直轴布局规格。
	// 调用该接口后会自动切换到 AnchorToEdges 布局模式；
	// 这是新布局模型的公开入口，后设置者会覆盖旧 setAnchor() 对垂直轴的映射结果。
	void setVerticalLayoutSpec(const StellarX::AxisLayoutSpec& spec);
	// 设置水平轴锚定边集合（left / right）。
	void setHorizontalAnchors(bool left, bool right);
	// 设置垂直轴锚定边集合（top / bottom）。
	void setVerticalAnchors(bool top, bool bottom);
	// 设置水平轴尺寸策略（Stretch / FixedSize）。
	void setHorizontalSizePolicy(StellarX::AxisSizePolicy policy);
	// 设置垂直轴尺寸策略（Stretch / FixedSize）。
	void setVerticalSizePolicy(StellarX::AxisSizePolicy policy);
	// 设置水平轴固定尺寸位移策略（Start / End / Center / Proportional）。
	void setHorizontalAlignPolicy(StellarX::AxisAlignPolicy policy);
	// 设置垂直轴固定尺寸位移策略（Start / End / Center / Proportional）。
	void setVerticalAlignPolicy(StellarX::AxisAlignPolicy policy);
	// 获取旧版锚点 1（兼容读取入口）
	StellarX::Anchor getAnchor_1() const;
	// 获取旧版锚点 2（兼容读取入口）
	StellarX::Anchor getAnchor_2() const;
	// 获取旧版布局模式
	StellarX::LayoutMode getLayoutMode() const;
	// 获取水平轴布局规格；返回的是当前生效的新模型状态，不要求可逆回旧 anchor 语义。
	StellarX::AxisLayoutSpec getHorizontalLayoutSpec() const;
	// 获取垂直轴布局规格；返回的是当前生效的新模型状态，不要求可逆回旧 anchor 语义。
	StellarX::AxisLayoutSpec getVerticalLayoutSpec() const;
	// 获取内部统一布局规格；供 Window / Canvas 等统一解算入口使用。
	const StellarX::LayoutSpec& getLayoutSpec() const;
	// 获取控件能力边界；用于判断某个轴是否允许 Stretch。
	const StellarX::LayoutCapability& getLayoutCapability() const;
	// 显式将当前运行态矩形提交为新的设计基线。
	// 这是“运行态 -> 设计态”的受控入口之一：
	// - 普通 resize / 重排过程中不得自动调用
	// - 公开 setter 也不得隐式调用
	// - 仅用于少数需要把当前结果固化为新设计基线的场景
	void commitCurrentGeometryAsDesignRect();
protected:
	// 第 1 层：根据父设计尺寸、父当前尺寸和本控件设计矩形，解算出当前运行态局部矩形。
	StellarX::ResolvedLayoutRect resolveLayoutRect(int parentDesignW, int parentDesignH,
		int parentWorldX, int parentWorldY, int parentCurrentW, int parentCurrentH) const;
	// 内部受控路径：把统一解算结果应用到运行态矩形。不要把公开 setter 当成纯赋值接口来复用。
	virtual void applyResolvedLayoutRect(const StellarX::ResolvedLayoutRect& rect);
	// 最底层运行态赋值入口：仅修改运行态矩形，不回写 local*。
	void applyRuntimeRectDirect(int worldX, int worldY, int width, int height);
	void saveStyle();
	void restoreStyle();
	void resetEventVisualChanged() { eventVisualChanged = false; }
	void markEventVisualChanged(bool changed = true) { eventVisualChanged = changed; }
};
