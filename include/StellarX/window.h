
#pragma once
#include"Control.h"
/*******************************************************************************
 * @类: Window
 * @摘要: 应用程序主窗口类，管理窗口生命周期和消息循环
 * @描述:
 *     创建和管理应用程序主窗口，作为所有控件的根容器。
 *     处理消息分发、事件循环和渲染调度。
 *
 * @特性:
 *     - 多种窗口模式配置（双缓冲、控制台等）
 *     - 背景图片和颜色支持
 *     - 集成的对话框管理系统
 *     - 完整的消息处理循环
 *     - 控件和对话框的生命周期管理
 *
 * @使用场景: 应用程序主窗口，GUI程序的入口和核心
 * @所属框架: 星垣(StellarX) GUI框架
 * @作者: 我在人间做废物
 ******************************************************************************/
class Window
{
	            int           width;              //窗口宽度
				int           height;             //窗口高度
				int           windowMode = NULL;     //窗口模式
	           HWND           hWnd = NULL;           //窗口句柄
	    std::string           headline;              //窗口标题
	       COLORREF           wBkcolor = BLACK;      //窗口背景
	          IMAGE*                  background = nullptr;  //窗口背景图片
std::vector<std::unique_ptr<Control>> controls;               //控件管理
std::vector<std::unique_ptr<Control>> dialogs;                //对话框管理


public:
	bool dialogClose = false; //是否有对话框关闭

	Window(int width, int height, int mode);
	Window(int width, int height, int mode, COLORREF bkcloc);
	Window(int width, int height, int mode , COLORREF bkcloc, std::string headline = "窗口");
	~Window();
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
	//添加对话框
	void addDialog(std::unique_ptr<Control>  dialogs);
	//检查是否已有对话框显示用于去重，防止工厂模式调用非模态对话框，多次打开污染对话框背景快照
	bool hasNonModalDialogWithCaption(const std::string& caption) const;

	//获取窗口句柄
	HWND getHwnd() const;
	//获取窗口宽度
	int getWidth() const;
	//获取窗口高度
	int getHeight() const;
	//获取窗口标题
	std::string getHeadline() const;
	//获取窗口背景颜色
	COLORREF getBkcolor() const;
	//获取窗口背景图片
	IMAGE* getBkImage() const;
	//获取控件管理
	std::vector<std::unique_ptr<Control>>& getControls();

};


