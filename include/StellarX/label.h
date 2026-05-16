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
	std::string lastMeasuredText;              // 最近一次参与尺寸测量的文本
	StellarX::ControlText lastMeasuredStyle;   // 最近一次参与尺寸测量的字体样式
	bool contentMeasureValid = false;          // 当前内容尺寸缓存是否有效

	//标签事件处理（标签无事件）不实现具体代码
	bool handleEvent(const ExMessage& msg) override { return false; }
	//用来检查对话框是否模态,此控件不做实现
	bool model() const override { return false; };
	// 根据当前文本和字体样式刷新运行态尺寸。
	// 这是 Label 的内容驱动几何入口：
	// - 覆盖文本变化
	// - 覆盖字体样式变化
	// - 只更新运行态宽高，不自动提交设计基线 local*
	void refreshContentDrivenRuntimeGeometry();
public:
	// 标签字体样式保持公开，便于用户直接改字段。
	// 但直接修改 textStyle 后，必须显式调用 setDirty(true)，
	// 才会触发内容驱动尺寸刷新并让新的运行态宽高生效。
	StellarX::ControlText   textStyle;   //标签文本样式
public:
	// 默认构造：创建一个内容驱动尺寸的标签
	Label();
	// 指定位置、文本和基础样式构造标签
	Label(int x, int y, std::string text = "标签", COLORREF textcolor = BLACK, COLORREF bkColor = RGB(255, 255, 255));

	// 绘制标签文本
	void draw() override;
	//用于“隐藏提示框”时调用（还原并释放快照）
	void hide();
	// Label 的内容尺寸由文本和字体样式共同决定。
	// 因此只要进入 dirty 状态，就在绘制前先刷新一次运行态尺寸，
	// 避免 draw() 阶段继续承担新的几何决策。
	void setDirty(bool dirty) override;
	// 设置标签背景是否透明
	void setTextdisap(bool key);
	// 设置标签背景颜色
	void setTextBkColor(COLORREF color);
	// 设置标签文本，并触发内容驱动尺寸刷新
	void setText(std::string text);
protected:
	// 统一布局器只负责安置 Label 的运行态位置；
	// 宽高仍由 Label 自己的内容驱动路径决定。
	void applyResolvedLayoutRect(const StellarX::ResolvedLayoutRect& rect) override;
};
