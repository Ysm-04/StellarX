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

class Control
{
protected:
    int x, y;           // 左上角坐标
    int width, height;  // 控件尺寸

    StellarX::RouRectangle rouRectangleSize; // 圆角矩形椭圆宽度和高度

    LOGFONT currentFont;          // 保存当前字体样式和颜色
    COLORREF currentColor = 0;
    COLORREF currentBkColor = 0;  // 保存当前填充色
    COLORREF currentBorderColor = 0; // 边框颜色
    LINESTYLE* currentLineStyle = new LINESTYLE; // 保存当前线型

public:
    Control(const Control&) = delete;
    Control& operator=(const Control&) = delete;
    Control(Control&&) = default;
    Control& operator=(Control&&) = default;

    Control() : x(0), y(0), width(100), height(100) {}
    Control(int x, int y, int width, int height)
        : x(x), y(y), width(width), height(height) {
    }

    virtual ~Control() {
        delete currentLineStyle;
        currentLineStyle = nullptr;
    }

    // 获取位置和尺寸
    int getX() const { return x; }
    int getY() const { return y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getRight() const { return x + width; }
    int getBottom() const { return y + height; }

    virtual void draw() = 0;
    virtual void handleEvent(const ExMessage& msg) = 0;

protected:
    void saveStyle();
    void restoreStyle();
};