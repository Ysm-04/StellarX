#pragma once
#include "Control.h"

// 文本框控件，支持输入和只读两种模式
// 特殊说明: 
//   - 在INPUT_MODE下点击会调用EasyX的InputBox
//   - 在READONLY_MODE下点击会显示信息对话框
//   - 最大字符长度受maxCharLen限制
class textBox : public Control
{
	std::string   text;   //文本
    StellarX::textBoxmode   mode;  //模式
    StellarX::controlShape  shape; //形状
    bool          dirty = true;   //是否重绘
    bool          click = false;  //是否点击
    int           maxCharLen = 10;//最大字符长度
    COLORREF      textBoxBkClor = RGB(255, 255, 255); //背景颜色
    COLORREF      textBoxBorderClor = RGB(0,0,0);     //边框颜色

public:
    StellarX::controlText textStyle; //文本样式

    textBox(int x, int y, int width, int height, std::string text = "", StellarX::textBoxmode mode = StellarX::textBoxmode::INPUT_MODE, StellarX::controlShape shape = StellarX::controlShape::RECTANGLE);
    void draw() override;
    void handleEvent(const ExMessage& msg) override;
    //设置模式

    void setMode(StellarX::textBoxmode mode);
    //设置可输入最大字符长度
    void setmaxCharLen(int len);    
    //设置形状
    void settextBoxshape(StellarX::controlShape shape);
    //设置边框颜色
    void settextBoxBorder(COLORREF color);
    //设置背景颜色
    void settextBoxBk(COLORREF color);       

    //获取文本
    std::string gettext();  

};


