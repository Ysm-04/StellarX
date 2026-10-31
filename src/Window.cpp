#include "Window.h"
#include "Dialog.h"
#include"SxLog.h"
#include "SxEventLoopInternal.h"
#include "SxImeCaretAnchorInternal.h"
#include "SxInputProbeInternal.h"
#include "SxNativeInputInternal.h"
#include "SxKeyboardRouterInternal.h"
#include "SxTextInputRouterInternal.h"
#include "SxTextSelectionGestureInternal.h"
#include <easyx.h>
#include <algorithm>
#include <cstdint>
#include <new>
// 可能频繁出现且对调试信息干扰较大的消息（例如鼠标移动），
// 可以在日志输出时特殊处理以减少干扰。
static bool SxIsNoisyMsg(UINT m)
{
	return m == WM_MOUSEMOVE;
}

static const char* SxMsgName(UINT m)
{
	switch (m)
	{
	case WM_MOUSEMOVE:   return "WM_MOUSEMOVE";
	case WM_LBUTTONDOWN: return "WM_LBUTTONDOWN";
	case WM_LBUTTONUP:   return "WM_LBUTTONUP";
	case WM_RBUTTONDOWN: return "WM_RBUTTONDOWN";
	case WM_RBUTTONUP:   return "WM_RBUTTONUP";
	case WM_KEYDOWN:     return "WM_KEYDOWN";
	case WM_KEYUP:       return "WM_KEYUP";
	case WM_CHAR:        return "WM_CHAR";
	case WM_SIZE:        return "WM_SIZE";
	default:             return "WM_?";
	}
}

static bool SxRectsIntersect(const RECT& a, const RECT& b)
{
	return a.left < b.right && a.right > b.left &&
		a.top < b.bottom && a.bottom > b.top;
}

static RECT SxUnionRect(const RECT& a, const RECT& b)
{
	RECT out{};
	out.left = (std::min)(a.left, b.left);
	out.top = (std::min)(a.top, b.top);
	out.right = (std::max)(a.right, b.right);
	out.bottom = (std::max)(a.bottom, b.bottom);
	return out;
}

static bool SxMakeEasyXPointerMessage(
	const StellarX::PointerEvent& pointer, ExMessage& message) noexcept
{
	message = {};
	if (!pointer.clientCoordinatesValid ||
		pointer.action == StellarX::PointerAction::CaptureChanged)
		return false;

	switch (pointer.action)
	{
	case StellarX::PointerAction::Move: message.message = WM_MOUSEMOVE; break;
	case StellarX::PointerAction::LeftDown: message.message = WM_LBUTTONDOWN; break;
	case StellarX::PointerAction::LeftUp: message.message = WM_LBUTTONUP; break;
	case StellarX::PointerAction::LeftDoubleClick: message.message = WM_LBUTTONDBLCLK; break;
	case StellarX::PointerAction::RightDown: message.message = WM_RBUTTONDOWN; break;
	case StellarX::PointerAction::RightUp: message.message = WM_RBUTTONUP; break;
	case StellarX::PointerAction::RightDoubleClick: message.message = WM_RBUTTONDBLCLK; break;
	case StellarX::PointerAction::MiddleDown: message.message = WM_MBUTTONDOWN; break;
	case StellarX::PointerAction::MiddleUp: message.message = WM_MBUTTONUP; break;
	case StellarX::PointerAction::MiddleDoubleClick: message.message = WM_MBUTTONDBLCLK; break;
	case StellarX::PointerAction::VerticalWheel: message.message = WM_MOUSEWHEEL; break;
	case StellarX::PointerAction::HorizontalWheel: message.message = WM_MOUSEHWHEEL; break;
	case StellarX::PointerAction::CaptureChanged:
		return false;
	}

	message.x = static_cast<short>(pointer.clientX);
	message.y = static_cast<short>(pointer.clientY);
	message.wheel = static_cast<short>(pointer.wheelDelta);
	message.lbutton = pointer.leftButton;
	message.mbutton = pointer.middleButton;
	message.rbutton = pointer.rightButton;
	message.ctrl = pointer.control;
	message.shift = pointer.shift;
	return true;
}

static bool SxIsSingleLineTextVirtualKey(WPARAM key) noexcept
{
	if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'Z') ||
		(key >= VK_NUMPAD0 && key <= VK_NUMPAD9))
		return true;
	switch (key)
	{
	case VK_SPACE:
	case VK_MULTIPLY:
	case VK_ADD:
	case VK_SEPARATOR:
	case VK_SUBTRACT:
	case VK_DECIMAL:
	case VK_DIVIDE:
	case VK_OEM_1:
	case VK_OEM_PLUS:
	case VK_OEM_COMMA:
	case VK_OEM_MINUS:
	case VK_OEM_PERIOD:
	case VK_OEM_2:
	case VK_OEM_3:
	case VK_OEM_4:
	case VK_OEM_5:
	case VK_OEM_6:
	case VK_OEM_7:
	case VK_OEM_8:
	case VK_OEM_102:
		return true;
	default:
		return false;
	}
}

static bool SxIsSingleLineTextCharacter(WPARAM character) noexcept
{
	return character >= 0x20u && character != 0x7fu &&
		character != UNICODE_NOCHAR;
}

namespace StellarX::Internal
{
	bool configureKeyboardPipelineForTesting(
		::Window& window, KeyboardPipelineMode mode) noexcept
	{
		if (window.keyboardPipelineLocked)
			return false;
		window.routedKeyboardPipeline.store(
			mode == KeyboardPipelineMode::Routed, std::memory_order_release);
		window.refreshDialogCodeFocusCapabilities();
		return true;
	}

	KeyboardPipelineMode keyboardPipelineMode(
		const ::Window& window) noexcept
	{
		return window.routedKeyboardPipeline.load(std::memory_order_acquire)
			? KeyboardPipelineMode::Routed
			: KeyboardPipelineMode::LegacyCompatibility;
	}

	void lockKeyboardPipeline(::Window& window) noexcept
	{
		window.keyboardPipelineLocked = true;
	}

	bool configurePointerPipelineForTesting(
		::Window& window, PointerPipelineMode mode) noexcept
	{
		if (window.pointerPipelineLocked)
			return false;
		window.routedPointerPipeline.store(
			mode == PointerPipelineMode::Routed, std::memory_order_release);
		return true;
	}

	PointerPipelineMode pointerPipelineMode(
		const ::Window& window) noexcept
	{
		return window.routedPointerPipeline.load(std::memory_order_acquire)
			? PointerPipelineMode::Routed
			: PointerPipelineMode::LegacyCompatibility;
	}

	void lockPointerPipeline(::Window& window) noexcept
	{
		window.pointerPipelineLocked = true;
	}

	bool shouldDeliverLegacyEasyXMessage(
		const ::Window& window, UINT message) noexcept
	{
		if (isEasyXKeyboardMessage(message))
			return keyboardPipelineMode(window) == KeyboardPipelineMode::LegacyCompatibility;
		if (isEasyXPointerMessage(message))
			return pointerPipelineMode(window) == PointerPipelineMode::LegacyCompatibility;
		return true;
	}

	LRESULT dialogCodeAdditionsForQuery(const ::Window& window,
		WPARAM wParam, LPARAM lParam) noexcept
	{
		std::uint32_t capabilities = window.dialogCodeCapabilitySnapshot.load(
			std::memory_order_acquire);
		if ((capabilities & ::Window::DialogCodeWindowActiveFlag) == 0)
			capabilities &= ~(DLGC_WANTARROWS | DLGC_WANTCHARS);

		auto available = [&](std::uint32_t value) noexcept -> LRESULT
		{
			return static_cast<LRESULT>(capabilities & value);
		};

		if (lParam == 0)
		{
			if (wParam == VK_TAB)
				return available(DLGC_WANTTAB);
			if (wParam == VK_LEFT || wParam == VK_RIGHT)
				return available(DLGC_WANTARROWS);
			if (SxIsSingleLineTextVirtualKey(wParam))
				return available(DLGC_WANTCHARS);
			return 0;
		}

		const MSG* current = reinterpret_cast<const MSG*>(lParam);
		switch (current->message)
		{
		case WM_KEYDOWN:
			if (current->wParam != wParam)
				return 0;
			// 第 29 位表示 Alt 上下文。即使合成消息被标为 WM_KEYDOWN 而不是
			// WM_SYSKEYDOWN，它仍属于系统键输入。
			if ((static_cast<ULONG_PTR>(current->lParam) & (1ull << 29)) != 0)
				return 0;
			if (wParam == VK_TAB)
				return available(DLGC_WANTTAB);
			if (wParam == VK_LEFT || wParam == VK_RIGHT)
				return available(DLGC_WANTARROWS);
			if (SxIsSingleLineTextVirtualKey(wParam))
				return available(DLGC_WANTCHARS);
			return 0;
		case WM_CHAR:
		case WM_DEADCHAR:
		case WM_UNICHAR:
			// WM_GETDLGCODE.wParam 保留触发查询的虚拟键，嵌套 MSG 则携带翻译后的字符，
			// 例如由 '1' 得到的 '!'。
			return SxIsSingleLineTextCharacter(current->wParam)
				? available(DLGC_WANTCHARS) : 0;
		default:
			return 0;
		}
	}

	bool shouldAddTabDialogCode(const ::Window& window,
		WPARAM wParam, LPARAM lParam) noexcept
	{
		return (dialogCodeAdditionsForQuery(window, wParam, lParam) &
			DLGC_WANTTAB) != 0;
	}
}

Window::ModalDialogRegistration::ModalDialogRegistration(
	Window& ownerWindow, Dialog& registeredDialog) noexcept
	: owner(&ownerWindow), dialog(&registeredDialog)
{
}

Window::ModalDialogRegistration::ModalDialogRegistration(
	ModalDialogRegistration&& other) noexcept
	: owner(other.owner), dialog(other.dialog)
{
	other.owner = nullptr;
	other.dialog = nullptr;
}

Window::ModalDialogRegistration& Window::ModalDialogRegistration::operator=(
	ModalDialogRegistration&& other) noexcept
{
	if (this == &other)
		return *this;
	if (owner && dialog)
		owner->unregisterModalDialog(*dialog);
	owner = other.owner;
	dialog = other.dialog;
	other.owner = nullptr;
	other.dialog = nullptr;
	return *this;
}

Window::ModalDialogRegistration::~ModalDialogRegistration()
{
	if (owner && dialog)
		owner->unregisterModalDialog(*dialog);
}

bool Window::isManagedDispatchActive() const
{
	return managedDispatchActive;
}

bool Window::isManagedRepaintCommitActive() const
{
	return managedRepaintCommitDepth > 0;
}

void Window::beginMutationScope()
{
	// Window 是顶层 mutation 边界；嵌套事务只增加深度，不立即提交任何结构变更。
	++mutationScopeDepth;
}

void Window::endMutationScope()
{
	// 只有最外层事务结束后才允许提交队列，保证事件遍历和回调链中的对象地址稳定。
	if (mutationScopeDepth > 0)
		--mutationScopeDepth;
	if (mutationScopeDepth == 0 && !managedDispatchActive)
		flushPendingMutationsInTree();
}

bool Window::isMutationTransactionActive() const
{
	// applyingControlMutations 也视作事务，避免提交队列时因回调递归修改同一容器。
	return managedDispatchActive || mutationScopeDepth > 0 || applyingControlMutations;
}

void Window::requestFullSceneRedraw()
{
	// 非局部变化只置位，不在调用点立即绘制；主循环安全点会统一消费该请求。
	fullSceneRedrawRequested = true;
}

