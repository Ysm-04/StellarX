/*******************************************************************************
 * @类: TabControl
 * @摘要: 选项卡容器控件，管理“页签按钮 + 对应页面(Canvas)”
 * @描述:
 *     提供页签栏布局（上/下/左/右）、选中切换、页内容区域定位；
 *     与 Button 一起工作，支持窗口大小变化、可见性联动与脏区重绘。
 *
 * @特性:
 *     - 页签栏四向排列（Top / Bottom / Left / Right）
 *     - 一键添加“页签+页”或为指定页添加子控件
 *     - 获取/设置当前激活页签索引
 *     - 自适应窗口变化，重算页签与页面区域
 *     - 与 Button 的 TOGGLE 模式联动显示/隐藏页面
 *
 * @使用场景: 在同一区域内承载多张页面，使用页签进行快速切换
 * @所属框架: 星垣(StellarX) GUI框架
 * @作者: 我在人间做废物
 ******************************************************************************/

#pragma once
#include "CoreTypes.h"
#include "Button.h"
#include "Canvas.h"
#define BUTMINHEIGHT 15
#define BUTMINWIDTH  30
class TabControl :public Canvas
{
    int tabBarHeight = BUTMINWIDTH;        //页签栏高度   
    StellarX::TabPlacement tabPlacement = StellarX::TabPlacement::Top ; //页签排列方式
	std::vector<std::pair<std::unique_ptr<Button>,std::unique_ptr<Canvas>>> controls; //页签/页列表

private:
    using Canvas::addControl;                 // 禁止外部误用
    void addControl(std::unique_ptr<Control>) = delete; // 精准禁用该重载
private:
    inline void initTabBar();
    inline void initTabPage();
public:
	TabControl();
	TabControl(int x,int y,int width,int height);
	~TabControl();
	
	void draw() override;
	bool handleEvent(const ExMessage& msg) override;
     
    //添加页签+页
    void add(std::pair<std::unique_ptr<Button> ,std::unique_ptr<Canvas>>&& control);
    //添加为某个页添加控件
    void add(std::string tabText,std::unique_ptr<Control> control);
    //设置页签位置 
    void setTabPlacement(StellarX::TabPlacement placement);
    //设置页签栏高度 两侧排列时为宽度
    void setTabBarHeight(int height);
    //设置不可见后传递给子控件重写
    void setIsVisible(bool visible) override;
    void onWindowResize() override;
    //获取当前激活页签索引
    int getActiveIndex() const;
    //设置当前激活页签索引
    void setActiveIndex(int idx);
    //获取页签数量
    int count() const; 
    //通过页签文本返回索引
    int indexOf(const std::string& tabText) const;
};

