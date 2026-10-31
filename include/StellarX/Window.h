/**
 * Window（头文件）
 *
 * 设计目标：
 *  - 提供一个基于 Win32 + EasyX 的“可拉伸且稳定不抖”的窗口容器。
 *  - 通过消息过程子类化（WndProcThunk）接管关键消息（WM_SIZING/WM_SIZE/...）。
 *  - 将“几何变化记录（pendingW/H）”与“统一收口重绘（needResizeDirty）”解耦。
 *  - 在事件分发阶段只改状态并登记重绘 root，在事件尾统一提交，减少分离绘制造成的闪烁。
 *
 * 关键点（与 .cpp 中实现对应）：
 *  - isSizing：处于交互拉伸阶段时，冻结重绘；松手后统一重绘，防止抖动。
 *  - WM_SIZING：只做“最小尺寸夹紧”，不回滚矩形、不做对齐；把其余交给系统。
 *  - WM_GETMINMAXINFO：按最小“客户区”换算到“窗口矩形”，提供系统层最小轨迹值。
 *  - runEventLoop：输入事件先分发给对话框/控件，控件只登记重绘请求；真正绘制在分发或 resize 收口时统一处理。
 */
#pragma once

#include "Control.h"
#include "FocusManager.h"
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <windows.h>

class Window;
class Dialog;

namespace StellarX::Internal
{
	class NativeInputContext;
	class ImeCaretAnchorContext;
	class KeyboardRouter;
	class TextInputRouter;
	class CompositionRouter;
	class TextSelectionGestureManager;
	struct NativeInputRuntimeSnapshot;
	struct ImeCaretAnchorRuntimeSnapshot;
	enum class ImeCandidatePositionMode : std::uint8_t;
	enum class ImeCandidateDismissMode : std::uint8_t;
	enum class ImeInputContextGateMode : std::uint8_t;
	enum class KeyboardPipelineMode : std::uint8_t;
	enum class PointerPipelineMode : std::uint8_t;
	// 仅供测试读取指定窗口的原生输入运行状态，不暴露控件裸指针。
	NativeInputRuntimeSnapshot inspectWindowNativeInputForTesting(const ::Window& window) noexcept;
	// 仅供测试读取指定窗口的 IME 锚点与定位统计快照。
	ImeCaretAnchorRuntimeSnapshot inspectWindowImeCaretAnchorForTesting(
		const ::Window& window) noexcept;
	// 仅供测试在事件循环锁定前选择候选窗定位策略。
	bool configureImeCandidatePositionModeForTesting(
		::Window& window, ImeCandidatePositionMode mode) noexcept;
	// 仅供测试在事件循环锁定前选择候选窗关闭策略。
	bool configureImeCandidateDismissModeForTesting(
		::Window& window, ImeCandidateDismissMode mode) noexcept;
	// 仅供测试在事件循环锁定前选择 HIMC 关联门控策略。
	bool configureImeInputContextGateModeForTesting(
		::Window& window, ImeInputContextGateMode mode) noexcept;
	// 仅供测试在窗口启动前选择键盘业务管线。
	bool configureKeyboardPipelineForTesting(
		::Window& window, KeyboardPipelineMode mode) noexcept;
	// 返回窗口当前选择的键盘业务管线。
	KeyboardPipelineMode keyboardPipelineMode(
		const ::Window& window) noexcept;
	// 锁定键盘业务管线，防止事件循环运行期间切换造成重复投递。
	void lockKeyboardPipeline(::Window& window) noexcept;
	// 仅供测试在窗口启动前选择 Pointer 业务管线。
	bool configurePointerPipelineForTesting(
		::Window& window, PointerPipelineMode mode) noexcept;
	// 返回窗口当前选择的 Pointer 业务管线。
	PointerPipelineMode pointerPipelineMode(
		const ::Window& window) noexcept;
	// 锁定 Pointer 业务管线，防止事件循环运行期间切换造成重复投递。
	void lockPointerPipeline(::Window& window) noexcept;
	// 根据当前活动态与焦点能力生成 WM_GETDLGCODE 的附加标志。
	LRESULT dialogCodeAdditionsForQuery(const ::Window& window,
		WPARAM wParam, LPARAM lParam) noexcept;
}

namespace StellarX
{
	class FocusManager;
}

