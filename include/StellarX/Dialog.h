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
	// 在事件安全点执行延迟清理；用于非模态关闭后统一回收
	void performDelayedCleanup();
	// 非模态工厂调用时用于回传结果
	std::function<void(StellarX::MessageBoxResult)> resultCallback;
	// 设置非模态结果回调
	void SetResultCallback(std::function<void(StellarX::MessageBoxResult)> cb);
	// 获取标题文本，用于非模态去重
	std::string GetCaption() const;
	// 获取正文文本，用于非模态去重
	std::string GetText() const;

public:
	// 构造对话框；modal=false 时按非模态浮层工作
	Dialog(Window& hWnd, std::string text, std::string message = "对话框", StellarX::MessageBoxType type = StellarX::MessageBoxType::OK, bool modal = true);
	~Dialog();
	//绘制对话框
	void draw() override;
	//事件处理
	bool  handleEvent(const ExMessage& msg) override;
	// 设置标题
	void SetTitle(const std::string& title);
	// 设置消息内容
	void SetMessage(const std::string& message);
	// 设置对话框类型
	void SetType(StellarX::MessageBoxType type);
	// 设置模态属性
	void SetModal(bool modal);
	// 设置对话框结果
	void SetResult(StellarX::MessageBoxResult result);
	// 获取对话框结果
	StellarX::MessageBoxResult GetResult() const;

	// 返回当前对话框是否为模态
	bool model() const override;

	// 显示对话框
	void Show();
	// 关闭对话框
	void Close();
	// 宿主窗口变化时仅重新居中，不拉伸 Dialog 自身
	void recenterInHostWindow();

private:
	// 初始化按钮
	void initButtons();
	// 初始化关闭按钮
	void initCloseButton();
	// 按行分割消息内容
	void splitMessageLines();
	// 获取文本大小
	void getTextSize();
	// 标记需要重新布局并按需清空旧控件
	void invalidateLayout(bool clearChildren);
	//初始化对话框尺寸
	void initDialogSize();
	// 依据当前 Dialog 的 x/y/width/height 重新创建标题和按钮
	void rebuildChrome();
	// 禁止走普通 Canvas 添加路径；Dialog 内部控件由自身 chrome 重建逻辑统一维护
	void addControl(std::unique_ptr<Control> control);
	bool canCommitManagedPartialRepaint() const override; // 判断当前 Dialog 是否可安全做局部提交
	void commitManagedRepaint() override;                 // 托管收口阶段执行 Dialog 的真正重绘

	// 清除所有控件
	void clearControls();
	// 创建标准对话框功能按钮
	std::unique_ptr<Button> createDialogButton(int x, int y, const std::string& text);
	void requestRepaint(Control* parent) override;        // 托管模式下登记为 Dialog root；非托管模式下立即更新内部按钮
};
