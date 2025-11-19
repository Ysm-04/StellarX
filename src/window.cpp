#include "Window.h"
#include "Dialog.h"

#include <easyx.h>
#include <algorithm>

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
    localwidth  = minClientW = pendingW = width = w;
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
    // 析构：释放背景图对象并关闭 EasyX 图形环境
    if (background) delete background;
    background = nullptr;
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

        ApplyMinSizeOnSizing(prc, w, h, self->minClientW, self->minClientH);
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

    ApplyResizableStyle(hWnd, useComposited);

    // 关闭类样式的整窗重绘标志（减少尺寸变化时的整窗 redraw）
    LONG_PTR cls = GetClassLongPtr(hWnd, GCL_STYLE);
    cls &= ~(CS_HREDRAW | CS_VREDRAW);
    SetClassLongPtr(hWnd, GCL_STYLE, cls);

    setbkcolor(wBkcolor);
    cleardevice();

    BeginBatchDraw();
    for (auto& c : controls)
    {
        c->draw();
    }
    for (auto& d : dialogs)
    {
        d->draw();
    }
    EndBatchDraw();
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
        delete background;
        background = nullptr;
    }
    background = new IMAGE;
    loadimage(background, bkImageFile.c_str(), width, height, true);
    putimage(0, 0, background);

    BeginBatchDraw();
    for (auto& c : controls)
    {
        c->setDirty(true);
        c->draw();
    }
    for (auto& d : dialogs)
    {
        d->draw();
    }
    EndBatchDraw();
}

// ---------------- 事件循环 ----------------

/**
 * runEventLoop()
 * 作用：驱动输入/窗口消息；集中处理“统一收口重绘”。
 * 关键策略：
 *  - WM_SIZE：始终更新 pendingW/H（即使在拉伸中也只记录不立即绘制）；
 *  - needResizeDirty：当尺寸确实变化时置位，随后在循环尾进行一次性重绘；
 *  - 非模态对话框优先消费事件（顶层从后往前）；再交给普通控件。
 */
