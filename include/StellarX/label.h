#pragma once
#include "Control.h"
// 标签控件，用于显示文本
// 特点:
//   - 支持背景透明/不透明模式
//   - 不支持用户交互（无事件处理）
//   - 自动适应文本内容大小

class Label : public Control
{
    std::string text;         //标签文本
    COLORREF    textColor;   //标签文本颜色
    COLORREF    textBkColor; //标签背景颜色
    bool        textBkDisap = false;   //标签背景是否透明

    //标签事件处理（标签无事件）不实现具体代码
    void handleEvent(const ExMessage& msg) override {}
public:
    StellarX::ControlText   textStyle;   //标签文本样式
public:
    Label();
    Label(int x, int y, std::string text = "标签",COLORREF textcolor = BLACK, COLORREF bkColor= RGB(255,255,255));
    //绘标签
    void draw() override;
    //设置标签背景是否透明
    void setTextdisap(bool key);
    //设置标签文本颜色
    void setTextColor(COLORREF color);
    //设置标签背景颜色
    void setTextBkColor(COLORREF color);
    //设置标签文本
    void setText(std::string text);
 
};