void Window::flushPendingControlMutations()
{
	// Window 是顶层所有权边界；这里只在最外层事务结束后修改 controls，保证回调遍历安全。
	if (applyingControlMutations || pendingControlMutations.empty())
		return;
	applyingControlMutations = true;
	bool changed = false;
	for (auto& mutation : pendingControlMutations)
	{
		switch (mutation.kind)
		{
		case ControlMutationKind::Add:
			if (mutation.control)
			{
				mutation.control->setHostWindow(this);
				controls.push_back(std::move(mutation.control));
				changed = true;
			}
			break;
		case ControlMutationKind::Remove:
		{
			auto it = std::find_if(controls.begin(), controls.end(), [&](const auto& item)
				{ return item && item->getHandle() == mutation.handle; });
			if (it != controls.end())
			{
				controls.erase(it);
				changed = true;
			}
			break;
		}
		case ControlMutationKind::Clear:
			if (!controls.empty())
			{
				controls.clear();
				changed = true;
			}
			break;
		}
	}
	pendingControlMutations.clear();
	applyingControlMutations = false;
	if (changed)
	{
		// 顶层结构变化会影响 z-order 和所有背景快照，因此不能只重画单个旧 root。
		clearManagedRepaintState();
		fullSceneRedrawRequested = true;
	}
}

void Window::flushPendingMutationsInTree()
{
	flushPendingControlMutations();
	for (auto& control : controls)
		if (control) control->flushPendingMutations();
	for (auto& dialog : dialogs)
		if (dialog) dialog->flushPendingMutations();
	for (Dialog* dialog : modalDialogStack)
		if (dialog) dialog->flushPendingMutations();
	if (focusManager)
		focusManager->onMutationCommitted();
}

void Window::markAllControlsDirty()
{
	for (auto& control : controls)
		if (control) control->setDirty(true);
}

/**
 * requestManagedRepaint(source)
 * 作用：在“事件分发期”登记一笔托管重绘请求，而不是立即绘制。
 * 关键点：
 *  - source 是真正发生视觉变化的控件；
 *  - root 是后续真正安全重绘的最小层级（通常是顶层控件/容器，或 Dialog 自身）；
 *  - coverage 记录这次变化影响的范围，用于判断哪些上层 Dialog 需要补画。
 */
void Window::requestManagedRepaint(Control* source)
{
	if (!source)
		return;
	requestManagedRepaint(source, source->getManagedRepaintCoverageRect());
}

void Window::requestManagedRepaint(Control* source, const RECT& previousCoverage)
{
	if (!source)
		return;

	managedSceneDirty = true;
	Control* root = source->getManagedRepaintRoot();
	if (!root)
		return;

	RECT coverage = previousCoverage;
	if (root->canCommitManagedPartialRepaint())
	{
		// 对支持局部提交的 root，coverage 不能再盯着最深处的 source；
		// 否则像“三层 Canvas 里的按钮变色”这种情况，只会登记成一个很小的叶子矩形，
		// 顶层 root 提交时既容易漏掉那条直接脏分支，也会低估后续 overlay 补画范围。
		Control* branch = source->getManagedRepaintDirectBranch(root);
		coverage = SxUnionRect(coverage, branch ? branch->getManagedRepaintCoverageRect() : source->getManagedRepaintCoverageRect());
		const RECT sourceCoverage = source->getManagedRepaintCoverageRect();
		coverage = SxUnionRect(coverage, sourceCoverage);
	}
	else
	{
		coverage = SxUnionRect(coverage, root->getManagedRepaintCoverageRect());
	}

	for (auto& item : managedRepaintItems)
	{
		if (item.root == root)
		{
			item.coverage = SxUnionRect(item.coverage, coverage);
			return;
		}
	}

	ManagedRepaintItem item;
	item.root = root;
	item.coverage = coverage;
	managedRepaintItems.push_back(item);
}

void Window::collectDirtyRootsForManagedRepaint()
{
	auto isAlreadyTracked = [this](Control* root)
		{
			for (const auto& item : managedRepaintItems)
			{
				if (item.root == root)
					return true;
			}
			return false;
		};

	auto collectRoot = [this, &isAlreadyTracked](Control* root)
		{
			if (!root || !root->IsVisible())
				return;
			if (!root->hasManagedDirtySubtree())
				return;
			if (isAlreadyTracked(root))
				return;

			// 事件回调可以修改另一个顶层 root 的控件，例如右侧页签按钮更新左侧状态 Label。
			// 这种跨 root 改脏不会经过当前事件分发链的 requestManagedRepaint()，
			// 因此在 flush 前补登记为 root 级重绘，保证本轮事件尾统一提交。
			ManagedRepaintItem item;
			item.root = root;
			item.coverage = root->getManagedRepaintCoverageRect();
			managedRepaintItems.push_back(item);
			managedSceneDirty = true;
		};

	for (auto& control : controls)
		collectRoot(control.get());
	for (auto& dialog : dialogs)
		collectRoot(dialog.get());
	for (Dialog* dialog : modalDialogStack)
		collectRoot(dialog);
}

// 清空本轮托管重绘状态；通常在 flush/全场景重绘/resize 收口后调用
void Window::clearManagedRepaintState()
{
	managedSceneDirty = false;
	managedRepaintItems.clear();
}

void Window::drawWindowBackground()
{
	if (!bkImageFile.empty())
	{
		if (!background || background->getwidth() != width || background->getheight() != height)
		{
			background = std::make_unique<IMAGE>();
			loadimage(background.get(), bkImageFile.c_str(), width, height, true);
		}
		putimage(0, 0, background.get());
	}
	else
	{
		setbkcolor(wBkcolor);
		cleardevice();
	}
}

void Window::redrawScene(bool forceControlsDirty, bool forceDialogsDirty)
{
	// 全场景重绘是 resize、对话框层级变化和结构提交的保守收口路径。
	beginMutationScope();
	drawWindowBackground();

	for (auto& c : controls)
	{
		if (forceControlsDirty)
			c->setDirty(true);
		c->draw();
	}
	for (auto& d : dialogs)
	{
		if (forceDialogsDirty && d->IsVisible())
			d->setDirty(true);
		d->draw();
	}
	endMutationScope();
}

/**
 * flushManagedRepaint()
 * 作用：提交当前事件分发阶段累计的托管重绘请求。
 * 提交顺序：
 *  1）把普通控件、非模态 Dialog、模态 Dialog 栈拼成完整 z-order；
 *  2）按 z-order 提交受影响的 root；
 *  3）每提交一个 root，都把与其实际 coverage 相交的更高层单元依次补画回来。
 * 说明：
 *  - 这里不做整场景重画，而是只画本轮登记的 root；
 *  - 模态 Dialog 虽不在 Window::dialogs owning 容器内，但注册期间仍是最高层托管 root；
 *  - 之所以按完整 z-order 提交，而不是按登记顺序，是为了保持现有覆盖关系。
 */
void Window::flushManagedRepaint()
{
	if (!managedSceneDirty || !hWnd)
		return;

	// 模态 Dialog 可能从外层 Button 回调中同步重入。此时外层 managed dispatch
	// 与 mutation scope 会在整个 Show() 生命周期内保持活动；若 commit 内部再次
	// 经过 requestRepaint()，就会被二次延期，最终在本函数清理登记时永远无法写屏。
	// 显式提交阶段只绕过“是否允许画”的判断，isMutationTransactionActive() 不变，
	// 因而控件树的 add/remove/clear 仍然只能排队到真正的事务安全点。
	struct ManagedRepaintCommitScope
	{
		int& depth;
		explicit ManagedRepaintCommitScope(int& value) : depth(value) { ++depth; }
		~ManagedRepaintCommitScope() { --depth; }
	};
	ManagedRepaintCommitScope commitScope(managedRepaintCommitDepth);

	BeginBatchDraw();
	auto unionCoverage = [](RECT& lhs, const RECT& rhs)
		{
			lhs.left = (std::min)(lhs.left, rhs.left);
			lhs.top = (std::min)(lhs.top, rhs.top);
			lhs.right = (std::max)(lhs.right, rhs.right);
			lhs.bottom = (std::max)(lhs.bottom, rhs.bottom);
		};

	auto redrawOverlayUnit = [](Control* unit)
		{
			if (!unit || !unit->IsVisible())
				return;
			// overlay 补画不是“沿用旧快照再贴回去”，而是重新站到当前顶层场景上合成一遍。
			unit->invalidateBackgroundSnapshot();
			unit->setDirty(true);
			unit->draw();
		};

	std::vector<Control*> zOrder;
	zOrder.reserve(controls.size() + dialogs.size() + modalDialogStack.size());
	for (auto& control : controls)
		if (control) zOrder.push_back(control.get());
	for (auto& dialog : dialogs)
		if (dialog) zOrder.push_back(dialog.get());
	for (Dialog* dialog : modalDialogStack)
		if (dialog) zOrder.push_back(dialog);

	auto processManagedRoot = [&](Control* root, const RECT& initialCoverage,
		std::size_t rootIndex)
		{
			if (!root || !root->IsVisible())
				return;

			root->commitManagedRepaint();
			RECT workingCoverage = initialCoverage;

			if (root->canCommitManagedPartialRepaint())
			{
				// 当前 Canvas / TabControl 的局部提交会在内部按兄弟覆盖关系继续扩张实际写屏区域，
				// 但 commitManagedRepaint() 还没有返回“本轮实际 coverage”的接口。
				// 为避免 Window 仍按初始叶子区域判断，漏补上层普通控件或 Dialog，
				// 这里对可局部提交的 root 使用 root 当前覆盖范围做保守兜底。
				// 这只扩大“上层是否需要补画”的判断，不改变 root 本身的局部提交策略。
				unionCoverage(workingCoverage, root->getManagedRepaintCoverageRect());
			}

			// overlay 补画必须是传递式的：如果 A 重画覆盖了更高层 B，
			// B 的补画范围又触及 C，就继续把 C 补回。完整 z-order 也保证
			// 非模态 Dialog 不会盖住模态 Dialog，外层模态不会盖住内层模态。
			for (std::size_t i = rootIndex + 1; i < zOrder.size(); ++i)
			{
				Control* current = zOrder[i];
				if (!current || !current->IsVisible())
					continue;
				const RECT currentCoverage = current->getManagedRepaintCoverageRect();
				if (!SxRectsIntersect(currentCoverage, workingCoverage))
					continue;

				redrawOverlayUnit(current);
				unionCoverage(workingCoverage, current->getManagedRepaintCoverageRect());
			}
		};

	for (std::size_t index = 0; index < zOrder.size(); ++index)
	{
		for (auto& item : managedRepaintItems)
		{
			if (item.root == zOrder[index])
			{
				processManagedRoot(item.root, item.coverage, index);
				break;
			}
		}
	}

	EndBatchDraw();
	clearManagedRepaintState();
}

// 合成一条 WM_MOUSEMOVE 并直接分发给 Window 顶层控件；常用于同步 hover 状态
void Window::dispatchSyntheticMouseMoveToControls(short x, short y)
{
	ExMessage mm{};
	mm.message = WM_MOUSEMOVE;
	mm.x = x;
	mm.y = y;
	for (auto it = controls.rbegin(); it != controls.rend(); ++it)
		(*it)->handleEvent(mm);
}

void Window::syncMouseStateAfterOverlayChanged(OverlayMouseSyncMode mode)
{
	short syncX = -32768;
	short syncY = -32768;

	if (mode == OverlayMouseSyncMode::RestoreAtCursor)
	{
		POINT pt{};
		if (!GetCursorPos(&pt))
			return;
		if (!ScreenToClient(this->hWnd, &pt))
			return;
		syncX = static_cast<short>(pt.x);
		syncY = static_cast<short>(pt.y);
	}

	// synthetic move 仍走既有事件路径，但必须临时进入托管分发模式：
	// 控件只更新 hover / tooltip / click 等瞬时状态并登记重绘，真正绘制留到事件尾统一收口。
	const bool oldManagedDispatchActive = managedDispatchActive;
	managedDispatchActive = true;
	dispatchSyntheticMouseMoveToControls(syncX, syncY);
	managedDispatchActive = oldManagedDispatchActive;
}

