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
constexpr int bordWith = 1; //边框宽度，用于快照恢复时的偏移计算
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

    COLORREF         buttonTrueColor;                    // 按钮被点击后的颜色
    COLORREF         buttonFalseColor;                   // 按钮未被点击的颜色
    COLORREF         buttonHoverColor;                   // 按钮被鼠标悬停的颜色
    COLORREF         buttonBorderColor = RGB(0,0,0);// 按钮边框颜色

    StellarX::ButtonMode  mode;	    // 按钮模式
    StellarX::ControlShape shape;     // 按钮形状

    StellarX::FillMode    buttonFillMode = StellarX::FillMode::Solid;   //按钮填充模式
    StellarX::FillStyle   buttonFillIma  = StellarX::FillStyle::BDiagonal;        //按钮填充图案
    IMAGE*      buttonFileIMAGE = nullptr;      //按钮填充图像

    
    
    std::function<void()> onClickCallback;      //回调函数
    std::function<void()> onToggleOnCallback;   //TOGGLE模式下的回调函数
    std::function<void()> onToggleOffCallback;  //TOGGLE模式下的回调函数

    StellarX::ControlText oldStyle = textStyle;   // 按钮文字样式
    int oldtext_width = -1;
    int oldtext_height = -1;
    int text_width = 0;
    int text_height = 0;

    // === Tooltip ===
    bool        tipEnabled = false;           // 是否启用
    bool        tipVisible = false;          // 当前是否显示
    bool        tipFollowCursor = false;      // 是否跟随鼠标
    bool        tipUserOverride = false;     // 是否用户自定义了tip文本
    int         tipDelayMs = 1000;            // 延时(毫秒)
    int         tipOffsetX = 12;             // 相对鼠标偏移
    int         tipOffsetY = 18;
    ULONGLONG   tipHoverTick = 0;            // 开始悬停的时间戳
    int         lastMouseX = 0;              // 最新鼠标位置(用于定位)
    int         lastMouseY = 0;

    std::string tipTextClick;  //NORMAL 模式下用
    std::string tipTextOn;    // click==true 时用
    std::string tipTextOff;   // click==false 时用
    Label       tipLabel;                    // 直接复用Label作为提示
   
public:
    StellarX::ControlText textStyle;  // 按钮文字样式

public:
    
    //默认按钮颜色
    Button(int x, int y, int width, int height, const std::string text,
        StellarX::ButtonMode mode = StellarX::ButtonMode::NORMAL, StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE);
    //自定义按钮未被点击和被点击颜色
    Button(int x, int y, int width, int height, const std::string text,
           COLORREF ct, COLORREF cf, StellarX::ButtonMode mode = StellarX::ButtonMode::NORMAL,
        StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE);
    //自定义按钮颜色和悬停颜色
    Button(int x, int y, int width, int height, const std::string text,
             COLORREF ct, COLORREF cf,COLORREF ch,
        StellarX::ButtonMode mode = StellarX::ButtonMode::NORMAL, StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE);
    //析构函数 释放图形指针内存
    ~Button();

    //绘制按钮
    void draw() override;
    //按钮事件处理
    bool handleEvent(const ExMessage& msg) override;
    
    //设置回调函数
    void setOnClickListener(const std::function<void()>&& callback);
    //设置TOGGLE模式下被点击的回调函数
    void setOnToggleOnListener(const std::function<void()>&& callback);
    //设置TOGGLE模式下取消点击的回调函数
    void setOnToggleOffListener(const std::function<void()>&& callback);
    //设置按钮模式
    void setbuttonMode(StellarX::ButtonMode mode);
    //设置圆角矩形椭圆宽度
    void setROUND_RECTANGLEwidth(int width);
    //设置圆角矩形椭圆高度
    void setROUND_RECTANGLEheight(int height);
    //设置按钮填充模式
    void setFillMode(StellarX::FillMode mode);
    //设置按钮填充图案
    void setFillIma(StellarX::FillStyle ima);
    //设置按钮填充图像
    void setFillIma(std::string imaName);
    //设置按钮边框颜色
    void setButtonBorder(COLORREF Border);
    //设置按钮未被点击颜色
    void setButtonFalseColor(COLORREF color);
    //设置按钮文本
    void setButtonText(const char* text);
    void setButtonText(std::string text);
    //设置按钮形状
    void setButtonShape(StellarX::ControlShape shape);
    //设置按钮点击状态
    void setButtonClick(BOOL click);

    //判断按钮是否被点击
    bool isClicked() const;
	
    //获取按钮文字
    std::string getButtonText() const;
    const char* getButtonText_c() const;
    //获取按钮模式
    StellarX::ButtonMode getButtonMode() const;
    //获取按钮形状
    StellarX::ControlShape getButtonShape() const;
    //获取按钮填充模式
    StellarX::FillMode getFillMode() const;
	//获取按钮填充图案
    StellarX::FillStyle getFillIma() const;
	//获取按钮填充图像
	IMAGE* getFillImaImage() const;
	//获取按钮边框颜色
    COLORREF getButtonBorder() const;
	//获取按钮文字颜色
    COLORREF getButtonTextColor() const;
	//获取按钮文字样式
    StellarX::ControlText getButtonTextStyle() const;
    //获取按钮宽度
    int getButtonWidth() const;
    //获取按钮高度
    int getButtonHeight() const;
public:
    // === Tooltip API===
    //设置是否启用提示框
    void enableTooltip(bool on) { tipEnabled = on; if (!on) tipVisible = false; }
    //设置提示框延时
    void setTooltipDelay(int ms) { tipDelayMs = (ms < 0 ? 0 : ms); }
    //设置提示框是否跟随鼠标
    void setTooltipFollowCursor(bool on) { tipFollowCursor = on; }
    //设置提示框位置偏移
    void setTooltipOffset(int dx, int dy) { tipOffsetX = dx; tipOffsetY = dy; }
    //设置提示框样式
    void setTooltipStyle(COLORREF text, COLORREF bk, bool transparent);
    //设置提示框文本
    void setTooltipText(const std::string& s){ tipTextClick = s; tipUserOverride = true; }
    void setTooltipTextsForToggle(const std::string& onText, const std::string& offText);
private:
    //初始化按钮
    void initButton(const std::string text, StellarX::ButtonMode mode, StellarX::ControlShape shape, COLORREF ct, COLORREF cf, COLORREF ch);
    //判断鼠标是否在圆形按钮内
    bool isMouseInCircle(int mouseX, int mouseY, int x, int y, int radius);
    //判断鼠标是否在椭圆按钮内
    bool isMouseInEllipse(int mouseX, int mouseY, int x, int y, int width, int height);
	//获取对话框类型
    bool model() const override { return false; }
    void cutButtonText();
    // 统一隐藏&恢复背景
    void hideTooltip();
    // 根据当前 click 状态选择文案
    void refreshTooltipTextForState(); 

};

