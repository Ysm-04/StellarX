/*******************************************************************************
 * @类: TextBox
 * @摘要: 单行文本编辑控件，支持输入、只读和密码模式
 * @描述:
 *     使用 UTF-16 编辑模型与宽字符 GDI 自绘，直接接入 StellarX 的
 *     键盘、文字、焦点和指针选区路由。
 *
 * @特性:
 *     - 输入、只读、密码三种工作模式
 *     - 按 Unicode scalar 计数的最大长度限制
 *     - caret、选区、拖选和单行水平滚动
 *     - 支持四种矩形形状变体
 *
 * @使用场景: 数据输入、文本显示、表单字段等
 * @所属框架: 星垣(StellarX) GUI框架
 * @作者: 我在人间做废物
 ******************************************************************************/
#pragma once
#include "Control.h"

#include <memory>
#include <string>

/// 使用 UTF-16 编辑模型和宽字符 GDI 自绘的单行文本框。
class TextBox : public Control
{
	struct Impl;
	std::unique_ptr<Impl> impl;
	StellarX::TextBoxmode mode;
	StellarX::ControlShape shape;
	COLORREF textBoxBkClor = RGB(255, 255, 255);
	COLORREF textBoxBorderClor = RGB(0, 0, 0);

public:
	/// 文本绘制样式。修改字段后调用 setDirty(true) 触发重新布局和绘制。
	StellarX::ControlText textStyle;

	/// 创建单行文本框，并设置初始几何、ACP 兼容文本、工作模式和外形。
	TextBox(int x, int y, int width, int height, std::string text = "",
		StellarX::TextBoxmode mode = StellarX::TextBoxmode::INPUT_MODE,
		StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE);
	/// 销毁文本框及其内部 UTF-16 编辑模型。
	~TextBox() override;

	/// 绘制背景、边框、正式文字、临时组合串、选区和 caret。
	void draw() override;
	/// 处理兼容 ExMessage 指针事件；键盘与文字不从该入口投递。
	bool handleEvent(const ExMessage& msg) override;
	/// 处理路由到当前文本框的编辑按键并返回冒泡裁决。
	StellarX::KeyRouteResult handleKeyEvent(
		StellarX::RoutedKeyEvent& event) override;
	/// 接收一次正式文字提交并写入编辑模型。
	void handleTextInputEvent(
		const StellarX::RoutedTextInputEvent& event) override;
	/// 更新或结束当前 IME 临时组合视图，不直接提交正式文字。
	void handleCompositionEvent(
		const StellarX::RoutedCompositionEvent& event) override;
	/// 处理客户区内的定向鼠标选区手势；返回是否接受该手势。
	bool handleTextSelectionGesture(
		const StellarX::RoutedTextSelectionGestureEvent& event) override;
	/// 推进 caret 闪烁等依赖时钟的视觉状态。
	void tick(ULONGLONG nowMs) override;
	/// 在宿主窗口尺寸变化后使文本布局与背景快照失效。
	void onWindowResize() override;

	/// 切换输入、只读或密码模式，并重新校验焦点与文字输入资格。
	void setMode(StellarX::TextBoxmode mode);
	/// 设置最大 Unicode scalar 数量；len 为 0 时保持当前限制不变。
	void setMaxCharLen(size_t len);
	/// 设置文本框的矩形外形变体。
	void setTextBoxshape(StellarX::ControlShape shape);
	/// 设置文本框边框颜色。
	void setTextBoxBorder(COLORREF color);
	/// 设置文本框背景颜色。
	void setTextBoxBk(COLORREF color);

	/// 使用 UTF-16 设置正式文本；输入非法或超过限制时返回 false 且不修改原值。
	[[nodiscard]] bool setTextUtf16(std::u16string text);
	/// 返回内部 UTF-16 正式文本的只读引用；引用生命周期不超过当前 TextBox。
	const std::u16string& getTextUtf16() const noexcept;

	/// 通过 Windows ACP 兼容边界设置文本；转换失败时保持原文本。
	void setText(std::string text);
	/// 通过 Windows ACP 兼容边界返回文本；转换失败时返回空字符串。
	std::string getText() const;
	/// 尝试通过 Windows ACP 输出文本；失败时返回 false 且保持 output 不变。
	bool tryGetText(std::string& output) const;

protected:
	/// 发布当前 TextBox 的焦点资格、可见可用状态和稳定身份快照。
	bool queryFocusTargetSnapshot(const StellarX::FocusTargetId& target,
		StellarX::FocusTargetSnapshot& snapshot) const override;
	/// 返回当前模式是否接受正式文字修改；只读模式返回 false。
	bool acceptsTextInput() const override;
	/// 返回当前模式是否允许框架接管并内联绘制 IME 临时组合串。
	bool supportsInlineComposition() const override;
	/// TextBox 始终支持客户区内的逻辑文字选区手势。
	bool supportsTextSelection() const override { return true; }
	/// 焦点变化时同步 caret、选区视觉、滚动位置和 IME anchor。
	void onLogicalFocusChanged(bool focused,
		StellarX::FocusReason reason) override;
	/// 窗口失活时暂停编辑并清理不应跨激活保留的瞬时状态。
	void onKeyboardRoutingSuspended() override;
	/// 窗口恢复激活时重新发布当前文本焦点的输入资格与 anchor。
	void onKeyboardRoutingResumed() override;

private:
	// TextBox 不作为复合焦点容器参与模型分支。
	bool model() const override { return false; }
	// 作废文字测量与水平滚动缓存；需要时同时请求一次托管重绘。
	void invalidateEditingLayout(bool requestVisual);
};