void Window::sweepClosedDialogs()
{
	for (auto it = dialogs.begin(); it != dialogs.end();)
	{
		Dialog* dialog = dynamic_cast<Dialog*>(it->get());
		if (!dialog || dialog->IsVisible())
		{
			++it;
			continue;
		}

		// Dialog::Close() 只负责关闭语义和结果回调；
		// 真正的快照回贴、子控件清理和对象移除必须放到事件安全点完成，
		// 避免在 dialogs 正被倒序分发或绘制遍历时 erase 导致迭代器失效。
		if (dialog->pendingCleanup && !dialog->isCleaning)
			dialog->performDelayedCleanup();

		if (!dialog->pendingCleanup && !dialog->isCleaning)
			it = dialogs.erase(it);
		else
			++it;
	}
}

/**
 * ApplyResizableStyle
 * 作用：统一设置可拉伸/裁剪样式，并按开关使用 WS_EX_COMPOSITED（合成双缓冲）。
 * 关键点：
 *  - WS_THICKFRAME：允许从四边/四角拖动改变尺寸。
 *  - WS_CLIPCHILDREN / WS_CLIPSIBLINGS：避免子控件互相覆盖时闪烁。
 *  - WS_EX_COMPOSITED：在一些环境更平滑，但个别显卡/驱动可能带来一帧延迟感。
 *  - SWP_FRAMECHANGED：通知窗口样式已变更，强制系统重算非客户区（标题栏/边框）。
 */
