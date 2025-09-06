#pragma once
/*********************************************************************
 * \文件： Control.h
 * \描述： 控件基类，所有控件都继承自此类。
 *         提供了控件的一些基本属性和方法。
 * 
 * \作者： 我在人间做废物 
 * \日期： September 2025
 *********************************************************************/
#include <vector>
#include <memory>
#include <easyx.h>
#include <iostream>
#include <string>
#include <functional>
#include <initializer_list>
#include "CoreTypes.h"
/**
 * @类名称: Control
 * @功能描述: 控件基类，所有控件都继承自此类。
 *           提供了控件的一些基本属性和方法。
 *
 * @成员说明:
 *      int x, y, width, height; // 控件的基本属性
 *
 *      LOGFONT     currentFont;       //保存当前字体样式和颜色
 *      COLORREF    currentColor = 0;
 *      COLORREF    currentBkColor = 0; //保存当前填充色
 *      COLORREF    currentBorderColor = 0;//边框颜色
 *      LINESTYLE*  currentLineStyle = new LINESTYLE;    //保存当前线型
 *
 *      Control():x(0), y(0), width(0), height(0) {}
 *      Control(int x, int y, int width, int height) :x(x),y(y),width(width),height(height){}
 *      virtual void draw() = 0; // 绘制控件
 *      virtual void handleEvent(const ExMessage& msg) = 0; // 处理事件
 *      virtual ~Control() { delete currentLineStyle; }
 *
 *      //保存当前字体样式和颜色
 *      void saveStyle();
 *      //恢复默认字体样式和颜色
 *      void restoreStyle();
 *
 * @使用示例:
 *   Control* ctrl = new Button(10, 10, 100, 50, "Click Me");
 *   ctrl->draw();
 *   delete ctrl;
 *
 * @备注:
 *   这是一个抽象基类，不能直接实例化。
 *   所有具体的控件类都必须实现draw和handleEvent方法。
 */
class Control 
{
protected:
    int x, y, width, height; // 控件的基本属性

    LOGFONT     currentFont;       //保存当前字体样式和颜色
    COLORREF    currentColor = 0;
    COLORREF    currentBkColor = 0; //保存当前填充色
    COLORREF    currentBorderColor = 0;//边框颜色
    LINESTYLE*  currentLineStyle = new LINESTYLE;    //保存当前线型
public:
	Control(const Control&) = delete;            // 禁用拷贝构造
	Control& operator=(const Control&) = delete; // 禁用拷贝赋值
	Control(Control&&) = default;                // 允许移动构造
	Control& operator=(Control&&) = default;     // 允许移动赋值
    Control():x(0), y(0), width(100), height(100) {}
    Control(int x, int y, int width, int height) :x(x),y(y),width(x + width),height(y + height){}
    
    virtual void draw() = 0; // 绘制控件
    virtual void handleEvent(const ExMessage& msg) = 0; // 处理事件
	virtual ~Control() {
        delete currentLineStyle; currentLineStyle = nullptr; }
	
protected:
    //保存当前样式和颜色
    void saveStyle();
    //恢复默认样式和颜色
    void restoreStyle();
};