/// StellarX 顶层窗口，拥有控件树并协调原生输入、布局与托管重绘。
class Window
{
	friend class Dialog;
	friend class StellarX::FocusManager;
	friend StellarX::Internal::NativeInputRuntimeSnapshot
		StellarX::Internal::inspectWindowNativeInputForTesting(const Window& window) noexcept;
	friend StellarX::Internal::ImeCaretAnchorRuntimeSnapshot
		StellarX::Internal::inspectWindowImeCaretAnchorForTesting(
			const Window& window) noexcept;
	friend bool StellarX::Internal::configureImeCandidatePositionModeForTesting(
		Window& window, StellarX::Internal::ImeCandidatePositionMode mode) noexcept;
	friend bool StellarX::Internal::configureImeCandidateDismissModeForTesting(
		Window& window, StellarX::Internal::ImeCandidateDismissMode mode) noexcept;
	friend bool StellarX::Internal::configureImeInputContextGateModeForTesting(
		Window& window, StellarX::Internal::ImeInputContextGateMode mode) noexcept;
	friend bool StellarX::Internal::configureKeyboardPipelineForTesting(
		Window& window, StellarX::Internal::KeyboardPipelineMode mode) noexcept;
	friend StellarX::Internal::KeyboardPipelineMode
		StellarX::Internal::keyboardPipelineMode(const Window& window) noexcept;
	friend void StellarX::Internal::lockKeyboardPipeline(Window& window) noexcept;
	friend bool StellarX::Internal::configurePointerPipelineForTesting(
		Window& window, StellarX::Internal::PointerPipelineMode mode) noexcept;
	friend StellarX::Internal::PointerPipelineMode
		StellarX::Internal::pointerPipelineMode(const Window& window) noexcept;
	friend void StellarX::Internal::lockPointerPipeline(Window& window) noexcept;
	friend LRESULT StellarX::Internal::dialogCodeAdditionsForQuery(
		const Window& window, WPARAM wParam, LPARAM lParam) noexcept;
	friend class StellarX::Internal::KeyboardRouter;
	friend class StellarX::Internal::TextInputRouter;
	friend class StellarX::Internal::CompositionRouter;
	friend class StellarX::Internal::TextSelectionGestureManager;

	// —— 尺寸状态 ——（绘制尺寸与待应用尺寸分离；收口时一次性更新）
	int           width;                 // 当前有效宽（已应用到画布/控件的客户区宽）
	int           height;                // 当前有效高（已应用到画布/控件的客户区高）
	int           localwidth;            // 基准宽（创建时的宽度）
	int           localheight;           // 基准高（创建是的高度）
	int           pendingW;              // 待应用宽（WM_SIZE/拉伸中记录用）
	int           pendingH;              // 待应用高
	int           minClientW;            // 业务设定的最小客户区宽（用于 GETMINMAXINFO 与 SIZING 夹紧）
	int           minClientH;            // 业务设定的最小客户区高
	int           windowMode = NULL;     // EasyX 初始化模式（EX_SHOWCONSOLE/EX_TOPMOST/...）
	bool          needResizeDirty = false; // 统一收口重绘标志（置位后在事件环末尾处理）
	bool          isSizing = false;      // 是否处于拖拽阶段（ENTER/EXIT SIZEMOVE 切换）

