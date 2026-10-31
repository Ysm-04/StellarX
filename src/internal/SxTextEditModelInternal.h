#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace StellarX::Internal
{
	// 一次模型修改请求的原子结果。
	enum class TextEditResult
	{
		Rejected, ///< 输入非法或违反容量规则，模型保持原样。
		Unchanged, ///< 请求合法，但结果与当前状态一致。
		Changed   ///< 文本、caret 或选区发生变化。
	};

	// 按 UTF-16 code unit 偏移表示的半开选区 [start, end)。
	struct TextSelectionRange
	{
		std::size_t start = 0;
		std::size_t end = 0;
	};

	// 纯 UTF-16 单行编辑状态，不拥有窗口、控件、绘制、剪贴板、焦点或路由。
	// 所有位置均为 UTF-16 code unit 偏移，并始终停在 Unicode scalar 边界。
	class TextEditModel final
	{
	public:
		// 未显式设置时允许的最大 Unicode scalar 数量。
		static constexpr std::size_t DefaultMaxScalarCount = 10;

		// 创建空文本、折叠选区和默认容量的模型。
		TextEditModel() = default;

		// 返回模型拥有的正式文本。
		const std::u16string& text() const noexcept { return text_; }
		// 返回当前 caret 的 UTF-16 code unit 偏移。
		std::size_t caret() const noexcept { return caret_; }
		// 返回选区锚点的 UTF-16 code unit 偏移。
		std::size_t selectionAnchor() const noexcept { return selectionAnchor_; }
		// 返回当前最大 Unicode scalar 数量。
		std::size_t maxScalarCount() const noexcept { return maxScalarCount_; }
		// 统计正式文本包含的 Unicode scalar 数量。
		std::size_t scalarCount() const noexcept;

		// 返回按从小到大排序的半开选区。
		TextSelectionRange selection() const noexcept;
		// 查询 caret 与锚点是否形成非空选区。
		bool hasSelection() const noexcept { return caret_ != selectionAnchor_; }
		// 查询 caret 是否位于文本开头。
		bool caretAtStart() const noexcept { return caret_ == 0; }
		// 查询 caret 是否位于文本末尾。
		bool caretAtEnd() const noexcept { return caret_ == text_.size(); }

		// 只接受结构完整的 UTF-16。超过 scalar 上限的尾部会在完整字符边界截断；
		// 赋值成功后，caret 与选区锚点都折叠到实际接收文本的末尾。
		TextEditResult assign(std::u16string text);

		// 替换当前选区；没有选区时在 caret 处插入。replacement 会按剩余 scalar
		// 容量截断，但绝不会为容纳新文本而丢弃未选中的既有内容。
		TextEditResult replaceSelection(std::u16string_view replacement);

		// 删除 caret 前一个完整 Unicode scalar，或删除当前选区。
		TextEditResult eraseBackward();
		// 删除 caret 后一个完整 Unicode scalar，或删除当前选区。
		TextEditResult eraseForward();

		// 向左移动一个 scalar；extendSelection 决定是否保留锚点扩展选区。
		bool moveLeft(bool extendSelection) noexcept;
		// 向右移动一个 scalar；extendSelection 决定是否保留锚点扩展选区。
		bool moveRight(bool extendSelection) noexcept;
		// 移到行首，并按 extendSelection 决定扩展或折叠选区。
		bool moveHome(bool extendSelection) noexcept;
		// 移到行尾，并按 extendSelection 决定扩展或折叠选区。
		bool moveEnd(bool extendSelection) noexcept;
		// 选中全部正式文本。
		bool selectAll() noexcept;

		// 设置 Pointer 命中换算后的锚点和 caret。越界位置或 surrogate pair
		// 内部偏移会被拒绝，原选区保持不变。
		bool setSelection(std::size_t anchor, std::size_t caret) noexcept;

		// 更新最大 scalar 数量并按完整字符边界收缩文本。0 按公开 TextBox 约定
		// 视为无效请求，不替换当前限制。
		TextEditResult setMaxScalarCount(std::size_t count);

	private:
		// 以下辅助函数只处理 UTF-16 边界和计数，不修改模型状态。
		static bool isScalarBoundary(std::u16string_view text,
			std::size_t offset) noexcept;
		static std::size_t previousScalarBoundary(std::u16string_view text,
			std::size_t offset) noexcept;
		static std::size_t nextScalarBoundary(std::u16string_view text,
			std::size_t offset) noexcept;
		static std::size_t countScalars(std::u16string_view text) noexcept;
		static std::size_t prefixForScalarCount(std::u16string_view text,
			std::size_t count) noexcept;
		static std::size_t countScalarsInRange(std::u16string_view text,
			std::size_t start, std::size_t end) noexcept;

		TextEditResult replaceRange(std::size_t start, std::size_t end,
			std::u16string_view replacement);
		bool moveTo(std::size_t offset, bool extendSelection) noexcept;

		std::u16string text_;
		std::size_t caret_ = 0;
		std::size_t selectionAnchor_ = 0;
		std::size_t maxScalarCount_ = DefaultMaxScalarCount;
	};
}
