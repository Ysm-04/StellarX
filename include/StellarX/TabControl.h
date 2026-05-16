/*******************************************************************************
 * @类: TabControl
 * @摘要: 选项卡容器控件，管理“页签按钮 + 对应页面(Canvas)”
 * @描述:
 *     提供页签栏布局（上/下/左/右）、选中切换、页内容区域定位；
 *     与 Button 一起工作，支持窗口大小变化、可见性联动与脏区重绘。
 *     在托管重绘模式下，TabControl 作为“页签栏 + 当前页面”的统一重绘 root。
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
#define BUTMINHEIGHT 15   //页签按钮最小尺寸，过小会导致显示问题
#define BUTMINWIDTH  30   //页签按钮最小尺寸，过小会导致显示问题
class TabControl :public Canvas
{
	int tabBarHeight = BUTMINWIDTH;   //页签栏高度
	bool IsFirstDraw = true;          //首次绘制标记
	int defaultActivation = -1;       //默认激活页签索引
	StellarX::TabPlacement tabPlacement = StellarX::TabPlacement::Top; //页签排列方式
	std::vector<std::pair<std::unique_ptr<Button>, std::unique_ptr<Canvas>>> controls; //页签/页列表

private:
	using Canvas::addControl;                 // 禁止外部误用
	void addControl(std::unique_ptr<Control>) = delete; // 精准禁用该重载
private:
	// 初始化页签按钮位置和尺寸
	inline void initTabBar();
	inline void initTabPage();
	// 统一刷新 TabControl 当前运行态下的页签栏和页面区布局。
	void refreshRuntimeLayout();
public:
	// 默认构造：创建一个空 TabControl
	TabControl();
	// 指定初始位置和尺寸构造 TabControl
	TabControl(int x, int y, int width, int height);
	~TabControl();

	//重写位置设置以适应页签和页面布局
	void setX(int x)override;
	void setY(int y)override;
	void setWidth(int width) override;
	void setHeight(int height) override;

	void draw() override;
	bool handleEvent(const ExMessage& msg) override;
	// 只转发清理页签按钮和当前可见页面中的鼠标临时状态，不升级为整块 TabControl 重绘。
	bool clearTransientMouseState() override;

	// 添加一个“页签按钮 + 页面 Canvas”组合
	void add(std::pair<std::unique_ptr<Button>, std::unique_ptr<Canvas>>&& control);
	// 按页签文本把控件添加到对应页面
	void add(std::string tabText, std::unique_ptr<Control> control);
	//设置页签位置
	void setTabPlacement(StellarX::TabPlacement placement);
	//设置页签栏高度 两侧排列时为宽度
	void setTabBarHeight(int height);
	//设置不可见后传递给子控件重写
	void setIsVisible(bool visible) override;
	void onWindowResize() override;
	//获取当前激活页签索引
	int getActiveIndex() const;
	// 设置当前激活页签索引；若已是当前激活页则直接返回
	void setActiveIndex(int idx);
	// 获取页签数量
	int count() const;
	// 通过页签文本查找索引
	int indexOf(const std::string& tabText) const;
	//设置脏区并请求重绘
	void setDirty(bool dirty) override;
	//请求父控件重绘
	void requestRepaint(Control* parent)override;          // 托管模式下登记为 root；非托管模式下局部更新脏按钮/脏页面
	bool hasManagedDirtySubtree() const override;
	RECT getManagedRepaintCoverageRect() const override;
	RECT getManagedRepaintPersistentCoverageRect() const override;
	bool canCommitManagedPartialRepaint() const override;  // 判断当前 TabControl 是否可安全做局部提交
	void commitManagedRepaint() override;                  // 托管收口阶段执行 TabControl 的真正重绘
protected:
	// 外层统一解算后，TabControl 需要同步刷新其内部页签栏和页面区。
	void applyResolvedLayoutRect(const StellarX::ResolvedLayoutRect& rect) override;
};
