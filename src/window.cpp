#include "Window.h"
#include "Dialog.h"
#include <windows.h>  // 确保包含 Windows API 头文件

Window::Window(int width, int height, int mode)
{
	
	this->pendingW = this->width = width;
	this->pendingH = this->height = height;
    this->windowMode = mode;
}

Window::Window(int width, int height, int mode, COLORREF bkcloc)
{
	this->pendingW = this->width = width;
	this->pendingH = this->height = height;
    this->windowMode = mode;
    this->wBkcolor = bkcloc;
}

Window::Window(int width, int height, int mode, COLORREF bkcloc, std::string headline)
{
	this->pendingW = this->width = width;
	this->pendingH = this->height = height;
    this->windowMode = mode;
    this->wBkcolor = bkcloc;
    this->headline = headline;
}

Window::~Window()
{
    if (background)
        delete background;
    background = nullptr;
    closegraph(); // 确保关闭图形上下文
}


void Window::draw() {
    // 使用 EasyX 创建基本窗口
    hWnd = initgraph(width, height, windowMode);
    SetWindowText(hWnd, headline.c_str());
    // **启用窗口拉伸支持**：添加厚边框和最大化按钮样式  
    LONG style = GetWindowLong(hWnd, GWL_STYLE);
    style |= WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;  // 可调整边框，启用最大化/最小化按钮
    SetWindowLong(hWnd, GWL_STYLE, style);
    // 通知窗口样式变化生效
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    // 设置背景色并清屏
    setbkcolor(wBkcolor);
    cleardevice();
    // 初次绘制所有控件（双缓冲）
    BeginBatchDraw();
    for (auto& control : controls)
	{
        control->draw();
    }
    // （如果有初始对话框，也可绘制 dialogs）
    EndBatchDraw();
}

void Window::draw(std::string imagePath)
{
    // 使用指定图片绘制窗口背景（铺满窗口）
    this->background = new IMAGE(width, height);
	bkImageFile = imagePath;
    hWnd = initgraph(width, height, windowMode);
    SetWindowText(hWnd, headline.c_str());
    loadimage(background, imagePath.c_str(), width, height, true);
    putimage(0, 0, background);
    // 同样应用可拉伸样式
    LONG style = GetWindowLong(hWnd, GWL_STYLE);
    style |= WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;
    SetWindowLong(hWnd, GWL_STYLE, style);
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    // 绘制控件（含对话框）到窗口
    BeginBatchDraw();
    for (auto& control : controls) control->draw();
    for (auto& dlg : dialogs) dlg->draw();
    EndBatchDraw();
}

// 运行主事件循环，处理用户输入和窗口消息
// 此方法会阻塞直到窗口关闭
// 主消息循环优先级：对话框 > 普通控件。
// 重绘策略：为保证视觉一致性，每次有对话框状态变化（打开/关闭）时，
// 会强制重绘所有控件。先绘制普通控件，再绘制对话框（确保对话框在最上层）。
int Window::runEventLoop()
{
	ExMessage msg;
	bool running = true;

	while (running)
	{
		bool consume = false;// 是否处理了消息

		// 处理所有消息
		if (peekmessage(&msg, EX_MOUSE | EX_KEY | EX_WINDOW, true))
		{
			if (msg.message == WM_CLOSE)
			{
				running = false;
				return 0;
			}
			if (msg.message == WM_SIZE)
			{
				if (msg.wParam != SIZE_MINIMIZED)
				{
					const int nw = LOWORD(msg.lParam);
					const int nh = HIWORD(msg.lParam);

					// 仅在尺寸真的变化时标脏
					if (nw > 0 && nh > 0 || (nw != width || nh != height))
					{
						pendingW = nw;
						pendingH = nh;
						needResizeDirty = true;
					}
				}
				continue;//在末尾重绘制窗口
			}
			// 优先处理对话框事件
			for (auto it = dialogs.rbegin(); it != dialogs.rend(); ++it)
			{
				auto& d = *it;
				if (d->IsVisible() && !d->model())
					consume = d->handleEvent(msg);
				if (consume)
					break;
			}

			//普通控件
			if (!consume)
				for (auto it = controls.rbegin(); it != controls.rend(); ++it)
				{
					consume = (*it)->handleEvent(msg);
					if (consume)
						break;
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
		//—— 统一“收口”：尺寸变化后的** 一次性** 重绘 ——
		if (needResizeDirty)
		{
			//确保窗口不会小于初始尺寸
			if (pendingW >= width && pendingH >= height)
				Resize(nullptr, pendingW, pendingH);
			else 
				Resize(nullptr, width, height);
			if (background)
			{
				delete background;
				background = new IMAGE;
				loadimage(background, bkImageFile.c_str(), pendingW, pendingH);
				putimage(0, 0, background);
			}
			
			// 标记所有控件/对话框为脏，确保都补一次背景/外观
			for (auto& c : controls)
			{
				c->setDirty(true);
				c->updateBackground();
				c->draw();
			}
			for (auto& d : dialogs)
			{
				auto dd = dynamic_cast<Dialog*>(d.get());
				dd->setDirty(true);
				dd->setInitialization(true);
				dd->draw();
			}
			needResizeDirty = false;
		}

		// 降低占用
		Sleep(10);
	}
	return 1;
}

void Window::setBkImage(std::string pImgFile)
{
	if(nullptr == background)
		this->background = new IMAGE;
	else
		delete background;
	this->background = new IMAGE;
	this->bkImageFile = pImgFile;
	loadimage(background, pImgFile.c_str(), width, height, true);
	putimage(0, 0, background);
	BeginBatchDraw();
	for (auto& c : controls)
	{
		c->setDirty(true);
		c->draw();
	}
	for (auto& c : dialogs)
	{
		c->setDirty(true);
		c->draw();
	}
	EndBatchDraw();
	
}

void Window::setBkcolor(COLORREF c)
{
	wBkcolor = c;

	setbkcolor(wBkcolor);
	cleardevice();
	// 初次绘制所有控件（双缓冲）
	BeginBatchDraw();
	for (auto& c : controls)
	{
		c->setDirty(true);
		c->draw();
	}
	for (auto& c : dialogs)
	{
		c->setDirty(true);
		c->draw();
	}
	EndBatchDraw();

}

void Window::setHeadline(std::string headline)
{
	this->headline = headline;
	SetWindowText(this->hWnd, headline.c_str());
}

void Window::addControl(std::unique_ptr<Control> control)
{
	this->controls.push_back(std::move(control));
}

void Window::addDialog(std::unique_ptr<Control> dialogs)
{
	this->dialogs.push_back(std::move(dialogs));
}

bool Window::hasNonModalDialogWithCaption(const std::string& caption, const std::string& message) const
{
	for (const auto& dptr : dialogs) 
	{
		if (!dptr) continue;
		// 只检查 Dialog 类型的控件
		Dialog* d = dynamic_cast<Dialog*>(dptr.get());
		//检查是否有非模态对话框可见，并且消息内容一致
		if (d && d->IsVisible() && !d->model() && d->GetCaption() == caption && d->GetText() == message)
			return true;   
	}
	return false;
}


HWND Window::getHwnd() const
{
	return hWnd;
}

int Window::getWidth() const
{
	return this->pendingW;
}

int Window::getHeight() const
{
	return this->pendingH;
}

std::string Window::getHeadline() const
{
	return this->headline;
}

COLORREF Window::getBkcolor() const
{
	return this->wBkcolor;
}

IMAGE* Window::getBkImage() const
{
	return this->background;
}

std::vector<std::unique_ptr<Control>>& Window::getControls()
{
	return this->controls;
}