static void ApplyResizableStyle(HWND h, bool useComposited)
{
	LONG style = GetWindowLong(h, GWL_STYLE);
	style |= WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	SetWindowLong(h, GWL_STYLE, style);

	LONG ex = GetWindowLong(h, GWL_EXSTYLE);
	if (useComposited)
	{
		ex |= WS_EX_COMPOSITED;
	}
	else
	{
		ex &= ~WS_EX_COMPOSITED;
	}
	SetWindowLong(h, GWL_EXSTYLE, ex);

	SetWindowPos(h, NULL, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

/**
 * ApplyMinSizeOnSizing
 * 作用：在 WM_SIZING 阶段执行“最小尺寸夹紧”。
 * 规则：只回推“被拖动的那一侧”，另一侧当锚点（避免几何回弹/位置漂移）。
 * 步骤：
 *  1）将“最小客户区尺寸”通过 AdjustWindowRectEx 换算为“最小窗口矩形”（含非客户区）。
 *  2）若当前矩形比最小还小，则根据 edge（哪条边/角在被拖）调整对应边，另一侧保持不动。
 * 说明：仅保证不小于最小值；不做对齐/回滚等操作，把其余交给系统尺寸逻辑。
 */
static void ApplyMinSizeOnSizing(RECT* prc, WPARAM edge, HWND hWnd, int minClientW, int minClientH)
{
	RECT rcFrame{ 0, 0, minClientW, minClientH };
	DWORD style = GetWindowLong(hWnd, GWL_STYLE);
	DWORD ex = GetWindowLong(hWnd, GWL_EXSTYLE);
	AdjustWindowRectEx(&rcFrame, style, FALSE, ex);

	const int minW = rcFrame.right - rcFrame.left;
	const int minH = rcFrame.bottom - rcFrame.top;

	const int curW = prc->right - prc->left;
	const int curH = prc->bottom - prc->top;

	if (curW < minW)
	{
		switch (edge)
		{
		case WMSZ_LEFT:
		case WMSZ_TOPLEFT:
		case WMSZ_BOTTOMLEFT:
			prc->left = prc->right - minW;   // 锚定右侧，回推左侧（左边被拖）
			break;
		default:
			prc->right = prc->left + minW;   // 锚定左侧，回推右侧（右边被拖）
			break;
		}
	}

	if (curH < minH)
	{
		switch (edge)
		{
		case WMSZ_TOP:
		case WMSZ_TOPLEFT:
		case WMSZ_TOPRIGHT:
			prc->top = prc->bottom - minH;   // 锚定下侧，回推上侧（上边被拖）
			break;
		default:
			prc->bottom = prc->top + minH;   // 锚定上侧，回推下侧（下边被拖）
			break;
		}
	}
}

// ---------------- 构造 / 析构 ----------------

/**
 * 构造：初始化当前尺寸、待应用尺寸、最小客户区尺寸与 EasyX 模式。
 * 注意：样式设置与子类化放在 draw() 内第一次绘制时完成。
 */
Window::Window(int w, int h, int mode)
{
	localwidth = minClientW = pendingW = width = w;
	localheight = minClientH = pendingH = height = h;
	windowMode = mode;
	imeCaretAnchorContext =
		std::make_unique<StellarX::Internal::ImeCaretAnchorContext>();
	textSelectionGestureManager = std::make_unique<
		StellarX::Internal::TextSelectionGestureManager>();
	focusManager = std::make_unique<StellarX::FocusManager>(*this);
}

Window::Window(int w, int h, int mode, COLORREF bk)
{
	localwidth = minClientW = pendingW = width = w;
	localheight = minClientH = pendingH = height = h;
	windowMode = mode;
	wBkcolor = bk;
	imeCaretAnchorContext =
		std::make_unique<StellarX::Internal::ImeCaretAnchorContext>();
	textSelectionGestureManager = std::make_unique<
		StellarX::Internal::TextSelectionGestureManager>();
	focusManager = std::make_unique<StellarX::FocusManager>(*this);
}

Window::Window(int w, int h, int mode, COLORREF bk, std::string title)
{
	localwidth = minClientW = pendingW = width = w;
	localheight = minClientH = pendingH = height = h;
	windowMode = mode;
	wBkcolor = bk;
	headline = std::move(title);
	imeCaretAnchorContext =
		std::make_unique<StellarX::Internal::ImeCaretAnchorContext>();
	textSelectionGestureManager = std::make_unique<
		StellarX::Internal::TextSelectionGestureManager>();
	focusManager = std::make_unique<StellarX::FocusManager>(*this);
}

void Window::publishDialogCodeActivation(bool active) noexcept
{
	if (imeCaretAnchorContext)
		imeCaretAnchorContext->setWindowActive(active);
	std::uint32_t current = dialogCodeCapabilitySnapshot.load(
		std::memory_order_acquire);
	for (;;)
	{
		std::uint32_t desired = current &
			~(DialogCodeWindowActiveFlag | DLGC_WANTARROWS | DLGC_WANTCHARS);
		if (active)
			desired |= DialogCodeWindowActiveFlag;
		if (dialogCodeCapabilitySnapshot.compare_exchange_weak(
			current, desired, std::memory_order_acq_rel,
			std::memory_order_acquire))
			return;
	}
}

void Window::publishImeCaretAnchor(
	const StellarX::ImeCaretAnchorSnapshot& snapshot) noexcept
{
	if (imeCaretAnchorContext)
		imeCaretAnchorContext->publish(snapshot);
}

void Window::publishImeInlineCompositionOwnership(bool owned) noexcept
{
	if (imeCaretAnchorContext)
		imeCaretAnchorContext->setInlineCompositionOwned(owned);
}

void Window::publishImeTextInputEligibility(bool eligible) noexcept
{
	if (imeCaretAnchorContext)
		imeCaretAnchorContext->setTextInputEligible(eligible);
}

void Window::retireImeCompositionForLogicalFocusChange(
	StellarX::FocusTargetId oldTarget) noexcept
{
	if (!oldTarget || !imeCaretAnchorContext)
		return;

	bool requestNativeCancel = false;
	if (compositionRouteSession &&
		compositionRouteSession.target == oldTarget)
	{
		compositionRouteSession.focusRetired = true;
		requestNativeCancel = !compositionRouteSession.committed;
	}
	else if (!compositionRouteSession)
	{
		// HWND 属主线程可能已进入组合，而对应队列事件尚未在宿主线程建立路由会话。
		requestNativeCancel = true;
	}

	if (requestNativeCancel)
		(void)imeCaretAnchorContext->requestCompositionCancel();
}

void Window::publishDialogCodeFocusCapabilities(
	bool wantArrows, bool wantChars) noexcept
{
	std::uint32_t current = dialogCodeCapabilitySnapshot.load(
		std::memory_order_acquire);
	for (;;)
	{
		std::uint32_t desired = current &
			~(DLGC_WANTTAB | DLGC_WANTARROWS | DLGC_WANTCHARS);
		const bool routed = routedKeyboardPipeline.load(
			std::memory_order_acquire);
		if (routed)
			desired |= DLGC_WANTTAB;
		if (routed && (current & DialogCodeWindowActiveFlag) != 0)
		{
			if (wantArrows)
				desired |= DLGC_WANTARROWS;
			if (wantChars)
				desired |= DLGC_WANTCHARS;
		}
		if (dialogCodeCapabilitySnapshot.compare_exchange_weak(
			current, desired, std::memory_order_acq_rel,
			std::memory_order_acquire))
			return;
	}
}

void Window::refreshDialogCodeFocusCapabilities() noexcept
{
	if (focusManager)
		focusManager->publishDialogCodeCapabilities();
	else
		publishDialogCodeFocusCapabilities(false, false);
}

bool Window::hasLiveNativeWindow() const noexcept
{
	// 正常路径由已安装的 WndProcThunk 在 WM_NCDESTROY 完成后发布死亡状态。
	// 若子类化失败，则退回 IsWindow，保证事件循环仍有退出兜底。
	if (procHooked)
		return nativeWindowAlive.load(std::memory_order_acquire);
	return hWnd != nullptr && IsWindow(hWnd) != FALSE;
}

bool Window::drainNativeInputAtSafePoint(bool modalBusinessSuppressed,
	const bool* continueRouting)
{
	if (!nativeInputContext)
		return true;

	const bool routeKeyboard = StellarX::Internal::keyboardPipelineMode(*this) ==
		StellarX::Internal::KeyboardPipelineMode::Routed;
	const bool routePointer = StellarX::Internal::pointerPipelineMode(*this) ==
		StellarX::Internal::PointerPipelineMode::Routed;
	if (!routeKeyboard && !routePointer)
	{
		// 两类输入均为兼容模式时只观察排空；旧 EasyX 队列仍是业务路径。
		nativeInputContext->drainObserved(StellarX::Internal::MessageBatchLimit);
	}
	else
	{
		// 定向路由每次只弹出一条事件，释放队列锁后再进入焦点或控件回调；
		// 不能把整个批次预取到局部容器，以免回调重入时仍占用队列状态。
		std::size_t drained = 0;
		while (drained < StellarX::Internal::MessageBatchLimit &&
			(!continueRouting || *continueRouting))
		{
			StellarX::NativeInputEvent event;
			const auto popResult = nativeInputContext->tryPopObserved(event);
			if (popResult == StellarX::Internal::NativeInputQueuePopResult::Empty)
				break;
			if (popResult == StellarX::Internal::NativeInputQueuePopResult::UnexpectedFailure)
				break;
			try
			{
				const auto activation = nativeInputContext->activationSnapshot();
				if (const auto* pointer =
					std::get_if<StellarX::PointerEvent>(&event.payload))
				{
					if (routePointer)
					{
						// 先执行既有的、感知 z-order 与 mutation 的 Pointer 分发。
						// TextBox 不会打开 InputBox；Down 会提交精确命中的焦点目标。
						ExMessage message{};
						if (SxMakeEasyXPointerMessage(*pointer, message))
						{
							++nativePointerBusinessDeliveries;
							dispatchManagedBusinessMessage(message);
						}

						StellarX::Internal::TextSelectionGestureEnvironment selectionEnvironment;
						selectionEnvironment.currentActivation = activation;
						selectionEnvironment.modalBusinessSuppressed = modalBusinessSuppressed;
						const StellarX::FocusTargetId hitTarget =
							pointer->action == StellarX::PointerAction::LeftDown ?
							getFocusManager().focusedTarget() : StellarX::FocusTargetId{};
						if (textSelectionGestureManager)
							textSelectionGestureManager->route(
								*this, event, selectionEnvironment, hitTarget);
					}
				}
				else if (routeKeyboard)
				{
					// 即使不是 Pointer 事件，活动拖选仍需在下一条 Pointer 到达前观察
					// 激活、作用域和目标失效。
					if (textSelectionGestureManager && textSelectionGestureManager->active())
					{
						StellarX::Internal::TextSelectionGestureEnvironment selectionEnvironment;
						selectionEnvironment.currentActivation = activation;
						selectionEnvironment.modalBusinessSuppressed = modalBusinessSuppressed;
						textSelectionGestureManager->route(
							*this, event, selectionEnvironment);
					}

					if (std::holds_alternative<StellarX::TextInputEvent>(event.payload))
					{
						StellarX::Internal::TextInputRoutingEnvironment environment;
						environment.currentActivation = activation;
						environment.modalBusinessSuppressed = modalBusinessSuppressed;
						StellarX::Internal::TextInputRouter::route(*this, event, environment);
					}
					else if (std::holds_alternative<StellarX::CompositionEvent>(event.payload))
					{
						StellarX::Internal::CompositionRoutingEnvironment environment;
						environment.currentActivation = activation;
						environment.modalBusinessSuppressed = modalBusinessSuppressed;
						StellarX::Internal::CompositionRouter::route(
							*this, event, environment);
					}
					else
					{
						if (const auto* activationEvent =
							std::get_if<StellarX::WindowActivationEvent>(&event.payload))
						{
							if (!activationEvent->keyboardFocusActive)
								StellarX::Internal::CompositionRouter::cancelForDeactivation(
									*this, event);
						}
						StellarX::Internal::KeyboardRoutingEnvironment environment;
						// 这次 acquire load 是路由门控的线性化点。
						environment.currentActivation = activation;
						environment.modalBusinessSuppressed = modalBusinessSuppressed;
						StellarX::Internal::KeyboardRouter::route(*this, event, environment);
					}
				}
			}
			catch (const std::bad_alloc&)
			{
				nativeInputContext->reportHostFault(
					StellarX::Internal::NativeInputFaultCode::OutOfMemory,
					event.sequence, 0, event.hwnd);
				break;
			}
			catch (...)
			{
				nativeInputContext->reportHostFault(
					StellarX::Internal::NativeInputFaultCode::UnexpectedException,
					event.sequence, 0, event.hwnd);
				break;
			}
			++drained;
		}
	}
	const auto normalization = nativeInputContext->normalizationStats();
	if (normalization.rejectedTextSequences != nativeInputRejectedTextReported ||
		normalization.imeQueryFailures != nativeInputImeQueryFailuresReported ||
		normalization.discardedDecoderFragments != nativeInputDiscardedFragmentsReported ||
		normalization.pointerCoordinateFailures !=
			nativeInputPointerCoordinateFailuresReported)
	{
		nativeInputRejectedTextReported = normalization.rejectedTextSequences;
		nativeInputImeQueryFailuresReported = normalization.imeQueryFailures;
		nativeInputDiscardedFragmentsReported = normalization.discardedDecoderFragments;
		nativeInputPointerCoordinateFailuresReported =
			normalization.pointerCoordinateFailures;
		SX_LOGW("Input")
			<< SX_T("原生输入规范化异常累计：rejected=", "Native input normalization anomalies: rejected=")
			<< normalization.rejectedTextSequences
			<< " imeQueryFailures=" << normalization.imeQueryFailures
			<< " discardedFragments=" << normalization.discardedDecoderFragments
			<< " pointerCoordinateFailures=" << normalization.pointerCoordinateFailures;
	}
	if (!nativeInputContext->hasFault())
		return true;

	if (!nativeInputFailureReported)
	{
		nativeInputFailureReported = true;
		const auto fault = nativeInputContext->fault();
		SX_LOGE("Input")
			<< SX_T("原生输入子系统进入 fail-stop：code=", "Native input subsystem entered fail-stop: code=")
			<< static_cast<unsigned>(fault.code)
			<< " sequence=" << fault.sequence
			<< " message=" << fault.message
			<< " hwnd=" << reinterpret_cast<std::uintptr_t>(fault.hwnd);
	}
	return false;
}

StellarX::Internal::NativeInputRuntimeSnapshot
StellarX::Internal::inspectWindowNativeInputForTesting(const ::Window& window) noexcept
{
	NativeInputRuntimeSnapshot snapshot;
	if (!window.nativeInputContext)
		return snapshot;
	snapshot.queue = window.nativeInputContext->queueStats();
	snapshot.observations = window.nativeInputContext->observationStats();
	snapshot.normalization = window.nativeInputContext->normalizationStats();
	snapshot.activation = window.nativeInputContext->activationSnapshot();
	snapshot.easyXKeyboardMessagesDrained = window.easyXKeyboardMessagesDrained;
	snapshot.legacyEasyXKeyboardBusinessDeliveries =
		window.legacyEasyXKeyboardBusinessDeliveries;
	snapshot.easyXPointerMessagesDrained = window.easyXPointerMessagesDrained;
	snapshot.legacyEasyXPointerBusinessDeliveries =
		window.legacyEasyXPointerBusinessDeliveries;
	snapshot.nativePointerBusinessDeliveries = window.nativePointerBusinessDeliveries;
	snapshot.faulted = window.nativeInputContext->hasFault();
	snapshot.fault = window.nativeInputContext->fault();
	return snapshot;
}

StellarX::Internal::ImeCaretAnchorRuntimeSnapshot
StellarX::Internal::inspectWindowImeCaretAnchorForTesting(
	const ::Window& window) noexcept
{
	return window.imeCaretAnchorContext
		? window.imeCaretAnchorContext->inspectCrossThread()
		: ImeCaretAnchorRuntimeSnapshot{};
}

bool StellarX::Internal::configureImeCandidatePositionModeForTesting(
	::Window& window, ImeCandidatePositionMode mode) noexcept
{
	if (!window.imeCaretAnchorContext)
		return false;
	window.imeCaretAnchorContext->setCandidatePositionModeForTesting(mode);
	return true;
}

bool StellarX::Internal::configureImeCandidateDismissModeForTesting(
	::Window& window, ImeCandidateDismissMode mode) noexcept
{
	if (!window.imeCaretAnchorContext)
		return false;
	window.imeCaretAnchorContext->setCandidateDismissModeForTesting(mode);
	return true;
}

bool StellarX::Internal::configureImeInputContextGateModeForTesting(
	::Window& window, ImeInputContextGateMode mode) noexcept
{
	if (!window.imeCaretAnchorContext)
		return false;
	window.imeCaretAnchorContext->setInputContextGateModeForTesting(mode);
	return true;
}

Window::~Window()
{
	// 先销毁控件树，再关闭图形环境，避免控件析构时访问已关闭的 EasyX 上下文。
	pendingControlMutations.clear();
	dialogs.clear();
	controls.clear();
	background.reset();
	bool inputContextRestoredForUnhook = true;
	if (hWnd && procHooked && oldWndProc && hasLiveNativeWindow() &&
		imeCaretAnchorContext)
	{
		inputContextRestoredForUnhook = imeCaretAnchorContext->
			restoreInputContextBeforeWindowTeardown(hWnd);
	}
	StellarX::Internal::inputProbeDetach(hWnd);
	if (hWnd && procHooked && oldWndProc && hasLiveNativeWindow() &&
		inputContextRestoredForUnhook)
	{
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
	}
	if (hWnd)
		closegraph();
}

// ---------------- 原生消息钩子----------------

/**
 * WndProcThun
 * 作用：替换 EasyX 的窗口过程，接管关键消息。
 * 关键处理：
 *  - WM_ERASEBKGND：返回 1，交由自绘清屏，避免系统擦背景造成闪烁。
 *  - WM_ENTERSIZEMOVE：开始拉伸 → isSizing=true 且 WM_SETREDRAW(FALSE) 冻结重绘。
 *  - WM_SIZING：拉伸中 → 仅做“最小尺寸夹紧”（按被拖边回推），不回滚、不绘制。
 *  - WM_EXITSIZEMOVE：结束拉伸 → 读取最终客户区尺寸 → 标记 needResizeDirty，解冻并刷新。
 *  - WM_GETMINMAXINFO：提供系统最小轨迹限制（四边一致）。
 */
LRESULT CALLBACK Window::WndProcThunk(HWND h, UINT m, WPARAM w, LPARAM l)
{
	auto* self = reinterpret_cast<Window*>(GetWindowLongPtr(h, GWLP_USERDATA));
	if (!self)
	{
		return DefWindowProc(h, m, w, l);
	}
	const auto probeCall = StellarX::Internal::inputProbeWndProcEnter(h, m, w, l);
	if ((m == WM_NCDESTROY || !probeCall.observeAndConsume) &&
		self->imeCaretAnchorContext)
	{
		// 文字输入资格门控只能在属主线程操作。WM_NCDESTROY 到达 EasyX 前恢复原始
		// 上下文，确保系统管理的上下文生命周期仍绑定到最初接收它的 HWND。
		(void)self->imeCaretAnchorContext->synchronizeInputContextAssociation(
			h, m == WM_NCDESTROY);
	}
	const bool imeCompositionEpochPrepared =
		!probeCall.observeAndConsume && self->imeCaretAnchorContext
		? self->imeCaretAnchorContext->prepareCompositionEpochForCollection(m)
		: false;
	if (!probeCall.observeAndConsume && self->imeCaretAnchorContext)
	{
		(void)self->imeCaretAnchorContext->observeProcessKeyActivityForCollection(
			h, m, w);
		if (m == WM_LBUTTONDOWN)
		{
			// 属主线程取消必须先于 Pointer 事件。IMM32 可能在此同步发出 Cancel/End；
			// NativeInputContext 必须先采集这些通知，再发布会改变焦点的 LeftDown。
			(void)self->imeCaretAnchorContext->
				cancelCompositionBeforePointerCollection(h, l);
		}
	}
	const auto nativeInputResult = self->nativeInputContext
		? self->nativeInputContext->collect(h, m, w, l)
		: StellarX::Internal::NativeInputCollectResult{};
	if (m == WM_SETFOCUS || m == WM_KILLFOCUS)
		self->publishDialogCodeActivation(m == WM_SETFOCUS);
	if (probeCall.observeAndConsume)
	{
		// 探针消费规则的唯一出口：只有启用诊断、规则唯一且环境匹配时可达。
		// 未启用探针时 token 恒为 false，不改变正常窗口过程行为。
		return StellarX::Internal::inputProbeWndProcLeave(probeCall, 0, false);
	}
	if (nativeInputResult.handledSynchronously)
	{
		return StellarX::Internal::inputProbeWndProcLeave(
			probeCall, nativeInputResult.result, false);
	}
	LPARAM forwardedLParam = l;
	if (self->imeCaretAnchorContext)
	{
		if (m == WM_IME_SETCONTEXT)
			forwardedLParam = self->imeCaretAnchorContext->filterSetContextLParam(w, l);
		const auto imeCaretResult =
			self->imeCaretAnchorContext->onWndProcMessage(
				h, m, w, l, imeCompositionEpochPrepared);
		if (imeCaretResult.handledSynchronously)
		{
			return StellarX::Internal::inputProbeWndProcLeave(
				probeCall, imeCaretResult.result, false);
		}
	}
	if (m == WM_GETDLGCODE)
	{
		// 基础返回值由 EasyX 原窗口过程决定；StellarX 只追加无锁 dialog-code
		// 快照中已经发布的能力位。
		const bool forwardedToOldProc = self->oldWndProc != nullptr;
		LRESULT result = forwardedToOldProc
			? CallWindowProc(self->oldWndProc, h, m, w, l)
			: DefWindowProc(h, m, w, l);
		result |= StellarX::Internal::dialogCodeAdditionsForQuery(*self, w, l);
		return StellarX::Internal::inputProbeWndProcLeave(
			probeCall, result, forwardedToOldProc);
	}
	if (m == WM_CLOSE)
	{
		// EasyX 的旧窗口过程可能在 WM_CLOSE 内直接终止进程，不保证返回到 leave、
		// WM_NCDESTROY 或宿主循环。这里是诊断会话最后一个可证明到达的耐久边界。
		// 未启用探针时 inputProbeDetach 为空操作，不改变正常关闭路径。
		StellarX::Internal::inputProbeDetach(h);
	}

	// 关键点①：禁止系统擦背景，避免和我们自己的清屏/双缓冲打架造成闪烁
	if (m == WM_ERASEBKGND)
	{
		return StellarX::Internal::inputProbeWndProcLeave(probeCall, 1, false);
	}

	// 关键点②：拉伸开始 → 冻结重绘（系统调整窗口矩形时不触发即时重绘，防止抖）
	if (m == WM_ENTERSIZEMOVE)
	{
		SX_LOGI("Resize") << SX_T("WM_ENTERSIZEMOVE: 开始测量尺寸","WM_ENTERSIZEMOVE: begin sizing");
		self->isSizing = true;
		SendMessage(h, WM_SETREDRAW, FALSE, 0);
		return StellarX::Internal::inputProbeWndProcLeave(probeCall, 0, false);
	}

	// 关键点③：拉伸中 → 仅执行“最小尺寸夹紧”，不做对齐/节流/回滚，保持系统自然流畅
	if (m == WM_SIZING)
	{
		RECT* prc = reinterpret_cast<RECT*>(l);
		// “尺寸异常值”快速过滤：仅保护极端值，不影响正常拖动
		int currentWidth = prc->right - prc->left;
		int currentHeight = prc->bottom - prc->top;
		if (currentWidth < 0 || currentHeight < 0 || currentWidth > 10000 || currentHeight > 10000)
		{
			return StellarX::Internal::inputProbeWndProcLeave(probeCall, TRUE, false);
		}

		RECT before = *prc;// 记录调整前矩形以便日志输出
		ApplyMinSizeOnSizing(prc, w, h, self->minClientW, self->minClientH);
		//if (before.left != prc->left || before.top != prc->top || before.right != prc->right || before.bottom != prc->bottom)
		if (memcmp(&before, prc, sizeof(RECT)) != 0)
		{
			SX_LOGD("Resize")
				<< SX_T("WM_SIZING 夹具：","WM_SIZING clamp: ")
				<< SX_T("之前=(","before=(") << (before.right - before.left) << "x" << (before.bottom - before.top) << ") "
				<< SX_T("之后=（","after=(") << (prc->right - prc->left) << "x" << (prc->bottom - prc->top) << ")";
		}
		return StellarX::Internal::inputProbeWndProcLeave(probeCall, TRUE, false);
	}

	// 关键点④：拉伸结束 → 解冻重绘 + 统一收口（记录最终尺寸 -> 标记 needResizeDirty）
	if (m == WM_EXITSIZEMOVE)
	{
		self->isSizing = false;

		RECT rc; GetClientRect(h, &rc);
		const int aw = rc.right - rc.left;
		const int ah = rc.bottom - rc.top;
		if (aw >= self->minClientW && ah >= self->minClientH && aw <= 10000 && ah <= 10000)
		{
			self->pendingW = aw;
			self->pendingH = ah;
			self->needResizeDirty = true;
			SX_LOGI("Resize") << SX_T("WM_EXITSIZEMOVE: 最终尺寸，待重绘=(","WM_EXITSIZEMOVE: end sizing, pending=(" )<< self->pendingW << "x" << self->pendingH << "), needResizeDirty=1";
		}

		// 结束拉伸后不立即执行重绘，待事件循环统一收口。
		// 立即解冻重绘标志，同时标记区域为有效，避免触发额外 WM_PAINT。
		SendMessage(h, WM_SETREDRAW, TRUE, 0);
		ValidateRect(h, nullptr);
		return StellarX::Internal::inputProbeWndProcLeave(probeCall, 0, false);
	}

	// 关键点⑤：系统级最小轨迹限制（与 WM_SIZING 的夹紧互相配合）
	if (m == WM_GETMINMAXINFO)
	{
		auto* mmi = reinterpret_cast<MINMAXINFO*>(l);
		RECT rc{ 0, 0, self->minClientW, self->minClientH };
		DWORD style = GetWindowLong(h, GWL_STYLE);
		DWORD ex = GetWindowLong(h, GWL_EXSTYLE);
		// 若后续添加菜单，请把第三个参数改为 HasMenu(h)
		AdjustWindowRectEx(&rc, style, FALSE, ex);
		mmi->ptMinTrackSize.x = rc.right - rc.left;
		mmi->ptMinTrackSize.y = rc.bottom - rc.top;
		return StellarX::Internal::inputProbeWndProcLeave(probeCall, 0, false);
	}

	// 其它消息：回落到旧过程
	const bool forwardedToOldProc = self->oldWndProc != nullptr;
	const LRESULT result = forwardedToOldProc
		? CallWindowProc(self->oldWndProc, h, m, w, forwardedLParam)
		: DefWindowProc(h, m, w, forwardedLParam);
	const LRESULT observedResult = StellarX::Internal::inputProbeWndProcLeave(
		probeCall, result, forwardedToOldProc);
	if (m == WM_NCDESTROY)
	{
		// 必须放在旧窗口过程返回之后，并作为本函数最后一次 self 访问。
		// 宿主线程看到 false 后即可安全离开 runEventLoop 并析构 Window。
		self->nativeWindowAlive.store(false, std::memory_order_release);
	}
	return observedResult;
}

// ---------------- 绘制 ----------------

/**
 * draw()
 * 作用：首次初始化 EasyX 窗口与子类化过程；应用可拉伸样式；清屏并批量绘制。
 * 关键步骤：
 *  1）initgraph 拿到 hWnd；
 *  2）SetWindowLongPtr 子类化到 WndProcThunk（只做一次）；
 *  3）ApplyResizableStyle 设置 WS_THICKFRAME/裁剪/（可选）合成双缓冲；
 *  4）去掉类样式 CS_HREDRAW/CS_VREDRAW，避免全窗无效化引发闪屏；
 *  5）清屏 + Begin/EndBatchDraw 批量绘制控件&对话框。
 */
void Window::draw()
{
	if (!hWnd)
	{
		hWnd = initgraph(width, height, windowMode);
	}
	nativeWindowAlive.store(hWnd != nullptr && IsWindow(hWnd) != FALSE,
		std::memory_order_release);

	// 子类化：让我们的 WndProcThunk 接管窗口消息（仅执行一次）
	if (!procHooked)
	{
		if (!nativeInputContext)
		{
			nativeInputContext = std::make_unique<StellarX::Internal::NativeInputContext>(hWnd);
			publishDialogCodeActivation(
				nativeInputContext->activationSnapshot().keyboardFocusActive);
			refreshDialogCodeFocusCapabilities();
		}
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);
		StellarX::Internal::inputProbeAttach(hWnd);
		oldWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)&Window::WndProcThunk);
		procHooked = (oldWndProc != nullptr);
		if (!procHooked)
			StellarX::Internal::inputProbeDetach(hWnd);
		else if (imeCaretAnchorContext)
			// 必须在 WndProcThunk 安装后再投递刷新消息；否则消息会被 EasyX 消费，
			// 用于合并请求的 pending 位将无法清除。
			imeCaretAnchorContext->attachWindow(hWnd);
	}
	if (!headline.empty())
	{
		SetWindowText(hWnd, headline.c_str());
	}
	ApplyResizableStyle(hWnd, useComposited);

	// 关闭类样式的整窗重绘标志（减少尺寸变化时的整窗 redraw）
	LONG_PTR cls = GetClassLongPtr(hWnd, GCL_STYLE);
	cls &= ~(CS_HREDRAW | CS_VREDRAW);
	SetClassLongPtr(hWnd, GCL_STYLE, cls);

	BeginBatchDraw();
	redrawScene(true, true);
	EndBatchDraw();
	clearManagedRepaintState();
}

