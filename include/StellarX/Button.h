 /*******************************************************************************
  * @文件: Button.h
  * @摘要: 按钮控件类
  * @描述:
  *     提供多种样式和行为的按钮控件，支持点击、悬停、切换等状态。
  *     继承自Control基类，是框架的核心交互组件之一。
  *
  * @所属框架: 星垣(StellarX) GUI框架
  * @作者: 我在人间做废物
  * @日期: September 2025
  ******************************************************************************/
#pragma once
#include "Control.h"

class Button : public Control
{
    
    std::string text;             // 按钮上的文字
    bool        click;            // 是否被点击
    bool        hover;            // 是否被悬停
    bool        dirty = true;     // 是否重绘

    COLORREF         buttonTrueColor;                    // 按钮被点击后的颜色
    COLORREF         buttonFalseColor;                   // 按钮未被点击的颜色
    COLORREF         buttonHoverColor;                   // 按钮被鼠标悬停的颜色
    COLORREF         buttonBorderColor = RGB(0,0,0);// 按钮边框颜色

    StellarX::ButtonMode  mode;	    // 按钮模式
    StellarX::ControlShape shape;     // 按钮形状

    int         buttonFillMode = BS_SOLID;   //按钮填充模式
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
public:
    StellarX::ControlText textStyle;  // 按钮文字样式

public:
    /*************************************************************************/
    /********************************构造函数*********************************/
    /*************************************************************************/
    
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

/*************************************************************************/
/********************************Set方法**********************************/
/*************************************************************************/
    //绘制按钮
    void draw() override;
    //按钮事件处理
    
    void handleEvent(const ExMessage& msg) override;
    
    //设置回调函数
    //************************************
    // 名称:    setOnClickListener | setOnToggleOnListener | setOnToggleOffListener
    // 全名:  Button::setOnClickListener
    // 访问:    public 
    // 返回类型:   void
	// Parameter: const std::function<> & & callback 设置回调函数  传入回调函数名即可，不需要传入()，不需要传入参数，不需要传入返回值
	//                                                         如果要传入参数，可以使用lambda表达式
    //************************************
    void setOnClickListener(const std::function<void()>&& callback);
    //设置TOGGLE模式下被点击的回调函数
    void setOnToggleOnListener(const std::function<void()>&& callback);
    //设置TOGGLE模式下取消点击的回调函数
    void setOnToggleOffListener(const std::function<void()>&& callback);
    //设置按钮模式
    void setbuttonMode(StellarX::ButtonMode mode);
    //设置圆角矩形椭圆宽度
    int setROUND_RECTANGLEwidth(int width);
    //设置圆角矩形椭圆高度
    int setROUND_RECTANGLEheight(int height);
    //设置按钮填充模式
    void setFillMode(int mode);
    //设置按钮填充图案
    void setFillIma(StellarX::FillStyle ima);
    //设置按钮填充图像
    void setFillIma(std::string imaName);
    //设置按钮边框颜色
    void setButtonBorder(COLORREF Border);
    //设置按钮文本
    void setButtonText(const char* text);
    void setButtonText(std::string text);
    //设置按钮形状
    void setButtonShape(StellarX::ControlShape shape);

    //判断按钮是否被点击
    bool isClicked() const;
  
    /*************************************************************************/
    /********************************Get方法**********************************/
    /*************************************************************************/
	
    //获取按钮文字
    std::string getButtonText() const;
    const char* getButtonText_c() const;
    //获取按钮模式
    StellarX::ButtonMode getButtonMode() const;
    //获取按钮形状
    StellarX::ControlShape getButtonShape() const;
    //获取按钮填充模式
    int getFillMode() const;
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
	
private:
    //初始化按钮
    void initButton(const std::string text, StellarX::ButtonMode mode, StellarX::ControlShape shape, COLORREF ct, COLORREF cf, COLORREF ch);
    //判断鼠标是否在圆形按钮内
    bool isMouseInCircle(int mouseX, int mouseY, int x, int y, int radius);
    //判断鼠标是否在椭圆按钮内
    bool isMouseInEllipse(int mouseX, int mouseY, int x, int y, int width, int height);  
};