int Window::runEventLoop()
{
    ExMessage msg;
    bool running = true;

    // 说明：统一使用 needResizeDirty 作为“收口重绘”的唯一标志位
    //       不再引入额外 pendingResize 等状态，避免分叉导致状态不一致。
    while (running)
    {
        bool consume = false;

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
                    }
                }
                continue;
            }

            // 输入优先：先给顶层“非模态对话框”，再传给普通控件
            for (auto it = dialogs.rbegin(); it != dialogs.rend(); ++it)
            {
                auto& d = *it;
                if (d->IsVisible() && !d->model())
                {
                    consume = d->handleEvent(msg);
                }
                if (consume) break;
            }
            if (!consume)
            {
                for (auto& c : controls)
                {
                    consume = c->handleEvent(msg);
                    if (consume) break;
                }
            }
        }
         //如果有对话框打开或者关闭强制重绘
		bool needredraw = false;
		for (auto& d : dialogs)
		{
			needredraw = d->IsVisible();
			if (needredraw)break;
		}
		if (needredraw || dialogClose)
		{
			// 对话框关闭后，需要手动合成一个鼠标移动消息并分发给所有普通控件，
			// 以便它们能及时更新悬停状态（hover），否则悬停状态可能保持错误状态。
			// 先把当前鼠标位置转换为客户区坐标，并合成一次 WM_MOUSEMOVE，先分发给控件更新 hover 状态
			POINT pt;
			if (GetCursorPos(&pt))
			{
				ScreenToClient(this->hWnd, &pt);
				ExMessage mm;
				mm.message = WM_MOUSEMOVE;
				mm.x = (short)pt.x;
				mm.y = (short)pt.y;
				// 只分发给 window 层控件（因为 dialog 已经关闭或即将关闭）
				for (auto& c : controls)
					c->handleEvent(mm);
			}

			BeginBatchDraw();
			// 先绘制普通控件
			for (auto& c : controls)
				c->draw();
			// 然后绘制对话框（确保对话框在最上层）
			for (auto& d : dialogs)
			{
				if (!d->model() && d->IsVisible())
					d->setDirty(true);
				d->draw();
			}
			EndBatchDraw();
			needredraw = false;

		}
        // —— 统一收口（needResizeDirty 为真时执行一次性重绘）——
        if (needResizeDirty)
        {
            // 以“实际客户区尺寸”为准，防止 pending 与真实尺寸出现偏差
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            int actualWidth = clientRect.right - clientRect.left;
            int actualHeight = clientRect.bottom - clientRect.top;

            const int finalW = (std::max)(minClientW, actualWidth);
            const int finalH = (std::max)(minClientH, actualHeight);

            // 变化过大/异常场景保护
            if (finalW != width || finalH != height)
            {
                if (abs(finalW - width) > 1000 || abs(finalH - height) > 1000)
                {
                    // 认为是异常帧，跳过本次（不改变任何状态）
                    needResizeDirty = false;
                    continue;
                }

                // 再次冻结窗口更新，保证批量绘制的原子性
                SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);

                BeginBatchDraw();

                // 调整底层画布尺寸
                if (finalW != width || finalH != height)
                {
                    Resize(nullptr, finalW, finalH);

                    // 重取一次实际客户区尺寸做确认
                    GetClientRect(hWnd, &clientRect);
                    int confirmedWidth = clientRect.right - clientRect.left;
                    int confirmedHeight = clientRect.bottom - clientRect.top;

                    int renderWidth = confirmedWidth;
                    int renderHeight = confirmedHeight;

                    // 背景：若设置了背景图则重载并铺满；否则清屏为纯色
                    if (background && !bkImageFile.empty())
                    {
                        delete background;
                        background = new IMAGE;
                        loadimage(background, bkImageFile.c_str(), renderWidth, renderHeight, true);
                        putimage(0, 0, background);
                    }
                    else
                    {
                        setbkcolor(wBkcolor);
                        cleardevice();
                    }

                    // 最终提交“当前已应用尺寸”（用于外部查询/下次比较）
                    width = renderWidth;
                    height = renderHeight;
                }

                // 批量通知控件“窗口尺寸变化”，并标记重绘
                for (auto& c : controls)
                {
                    adaptiveLayout(c,finalH,finalW);
                    c->onWindowResize();
                }
                for (auto& d : dialogs)
                {
                    if (auto dd = dynamic_cast<Dialog*>(d.get()))
                    {
                        dd->setDirty(true);
                        dd->setInitialization(true);
                    }
                }

                // 统一批量绘制
                for (auto& c : controls) c->draw();
                for (auto& d : dialogs) d->draw();

                EndBatchDraw();

                // 解冻后标记区域有效，避免系统再次触发 WM_PAINT 覆盖自绘内容。
                SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
                ValidateRect(hWnd, nullptr);
            }

            needResizeDirty = false; // 收口完成，清标志
        }

        // 轻微睡眠，削峰填谷（不阻塞拖拽体验）
        Sleep(10);
    }

    return 1;
}

// ---------------- 其余接口 ----------------

void Window::setBkImage(std::string pImgFile)
{
    // 更换背景图：立即加载并绘制一次；同时将所有控件标 dirty 并重绘
    if (background) delete background;
    background = new IMAGE;
    bkImageFile = std::move(pImgFile);

    loadimage(background, bkImageFile.c_str(), width, height, true);
    putimage(0, 0, background);

    BeginBatchDraw();
    for (auto& c : controls)
    {
        c->setDirty(true);
        c->draw();
    }
    for (auto& d : dialogs)
    {
        d->setDirty(true);
        d->draw();
    }
    EndBatchDraw();
}

void Window::setBkcolor(COLORREF c)
{
    // 更换纯色背景：立即清屏并批量重绘控件/对话框
    wBkcolor = c;
    setbkcolor(wBkcolor);
    cleardevice();

    BeginBatchDraw();
    for (auto& c : controls)
    {
        c->setDirty(true);
        c->draw();
    }
    for (auto& d : dialogs)
    {
        d->setDirty(true);
        d->draw();
    }
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
    controls.push_back(std::move(control));
}