	// —— 原生窗口句柄与子类化钩子 ——（子类化 EasyX 的窗口过程以拦截关键消息）
	HWND          hWnd = NULL;           // EasyX 初始化后的窗口句柄
	WNDPROC       oldWndProc = nullptr;  // 保存旧过程（CallWindowProc 回落）
	bool          procHooked = false;    // 避免重复子类化
	// EasyX 的 WndProc 与宿主事件循环分属不同线程；只有 WndProc 完成 WM_NCDESTROY
	// 转发后才发布 false，避免宿主提前析构仍在执行 WndProcThunk 的 Window 对象。
	std::atomic<bool> nativeWindowAlive{ false };
	// 原生输入在宿主安全点逐条交给 KeyboardRouter；兼容管线只供启动前的定向测试。
	std::unique_ptr<StellarX::Internal::NativeInputContext> nativeInputContext;
	// 宿主线程发布焦点文字叶节点的客户区 caret；只有 EasyX HWND 属主线程
	// 读取该快照并调用 IMM32。
	std::unique_ptr<StellarX::Internal::ImeCaretAnchorContext> imeCaretAnchorContext;
	// 仅宿主线程访问的定向选区状态。在 HWND 客户区拖选期间只保留稳定身份，
	// 不保留 TextBox 指针。
	std::unique_ptr<StellarX::Internal::TextSelectionGestureManager>
		textSelectionGestureManager;
	bool          nativeInputFailureReported = false;
	std::uint64_t nativeInputRejectedTextReported = 0;
	std::uint64_t nativeInputImeQueryFailuresReported = 0;
	std::uint64_t nativeInputDiscardedFragmentsReported = 0;
	std::uint64_t nativeInputPointerCoordinateFailuresReported = 0;
	// 键盘业务默认使用定向路由。WndProc 还会同步读取该状态回答
	// WM_GETDLGCODE，因此该跨线程快照必须使用原子变量。
	std::atomic<bool> routedKeyboardPipeline{ true };
	bool keyboardPipelineLocked = false;
	// WM_GETDLGCODE 使用的同步跨线程快照。最高位记录当前原生激活状态，
	// 低位复用 Win32 DLGC_* 能力值。
	static constexpr std::uint32_t DialogCodeWindowActiveFlag = 1u << 31;
	std::atomic<std::uint32_t> dialogCodeCapabilitySnapshot{
		DialogCodeWindowActiveFlag | DLGC_WANTTAB };
	// Pointer 业务默认使用定向路由。EasyX EX_MOUSE 只继续排空，
	// NativeInputQueue 是唯一业务来源。
	std::atomic<bool> routedPointerPipeline{ true };
	bool pointerPipelineLocked = false;
	// 宿主线程诊断计数：EX_KEY/EX_MOUSE 必须持续排空，定向路由启用时
	// 对应的兼容业务交付必须严格为 0。
	std::uint64_t easyXKeyboardMessagesDrained = 0;
	std::uint64_t legacyEasyXKeyboardBusinessDeliveries = 0;
	std::uint64_t easyXPointerMessagesDrained = 0;
	std::uint64_t legacyEasyXPointerBusinessDeliveries = 0;
	std::uint64_t nativePointerBusinessDeliveries = 0;
	// 定向文字与组合路由诊断；计数器只保留数量，绝不保留文字内容。
	StellarX::TextInputRoutingStats textInputRoutingStats{};
	StellarX::CompositionRoutingStats compositionRoutingStats{};
	struct CompositionRouteSession
	{
		std::uint64_t compositionId = 0;
		StellarX::FocusTargetId target{};
		StellarX::FocusScopeId scope{};
		std::uint64_t activationGeneration = 0;
		bool committed = false;
		bool resultDelivered = false;
		// 原生 IME 轮次仍存活时，逻辑焦点已经离开绑定目标。退休 id 只能由
		// Cancel/End 收口，绝不能惰性绑定到后来获得焦点的 TextBox。
		bool focusRetired = false;

		explicit operator bool() const noexcept
		{
			return compositionId != 0 && static_cast<bool>(target);
		}
	};
	CompositionRouteSession compositionRouteSession{};
	// 逻辑焦点离开文字目标时退休旧组合轮次，并拒绝迟到提交。
	void retireImeCompositionForLogicalFocusChange(
		StellarX::FocusTargetId oldTarget) noexcept;
	// 判断 EasyX 原生窗口是否仍然存在且允许继续进行属主线程操作。
	bool hasLiveNativeWindow() const noexcept;
	// 发布窗口激活态，供 WM_GETDLGCODE 和输入门控读取。
	void publishDialogCodeActivation(bool active) noexcept;
	// 发布当前焦点是否需要方向键和字符消息。
	void publishDialogCodeFocusCapabilities(
		bool wantArrows, bool wantChars) noexcept;
	// 发布当前文本焦点的客户区 caret 锚点。
	void publishImeCaretAnchor(
		const StellarX::ImeCaretAnchorSnapshot& snapshot) noexcept;
	// 发布当前焦点是否由框架内联绘制 IME 临时组合串。
	void publishImeInlineCompositionOwnership(bool owned) noexcept;
	// 发布当前焦点是否允许关联 HIMC 并接收正式文字。
	void publishImeTextInputEligibility(bool eligible) noexcept;
	// 从最新焦点快照重新计算并发布原生对话框代码能力。
	void refreshDialogCodeFocusCapabilities() noexcept;
	// 在宿主安全点排空规范化输入队列；模态抑制时只维护状态而不交付底层业务。
	bool drainNativeInputAtSafePoint(bool modalBusinessSuppressed = false,
		const bool* continueRouting = nullptr);
	// 把规范化 Pointer 转回既有 ExMessage 入口并执行托管业务分发。
	void dispatchManagedBusinessMessage(const ExMessage& message);
	static LRESULT CALLBACK WndProcThunk(HWND h, UINT m, WPARAM w, LPARAM l); // 静态过程分发到 this