/**
 * draw(imagePath)
 * 作用：在 draw() 的基础上加载并绘制背景图；其它流程完全一致。
 * 注意：这里按当前窗口客户区大小加载背景图（loadimage 的 w/h），保证铺满。
 */
void Window::draw(std::string imagePath)
{
	if (!hWnd)
	{
		hWnd = initgraph(width, height, windowMode);
	}
	nativeWindowAlive.store(hWnd != nullptr && IsWindow(hWnd) != FALSE,
		std::memory_order_release);

	if (!procHooked)
	{
		if (!nativeInputContext)
		{
			nativeInputContext = std::make_unique<StellarX::Internal::NativeInputContext>(hWnd);
			publishDialogCodeActivation(
				nativeInputContext->activationSnapshot().keyboardFocusActive);
			refreshDialogCodeFocusCapabilities();
		}
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);
		StellarX::Internal::inputProbeAttach(hWnd);
		oldWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)&Window::WndProcThunk);
		procHooked = (oldWndProc != nullptr);
		if (!procHooked)
			StellarX::Internal::inputProbeDetach(hWnd);
		else if (imeCaretAnchorContext)
			imeCaretAnchorContext->attachWindow(hWnd);
	}

	bkImageFile = std::move(imagePath);
	if (!headline.empty())
	{
		SetWindowText(hWnd, headline.c_str());
	}

	ApplyResizableStyle(hWnd, useComposited);

	LONG_PTR cls = GetClassLongPtr(hWnd, GCL_STYLE);
	cls &= ~(CS_HREDRAW | CS_VREDRAW);
	SetClassLongPtr(hWnd, GCL_STYLE, cls);

	if (background)
	{
		background.reset();
	}
	background = std::make_unique<IMAGE>();

	BeginBatchDraw();
	redrawScene(true, true);
	EndBatchDraw();
	clearManagedRepaintState();
}

// ---------------- 事件循环 ----------------

void Window::dispatchManagedBusinessMessage(const ExMessage& message)
{
	const bool oldManagedDispatchActive = managedDispatchActive;
	managedDispatchActive = true;

	// 已注册的模态循环是唯一 Pointer 目标；活动期间，根控件和非模态 Dialog
	// 不能收到同一事件。
	if (!modalDialogStack.empty())
	{
		Dialog* currentModal = modalDialogStack.back();
		if (currentModal && currentModal->IsVisible())
			currentModal->handleEvent(message);
	}
	else
	{
		bool consume = false;
		if (message.message == WM_LBUTTONDOWN)
		{
			// Pointer Down 必须先切换活动非模态作用域，子控件随后才能申请焦点；
			// 点击所有 Dialog 之外时会先退出该作用域。
			if (Dialog* hitDialog = hitTestTopmostNonModalDialog(message.x, message.y))
				activateNonModalDialog(*hitDialog);
			else
				deactivateNonModalDialog(true);
		}

		for (auto it = dialogs.rbegin(); it != dialogs.rend(); ++it)
		{
			auto& dialog = *it;
			if (dialog->IsVisible() && !dialog->model())
				consume = dialog->handleEvent(message);
			if (consume)
			{
				if (!SxIsNoisyMsg(message.message))
					SX_LOGD("Event")
						<< SX_T("事件被非模态对话框处理：", "Event consumed by non-modal dialog: ")
						<< SxMsgName(message.message);
				if (message.message == WM_MOUSEMOVE)
					syncMouseStateAfterOverlayChanged(OverlayMouseSyncMode::ClearBehindOverlay);
				break;
			}
		}

		if (!consume)
		{
			Control* firstConsumer = nullptr;
			if (message.message == WM_MOUSEMOVE)
			{
				for (auto it = controls.rbegin(); it != controls.rend(); ++it)
				{
					Control* current = it->get();
					if (!consume)
					{
						consume = current->handleEvent(message);
						if (consume)
							firstConsumer = current;
					}
					else
					{
						const RECT previousCoverage = current->getManagedRepaintCoverageRect();
						if (current->clearTransientMouseState())
							requestManagedRepaint(current, previousCoverage);
					}
				}
			}
			else
			{
				for (auto it = controls.rbegin(); it != controls.rend(); ++it)
				{
					Control* current = it->get();
					consume = current->handleEvent(message);
					if (consume)
					{
						firstConsumer = current;
						break;
					}
				}
			}

			if (firstConsumer && !SxIsNoisyMsg(message.message))
				SX_LOGD("Event") << SX_T("事件被控件处理：", "Event consumed by control: ")
				<< SxMsgName(message.message)
				<< SX_T(" id=", " id=") << firstConsumer->getId();
		}

		if (!consume && message.message == WM_LBUTTONDOWN)
			focusManager->clearFocus(StellarX::FocusReason::Pointer);
	}

	// 外层分发暂停期间可能进入嵌套模态循环；退出时恢复继承状态，不能无条件清空。
	managedDispatchActive = oldManagedDispatchActive;
	if (!managedDispatchActive)
		flushPendingMutationsInTree();
}

