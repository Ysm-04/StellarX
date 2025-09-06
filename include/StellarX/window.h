#pragma once
#include "Control.h"
//窗口模式
//#define EX_DBLCLKS	   1   - 在绘图窗口中支持鼠标双击事件。
//#define EX_NOCLOSE	   2   - 禁用绘图窗口的关闭按钮。
//#define EX_NOMINIMIZE    3   - 禁用绘图窗口的最小化按钮。
//#define EX_SHOWCONSOLE   4   - 显示控制台窗口。
/*******************************************************************************
 * @类: window
 * @摘要: 应用程序主窗口类
 * @描述:
 *     负责创建和管理应用程序的主窗口，是所有控件的根容器。
 *     处理消息循环、事件分发和窗口生命周期管理。
 *
 * @重要说明:
 *     - 使用 initgraph() 创建窗口
 *     - 使用 BeginBatchDraw()/EndBatchDraw() 实现双缓冲
 *     - 使用 getmessage() 处理消息循环
 ******************************************************************************/
class window
{
	            int           width;                 //窗口宽度
	            int           height;                //窗口高度
				int           windowMode = NULL;     //窗口模式
	           HWND           hWnd = NULL;           //窗口句柄
	    std::string           headline;              //窗口标题
	       COLORREF           wBkcolor = BLACK;      //窗口背景
	          IMAGE*                  background = nullptr;  //窗口背景图片
std::vector<std::unique_ptr<Control>> controls;               //控件管理

public:
	window(int width, int height, int mode);
	window(int width, int height, int mode, COLORREF bkcloc);
	window(int width, int height, int mode , COLORREF bkcloc, std::string headline = "窗口");
	~window();
	//绘制窗口
	void draw();
	void draw(std::string pImgFile);
	//事件循环
	void runEventLoop();
	//设置窗口背景图片
	
	void setBkImage(std::string pImgFile);
	//设置窗口背景颜色
	void setBkcolor(COLORREF c);
	//设置窗口标题
	void setHeadline(std::string headline);
	//添加控件
	void addControl(std::unique_ptr<Control> control);

};