	// —— 绘制相关 ——（是否使用合成双缓冲、窗口标题、背景等）
	bool          useComposited = true;  // 是否启用 WS_EX_COMPOSITED（部分机器可能增加一帧观感延迟）
	std::string   headline;              // 窗口标题文本
	COLORREF      wBkcolor = BLACK;      // 纯色背景（无背景图时使用）
	std::unique_ptr<IMAGE> background;  // 背景图对象指针（存在时优先绘制）
	std::string   bkImageFile;           // 背景图文件路径（loadimage 用）

	// —— 控件/对话框 ——（容器内的普通控件与非模态对话框）
	std::vector<std::unique_ptr<Control>> controls; // 普通顶层控件；绘制顺序也决定层级顺序
	std::vector<std::unique_ptr<Control>> dialogs;  // 非模态对话框；始终位于普通控件之上
	// 栈上模态 Dialog 不进入 owning dialogs；只在 Dialog::Show() 的严格
	// RAII 生命周期内登记借用指针，按 LIFO 支持嵌套模态解析。
	std::vector<Dialog*> modalDialogStack;
	class ModalDialogRegistration
	{
		Window* owner = nullptr;
		Dialog* dialog = nullptr;
		friend class Window;
		ModalDialogRegistration(Window& owner, Dialog& dialog) noexcept;
	public:
		// 创建不持有任何模态对话框的空登记守卫。
		ModalDialogRegistration() = default;
		// 登记守卫独占一次栈登记，不允许复制。
		ModalDialogRegistration(const ModalDialogRegistration&) = delete;
		// 登记守卫独占一次栈登记，不允许复制赋值。
		ModalDialogRegistration& operator=(const ModalDialogRegistration&) = delete;
		// 转移模态栈登记责任，源守卫随即失效。
		ModalDialogRegistration(ModalDialogRegistration&& other) noexcept;
		// 释放当前登记后接管另一守卫的模态栈登记责任。
		ModalDialogRegistration& operator=(ModalDialogRegistration&& other) noexcept;
		// 若仍持有登记，则按 LIFO 约束从宿主模态栈注销。
		~ModalDialogRegistration();
		// 查询守卫当前是否持有有效的宿主与对话框登记。
		explicit operator bool() const noexcept { return owner != nullptr && dialog != nullptr; }
	};
	// 非模态 Dialog 采用单活动作用域：新建 Dialog 等首次 draw 完成 chrome
	// 物化后再激活；其它可见 Dialog 仍可鼠标命中，但不混入根 Tab 顺序。
	StellarX::FocusTargetId activeNonModalDialogRoot{};
	StellarX::FocusScopeId activeNonModalDialogScope{};
	StellarX::FocusTargetId pendingNonModalDialogRoot{};
	bool nonModalDialogScopeNeedsReconcile = false;
	std::unique_ptr<StellarX::FocusManager> focusManager; // Window 内逻辑焦点唯一所有者
	// 顶层控件的结构变更与事件分发解耦，防止回调直接改动正在遍历的 controls。
	// add/remove/clear 在事件、绘制和回调期间只入队；最外层 mutation scope 结束后再提交。
	enum class ControlMutationKind { Add, Remove, Clear };
	struct PendingControlMutation
	{
		ControlMutationKind kind = ControlMutationKind::Clear;
		StellarX::ControlHandle handle{};
		std::unique_ptr<Control> control;
	};
	std::vector<PendingControlMutation> pendingControlMutations;
	int mutationScopeDepth = 0;         // 允许嵌套的结构变更事务深度
	// 队列提交期间保持该标志，避免提交过程中再次递归提交同一队列。
	bool applyingControlMutations = false;
	bool fullSceneRedrawRequested = false; // 非局部变化请求整场景重绘；在事件收口安全点消费
	bool          managedDispatchActive = false; // 事件分发期：控件只改状态并登记重绘，不立即画
	int           managedRepaintCommitDepth = 0; // 显式 flush 提交深度；只放开绘制，不放开结构变更
	bool          managedSceneDirty = false;     // 当前分发轮次是否已经登记了至少一笔托管重绘请求
	struct ManagedRepaintItem  // 托管重绘项：记录由哪个控件发起、需要重绘的根控件和覆盖范围（用于后续判断哪些对话框需要补画）
	{
		Control* root = nullptr;      // 顶层重绘根（直接挂在 Window 下的控件，或 Dialog 自身）
		RECT     coverage{};          // 本轮脏区覆盖范围；用于判断哪些上层 Dialog 需要补画
	};
	std::vector<ManagedRepaintItem> managedRepaintItems; // 本轮事件分发累计的重绘项

public:
	bool dialogClose = false;            // 项目内使用的状态位,对话框关闭标志
	mutable bool dialogOpen = false;     // 项目内使用的状态位,对话框打开标志