/**
 * runEventLoop()
 * 作用：驱动输入/窗口消息；集中处理“统一收口重绘”。
 * 关键策略：
 *  - WM_SIZE：始终更新 pendingW/H（即使在拉伸中也只记录不立即绘制）；
 *  - needResizeDirty：当尺寸确实变化时置位，随后在循环尾进行一次性重绘；
 *  - 非模态对话框优先消费事件（顶层从后往前）；再交给普通控件；
 *  - managedDispatchActive=true 期间，控件 requestRepaint 不会立即画，而是登记到 managedRepaintItems；
 *  - 事件尾通过 flushManagedRepaint 提交本轮 root 重绘，再按需补画 Dialog。
 */
int Window::runEventLoop()
{
	ExMessage msg;
	StellarX::Internal::lockKeyboardPipeline(*this);
	StellarX::Internal::lockPointerPipeline(*this);

	// 说明：统一使用 needResizeDirty 作为“收口重绘”的唯一标志位
	//       不再引入额外 pendingResize 等状态，避免分叉导致状态不一致。
	while (true)
	{
		if (nativeInputContext && nativeInputContext->hasFault())
		{
			drainNativeInputAtSafePoint();
			StellarX::Internal::inputProbeDetach(hWnd);
			return StellarX::Internal::NativeInputFailureExitCode;
		}
		if (!hasLiveNativeWindow())
		{
			const bool inputHealthy = drainNativeInputAtSafePoint();
			// 宿主循环已经确定退出；在 Window/EasyX 进一步改写句柄状态前结束探测会话。
			StellarX::Internal::inputProbeDetach(hWnd);
			return inputHealthy ? 0 : StellarX::Internal::NativeInputFailureExitCode;
		}

		// drainMessageBatch 将一轮消息限制在固定上限；批次结束后统一处理 resize、tick 和重绘。
		auto probeBatch = StellarX::Internal::inputProbeHostBatchBegin(hWnd, false);
		const auto batch = StellarX::Internal::drainMessageBatch(msg,
			[](ExMessage& next) { return peekmessage(&next, EX_MOUSE | EX_KEY | EX_WINDOW, true); },
			[&](const ExMessage& next) {
			StellarX::Internal::inputProbeObserveEasyXMessage(hWnd, probeBatch, next);
			const bool easyXKeyboardMessage =
				StellarX::Internal::isEasyXKeyboardMessage(next.message);
			const bool easyXPointerMessage =
				StellarX::Internal::isEasyXPointerMessage(next.message);
			if (easyXKeyboardMessage)
				++easyXKeyboardMessagesDrained;
			if (easyXPointerMessage)
				++easyXPointerMessagesDrained;
			if (next.message == WM_CLOSE)
				return false;

			// 保险：如果 EX_WINDOW 转译了 GETMINMAXINFO，同样按最小客户区折算处理
			if (next.message == WM_GETMINMAXINFO)
			{
				auto* mmi = reinterpret_cast<MINMAXINFO*>(next.lParam);
				RECT rc{ 0, 0, minClientW, minClientH };
				DWORD style = GetWindowLong(hWnd, GWL_STYLE);
				DWORD ex = GetWindowLong(hWnd, GWL_EXSTYLE);
				AdjustWindowRectEx(&rc, style, FALSE, ex);
				mmi->ptMinTrackSize.x = rc.right - rc.left;
				mmi->ptMinTrackSize.y = rc.bottom - rc.top;
				return true;
			}

			// 关键点⑥：WM_SIZE 只记录新尺寸；若非拉伸阶段则立即置位 needResizeDirty
			if (next.message == WM_SIZE && next.wParam != SIZE_MINIMIZED)
			{
				const int nw = LOWORD(next.lParam);
				const int nh = HIWORD(next.lParam);

				// 基本合法性校验（不小于最小值、不过大）
				if (nw >= minClientW && nh >= minClientH && nw <= 10000 && nh <= 10000)
				{
					if (nw != width || nh != height)
					{
						pendingW = nw;
						pendingH = nh;
						// 在“非拉伸阶段”的 WM_SIZE（例如最大化/还原/程序化调整）直接触发收口
						needResizeDirty = true;
						SX_LOGD("Resize") <<SX_T("WM_SIZE：待处理=(", "WM_SIZE: pending=(") << pendingW << "x" << pendingH << "), isSizing=" << (isSizing ? 1 : 0);

					}
				}
				return true;
			}

			// Keyboard 和 Pointer 都走定向路由。这里仍排空 EasyX 的两类内部队列，
			// 但不会再次进入业务交付。
			if (!StellarX::Internal::shouldDeliverLegacyEasyXMessage(*this, next.message))
				return true;
			if (easyXKeyboardMessage)
				++legacyEasyXKeyboardBusinessDeliveries;
			if (easyXPointerMessage)
				++legacyEasyXPointerBusinessDeliveries;

			dispatchManagedBusinessMessage(next);
			return true;
		});
		StellarX::Internal::inputProbeHostBatchEnd(hWnd, probeBatch, batch.processed, batch.stopped);
		if (!drainNativeInputAtSafePoint())
		{
			StellarX::Internal::inputProbeDetach(hWnd);
			return StellarX::Internal::NativeInputFailureExitCode;
		}
		if (batch.stopped || !hasLiveNativeWindow())
		{
			// batch.stopped（典型为 WM_CLOSE）是宿主线程最后一个可靠安全点。
			// detach 会落下 window_detach/session_end；默认构建中该调用为空操作。
			StellarX::Internal::inputProbeDetach(hWnd);
			return 0;
		}

		const ULONGLONG nowMs = GetTickCount64();
		// tick 独立于鼠标消息推进 Tooltip 等延时状态，仍放在事务内避免回调立即改 vector。
		beginMutationScope();
		for (auto& control : controls)
			if (control && control->IsVisible()) control->tick(nowMs);
		for (auto& dialog : dialogs)
			if (dialog && dialog->IsVisible()) dialog->tick(nowMs);
		endMutationScope();

		// 对话框打开/关闭属于全局层级变化：这里仍然使用整场景重绘兜底，
		// 并在结束后清空本轮托管重绘登记，避免旧的 root 请求延后提交。
		bool needredraw = fullSceneRedrawRequested;
		if(dialogOpen)
		{
			for (auto& d : dialogs)
			{
				needredraw = d->IsVisible();
				if (needredraw)break;
			}
		}
		if (needredraw || dialogClose)
		{
			if (dialogClose)
			{
				// 对话框关闭后，需要手动合成一个鼠标移动消息并分发给所有普通控件，
				// 以便它们能及时更新悬停状态（hover），否则悬停状态可能保持错误状态。
				// 先按当前鼠标位置刷新底层控件状态，真正绘制仍由后续全场景重绘兜底。
				SX_LOGD("Event") << SX_T("对话框关闭，合成WM_MOUSEMOVE已下发", "Dialog closed; synthetic WM_MOUSEMOVE dispatched");
				syncMouseStateAfterOverlayChanged(OverlayMouseSyncMode::RestoreAtCursor);
				dialogClose = false; // 重置标志
			}

			BeginBatchDraw();
			SX_LOGD("Event") << SX_T("对话框打开/关闭，触发全量重绘", "The dialog box opens/closes, triggering a full redraw");
			redrawScene(true, true);
			EndBatchDraw();
			needredraw = false;
			dialogOpen = false;
			fullSceneRedrawRequested = false;
			clearManagedRepaintState();

		}
		// —— 统一收口（needResizeDirty 为真时执行一次性重绘）——
		// resize 会改变布局和背景基线，因此仍然走整场景重绘，而不是局部 root 提交。
		if (needResizeDirty)
		{
			SX_LOGI("Resize") << SX_T("调整窗口尺寸开始：width=","Resize settle start: width=") << width << " height=" << height;
			SX_TRACE_SCOPE(SX_T("调整尺寸","Resize"),SX_T("窗口：调整尺寸", "Window::resize_settle"));

			// 以“实际客户区尺寸”为准，防止 pending 与真实尺寸出现偏差
			RECT clientRect;
			GetClientRect(hWnd, &clientRect);
			int actualWidth = clientRect.right - clientRect.left;
			int actualHeight = clientRect.bottom - clientRect.top;

			const int virtualScreenWidth = (std::max)(1, GetSystemMetrics(SM_CXVIRTUALSCREEN));
			const int virtualScreenHeight = (std::max)(1, GetSystemMetrics(SM_CYVIRTUALSCREEN));
			const int maxReasonableWidth = (std::max)(10000, virtualScreenWidth * 2);
			const int maxReasonableHeight = (std::max)(10000, virtualScreenHeight * 2);

			// 仅拦截“明显非法”的客户区尺寸，不再按“变化跨度”误杀正常最大化。
			if (actualWidth <= 0 || actualHeight <= 0
				|| actualWidth > maxReasonableWidth || actualHeight > maxReasonableHeight)
			{
				SX_LOGD("Resize")
					<< SX_T("尺寸调整被非法尺寸保护跳过：old=(", "Resize settle skipped by invalid-size guard: old=(")
					<< width << "x" << height
					<< SX_T(") pending=(", ") pending=(")
					<< pendingW << "x" << pendingH
					<< SX_T(") actual=(", ") actual=(")
					<< actualWidth << "x" << actualHeight
					<< SX_T(") virtual=(", ") virtual=(")
					<< virtualScreenWidth << "x" << virtualScreenHeight
					<< SX_T(") maxAllowed=(", ") maxAllowed=(")
					<< maxReasonableWidth << "x" << maxReasonableHeight
					<< SX_T(")", ")");
				needResizeDirty = false;
				continue;
			}

			const int finalW = (std::max)(minClientW, actualWidth);
			const int finalH = (std::max)(minClientH, actualHeight);

			if (finalW != width || finalH != height)
			{
				const int diffW = abs(finalW - width);
				const int diffH = abs(finalH - height);
				if (diffW > 1000 || diffH > 1000)
				{
					SX_LOGD("Resize")
						<< SX_T("检测到大跨度尺寸调整，继续执行收口：old=(", "Large-span resize detected; continue settle: old=(")
						<< width << "x" << height
						<< SX_T(") new=(", ") new=(")
						<< finalW << "x" << finalH
						<< SX_T(") diff=(", ") diff=(")
						<< diffW << "x" << diffH
						<< SX_T(") actual=(", ") actual=(")
						<< actualWidth << "x" << actualHeight
						<< SX_T(") virtual=(", ") virtual=(")
						<< virtualScreenWidth << "x" << virtualScreenHeight
						<< SX_T(")", ")");
				}

				// 再次冻结窗口更新，保证批量绘制的原子性
				SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);

				BeginBatchDraw();

				// 调整底层画布尺寸
				if (finalW != width || finalH != height)
				{
					// 批量通知控件“窗口尺寸变化”，并标记重绘
					for (auto& c : controls)
						adaptiveLayout(c, finalH, finalW);
					//重绘窗口
					Resize(nullptr, finalW, finalH);

					// 重取一次实际客户区尺寸做确认
					GetClientRect(hWnd, &clientRect);
					int confirmedWidth = clientRect.right - clientRect.left;
					int confirmedHeight = clientRect.bottom - clientRect.top;

					// 最终提交“当前已应用尺寸”（用于外部查询/下次比较）
					width = confirmedWidth;
					height = confirmedHeight;

					for (auto& d : dialogs)
					{
						if (auto dd = dynamic_cast<Dialog*>(d.get()))
						{
							dd->recenterInHostWindow();
						}
					}
				}

				// 统一批量绘制
				redrawScene(true, true);

				EndBatchDraw();

				// 解冻后标记区域有效，避免系统再次触发 WM_PAINT 覆盖自绘内容。
				SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
				ValidateRect(hWnd, nullptr);
			}
			SX_LOGI("Resize") << SX_T("尺寸调整已完成：width=","Resize settle done: width=") << width << " height=" << height;

			needResizeDirty = false; // 收口完成，清标志
			clearManagedRepaintState();
		}

		// 普通输入事件收口：只在没有 resize / 对话框开关这种全局变化时，才提交本轮托管重绘。
		if (!needResizeDirty && !dialogOpen && !dialogClose)
		{
			collectDirtyRootsForManagedRepaint();
			flushManagedRepaint();
		}

		sweepClosedDialogs();
		reconcileNonModalDialogScope();
		if (!isMutationTransactionActive())
		{
			if (StellarX::Internal::inputProbeGeometrySafePointEnabled(hWnd))
			{
				const auto imeRuntime = imeCaretAnchorContext
					? imeCaretAnchorContext->inspectCrossThread()
					: StellarX::Internal::ImeCaretAnchorRuntimeSnapshot{};
				StellarX::Internal::inputProbeObserveGeometrySafePoint(
					hWnd, probeBatch.batchId, imeRuntime);
			}
			StellarX::Internal::inputProbeFlushAtSafePoint(hWnd);
		}

		// 只在真正空闲时短暂让出 CPU；有消息的批次不得人为限速。
		if (batch.shouldSleep())
			Sleep(1);
	}
}

