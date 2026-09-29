/*******************************************************************************
 * @类: Button
 * @摘要: 多功能按钮控件，支持多种状态和样式
 * @描述:
 *     提供完整的按钮功能，包括普通点击、切换模式、禁用状态。
 *     支持多种形状（矩形、圆形、椭圆等）和丰富的视觉样式。
 *     通过回调函数机制实现灵活的交互逻辑。
 *
 * @特性:
 *     - 支持三种工作模式：普通、切换、禁用
 *     - 八种几何形状，各有边框和无边框版本
 *     - 自定义颜色（默认、悬停、点击状态）
 *     - 多种填充模式（纯色、图案、图像）
 *     - 完整的鼠标事件处理（点击、悬停、移出）
 *
 * @使用场景: 作为主要交互控件，用于触发动作或表示状态
 * @所属框架: 星垣(StellarX) GUI框架
 * @作者: 我在人间做废物
 ******************************************************************************/
#pragma once
#include "Control.h"
#include"label.h"

#define DISABLEDCOLOUR RGB(96, 96, 96) //禁用状态颜色
#define TEXTMARGINS_X 6
#define TEXTMARGINS_Y 4
constexpr int bordWith = 1;   //边框宽度，用于快照恢复时的偏移计算
constexpr int bordHeight = 1; //边框高度，用于快照恢复时的偏移计算

class Button : public Control
{
	std::string text;             // 按钮上的文字
	bool        click;            // 是否被点击
	bool        hover;            // 是否被悬停

	std::string cutText;       // 切割后的文本
	bool needCutText = true;   // 是否需要切割文本
	bool isUseCutText = false; // 是否使用切割文本
	int  padX = TEXTMARGINS_X; // 文本最小左右内边距
	int  padY = TEXTMARGINS_Y; // 文本最小上下内边距

	COLORREF         buttonTrueColor;                 // 按钮被点击后的颜色
	COLORREF         buttonFalseColor;                // 按钮未被点击的颜色
	COLORREF         buttonHoverColor;                // 按钮被鼠标悬停的颜色
	COLORREF         buttonBorderColor = RGB(0, 0, 0);// 按钮边框颜色

	StellarX::ButtonMode  mode;	      // 按钮模式
	StellarX::ControlShape shape;     // 按钮形状

	StellarX::FillMode    buttonFillMode = StellarX::FillMode::Solid;     //按钮填充模式
	StellarX::FillStyle   buttonFillIma = StellarX::FillStyle::BDiagonal; //按钮填充图案
	std::unique_ptr<IMAGE> buttonFileIMAGE;      //按钮填充图像

	std::function<void()> onClickCallback;      //回调函数
	std::function<void()> onToggleOnCallback;   //TOGGLE模式下的回调函数
	std::function<void()> onToggleOffCallback;  //TOGGLE模式下的回调函数

	public:
	// 必须先构造公开字体样式，再用它初始化旧样式快照。
	// C++ 按成员声明顺序构造；若顺序反过来，oldStyle 会读取尚未构造的 std::string。
	StellarX::ControlText textStyle;  // 按钮文字样式

	private:
	StellarX::ControlText oldStyle = textStyle;   // 按钮文字样式
	int oldtext_width = -1;
	int oldtext_height = -1;
	int text_width = 0;
	int text_height = 0;

	// === Tooltip ===
	bool        tipEnabled = false;           // 是否启用
	bool        tipVisible = false;           // 当前是否显示
	bool        tipFollowCursor = false;      // 是否跟随鼠标
	bool        tipUserOverride = false;      // 是否用户自定义了tip文本
	int         tipDelayMs = 1000;            // 延时(毫秒)
	int         tipOffsetX = 12;              // 相对鼠标偏移
	int         tipOffsetY = 18;
	ULONGLONG   tipHoverTick = 0;             // 开始悬停的时间戳
	int         lastMouseX = 0;               // 最新鼠标位置(用于定位)
	int         lastMouseY = 0;

	std::string tipTextClick; // NORMAL 模式下用
	std::string tipTextOn;    // click==true 时用
	std::string tipTextOff;   // click==false 时用
	Label       tipLabel;     // 直接复用Label作为提示

	public:

	// 创建使用默认颜色的按钮。
	Button(int x, int y, int width, int height, const std::string text,
		StellarX::ButtonMode mode = StellarX::ButtonMode::NORMAL, StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE);
	// 创建自定义默认态和点击态颜色的按钮。
	Button(int x, int y, int width, int height, const std::string text,
		COLORREF ct, COLORREF cf, StellarX::ButtonMode mode = StellarX::ButtonMode::NORMAL,
		StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE);
	// 创建自定义默认态、点击态和悬停态颜色的按钮。
	Button(int x, int y, int width, int height, const std::string text,
		COLORREF ct, COLORREF cf, COLORREF ch,
		StellarX::ButtonMode mode = StellarX::ButtonMode::NORMAL, StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE);
	// 析构按钮并释放图像填充资源。
	~Button();

	// 绘制按钮本体、文本和必要时的 Tooltip。
	void draw() override;
	// 处理按钮点击、悬停、切换状态和 Tooltip 触发。
	bool handleEvent(const ExMessage& msg) override;
	// 由窗口时钟推进 Tooltip 到期状态，即使没有新的鼠标消息也能触发显示。
	void tick(ULONGLONG nowMs) override;
	// 清理按钮的鼠标瞬时状态；用于父容器在 WM_MOUSEMOVE 短路后补做 hover/tooltip 收口。
	bool clearTransientMouseState() override;
	// Tooltip 可见时，按钮实际写像素范围不再等于按钮本体，需要把 Tooltip 矩形并入 coverage。
	// coverage 必须覆盖按钮本体和可见 Tooltip，才能正确恢复旧区域。
	RECT getManagedRepaintCoverageRect() const override;
	// Button 的持久绘制范围只包含本体；Tooltip 是临时浮层，不能污染兄弟控件背景快照。
	RECT getManagedRepaintPersistentCoverageRect() const override;

