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
#include <atomic>
#include "CoreTypes.h"

class Window;

class Control
{
	friend class Window;
	friend class Canvas;
protected:
	inline static std::atomic<std::uint64_t> nextHandleValue{ 1 };
	// 句柄随控件构造生成，容器只通过句柄排队删除，避免遍历期间直接持有失效指针。
	StellarX::ControlHandle handle{ nextHandleValue.fetch_add(1, std::memory_order_relaxed) };
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

	// 禁止拷贝构造，避免两个对象拥有同一个控件身份和背景快照。
	Control(const Control&) = delete;
	// 禁止拷贝赋值，保持控件所有权和句柄唯一性。
	Control& operator=(const Control&) = delete;
	// 禁止移动构造，避免容器外部移动后破坏父子关系和快照状态。
	Control(Control&&) = delete;
	// 禁止移动赋值，控件迁移统一通过 unique_ptr 所有权转移完成。
	Control& operator=(Control&&) = delete;

	// 构造默认控件几何，供派生类无参构造使用。
	Control() : localx(0), x(0), localy(0), y(0), localWidth(100), width(100), height(100), localHeight(100) {}
	// 构造指定设计态和运行态矩形的基础控件。
	Control(int x, int y, int width, int height)
		: localx(x), x(x), localy(y), y(y), localWidth(width), width(width), height(height), localHeight(height) {
	}
public:
	// 控件内部使用的 RAII 事务守卫。
	// 进入后子类可安全排队修改子树；析构时自动结束事务并在最外层触发提交。
	class MutationScope
	{
		// RAII 事务保证异常路径或提前 return 时也能成对结束嵌套变更范围。
		Control* owner = nullptr;
	public:
		// 进入 owner 的 mutation scope；owner 为空时为空操作。
		explicit MutationScope(Control* control) : owner(control) { if (owner) owner->beginMutationScope(); }
		// 禁止拷贝事务守卫，避免重复结束同一事务。
		MutationScope(const MutationScope&) = delete;
		// 禁止拷贝赋值事务守卫，事务所有权只能移动。
		MutationScope& operator=(const MutationScope&) = delete;
		// 移动事务守卫所有权，确保只有一个对象负责 endMutationScope。
		MutationScope(MutationScope&& other) noexcept : owner(other.owner) { other.owner = nullptr; }
		// 自动离开 owner 的 mutation scope。
		~MutationScope() { if (owner) owner->endMutationScope(); }
	};

	// 析构控件并释放可能存在的背景快照。
	virtual ~Control()
	{
		discardBackground();
	}
protected:
	// 向上请求重绘：普通路径交给父容器，托管路径则登记到 Window。
	// 子类在事件中改变可见像素后应调用它，而不是直接假设自己能立即 draw。
	virtual void requestRepaint(Control* parent);
	// 根控件/无父时触发重绘；顶层控件通常通过 Window 托管收口统一处理。
	virtual void onRequestRepaintAsRoot();
	// 当前是否处于 Window 托管分发阶段；若为真，则不应立即画。
	// 该判断用于避免事件遍历中边改状态边回贴背景导致闪烁或快照错乱。
	bool shouldDeferManagedRepaint() const;
protected:
	// 保存指定区域的背景快照，供局部重绘前后恢复使用。
	virtual void saveBackground(int x, int y, int w, int h);
	// 将已保存的背景快照回贴到屏幕。
	virtual void restBackground();
	// 回贴旧背景并释放快照
	void discardBackground();
public:
	// 仅作废快照，不回贴旧背景
	void invalidateBackgroundSnapshot();
	//“纯作废快照 + 标脏”，不再在 resize 路径里回贴旧背景
	virtual void onWindowResize();
	// 获取当前运行态 X 坐标。
	int getX() const { return x; }
	// 获取当前运行态 Y 坐标。
	int getY() const { return y; }
	// 获取当前运行态宽度。
	int getWidth() const { return width; }
	// 获取当前运行态高度。
	int getHeight() const { return height; }
	// 获取当前运行态右边界坐标。
	int getRight() const { return x + width; }
	// 获取当前运行态下边界坐标。
	int getBottom() const { return y + height; }

	// 获取设计态父局部 X 坐标。
	int getLocalX() const { return localx; }
	// 获取设计态父局部 Y 坐标。
	int getLocalY() const { return localy; }
	// 获取设计态宽度。
	int getLocalWidth() const { return localWidth; }
	// 获取设计态高度。
	int getLocalHeight() const { return localHeight; }
	// 获取设计态父局部右边界。
	int getLocalRight() const { return localx + localWidth; }
	// 获取设计态父局部下边界。
	int getLocalBottom() const { return localy + localHeight; }

