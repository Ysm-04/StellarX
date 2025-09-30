/*******************************************************************************
 * @类: TextBox
 * @摘要: 文本框控件，支持输入和只读两种模式
 * @描述:
 *     提供文本输入和显示功能，集成EasyX的InputBox用于数据输入。
 *     支持有限的形状样式和视觉定制。
 *
 * @特性:
 *     - 两种工作模式：输入模式和只读模式
 *     - 最大字符长度限制
 *     - 集成系统输入框简化文本输入
 *     - 支持四种矩形形状变体
 *
 * @使用场景: 数据输入、文本显示、表单字段等
 * @所属框架: 星垣(StellarX) GUI框架
 * @作者: 我在人间做废物
 ******************************************************************************/
#pragma once
#include "Control.h"


class TextBox : public Control
{
	std::string   text;   //文本
    StellarX::TextBoxmode   mode;  //模式
    StellarX::ControlShape  shape; //形状
    bool          click = false;  //是否点击
    size_t           maxCharLen = 10;//最大字符长度
    COLORREF      textBoxBkClor = RGB(255, 255, 255); //背景颜色
    COLORREF      textBoxBorderClor = RGB(0,0,0);     //边框颜色


	//检查是否对话框是否可见
	bool IsVisible() const override { return false; }
	//获取对话框类型
	bool model() const override { return false; }
public:
    StellarX::ControlText textStyle; //文本样式

    TextBox(int x, int y, int width, int height, std::string text = "", StellarX::TextBoxmode mode = StellarX::TextBoxmode::INPUT_MODE, StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE);
    void draw() override;
    bool handleEvent(const ExMessage& msg) override;
    //设置模式
    void setMode(StellarX::TextBoxmode mode);
    //设置可输入最大字符长度
    void setMaxCharLen(size_t len);
    //设置形状
    void setTextBoxshape(StellarX::ControlShape shape);
    //设置边框颜色
    void setTextBoxBorder(COLORREF color);
    //设置背景颜色
    void setTextBoxBk(COLORREF color);    
    //设置文本
    void setText(std::string text);

    //获取文本
    std::string getText() const;  

};


