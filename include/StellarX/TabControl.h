/*******************************************************************************
 * @类: TabControl
 * @摘要: 选项卡容器控件，管理“页签按钮 + 对应页面(Canvas)”
 * @描述:
 *     提供页签栏布局（上/下/左/右）、选中切换、页内容区域定位；
 *     与 Button 一起工作，支持窗口大小变化、可见性联动与脏区重绘。
 *     在托管重绘模式下，TabControl 作为“页签栏 + 当前页面”的统一重绘 root。
 *
 * @特性:
 *     - 页签栏四向排列（Top / Bottom / Left / Right）
 *     - 一键添加“页签+页”或为指定页添加子控件
 *     - 获取/设置当前激活页签索引
 *     - 自适应窗口变化，重算页签与页面区域
 *     - 与 Button 的 TOGGLE 模式联动显示/隐藏页面
 *
 * @使用场景: 在同一区域内承载多张页面，使用页签进行快速切换
 * @所属框架: 星垣(StellarX) GUI框架
 * @作者: 我在人间做废物
 ******************************************************************************/

#pragma once
#include "CoreTypes.h"
#include "Button.h"
#include "Canvas.h"
#define BUTMINHEIGHT 15   //页签按钮最小尺寸，过小会导致显示问题
#define BUTMINWIDTH  30   //页签按钮最小尺寸，过小会导致显示问题
class TabControl :public Canvas
{
	int tabBarHeight = BUTMINWIDTH;   //页签栏高度
	bool IsFirstDraw = true;          //首次绘制标记
	int defaultActivation = -1;       //默认激活页签索引
	StellarX::TabPlacement tabPlacement = StellarX::TabPlacement::Top; //页签排列方式
	std::vector<std::pair<std::unique_ptr<Button>, std::unique_ptr<Canvas>>> controls; //页签/页列表
	// addTab 在事务期间暂存完整的按钮/页面对，提交时保持两者的索引一致。
	std::vector<std::pair<std::unique_ptr<Button>, std::unique_ptr<Canvas>>> pendingTabs;

private:
	using Canvas::addControl;                 // 禁止外部误用
	void addControl(std::unique_ptr<Control>) = delete; // 精准禁用该重载
private:
	// 初始化页签按钮位置和尺寸
	inline void initTabBar();
	// 初始化页面 Canvas 的位置和尺寸
	inline void initTabPage();
	// 统一刷新 TabControl 当前运行态下的页签栏和页面区布局。
	void refreshRuntimeLayout();
	// 立即加入一组已校验的页签按钮和页面 Canvas，并建立内部回调联动。
	size_t addTabDirect(std::unique_ptr<Button> button, std::unique_ptr<Canvas> page);
public:
	// 默认构造：创建一个空 TabControl
	TabControl();
	// 指定初始位置和尺寸构造 TabControl
	TabControl(int x, int y, int width, int height);
	// 析构 TabControl；页签按钮和页面由 unique_ptr 自动释放。
	~TabControl();

	// 修改 TabControl 运行态 X，并刷新页签栏和页面区布局。
	void setX(int x)override;
	// 修改 TabControl 运行态 Y，并刷新页签栏和页面区布局。
	void setY(int y)override;
	// 修改 TabControl 运行态宽度，并刷新页签栏和页面区布局。
	void setWidth(int width) override;
	// 修改 TabControl 运行态高度，并刷新页签栏和页面区布局。
	void setHeight(int height) override;

	// 绘制页签栏和当前可见页面。
	void draw() override;
	// 处理页签按钮和当前页面的事件分发。
	bool handleEvent(const ExMessage& msg) override;
	// 只推进当前页按钮和页面中的时钟状态，避免不可见页签产生无效工作。
	void tick(ULONGLONG nowMs) override;
	// 只转发清理页签按钮和当前可见页面中的鼠标临时状态，不升级为整块 TabControl 重绘。
	bool clearTransientMouseState() override;

	// 添加一个“页签按钮 + 页面 Canvas”组合并返回页签索引。
	// button/page 为空时返回 size_t(-1)。事务期间返回包含 pendingTabs 的逻辑索引，
	// 因此调用方可以立即把控件追加到刚创建但尚未提交的页面中。
	size_t addTab(std::unique_ptr<Button> button, std::unique_ptr<Canvas> page);
	// 向指定页追加控件。成功时返回空指针；失败时把控件所有权原样返还调用方。
	// pageIndex 可指向已提交页面，也可指向 addTab 在同一事务中返回的 pending 逻辑索引。
	std::unique_ptr<Control> tryAddToPage(size_t pageIndex, std::unique_ptr<Control> control);
	// 按页签文本追加控件；若存在重复标题，只匹配第一个命中的页签。
	std::unique_ptr<Control> tryAddToPage(const std::string& tabText, std::unique_ptr<Control> control);
	// 设置页签位置，并重新计算页签栏和页面区布局。
	void setTabPlacement(StellarX::TabPlacement placement);
	// 设置页签栏高度；当页签在左右两侧时表示页签栏宽度。
	void setTabBarHeight(int height);
	// 设置 TabControl 可见性，并同步页签按钮和页面可见状态。
	void setIsVisible(bool visible) override;
	// 窗口尺寸变化时作废页签按钮和页面快照。
	void onWindowResize() override;
	// 获取当前激活页签索引；没有激活页时返回 -1。
	int getActiveIndex() const;
	// 设置当前激活页签索引。
	// 首次绘制前仅记录默认激活项；绘制后索引必须命中已提交页签。重复激活当前页会直接返回 true。
	bool setActiveIndex(int idx);
	// 获取页签数量
	int count() const;
	// 通过页签文本查找索引；重复标题时返回第一个命中项，未命中返回 -1。
	int indexOf(const std::string& tabText) const;
	// 设置 TabControl 及其当前页签/页面的脏状态。
	void setDirty(bool dirty) override;
	// 请求父控件重绘；托管模式下登记 TabControl 为 root。
	void requestRepaint(Control* parent)override;          // 托管模式下登记为 root；非托管模式下局部更新脏按钮/脏页面
	// 判断页签栏或当前页面中是否存在待提交的脏内容。
	bool hasManagedDirtySubtree() const override;
	// 获取页签栏和当前页面的托管重绘覆盖范围。
	RECT getManagedRepaintCoverageRect() const override;
	// 获取页签栏和当前页面的持久绘制覆盖范围。
	RECT getManagedRepaintPersistentCoverageRect() const override;
	bool canCommitManagedPartialRepaint() const override;  // 判断当前 TabControl 是否可安全做局部提交
	void commitManagedRepaint() override;                  // 托管收口阶段执行 TabControl 的真正重绘
protected:
	// 将回调期间加入的页签和页内控件在外层事务结束时一次性提交。
	void flushPendingMutations() override;
	// 外层统一解算后，TabControl 需要同步刷新其内部页签栏和页面区。
	void applyResolvedLayoutRect(const StellarX::ResolvedLayoutRect& rect) override;
};
