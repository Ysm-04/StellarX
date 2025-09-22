#include "Window.h"
#include"Dialog.h"

Window::Window(int width, int height, int mode)
{
	this->width = width;
	this->height = height;
	this->windowMode = mode;
}

Window::Window(int width, int height, int mode, COLORREF bkcloc)
{
	this->width = width;
	this->height = height;
	this->windowMode = mode;
	this->wBkcolor = bkcloc;
}

Window::Window(int width, int height, int mode, COLORREF bkcloc, std::string headline)
{
	this->width = width;
	this->height = height;
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

void Window::draw()
{
	hWnd = initgraph(width, height, windowMode);
	SetWindowText(hWnd,headline.c_str());
	setbkcolor(wBkcolor);
	cleardevice();
	BeginBatchDraw(); // 开始批量绘制
	// 绘制所有子控件
	for (auto& control : controls)
		control->draw();
	EndBatchDraw(); // 结束批量绘制
}
// 使用背景图片绘制窗口
// @参数 pImgFile: 图片文件路径，支持常见图片格式
// @备注: 会拉伸图片以适应窗口尺寸
void Window::draw(std::string pImgFile)
{
	this->background = new IMAGE(width, height);
	hWnd = initgraph(width, height, windowMode);
	SetWindowText(hWnd, headline.c_str());
	loadimage(background, pImgFile.c_str(), width, height, true);
	putimage(0,0, background);

	// 绘制所有子控件
	BeginBatchDraw(); // 开始批量绘制
	for (auto& control : controls)
		control->draw();
	for(auto& d: dialogs)
		d->draw();
	EndBatchDraw(); // 结束批量绘制

}
// 运行主事件循环，处理用户输入和窗口消息
// 此方法会阻塞直到窗口关闭
// 主消息循环优先级：对话框 > 普通控件。
// 重绘策略：为保证视觉一致性，每次有对话框状态变化（打开/关闭）时，
// 会强制重绘所有控件。先绘制普通控件，再绘制对话框（确保对话框在最上层）。
void Window::runEventLoop()
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
				break;
			}
		
			// 优先处理对话框事件
			for (auto& d : dialogs)
			{
				if (d->IsVisible() && !d->model())
					consume = d->handleEvent(msg);
				if (consume)
					break;
			}

			if(!consume)
				for (auto& c : controls)
				{
					consume = c->handleEvent(msg);
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
				mm.x =(short) pt.x;
				mm.y =(short) pt.y;
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

		// 避免CPU占用过高
		Sleep(10);
	}
}

void Window::setBkImage(std::string pImgFile)
{
	if(nullptr == background)
		this->background = new IMAGE;
	else
		delete background;
	this->background = new IMAGE;
	loadimage(background, pImgFile.c_str(), width, height, true);
	putimage(0, 0, background);
}

void Window::setBkcolor(COLORREF c)
{
	wBkcolor = c;
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

bool Window::hasNonModalDialogWithCaption(const std::string& caption) const {
	for (const auto& dptr : dialogs) {
		if (!dptr) continue;
		// 只检查 Dialog 类型的控件
		Dialog* d = dynamic_cast<Dialog*>(dptr.get());
		//检查是否有非模态对话框可见，并且消息内容一致
		if (d && d->IsVisible() && !d->model() && d->GetCaption() == caption)
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
	return this->width;
}

int Window::getHeight() const
{
	return this->height;
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