	// 设置 NORMAL 模式下的点击回调
	void setOnClickListener(std::function<void()> callback);
	// 设置 TOGGLE 模式下切换到打开状态时的回调
	void setOnToggleOnListener(std::function<void()> callback);
	// 设置 TOGGLE 模式下切换到关闭状态时的回调
	void setOnToggleOffListener(std::function<void()> callback);
	// 设置按钮模式（NORMAL / TOGGLE / DISABLED）
	void setbuttonMode(StellarX::ButtonMode mode);
	// 设置圆角矩形的圆角宽度
	void setROUND_RECTANGLEwidth(int width);
	// 设置圆角矩形的圆角高度
	void setROUND_RECTANGLEheight(int height);
	// 设置按钮填充模式
	void setFillMode(StellarX::FillMode mode);
	// 设置按钮图案填充样式
	void setFillIma(StellarX::FillStyle ima);
	// 设置按钮图像填充资源
	void setFillIma(std::string imaName);
	// 设置按钮边框颜色
	void setButtonBorder(COLORREF Border);
	// 设置按钮默认态颜色
	void setButtonFalseColor(COLORREF color);
	// 设置按钮文本（char* 重载）。
	// 会重新计算文本像素尺寸并让自动 Tooltip 文案同步为新文本；传入指针内容会被拷贝。
	void setButtonText(const char* text);
	// 设置按钮文本（std::string 重载）。
	// 若用户未显式覆盖 Tooltip 文案，默认 Tooltip 也会跟随按钮文本更新。
	void setButtonText(std::string text);
	// 设置按钮几何形状。
	void setButtonShape(StellarX::ControlShape shape);
	// 直接设置按钮点击状态。
	// NORMAL 设置为 true 时触发 onClick；TOGGLE 仅在状态实际变化时触发 on/off 回调，重复设置同值不会重跑回调。
	// 回调在 mutation scope 内执行，可以安全排队修改宿主控件树。
	void setButtonClick(BOOL click);

	// 查询按钮当前是否处于点击/选中状态。
	bool isClicked() const;

	// 获取按钮文本。
	std::string getButtonText() const;
	// 获取按钮文本的 C 字符串视图；指针仅在下一次修改按钮文本前有效，不应长期保存。
	const char* getButtonText_c() const;
	// 获取按钮模式。
	StellarX::ButtonMode getButtonMode() const;
	// 获取按钮形状。
	StellarX::ControlShape getButtonShape() const;
	// 获取按钮填充模式。
	StellarX::FillMode getFillMode() const;
	// 获取按钮图案填充样式。
	StellarX::FillStyle getFillIma() const;
	// 获取按钮图像填充资源；未设置图像时返回 nullptr。
	IMAGE* getFillImaImage() const;
	// 获取按钮边框颜色。
	COLORREF getButtonBorder() const;
	// 获取按钮文字颜色。
	COLORREF getButtonTextColor() const;
	// 获取按钮文字样式。
	StellarX::ControlText getButtonTextStyle() const;
public:
	// === Tooltip API===
	// 开关 Tooltip；关闭时只修改内部启用/显示状态，不负责智能选位。
	// Tooltip 是按钮内部临时浮层，会并入托管重绘 coverage，但不会成为持久背景快照的一部分。
	void enableTooltip(bool on);
	// 查询 Tooltip 当前是否已经显示。
	bool isTooltipVisible() const { return tipVisible; }
	// 设置 Tooltip 延时（毫秒）；负数会被夹紧为 0。
	void setTooltipDelay(int ms) { tipDelayMs = (ms < 0 ? 0 : ms); }
	// 设置 Tooltip 是否跟随鼠标
	void setTooltipFollowCursor(bool on) { tipFollowCursor = on; }
	// 设置 Tooltip 偏移量；当前仅在 setTooltipFollowCursor(true) 时参与定位。
	void setTooltipOffset(int dx, int dy) { tipOffsetX = dx; tipOffsetY = dy; }
	// 设置 Tooltip 的文字、背景和透明样式。
	void setTooltipStyle(COLORREF text, COLORREF bk, bool transparent);
	// 设置 NORMAL 模式下 Tooltip 文本，并阻止自动跟随按钮文本。
	void setTooltipText(const std::string& s) { tipTextClick = s; tipUserOverride = true; }
	// 设置 TOGGLE 模式开/关两种状态下的 Tooltip 文本。
	void setTooltipTextsForToggle(const std::string& onText, const std::string& offText);
private:
	// 初始化按钮内部状态、颜色和 Tooltip 默认样式
	void initButton(const std::string text, StellarX::ButtonMode mode, StellarX::ControlShape shape, COLORREF ct, COLORREF cf, COLORREF ch);
	// 判断鼠标是否在圆形按钮内
	bool isMouseInCircle(int mouseX, int mouseY, int x, int y, int radius);
	// 判断鼠标是否在椭圆按钮内
	bool isMouseInEllipse(int mouseX, int mouseY, int x, int y, int width, int height);
	// Button 不是模态对话框，固定返回 false。
	bool model() const override { return false; }
	// 按当前按钮宽度和语言特征裁剪文本
	void cutButtonText();
	// 统一隐藏 Tooltip 并恢复其背景快照。
	void hideTooltip();
	// 在最后一次记录的鼠标位置显示 Tooltip。
	void showTooltipAtLastMouse();
	// 根据当前 click 状态选择 Tooltip 文案
	void refreshTooltipTextForState();
};