	// —— 构造/析构 ——（仅初始化成员；实际样式与子类化在 draw() 中完成）
	// 创建纯色背景窗口
	Window(int width, int height, int mode);
	// 创建指定背景色窗口
	Window(int width, int height, int mode, COLORREF bkcloc);
	// 创建带标题与背景色的窗口
	Window(int width, int height, int mode, COLORREF bkcloc, std::string headline);
	// 析构窗口并释放背景、控件与子类化资源
	~Window();

	// —— 绘制与事件循环 ——（draw* 完成一次全量绘制；runEventLoop 驱动事件与统一收口）
	void draw();                         // 以纯色背景执行一次全量绘制
	void draw(std::string pImgFile);     // 以背景图执行一次全量绘制
	int  runEventLoop();                 // 主事件循环（PeekMessage + 统一收口重绘）

	// —— 背景/标题设置 ——（更换背景、背景色与标题；立即触发一次批量绘制）
	void setBkImage(std::string pImgFile); // 设置窗口背景图
	void setBkcolor(COLORREF c);           // 设置窗口纯色背景
	void setHeadline(std::string headline); // 设置窗口标题

	// —— 控件/对话框管理 ——（添加到容器，或做存在性判断）
	// 添加普通顶层控件并返回稳定句柄。
	// 在事件回调、绘制或其它 mutation transaction 中调用时，control 会先进入待提交队列；
	// 调用方仍可立即保存返回句柄，但对象要到最外层事务结束后才真正出现在 controls 中。
	StellarX::ControlHandle addControl(std::unique_ptr<Control> control);
	// 按句柄移除顶层控件；事务期间延迟删除，避免遍历中的对象或迭代器失效。
	// 返回 true 表示目标命中且删除请求已排队/提交；返回 false 表示句柄无效或未命中。
	bool removeControl(StellarX::ControlHandle handle);
	// 清空所有普通顶层控件；事务期间延迟到安全点提交。
	void clearControls();
	// 查找当前已提交的控件；不会暴露 pending 队列中的对象。
	Control* findControl(StellarX::ControlHandle handle);
	// 查找当前已提交控件的只读视图；不会暴露 pending 队列中的对象。
	const Control* findControl(StellarX::ControlHandle handle) const;
	// 返回当前已提交控件的句柄快照；调用方不得据此推断控件之后仍然存在。
	std::vector<StellarX::ControlHandle> getControlHandles() const;
	// 获取当前 Window 的通用逻辑焦点管理器。
	StellarX::FocusManager& getFocusManager();
	// 获取当前 Window 的只读逻辑焦点管理器。
	const StellarX::FocusManager& getFocusManager() const;
	// 添加非模态对话框；Window 接管生命周期并在事件安全点清理关闭对象。
	void addDialog(std::unique_ptr<Control> dialogs);
	//非模态对话框去重
	bool hasNonModalDialogWithCaption(const std::string& caption, const std::string& message) const; // 检查是否已有同 caption/message 的非模态对话框

