
/**
 * Window（头文件）
 *
 * 设计目标：
 *  - 提供一个基于 Win32 + EasyX 的“可拉伸且稳定不抖”的窗口容器。
 *  - 通过消息过程子类化（WndProcThunk）接管关键消息（WM_SIZING/WM_SIZE/...）。
 *  - 将“几何变化记录（pendingW/H）”与“统一收口重绘（needResizeDirty）”解耦。
 *
 * 关键点（与 .cpp 中实现对应）：
 *  - isSizing：处于交互拉伸阶段时，冻结重绘；松手后统一重绘，防止抖动。
 *  - WM_SIZING：只做“最小尺寸夹紧”，不回滚矩形、不做对齐；把其余交给系统。
 *  - WM_GETMINMAXINFO：按最小“客户区”换算到“窗口矩形”，提供系统层最小轨迹值。
 *  - runEventLoop：只记录 WM_SIZE 的新尺寸；真正绘制放在 needResizeDirty 时集中处理。
 */
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
    IMAGE* background = nullptr;  // 背景图对象指针（存在时优先绘制）
    std::string   bkImageFile;           // 背景图文件路径（loadimage 用）

    // —— 控件/对话框 ——（容器内的普通控件与非模态对话框）
    std::vector<std::unique_ptr<Control>> controls;
    std::vector<std::unique_ptr<Control>> dialogs;

public:
    bool dialogClose = false;            // 项目内使用的状态位

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
    std::string getHeadline() const;
    COLORREF    getBkcolor() const;
    IMAGE* getBkImage() const;
    std::string getBkImageFile() const;
    std::vector<std::unique_ptr<Control>>& getControls();

    // —— 配置开关 ——（动态调整最小客户区、合成双缓冲）
    inline void setMinClientSize(int w, int h)
    {
        // 仅更新阈值；实际约束在 WM_GETMINMAXINFO/WM_SIZING 中生效
        minClientW = w;
        minClientH = h;
    }

    inline void setComposited(bool on)
    {
        // 更新标志；真正应用在 draw()/样式 SetWindowLongEx + SWP_FRAMECHANGED
        useComposited = on;
    }

    void processWindowMessage(const ExMessage & msg); // 处理 EX_WINDOW 中的 WM_SIZE 等
    void pumpResizeIfNeeded();                       // 执行一次统一收口重绘
    void scheduleResizeFromModal(int w, int h);
private:
    void adaptiveLayout(std::unique_ptr<Control>& c,const int finalH, const int finalW);

};
