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

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600  
#endif
#ifndef WINVER
#define WINVER _WIN32_WINNT
#endif

#include <windows.h>

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
    std::string id;     // 控件ID
    int localx,x, localy,y;           // 左上角坐标
    int localWidth,width, localHeight,height;  // 控件尺寸
    Control* parent = nullptr;    // 父控件
    bool dirty = true; // 是否重绘
    bool show = true; // 是否显示

    /* == 背景快照 ==  */
    IMAGE* saveBkImage = nullptr;
    int saveBkX = 0, saveBkY = 0;      // 快照保存起始坐标
    int saveWidth = 0, saveHeight = 0; // 快照保存尺寸
    bool hasSnap = false;     //  当前是否持有有效快照

    StellarX::RouRectangle rouRectangleSize; // 圆角矩形椭圆宽度和高度

    LOGFONT* currentFont = new LOGFONT();       // 保存当前字体样式和颜色
    COLORREF* currentColor = new COLORREF();
    COLORREF* currentBkColor = new COLORREF();  // 保存当前填充色
    COLORREF* currentBorderColor = new COLORREF(); // 边框颜色
    LINESTYLE* currentLineStyle = new LINESTYLE(); // 保存当前线型

    Control(const Control&) = delete;
    Control& operator=(const Control&) = delete;
    Control(Control&&) = default;
    Control& operator=(Control&&) = default;

    Control() : localx(0),x(0), localy(0),y(0), localWidth(100),width(100),height(100), localHeight(100) {}
    Control(int x, int y, int width, int height)
        : localx(x), x(x), localy(y), y(y), localWidth(width), width(width), height(height), localHeight(height){}
public:

    virtual ~Control()
    {
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
        discardBackground();
    }
protected:
    virtual void requestRepaint();
    //根控件/无父时触发重绘
    virtual void onRequestRepaintAsRoot();
protected:
    //保存背景快照
    void saveBackground(int x, int y, int w, int h);
    // putimage 回屏
    void restBackground();         
    // 释放快照（窗口重绘/尺寸变化后必须作废）
    void discardBackground();
public:
    //释放快照重新保存，在尺寸变化时更新背景快照避免尺寸变化导致显示错位
    void updateBackground();
    //窗口变化丢快照
    virtual void onWindowResize();
    // 获取位置和尺寸
    int getX() const { return x; }
    int getY() const { return y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getRight() const { return x + width; }
    int getBottom() const { return y + height; }
   
    int getLocalX() const { return localx; }
    int getLocalY() const { return localy; }
    int getLocalWidth() const { return localWidth; }
    int getLocalHeight() const { return localHeight; }
    int getLocalRight() const { return localx + localWidth; }
    int getLocalBottom() const { return localy + localHeight; }

    void setX(int x) { this->x = x; dirty = true; }
    void setY(int y) { this->y = y; dirty = true; }
    void setWidth(int width) { this->width = width; dirty = true; }
    void setHeight(int height) { this->height = height; dirty = true; }
public:

    virtual void draw() = 0;
    virtual bool handleEvent(const ExMessage& msg) = 0;//返回true代表事件已消费
    //设置是否显示
    virtual void setIsVisible(bool show);
    //设置父容器指针
    void setParent(Control* parent) { this->parent = parent; }
    //设置是否重绘
    virtual void setDirty(bool dirty) { this->dirty = dirty; }

    //检查控件是否可见
    bool IsVisible() const { return show; };
    //获取控件id
    std::string getId() const { return id; }
    //检查是否为脏
    bool isDirty() { return dirty; }
    //用来检查对话框是否模态，其他控件不用实现
    virtual bool model()const = 0;
protected:
    void saveStyle();
    void restoreStyle();
};