	// —— 访问器 ——（只读接口，供外部查询当前窗口/标题/背景等）
	HWND        getHwnd() const;         // 获取窗口句柄
	int         getWidth() const;        // 获取当前有效客户区宽度
	int         getHeight() const;       // 获取当前有效客户区高度
	int         getPendingWidth() const; // 获取待应用宽度
	int         getPendingHeight() const; // 获取待应用高度
	std::string getHeadline() const;     // 获取窗口标题
	COLORREF    getBkcolor() const;      // 获取纯色背景
	IMAGE* getBkImage() const;           // 获取背景图对象
	std::string getBkImageFile() const;  // 获取背景图路径

	// —— 尺寸调整 / 托管重绘 ——（事件阶段登记，收口阶段提交）

	// 执行一次 resize 收口 + 统一重绘；只在 pending 尺寸或全场景脏标记存在时真正绘制。
	void pumpResizeIfNeeded();
	// 供模态 Dialog 上报宿主窗口尺寸变化；只记录待应用尺寸，实际布局/重绘由窗口收口统一完成。
	void scheduleResizeFromModal(int w, int h);
	// 当前是否处于“事件只改状态，不立即画”的阶段。
	// 控件在该阶段应请求托管重绘，而不是直接回贴背景或立即 draw。
	bool isManagedDispatchActive() const;
	// 当前是否已经进入 Window::flushManagedRepaint 的显式绘制提交阶段。
	// 该状态只允许托管 root 真正写屏；mutation transaction 仍保持活动，
	// add/remove/clear 等结构变更继续排队，不能借此提前提交。
	bool isManagedRepaintCommitActive() const;
	// 记录一笔由 source 发起的托管重绘请求。
	// Window 会提升到对应 root，并在事件收口时合并 coverage 后统一提交。
	void requestManagedRepaint(Control* source);
	// 记录一笔托管重绘请求，并显式并入状态清理前的旧覆盖范围。
	// 用于 hover/tooltip 等状态切换，保证旧区域和新区域都会被清理。
	void requestManagedRepaint(Control* source, const RECT& previousCoverage);
	// 在事件收口阶段按普通控件、非模态 Dialog、模态栈顺序提交 root 重绘；
	// 同时补画覆盖在脏区之上的更高层控件或 Dialog。
	void flushManagedRepaint();
	// 进入结构变更事务。允许嵌套，调用者必须保证 begin/end 成对。
	// 事务内的顶层控件变更只入队，不立即修改 controls/dialogs。
	void beginMutationScope();
	// 离开结构变更事务。最外层退出且当前不在托管分发阶段时，会提交整棵控件树的 pending mutation。
	void endMutationScope();
	// 判断当前是否处于不允许立即改动控件树的阶段，包括事件分发、显式事务或队列提交过程。
	bool isMutationTransactionActive() const;
	// 请求下一安全点执行整场景重绘；用于 Dialog 关闭、全局覆盖关系变化等无法安全局部修复的场景。
	void requestFullSceneRedraw();
private:
	// FocusManager 专用的递归焦点参与协议；不复用只查直接子级的 findControl()。
	bool queryFocusTargetSnapshot(const StellarX::FocusTargetId& target,
		StellarX::FocusTargetSnapshot& snapshot) const;
	void collectFocusTargetSnapshots(
		std::vector<StellarX::FocusTargetSnapshot>& snapshots) const;
	bool applyFocusState(const StellarX::FocusTargetId& target, bool focused,
		StellarX::FocusReason reason);
	// KeyboardRouter 专用：在当前调用栈内按稳定身份重新解析并立即投递，
	// 不把借用的 Control* 传出本函数。
	bool dispatchKeyEventToTarget(const StellarX::FocusTargetId& target,
		StellarX::RoutedKeyEvent& event, StellarX::KeyRouteResult& result);
	bool dispatchTextInputEventToTarget(const StellarX::FocusTargetId& target,
		const StellarX::RoutedTextInputEvent& event);
	bool dispatchCompositionEventToTarget(
		const StellarX::FocusTargetId& target,
		const StellarX::RoutedCompositionEvent& event);
	bool dispatchTextSelectionGestureToTarget(
		const StellarX::FocusTargetId& target,
		const StellarX::RoutedTextSelectionGestureEvent& event,
		bool& accepted);
	bool dispatchKeyboardSuspensionToTarget(
		const StellarX::FocusTargetId& target);
	bool dispatchKeyboardResumptionToTarget(
		const StellarX::FocusTargetId& target);
	// 从按键开始时冻结的身份路径中提取原始 Dialog 根；冒泡后只允许
	// 对同一身份执行默认动作，禁止查询“此刻活动 Dialog”造成串台。
	StellarX::FocusTargetId dialogRootOnFocusPath(
		const StellarX::FocusTargetId* path, std::size_t pathLength) const;
	bool dispatchDialogFrameworkDefault(
		StellarX::FocusTargetId dialogRoot, const StellarX::KeyEvent& key);
	// 提交 Window 顶层控件队列；只允许在最外层事务安全点调用。
	void flushPendingControlMutations();
	// Dialog::Show() 专用临时注册；registration 析构时严格按 LIFO 注销。
	ModalDialogRegistration registerModalDialog(Dialog& dialog);
	// 从模态栈注销指定 Dialog，并恢复下一层有效作用域。
	void unregisterModalDialog(Dialog& dialog) noexcept;
	// 非模态 Dialog 的单活动 FocusScope 协调。Dialog 是 Window friend，
	// 只能在 Show/draw/Close 的明确生命周期点调用这些入口。
	// 记录一次非模态 Dialog 激活请求，等待安全点提交。
	void requestNonModalDialogActivation(Dialog& dialog);
	// 在结构稳定后提交指定非模态 Dialog 的焦点作用域。
	void commitPendingNonModalDialogActivation(Dialog& dialog);
	// 非模态 Dialog 关闭时退休其作用域和待处理激活请求。
	void onNonModalDialogClosed(StellarX::ControlHandle dialogHandle);
	// 按稳定根身份查找仍由 Window 持有的非模态 Dialog。
	Dialog* findNonModalDialog(StellarX::FocusTargetId root) const;
	// 返回当前最上层且可见的非模态 Dialog。
	Dialog* topmostVisibleNonModalDialog() const;
	// 按客户区坐标命中最上层可交互的非模态 Dialog。
	Dialog* hitTestTopmostNonModalDialog(int x, int y) const;
	// 立即激活已经提交的非模态 Dialog 焦点作用域。
	bool activateNonModalDialog(Dialog& dialog);
	// 停用当前非模态 Dialog 作用域；可选择同时清理待处理请求。
	void deactivateNonModalDialog(bool clearPending);
	// 根据当前可见 Dialog 树修正非模态作用域与恢复焦点。
	void reconcileNonModalDialogScope();
	// 自顶向下提交 Window、Canvas、TabControl 等容器中累积的 pending mutation。
	void flushPendingMutationsInTree();
	// 将全部普通控件和非模态对话框标脏，用于强制全场景重绘前统一失效缓存。
	void markAllControlsDirty();
	// 根据最终窗口尺寸和控件布局规格，解算并应用控件运行态矩形。
	void adaptiveLayout(std::unique_ptr<Control>& c, const int finalH, const int finalW);
	// resize / 初次绘制 / 对话框开关这类全局场景的整场景重绘
	void redrawScene(bool forceControlsDirty, bool forceDialogsDirty);
	// 绘制窗口背景；存在背景图时优先贴图，否则填充纯色背景。
	void drawWindowBackground();
	// 合成 WM_MOUSEMOVE，用于同步底层 hover 状态
	void dispatchSyntheticMouseMoveToControls(short x, short y);
	enum class OverlayMouseSyncMode
	{
		ClearBehindOverlay, // overlay 吞掉鼠标移动：只清理底层 hover / tooltip，不重新命中底层控件
		RestoreAtCursor    // overlay 关闭：按当前鼠标位置刷新底层 hover 状态
	};
	// overlay 层级变化后，同步底层鼠标瞬时状态；内部仍复用既有 synthetic WM_MOUSEMOVE 路径
	void syncMouseStateAfterOverlayChanged(OverlayMouseSyncMode mode);
	// 在事件安全点清理已关闭的非模态 Dialog，避免 dialogs 容器长期积累无效对象。
	// 清理可能触发全场景重绘请求，因此必须放在遍历之外。
	void sweepClosedDialogs();
	// 事件回调可能改脏其他 root；flush 前补收集这些跨 root 脏子树
	void collectDirtyRootsForManagedRepaint();
	// 清空本轮托管重绘登记
	void clearManagedRepaintState();
};