// ---------------- 其余接口 ----------------

void Window::setBkImage(std::string pImgFile)
{
	// 更换背景图：立即加载并绘制一次；同时将所有控件标 dirty 并重绘
	background = std::make_unique<IMAGE>();
	bkImageFile = std::move(pImgFile);

	BeginBatchDraw();
	redrawScene(true, true);
	EndBatchDraw();
}

void Window::setBkcolor(COLORREF c)
{
	// 更换纯色背景：立即清屏并批量重绘控件/对话框
	wBkcolor = c;
	background.reset();
	bkImageFile.clear();

	BeginBatchDraw();
	redrawScene(true, true);
	EndBatchDraw();
}

void Window::setHeadline(std::string title)
{
	// 设置窗口标题（仅改文本，不触发重绘）
	headline = std::move(title);
	if (hWnd)
		SetWindowText(hWnd, headline.c_str());
}

StellarX::ControlHandle Window::addControl(std::unique_ptr<Control> control)
{
	if (!control)
		return {};
	const StellarX::ControlHandle handle = control->getHandle();
	// 回调或绘制阶段只排队所有权变更；非事务调用则立即提交，保持旧的同步使用体验。
	if (isMutationTransactionActive())
	{
		PendingControlMutation mutation;
		mutation.kind = ControlMutationKind::Add;
		mutation.handle = handle;
		mutation.control = std::move(control);
		pendingControlMutations.push_back(std::move(mutation));
		return handle;
	}
	// 新增控件：仅加入管理容器，具体绘制在 draw()/收口时统一进行
	control->setHostWindow(this);
	controls.push_back(std::move(control));
	return handle;
}

bool Window::removeControl(StellarX::ControlHandle handle)
{
	if (!handle)
		return false;
	bool exists = std::find_if(controls.begin(), controls.end(), [&](const auto& item)
		{ return item && item->getHandle() == handle; }) != controls.end();
	for (const auto& mutation : pendingControlMutations)
	{
		if (mutation.kind == ControlMutationKind::Clear)
			exists = false;
		else if (mutation.handle == handle)
			exists = mutation.kind == ControlMutationKind::Add;
	}
	if (!exists)
		return false;
	pendingControlMutations.push_back({ ControlMutationKind::Remove, handle, nullptr });
	if (focusManager)
		focusManager->notifySubtreePendingRemoval(handle);
	if (!isMutationTransactionActive())
		flushPendingMutationsInTree();
	return true;
}

void Window::clearControls()
{
	pendingControlMutations.push_back({ ControlMutationKind::Clear, {}, nullptr });
	if (focusManager)
	{
		std::vector<StellarX::ControlHandle> removedHandles;
		removedHandles.reserve(controls.size());
		for (const auto& control : controls)
			if (control) removedHandles.push_back(control->getHandle());
		focusManager->notifySubtreesPendingRemoval(removedHandles);
	}
	if (!isMutationTransactionActive())
		flushPendingMutationsInTree();
}

Control* Window::findControl(StellarX::ControlHandle handle)
{
	auto it = std::find_if(controls.begin(), controls.end(), [&](const auto& item)
		{ return item && item->getHandle() == handle; });
	return it == controls.end() ? nullptr : it->get();
}

const Control* Window::findControl(StellarX::ControlHandle handle) const
{
	auto it = std::find_if(controls.begin(), controls.end(), [&](const auto& item)
		{ return item && item->getHandle() == handle; });
	return it == controls.end() ? nullptr : it->get();
}

std::vector<StellarX::ControlHandle> Window::getControlHandles() const
{
	std::vector<StellarX::ControlHandle> handles;
	handles.reserve(controls.size());
	for (const auto& control : controls)
		if (control) handles.push_back(control->getHandle());
	return handles;
}

StellarX::FocusManager& Window::getFocusManager()
{
	return *focusManager;
}

const StellarX::FocusManager& Window::getFocusManager() const
{
	return *focusManager;
}

bool Window::queryFocusTargetSnapshot(const StellarX::FocusTargetId& target,
	StellarX::FocusTargetSnapshot& snapshot) const
{
	for (auto it = modalDialogStack.rbegin(); it != modalDialogStack.rend(); ++it)
		if (*it && (*it)->queryFocusTargetSnapshot(target, snapshot)) return true;
	for (auto it = dialogs.rbegin(); it != dialogs.rend(); ++it)
		if (*it && (*it)->queryFocusTargetSnapshot(target, snapshot)) return true;
	for (auto it = controls.rbegin(); it != controls.rend(); ++it)
		if (*it && (*it)->queryFocusTargetSnapshot(target, snapshot)) return true;
	return false;
}

void Window::collectFocusTargetSnapshots(
	std::vector<StellarX::FocusTargetSnapshot>& snapshots) const
{
	for (const auto& control : controls)
		if (control) control->collectFocusTargetSnapshots(snapshots);
	// 非活动 Dialog 仍可被 queryFocusTargetSnapshot() 按身份解析并接受鼠标，
	// 但不得混入根作用域或另一 Dialog 的 Tab 顺序。
	if (activeNonModalDialogRoot)
	{
		for (const auto& item : dialogs)
		{
			if (item && item->getHandle() == activeNonModalDialogRoot.owner)
			{
				item->collectFocusTargetSnapshots(snapshots);
				break;
			}
		}
	}
	for (Dialog* dialog : modalDialogStack)
		if (dialog) dialog->collectFocusTargetSnapshots(snapshots);
}

bool Window::applyFocusState(const StellarX::FocusTargetId& target, bool focused,
	StellarX::FocusReason reason)
{
	for (auto it = modalDialogStack.rbegin(); it != modalDialogStack.rend(); ++it)
		if (*it && (*it)->applyFocusState(target, focused, reason)) return true;
	for (auto it = dialogs.rbegin(); it != dialogs.rend(); ++it)
		if (*it && (*it)->applyFocusState(target, focused, reason)) return true;
	for (auto it = controls.rbegin(); it != controls.rend(); ++it)
		if (*it && (*it)->applyFocusState(target, focused, reason)) return true;
	return false;
}

bool Window::dispatchKeyEventToTarget(const StellarX::FocusTargetId& target,
	StellarX::RoutedKeyEvent& event, StellarX::KeyRouteResult& result)
{
	for (auto it = modalDialogStack.rbegin(); it != modalDialogStack.rend(); ++it)
		if (*it && (*it)->dispatchKeyEventToTarget(target, event, result)) return true;
	for (auto it = dialogs.rbegin(); it != dialogs.rend(); ++it)
		if (*it && (*it)->dispatchKeyEventToTarget(target, event, result)) return true;
	for (auto it = controls.rbegin(); it != controls.rend(); ++it)
		if (*it && (*it)->dispatchKeyEventToTarget(target, event, result)) return true;
	return false;
}

bool Window::dispatchTextInputEventToTarget(
	const StellarX::FocusTargetId& target,
	const StellarX::RoutedTextInputEvent& event)
{
	for (auto it = modalDialogStack.rbegin(); it != modalDialogStack.rend(); ++it)
		if (*it && (*it)->dispatchTextInputEventToTarget(target, event)) return true;
	for (auto it = dialogs.rbegin(); it != dialogs.rend(); ++it)
		if (*it && (*it)->dispatchTextInputEventToTarget(target, event)) return true;
	for (auto it = controls.rbegin(); it != controls.rend(); ++it)
		if (*it && (*it)->dispatchTextInputEventToTarget(target, event)) return true;
	return false;
}

bool Window::dispatchCompositionEventToTarget(
	const StellarX::FocusTargetId& target,
	const StellarX::RoutedCompositionEvent& event)
{
	for (auto it = modalDialogStack.rbegin(); it != modalDialogStack.rend(); ++it)
		if (*it && (*it)->dispatchCompositionEventToTarget(target, event)) return true;
	for (auto it = dialogs.rbegin(); it != dialogs.rend(); ++it)
		if (*it && (*it)->dispatchCompositionEventToTarget(target, event)) return true;
	for (auto it = controls.rbegin(); it != controls.rend(); ++it)
		if (*it && (*it)->dispatchCompositionEventToTarget(target, event)) return true;
	return false;
}

bool Window::dispatchTextSelectionGestureToTarget(
	const StellarX::FocusTargetId& target,
	const StellarX::RoutedTextSelectionGestureEvent& event,
	bool& accepted)
{
	for (auto it = modalDialogStack.rbegin(); it != modalDialogStack.rend(); ++it)
		if (*it && (*it)->dispatchTextSelectionGestureToTarget(
			target, event, accepted)) return true;
	for (auto it = dialogs.rbegin(); it != dialogs.rend(); ++it)
		if (*it && (*it)->dispatchTextSelectionGestureToTarget(
			target, event, accepted)) return true;
	for (auto it = controls.rbegin(); it != controls.rend(); ++it)
		if (*it && (*it)->dispatchTextSelectionGestureToTarget(
			target, event, accepted)) return true;
	return false;
}

bool Window::dispatchKeyboardSuspensionToTarget(
	const StellarX::FocusTargetId& target)
{
	for (auto it = modalDialogStack.rbegin(); it != modalDialogStack.rend(); ++it)
		if (*it && (*it)->dispatchKeyboardSuspensionToTarget(target)) return true;
	for (auto it = dialogs.rbegin(); it != dialogs.rend(); ++it)
		if (*it && (*it)->dispatchKeyboardSuspensionToTarget(target)) return true;
	for (auto it = controls.rbegin(); it != controls.rend(); ++it)
		if (*it && (*it)->dispatchKeyboardSuspensionToTarget(target)) return true;
	return false;
}

bool Window::dispatchKeyboardResumptionToTarget(
	const StellarX::FocusTargetId& target)
{
	for (auto it = modalDialogStack.rbegin(); it != modalDialogStack.rend(); ++it)
		if (*it && (*it)->dispatchKeyboardResumptionToTarget(target)) return true;
	for (auto it = dialogs.rbegin(); it != dialogs.rend(); ++it)
		if (*it && (*it)->dispatchKeyboardResumptionToTarget(target)) return true;
	for (auto it = controls.rbegin(); it != controls.rend(); ++it)
		if (*it && (*it)->dispatchKeyboardResumptionToTarget(target)) return true;
	return false;
}

StellarX::FocusTargetId Window::dialogRootOnFocusPath(
	const StellarX::FocusTargetId* path, std::size_t pathLength) const
{
	if (!path)
		return {};
	for (std::size_t index = 0; index < pathLength; ++index)
	{
		const StellarX::FocusTargetId candidate = path[index];
		if (!candidate || candidate.subNode != 0)
			continue;
		for (auto it = modalDialogStack.rbegin(); it != modalDialogStack.rend(); ++it)
			if (*it && (*it)->getHandle() == candidate.owner) return candidate;
		for (auto it = dialogs.rbegin(); it != dialogs.rend(); ++it)
		{
			auto* dialog = dynamic_cast<Dialog*>(it->get());
			if (dialog && dialog->getHandle() == candidate.owner) return candidate;
		}
	}
	return {};
}

