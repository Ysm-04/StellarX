/*******************************************************************************
 * @类: Dialog
 * @摘要: 模态和非模态对话框控件，提供丰富的消息框功能
 * @描述:
 *     实现完整的对话框功能，支持多种按钮组合和异步结果回调。
 *     自动处理布局、背景保存恢复和生命周期管理。
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

#define closeButtonWidth 20   //关闭按钮宽度
#define closeButtonHeight 20  //关闭按钮高度  同时作为对话框标题栏高度
#define functionButtonWidth  70 //按钮宽度
#define functionButtonHeight 30  //按钮高度
#define buttonMargin         10  //按钮间距

#define buttonAreaHeight     50 //按钮区域高度
#define titleToTextMargin    10 //标题到文本的距离
#define textToBorderMargin   10 //文本到边框的距离

class Dialog : public Canvas 
{
	Window& hWnd;               //窗口引用

	int textWidth = 0;			//文本宽度
	int textHeight = 0;		    //文本高度
	int buttonNum = 0;         // 按钮数量
	int BorderWidth = 2;  //边框宽度

	IMAGE* saveBkImage = nullptr; // 用于保存背景图像
	int saveBkX = 0, saveBkY = 0; // 保存背景的位置
	int saveBkWidth = 0, saveBkHeight = 0; // 保存背景的尺寸

	StellarX::MessageBoxType type = StellarX::MessageBoxType::OK;   //对话框类型
	std::string titleText = "提示";				     //标题文本
	std::unique_ptr<Label> title = nullptr;  //标题标签

	std::string message;                  //提示信息
	std::vector<std::string> lines;       //消息内容按行分割

	
	bool needsInitialization = true;      //是否需要初始化 
	bool close = false;					  //是否关闭
	bool modal = true;                    //是否模态
	bool isVisible = false;               //是否可见

	COLORREF backgroundColor = RGB(240, 240, 240);   //背景颜色            
	COLORREF borderColor = RGB(100, 100, 100);       //边框颜色

	COLORREF buttonTrueColor = RGB(211, 190, 190);	  //按钮被点击颜色
	COLORREF buttonFalseColor = RGB(215, 215, 215);   //按钮未被点击颜色
	COLORREF buttonHoverColor = RGB(224, 224, 224);	  //按钮悬浮颜色


	Button* closeButton = nullptr;  //关闭按钮

	StellarX::MessageBoxResult result = StellarX::MessageBoxResult::Cancel; // 对话框结果

public:
	bool shouldClose = false;              //是否应该关闭
	bool isCleaning = false;               //是否正在清理
	bool pendingCleanup = false;           //延迟清理
	StellarX::ControlText textStyle;   // 字体样式
	// 清理方法声明
	void performDelayedCleanup();
	//工厂模式下调用非模态对话框时用来返回结果
	std::function<void(StellarX::MessageBoxResult)> resultCallback;
	//设置非模态获取结果的回调函数
	void SetResultCallback(std::function<void(StellarX::MessageBoxResult)> cb);
	//获取对话框消息，用以去重
	std::string GetCaption() const;
	

public:
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
	// 获取模态属性
	bool getModal() const;
	// 检查是否可见
	bool IsVisible() const override;
	//获取对话框类型
	bool model() const override;

	// 显示对话框
	void Show();
	// 关闭对话框
	void Close(); 

private:
	// 初始化按钮
	void initButtons();       
	// 初始化关闭按钮
	void initCloseButton();   
	// 初始化标题
	void initTitle();		  
	// 按行分割消息内容
	void splitMessageLines(); 
	// 获取文本大小
	void getTextSize();	      
	//初始化对话框尺寸
	void initDialogSize(); 
	// 初始化对话框
	void initializeDialog();

	
	// 清除所有控件
	void clearControls(); 
	//创建对话框按钮
	std::unique_ptr<Button> createDialogButton(int x, int y, const std::string& text);
};