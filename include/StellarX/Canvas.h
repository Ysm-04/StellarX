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

/// 拥有子控件并负责递归布局、事件分发和托管重绘的画布容器。
class Canvas : public Control
{
	friend class TabControl;
	protected:
	std::vector<std::unique_ptr<Control>> controls;
	// 回调或遍历期间的增删清空先记录在队列中，防止 vector 变更使当前迭代器失效。
	enum class ControlMutationKind { Add, Remove, Clear };
	struct PendingControlMutation
	{
		ControlMutationKind kind = ControlMutationKind::Clear;
		StellarX::ControlHandle handle{};
		std::unique_ptr<Control> control;
	};
	std::vector<PendingControlMutation> pendingControlMutations;
	int mutationScopeDepth = 0;
	// 应用队列时禁止递归提交，避免回调再次加入变更造成重入。
	bool applyingControlMutations = false;

	StellarX::ControlShape               shape = StellarX::ControlShape::RECTANGLE;   //容器形状
	StellarX::FillMode          canvasFillMode = StellarX::FillMode::Solid;           //容器填充模式
	StellarX::LineStyle        canvasLineStyle = StellarX::LineStyle::Solid;          //线型
	int                        canvaslinewidth = 1;                                   //线宽

	COLORREF              canvasBorderClor = RGB(0, 0, 0);    //边框颜色
	COLORREF              canvasBkClor = RGB(255, 255, 255);    //背景颜色

public:
	// 默认构造：创建一个基础矩形容器
	Canvas();
	// 指定初始位置和尺寸构造容器
	Canvas(int x, int y, int width, int height);
	// 析构 Canvas；子控件由 unique_ptr 自动释放。
	~Canvas() {}

	// 修改容器运行态 X，并同步重算全部受管理子控件的世界坐标
	void setX(int x)override;
	// 修改容器运行态 Y，并同步重算全部受管理子控件的世界坐标
	void setY(int y)override;
	// 修改容器运行态宽度，并同步重算全部受管理子控件的世界坐标
	void setWidth(int width) override;
	// 修改容器运行态高度，并同步重算全部受管理子控件的世界坐标
	void setHeight(int height) override;

