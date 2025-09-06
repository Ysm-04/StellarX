#include "window.h"

window::window(int width, int height, int mode)
{
	this->width = width;
	this->height = height;
	this->windowMode = mode;
}

window::window(int width, int height, int mode, COLORREF bkcloc)
{
	this->width = width;
	this->height = height;
	this->windowMode = mode;
	this->wBkcolor = bkcloc;
}

window::window(int width, int height, int mode, COLORREF bkcloc, std::string headline)
{
	this->width = width;
	this->height = height;
	this->windowMode = mode;
	this->wBkcolor = bkcloc;
    this->headline = headline;
}

window::~window()
{
	if (background)
		delete background;
	background = nullptr;
	closegraph(); // 确保关闭图形上下文
}

void window::draw()
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
void window::draw(std::string pImgFile)
{
	this->background = new IMAGE;
	hWnd = initgraph(width, height, windowMode);
	SetWindowText(hWnd, headline.c_str());
	loadimage(background, pImgFile.c_str(), width, height, true);
	putimage(0,0, background);

	// 绘制所有子控件
	BeginBatchDraw(); // 开始批量绘制
	for (auto& control : controls)
		control->draw();
	EndBatchDraw(); // 结束批量绘制

}
// 运行主事件循环，处理用户输入和窗口消息
// 此方法会阻塞直到窗口关闭
void window::runEventLoop()
{
	ExMessage msg;
	bool running = true;
	while (running) {
		msg = getmessage(EX_MOUSE | EX_KEY);
		if (msg.message == WM_CLOSE) {
			running = false;
			continue;
		}
		for (auto& c : controls)
			c->handleEvent(msg);
		flushmessage(EX_MOUSE	|EX_KEY	|EX_CHAR|EX_WINDOW);
		Sleep(10);
	}
}

void window::setBkImage(std::string pImgFile)
{
	if(nullptr == background)
		this->background = new IMAGE;
	else
		delete background;
	this->background = new IMAGE;
	loadimage(background, pImgFile.c_str(), width, height, true);
	putimage(0, 0, background);
}

void window::setBkcolor(COLORREF c)
{
	wBkcolor = c;
}

void window::setHeadline(std::string headline)
{
	this->headline = headline;
	SetWindowText(this->hWnd, headline.c_str());
}

void window::addControl(std::unique_ptr<Control> control)
{
	this->controls.push_back(std::move(control));
}





