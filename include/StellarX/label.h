/*******************************************************************************
 * @类: Label
 * @摘要: 简单文本标签控件，用于显示静态文本
 * @描述:
 *     提供基本的文本显示功能，支持透明背景和自定义样式。
 *     不支持用户交互，专注于文本呈现。
 *
 * @特性:
 *     - 支持背景透明/不透明模式
 *     - 完整的文本样式控制（字体、颜色、效果）
 *     - 自动适应文本内容
 *     - 轻量级无事件处理开销
 *
 * @使用场景: 显示说明文字、标题、状态信息等静态内容
 * @所属框架: 星垣(StellarX) GUI框架
 * @作者: 我在人间做废物
 ******************************************************************************/

#pragma once
#include "Control.h"

class Label : public Control
{
    std::string text;         //标签文本
       COLORREF textBkColor; //标签背景颜色
           bool textBkDisap = false;   //标签背景是否透明

    //标签事件处理（标签无事件）不实现具体代码
    bool handleEvent(const ExMessage& msg) override { return false; }
    //用来检查对话框是否模态,此控件不做实现
    bool model() const override { return false; };
public:
    StellarX::ControlText   textStyle;   //标签文本样式
public:
    Label();
    Label(int x, int y, std::string text = "标签",COLORREF textcolor = BLACK, COLORREF bkColor= RGB(255,255,255));
  
    void draw() override;
    void hide();
    //设置标签背景是否透明
    void setTextdisap(bool key);
    //设置标签背景颜色
    void setTextBkColor(COLORREF color);
    //设置标签文本
    void setText(std::string text);
 
};

