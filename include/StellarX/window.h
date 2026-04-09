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
 //fuck windows 
 //fuck win32
 //fuck xiaomi
#pragma once

#include "Control.h"
#include <string>
#include <vector>
#include <memory>
#include <windows.h>

class Window
{
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
	bool          managedDispatchActive = false; // 事件分发期：控件只改状态并登记重绘，不立即画
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
	Window(int width, int height, int mode);
	Window(int width, int height, int mode, COLORREF bkcloc);
	Window(int width, int height, int mode, COLORREF bkcloc, std::string headline);
	~Window();

	// —— 绘制与事件循环 ——（draw* 完成一次全量绘制；runEventLoop 驱动事件与统一收口）
	void draw();                         // 纯色背景版本
	void draw(std::string pImgFile);     // 背景图版本
	int  runEventLoop();                 // 主事件循环（PeekMessage + 统一收口重绘）

	// —— 背景/标题设置 ——（更换背景、背景色与标题；立即触发一次批量绘制）
	void setBkImage(std::string pImgFile);
	void setBkcolor(COLORREF c);
	void setHeadline(std::string headline);

	// —— 控件/对话框管理 ——（添加到容器，或做存在性判断）
	void addControl(std::unique_ptr<Control> control);
	void addDialog(std::unique_ptr<Control> dialogs);
	bool hasNonModalDialogWithCaption(const std::string& caption, const std::string& message) const;

	// —— 访问器 ——（只读接口，供外部查询当前窗口/标题/背景等）
	HWND        getHwnd() const;
	int         getWidth() const;
	int         getHeight() const;
	int         getPendingWidth() const;
	int         getPendingHeight() const;
	std::string getHeadline() const;
	COLORREF    getBkcolor() const;
	IMAGE* getBkImage() const;
	std::string getBkImageFile() const;
	std::vector<std::unique_ptr<Control>>& getControls();

	// —— 尺寸调整 / 托管重绘 ——（事件阶段登记，收口阶段提交）
	
	// 执行一次 resize 收口 + 统一重绘
	void pumpResizeIfNeeded();    
	// 供模态 Dialog 上报宿主窗口尺寸变化
	void scheduleResizeFromModal(int w, int h);   
	// 当前是否处于“事件只改状态，不立即画”的阶段
	bool isManagedDispatchActive() const;    
	// 记录一笔由 source 发起的托管重绘请求
	void requestManagedRepaint(Control* source);  
	// 在事件收口阶段提交本轮登记的 root 重绘
	void flushManagedRepaint();                   
private:
	void adaptiveLayout(std::unique_ptr<Control>& c, const int finalH, const int finalW);
	// resize / 初次绘制 / 对话框开关这类全局场景的整场景重绘
	void redrawScene(bool forceControlsDirty, bool forceDialogsDirty);
	void drawWindowBackground();
	// 合成 WM_MOUSEMOVE，用于同步底层 hover 状态
	void dispatchSyntheticMouseMoveToControls(short x, short y); 
	// 清空本轮托管重绘登记
	void clearManagedRepaintState();                                  
	// 找出需要补画到最上层的对话框
	void collectManagedDialogOverlays(Control* repaintRoot, const RECT& coverage, std::vector<Control*>& overlays); 
};
