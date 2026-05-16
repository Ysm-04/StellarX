#include "Window.h"
#include "Dialog.h"
#include"SxLog.h"
#include <easyx.h>
#include <algorithm>
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

static void collectManagedControlOverlays(const std::vector<std::unique_ptr<Control>>& controls,
	Control* repaintRoot, const RECT& coverage, std::vector<Control*>& overlays);

bool Window::isManagedDispatchActive() const
{
	return managedDispatchActive;
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
}

/**
 * flushManagedRepaint()
 * 作用：提交当前事件分发阶段累计的托管重绘请求。
 * 提交顺序：
 *  1）先根据 coverage 找出需要补画的非模态 Dialog；
 *  2）再按 Window::controls 的层级顺序提交受影响的普通 root；
 *  3）最后把相交的 Dialog 补画回最上层。
 * 说明：
 *  - 这里不做整场景重画，而是只画本轮登记的 root；
 *  - 之所以按 controls 顺序提交，而不是按登记顺序，是为了保持顶层控件原有的 z-order。
 */
void Window::flushManagedRepaint()
{
	if (!managedSceneDirty || !hWnd)
		return;

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

	auto processManagedRoot = [&](Control* root, const RECT& initialCoverage)
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

			size_t controlStartIdx = controls.size();
			for (size_t i = 0; i < controls.size(); ++i)
			{
				if (controls[i].get() == root)
				{
					controlStartIdx = i + 1;
					break;
				}
			}

			// 顶层普通控件的 overlay 补画必须是“传递式”的：
			// 如果 A 重画后把上层按钮 B 也重画出来，而 B 自己又伸进更上层的 C，
			// 那么 C 也必须继续补画回来，而不能只看最初 root 的 coverage 一跳收集。
			if (controlStartIdx < controls.size())
			{
				for (size_t i = controlStartIdx; i < controls.size(); ++i)
				{
					Control* current = controls[i].get();
					if (!current || !current->IsVisible())
						continue;
					const RECT currentCoverage = current->getManagedRepaintCoverageRect();
					if (!SxRectsIntersect(currentCoverage, workingCoverage))
						continue;

					redrawOverlayUnit(current);
					unionCoverage(workingCoverage, current->getManagedRepaintCoverageRect());
				}
			}

			size_t dialogStartIdx = 0;
			for (size_t i = 0; i < dialogs.size(); ++i)
			{
				if (dialogs[i].get() == root)
				{
					dialogStartIdx = i + 1;
					break;
				}
			}

			// Dialog 永远位于普通顶层控件之上，也要使用扩张后的 coverage 做传递式补画。
			for (size_t i = dialogStartIdx; i < dialogs.size(); ++i)
			{
				Control* dialog = dialogs[i].get();
				if (!dialog || !dialog->IsVisible())
					continue;
				const RECT dialogCoverage = dialog->getManagedRepaintCoverageRect();
				if (!SxRectsIntersect(dialogCoverage, workingCoverage))
					continue;

				redrawOverlayUnit(dialog);
				unionCoverage(workingCoverage, dialog->getManagedRepaintCoverageRect());
			}
		};

	for (auto& control : controls)
	{
		for (auto& item : managedRepaintItems)
		{
			if (item.root == control.get())
			{
				processManagedRoot(item.root, item.coverage);
				break;
			}
		}
	}

	for (auto& dialog : dialogs)
	{
		for (auto& item : managedRepaintItems)
		{
			if (item.root == dialog.get())
			{
				processManagedRoot(item.root, item.coverage);
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
 * collectManagedDialogOverlays(repaintRoot, coverage, overlays)
 * 作用：找出在本轮提交后需要重新盖到最上层的非模态 Dialog。
 * 规则：
 *  - 如果 repaintRoot 本身就是 Dialog，则从它自己开始往上层 Dialog 收集；
 *  - 如果 repaintRoot 是普通控件，则收集所有与 coverage 相交的可见 Dialog。
 */
void Window::collectManagedDialogOverlays(Control* repaintRoot, const RECT& coverage, std::vector<Control*>& overlays)
{
	size_t startIdx = 0;
	if (auto* dialogRoot = dynamic_cast<Dialog*>(repaintRoot))
	{
		for (size_t i = 0; i < dialogs.size(); ++i)
		{
			if (dialogs[i].get() == dialogRoot)
			{
				startIdx = i;
				break;
			}
		}
	}

	for (size_t i = startIdx; i < dialogs.size(); ++i)
	{
		Control* dialog = dialogs[i].get();
		if (!dialog || !dialog->IsVisible())
			continue;

		if (dialog == repaintRoot || SxRectsIntersect(dialog->getBoundsRect(), coverage))
		{
			if (std::find(overlays.begin(), overlays.end(), dialog) == overlays.end())
				overlays.push_back(dialog);
		}
	}
}

/**
 * collectManagedControlOverlays(repaintRoot, coverage, overlays)
 * 作用：找出在本轮 root 提交后，需要重新补画回上层的普通顶层控件。
 * 规则：
 *  - 只处理 Window::controls 这一层的直接兄弟；
 *  - 从 repaintRoot 在 controls 中的位置之后开始收集；
 *  - 仅收集可见且与 coverage 相交的控件。
 */
static void collectManagedControlOverlays(const std::vector<std::unique_ptr<Control>>& controls,
	Control* repaintRoot, const RECT& coverage, std::vector<Control*>& overlays)
{
	size_t startIdx = controls.size();
	for (size_t i = 0; i < controls.size(); ++i)
	{
		if (controls[i].get() == repaintRoot)
		{
			startIdx = i + 1;
			break;
		}
	}

	if (startIdx > controls.size())
		return;

	for (size_t i = startIdx; i < controls.size(); ++i)
	{
		Control* control = controls[i].get();
		if (!control || !control->IsVisible())
			continue;

		if (SxRectsIntersect(control->getBoundsRect(), coverage))
		{
			if (std::find(overlays.begin(), overlays.end(), control) == overlays.end())
				overlays.push_back(control);
		}
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
}

Window::Window(int w, int h, int mode, COLORREF bk)
{
	localwidth = minClientW = pendingW = width = w;
	localheight = minClientH = pendingH = height = h;
	windowMode = mode;
	wBkcolor = bk;
}

Window::Window(int w, int h, int mode, COLORREF bk, std::string title)
{
	localwidth = minClientW = pendingW = width = w;
	localheight = minClientH = pendingH = height = h;
	windowMode = mode;
	wBkcolor = bk;
	headline = std::move(title);
}

Window::~Window()
{
	// 先销毁控件树，再关闭图形环境，避免控件析构时访问已关闭的 EasyX 上下文。
	dialogs.clear();
	controls.clear();
	background.reset();
	if (hWnd && procHooked && oldWndProc)
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

	// 关键点①：禁止系统擦背景，避免和我们自己的清屏/双缓冲打架造成闪烁
	if (m == WM_ERASEBKGND)
	{
		return 1;
	}

	// 关键点②：拉伸开始 → 冻结重绘（系统调整窗口矩形时不触发即时重绘，防止抖）
	if (m == WM_ENTERSIZEMOVE)
	{
		SX_LOGI("Resize") << SX_T("WM_ENTERSIZEMOVE: 开始测量尺寸","WM_ENTERSIZEMOVE: begin sizing");
		self->isSizing = true;
		SendMessage(h, WM_SETREDRAW, FALSE, 0);
		return 0;
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
			return TRUE;
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
		return TRUE;
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
		return 0;
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
		return 0;
	}

	// 其它消息：回落到旧过程
	return self->oldWndProc ? CallWindowProc(self->oldWndProc, h, m, w, l)
		: DefWindowProc(h, m, w, l);
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
	
	// 子类化：让我们的 WndProcThunk 接管窗口消息（仅执行一次）
	if (!procHooked)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);
		oldWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)&Window::WndProcThunk);
		procHooked = (oldWndProc != nullptr);
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

	if (!procHooked)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);
		oldWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)&Window::WndProcThunk);
		procHooked = (oldWndProc != nullptr);
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
	bool running = true;

	// 说明：统一使用 needResizeDirty 作为“收口重绘”的唯一标志位
	//       不再引入额外 pendingResize 等状态，避免分叉导致状态不一致。
	while (running)
	{
	
		bool consume = false; // 事件是否被消费的标志（用于输入事件分发）

		if (peekmessage(&msg, EX_MOUSE | EX_KEY | EX_WINDOW, true))
		{
			if (msg.message == WM_CLOSE)
			{
				running = false;
				return 0;
			}

			// 保险：如果 EX_WINDOW 转译了 GETMINMAXINFO，同样按最小客户区折算处理
			if (msg.message == WM_GETMINMAXINFO)
			{
				auto* mmi = reinterpret_cast<MINMAXINFO*>(msg.lParam);
				RECT rc{ 0, 0, minClientW, minClientH };
				DWORD style = GetWindowLong(hWnd, GWL_STYLE);
				DWORD ex = GetWindowLong(hWnd, GWL_EXSTYLE);
				AdjustWindowRectEx(&rc, style, FALSE, ex);
				mmi->ptMinTrackSize.x = rc.right - rc.left;
				mmi->ptMinTrackSize.y = rc.bottom - rc.top;
				continue;
			}

			// 关键点⑥：WM_SIZE 只记录新尺寸；若非拉伸阶段则立即置位 needResizeDirty
			if (msg.message == WM_SIZE && msg.wParam != SIZE_MINIMIZED)
			{
				const int nw = LOWORD(msg.lParam);
				const int nh = HIWORD(msg.lParam);

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
				continue;
			}

			// 输入优先：先给顶层“非模态对话框”，再传给普通控件。
			// 在 managedDispatchActive 期间，控件只改状态并登记重绘 root，不直接画。
			managedDispatchActive = true;
			for (auto it = dialogs.rbegin(); it != dialogs.rend(); ++it)
			{
				auto& d = *it;
				if (d->IsVisible() && !d->model())
					consume = d->handleEvent(msg);
				if (consume)
				{
					if (!SxIsNoisyMsg(msg.message))
						SX_LOGD("Event") << SX_T("事件被非模态对话框处理：", "Event consumed by non-modal dialog: ")
						<< SxMsgName(msg.message);

					// 非模态对话框吞掉自己的区域内鼠标移动后，底层普通控件收不到“离开”消息，
					// 会残留 hover。这里补一条落在窗口外的合成移动，只用于清理底层 hover，
					// 不会让底层控件重新命中。
					if (msg.message == WM_MOUSEMOVE)
						syncMouseStateAfterOverlayChanged(OverlayMouseSyncMode::ClearBehindOverlay);
					break;
				}
			}
			if (!consume)
			{
				Control* firstConsumer = nullptr;
				if (msg.message == WM_MOUSEMOVE)
				{
					// 顶层普通控件的 hover/tooltip 清理规则：
					// - 第一个命中的兄弟分支收到真实 WM_MOUSEMOVE；
					// - 后续兄弟不再重新命中，只清理旧 hover / tooltip 等瞬时鼠标状态。
					for (auto it = controls.rbegin(); it != controls.rend(); ++it)
					{
						Control* current = it->get();
						if (!consume)
						{
							consume = current->handleEvent(msg);
							if (consume)
								firstConsumer = current;
						}
						else
						{
							// 后续兄弟只清理 hover / press / tooltip 等临时鼠标状态；
							// 这条路径不会再走控件自己的 handleEvent()，所以必须在顶层补登记重绘。
							// 先记录旧 coverage，避免 Tooltip 隐藏后丢失原悬浮层范围。
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
						consume = current->handleEvent(msg);
						if (consume)
						{
							firstConsumer = current;
							break;
						}
					}
				}

				if (firstConsumer && !SxIsNoisyMsg(msg.message))
					SX_LOGD("Event") << SX_T("事件被控件处理：", "Event consumed by control: ")
					<< SxMsgName(msg.message)
					<< SX_T(" id=", " id=") << firstConsumer->getId();
			}
			managedDispatchActive = false;
		}

		// 对话框打开/关闭属于全局层级变化：这里仍然使用整场景重绘兜底，
		// 并在结束后清空本轮托管重绘登记，避免旧的 root 请求延后提交。
		bool needredraw = false;
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

		// 轻微睡眠，削峰填谷（不阻塞拖拽体验）
		Sleep(10);
	}

	return 1;
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

void Window::addControl(std::unique_ptr<Control> control)
{
	// 新增控件：仅加入管理容器，具体绘制在 draw()/收口时统一进行
	control->setHostWindow(this);
	controls.push_back(std::move(control));
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

std::vector<std::unique_ptr<Control>>& Window::getControls()
{
	return controls;
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

	// Resize + 背景
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

