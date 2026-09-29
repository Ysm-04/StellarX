/*******************************************************************************
 * @类: TextBox
 * @摘要: 文本框控件，支持输入和只读两种模式
 * @描述:
 *     提供文本输入和显示功能，集成EasyX的InputBox用于数据输入。
 *     支持有限的形状样式和视觉定制。
 *
 * @特性:
 *     - 两种工作模式：输入模式和只读模式
 *     - 最大字符长度限制
 *     - 集成系统输入框简化文本输入
 *     - 支持四种矩形形状变体
 *
 * @使用场景: 数据输入、文本显示、表单字段等
 * @所属框架: 星垣(StellarX) GUI框架
 * @作者: 我在人间做废物
 ******************************************************************************/
#pragma once
#include "Control.h"

class TextBox : public Control
{
	std::string   text;   //文本
	StellarX::TextBoxmode   mode;  //模式
	StellarX::ControlShape  shape; //形状
	bool          click = false;  //是否点击
	size_t           maxCharLen = 10;//最大字符长度
	COLORREF      textBoxBkClor = RGB(255, 255, 255); //背景颜色
	COLORREF      textBoxBorderClor = RGB(0, 0, 0);     //边框颜色

public:
	StellarX::ControlText textStyle; //文本样式

	// 构造文本框；支持输入模式和只读模式
	TextBox(int x, int y, int width, int height, std::string text = "", StellarX::TextBoxmode mode = StellarX::TextBoxmode::INPUT_MODE, StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE);
	// 绘制文本框
	void draw() override;
	// 处理输入框鼠标交互和文本录入
	bool handleEvent(const ExMessage& msg) override;
	// 设置文本框模式。
	// PASSWORD_MODE 只改变显示形态：内部仍保存真实文本，绘制时临时生成掩码。
	void setMode(StellarX::TextBoxmode mode);
	// 设置可输入最大长度；当前按 std::string 字节长度限制，不等同于 Unicode 字符数。
	// len 为 0 时保持原值；截断逻辑会尽量落在完整 MBCS/GBK 字符边界上。
	void setMaxCharLen(size_t len);
	// 设置文本框形状；仅支持矩形/圆角矩形，圆形和椭圆会回退为 RECTANGLE。
	void setTextBoxshape(StellarX::ControlShape shape);
	// 设置文本框边框颜色。
	void setTextBoxBorder(COLORREF color);
	// 设置文本框背景颜色。
	void setTextBoxBk(COLORREF color);
	// 设置当前文本。
	// 超过 maxCharLen 时按字节长度截断；已绘制且存在父控件时会请求父级托管重绘。
	void setText(std::string text);

	// 获取当前真实文本；PASSWORD_MODE 下也返回未掩码内容。
	std::string getText() const;

private:
	// TextBox 不是模态对话框，固定返回 false。
	bool model() const override { return false; };
};