	// 公开几何 setter：
	// - 面向用户使用
	// - 默认只修改运行态矩形
	// - 不允许自动回写设计基线 local*
	// 若某个复合控件需要在 setter 后维护自己拥有的运行态子树一致性，
	// 由该控件在 override 中自行处理，但仍不得越权承担统一布局器职责。
	// 修改运行态 X 坐标，不自动回写设计态 localx。
	virtual void setX(int x);
	// 修改运行态 Y 坐标，不自动回写设计态 localy。
	virtual void setY(int y);
	// 修改运行态宽度，不自动回写设计态 localWidth。
	virtual void setWidth(int width);
	// 修改运行态高度，不自动回写设计态 localHeight。
	virtual void setHeight(int height);
public:

	// 绘制入口。实现应只在 dirty/show 条件满足时写屏，并在完成后清理 dirty。
	virtual void draw() = 0;
	// 事件入口。返回 true 表示事件已消费；若事件造成视觉变化，应设置 eventVisualChanged 并请求重绘。
	virtual bool handleEvent(const ExMessage& msg) = 0;
	// 轻量清理接口：仅用于“没有机会收到本次 WM_MOUSEMOVE”的兄弟分支，
	// 清掉 hover / tooltip / 临时按下态等鼠标瞬时状态。
	// 该接口不是事件分发入口，不参与新的命中判断，也不允许回写设计基线。
	virtual bool clearTransientMouseState();
	// 由 Window 驱动的单调时钟更新；动画、延时提示等不得依赖额外鼠标消息。
	virtual void tick(ULONGLONG) {}
	// 设置控件是否显示；隐藏时派生类可同步清理快照和子控件状态。
	virtual void setIsVisible(bool show);
	// 设置父容器指针；通常由容器接管子控件时调用。
	void setParent(Control* newParent) { parent = newParent; }
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
	// 找到本控件对应的托管重绘 root；通常是直接挂在 Window 下的控件或 Dialog。
	Control* getManagedRepaintRoot();
	// 找到“root 下面承接本次脏变化的直接子分支”，用于局部提交时缩小重绘范围。
	Control* getManagedRepaintDirectBranch(Control* root);
	bool hasValidBackgroundSnapshot() const { return hasSnap && saveBkImage != nullptr; } // 当前是否持有可用于局部恢复的快照
	// 当前控件或其受管理子树中是否存在待提交的脏内容；容器需要递归检查子树。
	virtual bool hasManagedDirtySubtree() const;
	// 当前 root 是否可安全做“局部提交”而非整 root 重画；复杂容器可返回 false 升级为整 root 重绘。
	virtual bool canCommitManagedPartialRepaint() const;
	// 托管收口阶段真正执行绘制的入口；只应由 Window 的 flushManagedRepaint 调用。
	virtual void commitManagedRepaint();
	// 设置控件脏标记；true 表示下一轮需要重新绘制。
	virtual void setDirty(bool isDirty) { dirty = isDirty; }
	// 检查控件当前是否可见。
	bool IsVisible() const { return show; };
	// 获取控件类型/调试标识。
	std::string getId() const { return id; }
	// 返回稳定句柄而非对象地址；调用方不得据此推断控件当前仍在容器中。
	StellarX::ControlHandle getHandle() const noexcept { return handle; }
	// 检查控件当前是否处于待重绘状态。
	bool isDirty() { return dirty; }
	//获取控件最近一次事件处理是否引发了视觉变化
	bool didEventAffectVisual() const { return eventVisualChanged; }
	// 判断控件是否为模态对话框；普通控件应返回 false。
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
	// 容器和窗口在事件、绘制、tick 期间进入事务；具体容器在最外层结束时提交队列。
	MutationScope enterMutationScope() { return MutationScope(this); }
	// 进入子树结构变更事务。默认向父容器或宿主 Window 传播，使整棵树共享同一提交边界。
	virtual void beginMutationScope();
	// 离开子树结构变更事务。最外层结束后才允许容器提交 pending mutation。
	virtual void endMutationScope();
	// 提交子类容器自己的 pending mutation；普通叶子控件保持空实现。
	virtual void flushPendingMutations() {}
	// 第 1 层：根据父设计尺寸、父当前尺寸和本控件设计矩形，解算出当前运行态局部矩形。
	StellarX::ResolvedLayoutRect resolveLayoutRect(int parentDesignW, int parentDesignH,
		int parentWorldX, int parentWorldY, int parentCurrentW, int parentCurrentH) const;
	// 内部受控路径：把统一解算结果应用到运行态矩形。不要把公开 setter 当成纯赋值接口来复用。
	virtual void applyResolvedLayoutRect(const StellarX::ResolvedLayoutRect& rect);
	// 最底层运行态赋值入口：仅修改运行态矩形，不回写 local*。
	void applyRuntimeRectDirect(int worldX, int worldY, int width, int height);
	// 保存当前 EasyX 绘图状态，避免控件绘制污染全局状态。
	void saveStyle();
	// 恢复 saveStyle() 保存的 EasyX 绘图状态。
	void restoreStyle();
	// 清空本轮事件视觉变化标记。
	void resetEventVisualChanged() { eventVisualChanged = false; }
	// 标记本轮事件是否引发视觉变化。
	void markEventVisualChanged(bool changed = true) { eventVisualChanged = changed; }
};