void Window::addDialog(std::unique_ptr<Control> dlg)
{
    // 新增非模态对话框：管理顺序决定事件优先级（顶层从后往前）
    dialogs.push_back(std::move(dlg));
}

bool Window::hasNonModalDialogWithCaption(const std::string& caption, const std::string& message) const
{
    // 查询是否存在“可见且非模态”的对话框（用于避免重复弹）
    for (const auto& dptr : dialogs)
    {
        if (!dptr) continue;
        if (auto* d = dynamic_cast<Dialog*>(dptr.get()))
        {
            if (d->IsVisible() && !d->model() && d->GetCaption() == caption && d->GetText() == message)
            {
                return true;
            }
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
    // 注意：这里返回 pendingW
    // 表示“最近一次收到的尺寸”（可能尚未应用到画布，最终以收口时的 width 为准）
    return pendingW;
}

int Window::getHeight() const
{
    // 同上，返回 pendingH（与 getWidth 对应）
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
    return background;
}

std::string Window::getBkImageFile() const
{
    return bkImageFile;
}

std::vector<std::unique_ptr<Control>>& Window::getControls()
{
    return controls;
}

void Window::processWindowMessage(const ExMessage& msg)
{
    if (msg.message == WM_SIZE && msg.wParam != SIZE_MINIMIZED)
    {
        const int nw = LOWORD(msg.lParam);
        const int nh = HIWORD(msg.lParam);
        if (nw >= minClientW && nh >= minClientH && nw <= 10000 && nh <= 10000)
        {
            if (nw != width || nh != height)
            {
                pendingW = nw;
                pendingH = nh;
                needResizeDirty = true; // 统一由 pumpResizeIfNeeded 来收口
            }
        }
    }
}

void Window::pumpResizeIfNeeded()
{
    if (!needResizeDirty) return;

    RECT rc; GetClientRect(hWnd, &rc);
    const int finalW = max(minClientW, rc.right - rc.left);
    const int finalH = max(minClientH, rc.bottom - rc.top);
    if (finalW == width && finalH == height) { needResizeDirty = false; return; }

    SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);
    BeginBatchDraw();

    // Resize + 背景
    Resize(nullptr, finalW, finalH);
    GetClientRect(hWnd, &rc);
    if (background && !bkImageFile.empty())
    {
        delete background; background = new IMAGE;
        loadimage(background, bkImageFile.c_str(), rc.right - rc.left, rc.bottom - rc.top, true);
        putimage(0, 0, background);
    }
    else
    {
        setbkcolor(wBkcolor);
        cleardevice();

    }
    width = rc.right - rc.left; height = rc.bottom - rc.top;

    // 通知控件/对话框
    for (auto& c : controls)
    {
        adaptiveLayout(c, finalH, finalW);
        c->onWindowResize();
    }
    for (auto& d : dialogs)
        if (auto* dd = dynamic_cast<Dialog*>(d.get()))
            dd->setInitialization(true); // 强制对话框在新尺寸下重建布局/快照

    // 重绘
    for (auto& c : controls) c->draw();
    for (auto& d : dialogs) d->draw();

    EndBatchDraw();
    SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);

    // 原实现在此调用 InvalidateRect 导致系统再次发送 WM_PAINT，从而重复绘制，
    // 这里改为 ValidateRect：直接标记区域为有效，通知系统我们已完成绘制，不必再触发 WM_PAINT。
    // 这样可以避免收口阶段的绘制与系统重绘叠加造成顺序错乱。
    ValidateRect(hWnd, nullptr);

    needResizeDirty = false;
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
    }
}