	// 绘制容器背景、边框和全部可见子控件。
	void draw() override;
	// 将鼠标和键盘事件按绘制层级分发给子控件。
	bool handleEvent(const ExMessage& msg) override;
	// 推进容器内可见子控件的时钟状态。
	void tick(ULONGLONG nowMs) override;
	// 递归清理子树中的 hover / tooltip / 临时按下态，不把 Canvas 自己升级为整块重绘源。
	bool clearTransientMouseState() override;
	// 添加一个由 Canvas 托管的子控件并返回稳定句柄。
	// 若当前处于事件/绘制/tick 事务，unique_ptr 会先进入待提交队列，避免正在遍历的 controls 失效。
	StellarX::ControlHandle addControl(std::unique_ptr<Control> control);
	// 按句柄移除子控件；事务期间延迟删除。
	// 返回 true 表示目标命中且删除请求已排队/提交；返回 false 表示句柄无效或未命中。
	bool removeControl(StellarX::ControlHandle handle);
	// 清空全部子控件；事务期间延迟到最外层安全点提交。
	void clearControls();
	// 查找当前已提交子控件；不会返回仍在 pending 队列中的对象。
	Control* findControl(StellarX::ControlHandle handle);
	// 查找当前已提交子控件的只读视图；不会返回仍在 pending 队列中的对象。
	const Control* findControl(StellarX::ControlHandle handle) const;
	// 返回当前已提交子控件句柄快照；句柄只表示身份，不保证未来仍有效。
	std::vector<StellarX::ControlHandle> getControlHandles() const;
	// 设置容器几何形状。
	void setShape(StellarX::ControlShape shape);
	// 设置容器填充模式。
	void setCanvasfillMode(StellarX::FillMode mode);
	// 设置容器边框颜色。
	void setBorderColor(COLORREF color);
	// 设置容器背景填充颜色。
	void setCanvasBkColor(COLORREF color);
	// 设置容器边框线型。
	void setCanvasLineStyle(StellarX::LineStyle style);
	// 设置容器边框线宽。
	void setLinewidth(int width);
	// 设置容器可见性，并向子控件递归同步可见状态。
	void setIsVisible(bool visible) override;
	// 设置容器脏标记，并按需递归标记子控件。
	void setDirty(bool dirty) override;
	// 窗口尺寸变化时作废容器和子树快照。
	void onWindowResize() override;
	// 托管模式下登记为 root；非托管模式下走局部或根级重绘。
	// 子控件视觉变化会提升到 Canvas root，保证背景快照和兄弟覆盖关系一致。
	void requestRepaint(Control* parent)override;
	// 判断 Canvas 或其受管理子树是否存在待提交的脏内容。
	bool hasManagedDirtySubtree() const override;
	// 获取 Canvas 托管重绘覆盖范围，包含需要随 root 一起重绘的子树区域。
	RECT getManagedRepaintCoverageRect() const override;
	// 获取 Canvas 持久绘制范围，用于判断兄弟背景快照是否需要失效。
	RECT getManagedRepaintPersistentCoverageRect() const override;
	// 判断当前 Canvas 是否可安全做局部提交
	bool canCommitManagedPartialRepaint() const override;
	// 托管收口阶段执行 Canvas 的真正重绘
	void commitManagedRepaint() override;
protected:
	// 以下接口递归解析并投递稳定焦点身份；借用的 Control* 不得逃逸当前调用栈。
	/// 查询 target 对应的焦点参与快照。
	bool queryFocusTargetSnapshot(const StellarX::FocusTargetId& target,
		StellarX::FocusTargetSnapshot& snapshot) const override;
	/// 递归收集当前已提交且有效的焦点参与节点。
	void collectFocusTargetSnapshots(
		std::vector<StellarX::FocusTargetSnapshot>& snapshots) const override;
	/// 对 target 应用或清除逻辑焦点状态。
	bool applyFocusState(const StellarX::FocusTargetId& target, bool focused,
		StellarX::FocusReason reason) override;
	/// 把键盘事件定向投递到 target，并返回该节点的路由裁决。
	bool dispatchKeyEventToTarget(const StellarX::FocusTargetId& target,
		StellarX::RoutedKeyEvent& event, StellarX::KeyRouteResult& result) override;
	/// 把正式文字输入定向投递到 target。
	bool dispatchTextInputEventToTarget(const StellarX::FocusTargetId& target,
		const StellarX::RoutedTextInputEvent& event) override;
	/// 把 IME 临时组合事件定向投递到 target。
	bool dispatchCompositionEventToTarget(const StellarX::FocusTargetId& target,
		const StellarX::RoutedCompositionEvent& event) override;
	/// 把文字选区手势定向投递到 target，并通过 accepted 返回控件裁决。
	bool dispatchTextSelectionGestureToTarget(
		const StellarX::FocusTargetId& target,
		const StellarX::RoutedTextSelectionGestureEvent& event,
		bool& accepted) override;
	/// 通知 target 暂停键盘路由并清理瞬时按键状态。
	bool dispatchKeyboardSuspensionToTarget(
		const StellarX::FocusTargetId& target) override;
	/// 通知 target 恢复键盘路由。
	bool dispatchKeyboardResumptionToTarget(
		const StellarX::FocusTargetId& target) override;
	// 进入 Canvas 子树 mutation transaction；会向父容器/Window 传播事务深度。
	void beginMutationScope() override;
	// 离开 Canvas 子树 mutation transaction；最外层结束时提交子控件队列。
	void endMutationScope() override;
	// 提交 add/remove/clear 队列，并递归提交已存在子容器的 pending mutation。
	void flushPendingMutations() override;
	// 统一解算后，按当前运行态矩形把所有受管理子控件重新映射到新的世界坐标。
	void applyResolvedLayoutRect(const StellarX::ResolvedLayoutRect& rect) override;
private:
	// 容器自己的几何变化后，需要统一刷新所有子控件的运行态矩形。
	void relayoutManagedChildren();
	// Canvas 不是模态对话框，固定返回 false。
	bool model() const override { return false; };
};
