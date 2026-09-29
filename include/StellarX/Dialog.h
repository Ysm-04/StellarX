/*******************************************************************************
 * @类: Dialog
 * @摘要: 模态和非模态对话框控件，提供丰富的消息框功能
 * @描述:
 *     实现完整的对话框功能，支持多种按钮组合和异步结果回调。
 *     自动处理布局、背景保存恢复和生命周期管理。
 *     在窗口托管重绘模式下，Dialog 自身也是一个独立的重绘 root。
 *
 * @特性:
 *     - 支持六种标准消息框类型（OK、YesNo、YesNoCancel等）
 *     - 模态和非模态两种工作模式
 *     - 自动文本换行和尺寸计算
 *     - 背景保存和恢复机制
 *     - 工厂模式下的去重检测
 *
 * @使用场景: 用户提示、确认操作、输入请求等交互场景
 * @所属框架: 星垣(StellarX) GUI框架
 * @作者: 我在人间做废物
 ******************************************************************************/

#pragma once
#include"StellarX.h"

#define closeButtonWidth 30   //关闭按钮宽度
#define closeButtonHeight 20  //关闭按钮高度  同时作为对话框标题栏高度
#define functionButtonWidth  70 //按钮宽度
#define functionButtonHeight 30  //按钮高度
#define buttonMargin         10  //按钮间距

#define buttonAreaHeight     50 //按钮区域高度
#define titleToTextMargin    10 //标题到文本的距离
#define textToBorderMargin   10 //文本到边框的距离
#define BorderWidth          3 //边框宽度
class Dialog : public Canvas
{
	friend class Window;

	Window& hWnd;               //窗口引用

	int textWidth = 0;			//文本宽度
	int textHeight = 0;		    //文本高度
	int buttonNum = 0;         // 按钮数量

	StellarX::MessageBoxType type = StellarX::MessageBoxType::OK;   //对话框类型
	std::string titleText = "提示";				     //标题文本

	std::string message;                  //提示信息
	std::vector<std::string> lines;       //消息内容按行分割

	bool needsInitialization = true;      // 是否需要根据当前内容重新初始化布局和内部按钮
	bool modal = true;                    //是否模态

	COLORREF backgroundColor = RGB(240, 240, 240);   //背景颜色
	COLORREF borderColor = RGB(100, 100, 100);       //边框颜色

	COLORREF buttonTrueColor = RGB(211, 190, 190);	  //按钮被点击颜色
	COLORREF buttonFalseColor = RGB(215, 215, 215);   //按钮未被点击颜色
	COLORREF buttonHoverColor = RGB(224, 224, 224);	  //按钮悬浮颜色

	Button* closeButton = nullptr;  //关闭按钮

	StellarX::MessageBoxResult result = StellarX::MessageBoxResult::Cancel; // 对话框结果

	bool isCleaning = false;               //是否正在清理
	bool pendingCleanup = false;           //延迟清理
public:
	StellarX::ControlText textStyle;   // 字体样式
	// 在事件安全点执行延迟清理；用于非模态关闭后统一回收。
	// Close() 只隐藏并标记待清理，真正释放内部按钮和快照要等到这里执行。
	void performDelayedCleanup();
	// 非模态工厂调用时用于回传结果；回调可能修改宿主控件树，因此 Close() 会在 mutation scope 内触发它。
	std::function<void(StellarX::MessageBoxResult)> resultCallback;
	// 设置非模态结果回调；仅在非模态 Dialog 关闭时异步调用。
	void SetResultCallback(std::function<void(StellarX::MessageBoxResult)> cb);
	// 获取标题文本，用于非模态去重
	std::string GetCaption() const;
	// 获取正文文本，用于非模态去重
	std::string GetText() const;

public:
	// 构造对话框；modal=false 时按非模态浮层工作，由 Window 接管生命周期和层级。
	Dialog(Window& hWnd, std::string text, std::string message = "对话框", StellarX::MessageBoxType type = StellarX::MessageBoxType::OK, bool modal = true);
	// 析构对话框并释放内部控件资源。
	~Dialog();
	// 绘制对话框背景、标题、正文和功能按钮。
	void draw() override;
	// 处理关闭按钮和功能按钮事件。
	bool  handleEvent(const ExMessage& msg) override;
	// 设置标题并标记布局需要重建。
	void SetTitle(const std::string& title);
	// 设置消息内容并标记文本换行和布局需要重建。
	void SetMessage(const std::string& message);
	// 设置对话框类型并重建功能按钮组合。
	void SetType(StellarX::MessageBoxType type);
	// 设置模态属性，决定 Show() 使用阻塞循环还是交给 Window 主循环。
	void SetModal(bool modal);
	// 设置对话框结果，通常由功能按钮回调写入。
	void SetResult(StellarX::MessageBoxResult result);
	// 获取对话框结果
	StellarX::MessageBoxResult GetResult() const;

	// 返回当前对话框是否为模态；Window 用它区分阻塞式对话框和托管浮层。
	bool model() const override;

	// 显示对话框。模态对话框会进入自己的消息循环；非模态对话框只标记可见，由 Window 主循环驱动。
	void Show();
	// 关闭对话框。
	// 该调用只设置隐藏、结果和待清理标志，并请求整场景重绘；对象释放延迟到 performDelayedCleanup()。
	void Close();
	// 宿主窗口变化时仅重新居中，不拉伸 Dialog 自身
	void recenterInHostWindow();

private:
	// 根据当前 MessageBoxType 初始化功能按钮。
	void initButtons();
	// 初始化标题栏关闭按钮。
	void initCloseButton();
	// 按当前消息内容和宽度约束分割显示行。
	void splitMessageLines();
	// 计算当前标题和正文所需的文本尺寸。
	void getTextSize();
	// 标记需要重新布局并按需清空旧控件
	void invalidateLayout(bool clearChildren);
	// 根据文本和按钮数量初始化对话框尺寸。
	void initDialogSize();
	// 依据当前 Dialog 的 x/y/width/height 重新创建标题和按钮
	void rebuildChrome();
	// 禁止走普通 Canvas 添加路径；Dialog 内部控件由自身 chrome 重建逻辑统一维护
	void addControl(std::unique_ptr<Control> control);
	// 判断当前 Dialog 是否可安全做局部提交。
	bool canCommitManagedPartialRepaint() const override;
	// 托管收口阶段执行 Dialog 的真正重绘。
	void commitManagedRepaint() override;

	// 清除 Dialog 内部所有 chrome 控件。
	void clearControls();
	// 创建标准对话框功能按钮。
	std::unique_ptr<Button> createDialogButton(int x, int y, const std::string& text);
	// 请求宿主窗口重绘 Dialog root；非托管模式下立即更新内部按钮。
	void requestRepaint(Control* parent) override;
};
