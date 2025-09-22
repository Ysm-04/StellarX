/*******************************************************************************
 * @类: Control
 * @摘要: 所有控件的抽象基类，定义通用接口和基础功能
 * @描述:
 *     提供控件的基本属性和方法，包括位置、尺寸、重绘标记等。
 *     实现绘图状态保存和恢复机制，确保控件绘制不影响全局状态。
 *
 * @特性:
 *     - 定义控件基本属性（坐标、尺寸、脏标记）
 *     - 提供绘图状态管理（saveStyle/restoreStyle）
 *     - 声明纯虚接口（draw、handleEvent等）
 *     - 支持移动语义，禁止拷贝语义
 *
 * @使用场景: 作为所有具体控件类的基类，不直接实例化
 * @所属框架: 星垣(StellarX) GUI框架
 * @作者: 我在人间做废物
 ******************************************************************************/
#pragma once
#include <vector>
#include <memory>
#include <easyx.h>
#undef MessageBox
#include <iostream>
#include <string>
#include <functional>
#include "CoreTypes.h"

class Control
{
protected:
    int x, y;           // 左上角坐标
    int width, height;  // 控件尺寸
    bool dirty = true; // 是否重绘

    StellarX::RouRectangle rouRectangleSize; // 圆角矩形椭圆宽度和高度

    LOGFONT* currentFont = new LOGFONT();       // 保存当前字体样式和颜色
    COLORREF* currentColor = new COLORREF();
    COLORREF* currentBkColor = new COLORREF();  // 保存当前填充色
    COLORREF* currentBorderColor = new COLORREF(); // 边框颜色
    LINESTYLE* currentLineStyle = new LINESTYLE(); // 保存当前线型

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
        delete currentFont;
        delete currentColor;
        delete currentBkColor;
        delete currentBorderColor;
        delete currentLineStyle;

		currentFont = nullptr;
        currentColor = nullptr;
        currentBkColor = nullptr;
        currentBorderColor = nullptr;
        currentLineStyle = nullptr;


    }
protected:
    // 获取位置和尺寸
    int getX() const { return x; }
    int getY() const { return y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getRight() const { return x + width; }
    int getBottom() const { return y + height; }
public:
    //设置是否重绘
    void setDirty(bool dirty) { this->dirty = dirty; }
    virtual void draw() = 0;
    virtual bool handleEvent(const ExMessage& msg) = 0;//返回true代表事件已消费
    //用来检查非模态对话框是否可见，其他控件不用实现
    virtual bool IsVisible() const = 0;
    //用来检查对话框是否模态，其他控件不用实现
    virtual bool model()const = 0;

protected:
    void saveStyle();
    void restoreStyle();
};