bool Window::dispatchDialogFrameworkDefault(
	StellarX::FocusTargetId dialogRoot, const StellarX::KeyEvent& key)
{
	if (!dialogRoot || dialogRoot.subNode != 0)
		return false;
	for (auto it = modalDialogStack.rbegin(); it != modalDialogStack.rend(); ++it)
	{
		Dialog* dialog = *it;
		if (dialog && dialog->getHandle() == dialogRoot.owner)
			return dialog->handleFrameworkDefault(key);
	}
	for (auto it = dialogs.rbegin(); it != dialogs.rend(); ++it)
	{
		auto* dialog = dynamic_cast<Dialog*>(it->get());
		if (dialog && dialog->getHandle() == dialogRoot.owner)
			return dialog->handleFrameworkDefault(key);
	}
	return false;
}

Window::ModalDialogRegistration Window::registerModalDialog(Dialog& dialog)
{
	if (std::find(modalDialogStack.begin(), modalDialogStack.end(), &dialog) !=
		modalDialogStack.end())
	{
		SX_LOGE("Dialog") << "modal dialog registered twice";
		return {};
	}
	modalDialogStack.push_back(&dialog);
	return ModalDialogRegistration(*this, dialog);
}

void Window::unregisterModalDialog(Dialog& dialog) noexcept
{
	if (!modalDialogStack.empty() && modalDialogStack.back() == &dialog)
	{
		modalDialogStack.pop_back();
		return;
	}

	const auto it = std::find(modalDialogStack.begin(), modalDialogStack.end(), &dialog);
	if (it != modalDialogStack.end())
	{
		SX_LOGE("Dialog") << "modal dialog registration order violated";
		modalDialogStack.erase(it);
	}
}

Dialog* Window::findNonModalDialog(StellarX::FocusTargetId root) const
{
	if (!root || root.subNode != 0)
		return nullptr;
	for (const auto& item : dialogs)
	{
		auto* dialog = dynamic_cast<Dialog*>(item.get());
		if (dialog && !dialog->model() && dialog->getHandle() == root.owner)
			return dialog;
	}
	return nullptr;
}

Dialog* Window::topmostVisibleNonModalDialog() const
{
	for (auto it = dialogs.rbegin(); it != dialogs.rend(); ++it)
	{
		auto* dialog = dynamic_cast<Dialog*>(it->get());
		if (dialog && !dialog->model() && dialog->IsVisible())
			return dialog;
	}
	return nullptr;
}

Dialog* Window::hitTestTopmostNonModalDialog(int x, int y) const
{
	for (auto it = dialogs.rbegin(); it != dialogs.rend(); ++it)
	{
		auto* dialog = dynamic_cast<Dialog*>(it->get());
		if (!dialog || dialog->model() || !dialog->IsVisible())
			continue;
		if (x >= dialog->getX() && x <= dialog->getX() + dialog->getWidth() &&
			y >= dialog->getY() && y <= dialog->getY() + dialog->getHeight())
			return dialog;
	}
	return nullptr;
}

void Window::requestNonModalDialogActivation(Dialog& dialog)
{
	if (dialog.model() || !dialog.IsVisible())
		return;
	pendingNonModalDialogRoot = { dialog.getHandle(), 0 };
}

void Window::commitPendingNonModalDialogActivation(Dialog& dialog)
{
	const StellarX::FocusTargetId root{ dialog.getHandle(), 0 };
	if (pendingNonModalDialogRoot != root || dialog.model() || !dialog.IsVisible())
		return;
	pendingNonModalDialogRoot = {};
	activateNonModalDialog(dialog);
}

bool Window::activateNonModalDialog(Dialog& dialog)
{
	const StellarX::FocusTargetId root{ dialog.getHandle(), 0 };
	if (dialog.model() || !dialog.IsVisible() || findNonModalDialog(root) != &dialog)
		return false;
	if (activeNonModalDialogRoot == root && activeNonModalDialogScope)
		return true;
	if (dialog.needsInitialization || !dialog.defaultFocusTarget)
	{
		pendingNonModalDialogRoot = root;
		return false;
	}

	deactivateNonModalDialog(false);
	// 非模态 scope 必须位于当前栈顶。模态循环期间若有程序化 Show，
	// 只保留 pending，请求等最内层模态退出后再提交。
	if (!modalDialogStack.empty())
	{
		pendingNonModalDialogRoot = root;
		nonModalDialogScopeNeedsReconcile = true;
		return false;
	}

	const StellarX::FocusScopeId scope = focusManager->pushScope(
		root, dialog.defaultFocusTarget);
	if (!scope)
	{
		SX_LOGE("Dialog") << "failed to activate non-modal focus scope";
		return false;
	}
	activeNonModalDialogRoot = root;
	activeNonModalDialogScope = scope;
	nonModalDialogScopeNeedsReconcile = false;
	return true;
}

void Window::deactivateNonModalDialog(bool clearPending)
{
	if (clearPending)
		pendingNonModalDialogRoot = {};
	if (!activeNonModalDialogScope)
	{
		activeNonModalDialogRoot = {};
		if (clearPending)
			nonModalDialogScopeNeedsReconcile = false;
		return;
	}

	if (focusManager->activeScope() != activeNonModalDialogScope)
	{
		nonModalDialogScopeNeedsReconcile = true;
		return;
	}

	const StellarX::FocusScopeId scope = activeNonModalDialogScope;
	activeNonModalDialogRoot = {};
	activeNonModalDialogScope = {};
	if (!focusManager->popScope(scope))
		SX_LOGE("Dialog") << "failed to pop non-modal focus scope";
	else if (clearPending)
		nonModalDialogScopeNeedsReconcile = false;
}

void Window::onNonModalDialogClosed(StellarX::ControlHandle dialogHandle)
{
	const StellarX::FocusTargetId root{ dialogHandle, 0 };
	if (pendingNonModalDialogRoot == root)
		pendingNonModalDialogRoot = {};
	if (activeNonModalDialogRoot != root)
		return;

	deactivateNonModalDialog(false);
	if (activeNonModalDialogRoot)
		return; // 仍被更内层模态 scope 压住，退出模态后再协调。
	if (Dialog* next = topmostVisibleNonModalDialog())
		activateNonModalDialog(*next);
}

void Window::reconcileNonModalDialogScope()
{
	if (!modalDialogStack.empty())
	{
		Dialog* active = findNonModalDialog(activeNonModalDialogRoot);
		if (pendingNonModalDialogRoot ||
			(activeNonModalDialogRoot && (!active || !active->IsVisible())))
			nonModalDialogScopeNeedsReconcile = true;
		return;
	}

	Dialog* active = findNonModalDialog(activeNonModalDialogRoot);
	if (activeNonModalDialogRoot && (!active || !active->IsVisible()))
	{
		nonModalDialogScopeNeedsReconcile = true;
		deactivateNonModalDialog(false);
	}

	if (pendingNonModalDialogRoot)
	{
		if (Dialog* pending = findNonModalDialog(pendingNonModalDialogRoot))
		{
			if (pending->IsVisible() && !pending->needsInitialization)
			{
				pendingNonModalDialogRoot = {};
				activateNonModalDialog(*pending);
				return;
			}
		}
		else
		{
			pendingNonModalDialogRoot = {};
		}
	}

	if (!activeNonModalDialogRoot && nonModalDialogScopeNeedsReconcile)
	{
		nonModalDialogScopeNeedsReconcile = false;
		if (Dialog* next = topmostVisibleNonModalDialog())
			activateNonModalDialog(*next);
	}
}

void Window::addDialog(std::unique_ptr<Control> dlg)
{
	// 新增非模态对话框：管理顺序决定事件优先级（顶层从后往前）
	dlg->setHostWindow(this);
	dialogs.push_back(std::move(dlg));
}

bool Window::hasNonModalDialogWithCaption(const std::string& caption, const std::string& message) const
{
	// 查询是否存在“可见且非模态”的对话框（用于避免重复弹）
	for (const auto& dptr : dialogs)
	{
		if (!dptr) continue;
		if (auto* d = dynamic_cast<Dialog*>(dptr.get()))
			if (d->IsVisible() && !d->model() && d->GetCaption() == caption && d->GetText() == message)
			{
				dialogOpen = true;
				return true;
			}
	}
	return false;
}

HWND Window::getHwnd() const
{
	return hWnd;
}

int Window::getWidth() const
{
	return width;
}

int Window::getHeight() const
{
	return height;
}

int Window::getPendingWidth() const
{
	return pendingW;
}

int Window::getPendingHeight() const
{
	return pendingH;
}

std::string Window::getHeadline() const
{
	return headline;
}

COLORREF Window::getBkcolor() const
{
	return wBkcolor;
}

IMAGE* Window::getBkImage() const
{
	return background.get();
}

std::string Window::getBkImageFile() const
{
	return bkImageFile;
}

void Window::pumpResizeIfNeeded()
{
	if (!needResizeDirty) return;
	SX_LOGD("Resize")
		<< SX_T("执行 pumpResizeIfNeeded：needResizeDirty=",
			"pumpResizeIfNeeded: needResizeDirty=")
		<< (needResizeDirty ? 1 : 0)
		<< SX_T("（需要进行一次缩放收口/重排重绘）", "");


	RECT rc; GetClientRect(hWnd, &rc);
	const int finalW = max(minClientW, rc.right - rc.left);
	const int finalH = max(minClientH, rc.bottom - rc.top);
	if (finalW == width && finalH == height) { needResizeDirty = false; return; }

	SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);
	BeginBatchDraw();

	// 调整尺寸与背景
	Resize(nullptr, finalW, finalH);
	GetClientRect(hWnd, &rc);
	width = rc.right - rc.left; height = rc.bottom - rc.top;

	// 通知控件/对话框
	for (auto& c : controls)
	{
		adaptiveLayout(c, finalH, finalW);
	}
	for (auto& d : dialogs)
		if (auto* dd = dynamic_cast<Dialog*>(d.get()))
			dd->recenterInHostWindow(); // 窗口变化时仅重新居中，不拉伸 Dialog 自身

	// 重绘
	redrawScene(true, true);

	EndBatchDraw();
	SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);

	// 原实现在此调用 InvalidateRect 导致系统再次发送 WM_PAINT，从而重复绘制，
	// 这里改为 ValidateRect：直接标记区域为有效，通知系统我们已完成绘制，不必再触发 WM_PAINT。
	// 这样可以避免收口阶段的绘制与系统重绘叠加造成顺序错乱。
	ValidateRect(hWnd, nullptr);

	needResizeDirty = false;
	clearManagedRepaintState();
}
void Window::scheduleResizeFromModal(int w, int h)
{
	if (w < minClientW) w = minClientW;
	if (h < minClientH) h = minClientH;
	if (w > 10000) w = 10000;
	if (h > 10000) h = 10000;

	if (w != width || h != height)
	{
		pendingW = w;
		pendingH = h;
		needResizeDirty = true;   // 交给 pumpResizeIfNeeded 做统一收口+重绘
		SX_LOGD("Resize")
			<< SX_T("模态对话框触发缩放调度：pending=(",
				"scheduleResizeFromModal: pending=(")
			<< pendingW << "x" << pendingH
			<< SX_T(")，needResizeDirty=1（标记需要缩放收口）",
				"), needResizeDirty=1");


	}
}

void Window::adaptiveLayout(std::unique_ptr<Control>& c, const int finalH, const int finalW)
{
	// 顶层窗口布局收口：
	// 先用统一解算器求出控件新的运行态矩形，再通知控件“外部环境已变化”。
	// onWindowResize() 负责快照失效/标脏，不再承担布局公式。
	const StellarX::ResolvedLayoutRect rect =
		c->resolveLayoutRect(this->localwidth, this->localheight, 0, 0, finalW, finalH);
	c->applyResolvedLayoutRect(rect);
	c->onWindowResize();
}