void Window::adaptiveLayout(std::unique_ptr<Control>& c, const int finalH, const int finalW)
{
    int origParentW = this->localwidth;
    int origParentH = this->localheight;
    if (c->getLayoutMode() == StellarX::LayoutMode::AnchorToEdges)
    {
        if ((StellarX::Anchor::Left == c->getAnchor_1() && StellarX::Anchor::Right == c->getAnchor_2())
            || (StellarX::Anchor::Right == c->getAnchor_1() && StellarX::Anchor::Left == c->getAnchor_2()))
        {
            int origRightDist = origParentW - (c->getLocalX() + c->getLocalWidth());
            int newWidth = finalW - c->getLocalX() - origRightDist;
            c->setWidth(newWidth);
            // 左侧距离固定，ctrl->x 保持为 localx 相对窗口左侧（父容器为窗口，偏移0）
            c->setX(c->getLocalX());
        }
        else if ((StellarX::Anchor::Left == c->getAnchor_1() && StellarX::Anchor::NoAnchor == c->getAnchor_2())
            || (StellarX::Anchor::NoAnchor == c->getAnchor_1() && StellarX::Anchor::Left == c->getAnchor_2())
            || (StellarX::Anchor::Left == c->getAnchor_1() && StellarX::Anchor::Left == c->getAnchor_2()))
        {
            // 仅左锚定：宽度固定不变
            c->setX(c->getLocalX());
            c->setWidth(c->getLocalWidth());
        }
        else if ((StellarX::Anchor::Right == c->getAnchor_1() && StellarX::Anchor::NoAnchor == c->getAnchor_2())
            || (StellarX::Anchor::NoAnchor == c->getAnchor_1() && StellarX::Anchor::Right == c->getAnchor_2())
            || (StellarX::Anchor::Right == c->getAnchor_1() && StellarX::Anchor::Right == c->getAnchor_2()))
        {
            int origRightDist = origParentW - (c->getLocalX() + c->getLocalWidth());
            c->setWidth(c->getLocalWidth()); // 宽度不变
            c->setX(finalW - origRightDist - c->getWidth());
        }
        else if (StellarX::Anchor::NoAnchor == c->getAnchor_1() && StellarX::Anchor::NoAnchor == c->getAnchor_2())
        {
            c->setX(c->getLocalX());
            c->setWidth(c->getLocalWidth());
        }

        if ((StellarX::Anchor::Top == c->getAnchor_1() && StellarX::Anchor::Bottom == c->getAnchor_2())
            || (StellarX::Anchor::Bottom == c->getAnchor_1() && StellarX::Anchor::Top == c->getAnchor_2()))
        {
            // 上下锚定：高度随窗口变化
            int origBottomDist = origParentH - (c->getLocalY() + c->getLocalHeight());
            int newHeight = finalH - c->getLocalY() - origBottomDist;
            c->setHeight(newHeight);
            c->setY(c->getLocalY());
        }
        else if ((StellarX::Anchor::Top == c->getAnchor_1() && StellarX::Anchor::NoAnchor == c->getAnchor_2())
            || (StellarX::Anchor::NoAnchor == c->getAnchor_1() && StellarX::Anchor::Top == c->getAnchor_2())
            || (StellarX::Anchor::Top == c->getAnchor_1() && StellarX::Anchor::Top == c->getAnchor_2()))
        {
            c->setY(c->getLocalY());
            c->setHeight(c->getLocalHeight());
        }
        else if ((StellarX::Anchor::Bottom == c->getAnchor_1() && StellarX::Anchor::NoAnchor == c->getAnchor_2())
            || (StellarX::Anchor::NoAnchor == c->getAnchor_1() && StellarX::Anchor::Bottom == c->getAnchor_2())
            || (StellarX::Anchor::Bottom == c->getAnchor_1() && StellarX::Anchor::Bottom == c->getAnchor_2()))
        {
            int origBottomDist = origParentH - (c->getLocalY() + c->getLocalHeight());
            c->setHeight(c->getLocalHeight());
            c->setY(finalH - origBottomDist - c->getHeight());
        }
        else {
            // 垂直无锚点：默认为顶部定位，高度固定
            c->setY(c->getLocalY());
            c->setHeight(c->getLocalHeight());
        }
    }
}

