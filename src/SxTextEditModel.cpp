#include "SxTextEditModelInternal.h"

#include "SxUnicodeInternal.h"

#include <algorithm>
#include <type_traits>
#include <utility>

namespace StellarX::Internal
{
	// 所有可能分配内存的工作都先在候选字符串上完成；最终提交依赖不抛异常的
	// 移动赋值，避免模型只更新了一半。
	static_assert(std::is_nothrow_move_assignable_v<std::u16string>,
		"TextEditModel requires no-throw string commit after preparing a candidate.");

	std::size_t TextEditModel::scalarCount() const noexcept
	{
		return countScalars(text_);
	}

	TextSelectionRange TextEditModel::selection() const noexcept
	{
		return {
			(std::min)(caret_, selectionAnchor_),
			(std::max)(caret_, selectionAnchor_)
		};
	}

	TextEditResult TextEditModel::assign(std::u16string text)
	{
		if (!isValidUtf16(text))
			return TextEditResult::Rejected;

		// 容量以 Unicode scalar 计数，截断点必须落在完整代理项对之后。
		const std::size_t acceptedUnits = prefixForScalarCount(text, maxScalarCount_);
		if (acceptedUnits < text.size())
			text.resize(acceptedUnits);

		const std::size_t newCaret = text.size();
		if (text_ == text && caret_ == newCaret && selectionAnchor_ == newCaret)
			return TextEditResult::Unchanged;

		text_ = std::move(text);
		caret_ = newCaret;
		selectionAnchor_ = newCaret;
		return TextEditResult::Changed;
	}

	TextEditResult TextEditModel::replaceSelection(std::u16string_view replacement)
	{
		if (!isValidUtf16(replacement))
			return TextEditResult::Rejected;

		const TextSelectionRange selected = selection();
		return replaceRange(selected.start, selected.end, replacement);
	}

	TextEditResult TextEditModel::eraseBackward()
	{
		if (hasSelection())
			return replaceSelection(u"");
		if (caret_ == 0)
			return TextEditResult::Unchanged;

		const std::size_t eraseStart = previousScalarBoundary(text_, caret_);
		return replaceRange(eraseStart, caret_, u"");
	}

	TextEditResult TextEditModel::eraseForward()
	{
		if (hasSelection())
			return replaceSelection(u"");
		if (caret_ == text_.size())
			return TextEditResult::Unchanged;

		return replaceRange(caret_, nextScalarBoundary(text_, caret_), u"");
	}

	bool TextEditModel::moveLeft(bool extendSelection) noexcept
	{
		if (!extendSelection && hasSelection())
			return moveTo(selection().start, false);
		return moveTo(previousScalarBoundary(text_, caret_), extendSelection);
	}

	bool TextEditModel::moveRight(bool extendSelection) noexcept
	{
		if (!extendSelection && hasSelection())
			return moveTo(selection().end, false);
		return moveTo(nextScalarBoundary(text_, caret_), extendSelection);
	}

	bool TextEditModel::moveHome(bool extendSelection) noexcept
	{
		return moveTo(0, extendSelection);
	}

	bool TextEditModel::moveEnd(bool extendSelection) noexcept
	{
		return moveTo(text_.size(), extendSelection);
	}

	bool TextEditModel::selectAll() noexcept
	{
		if (selectionAnchor_ == 0 && caret_ == text_.size())
			return false;
		selectionAnchor_ = 0;
		caret_ = text_.size();
		return true;
	}

	bool TextEditModel::setSelection(std::size_t anchor, std::size_t caret) noexcept
	{
		if (!isScalarBoundary(text_, anchor) || !isScalarBoundary(text_, caret))
			return false;
		selectionAnchor_ = anchor;
		caret_ = caret;
		return true;
	}

	TextEditResult TextEditModel::setMaxScalarCount(std::size_t count)
	{
		if (count == 0)
			return TextEditResult::Rejected;
		if (count == maxScalarCount_)
			return TextEditResult::Unchanged;

		const std::size_t acceptedUnits = prefixForScalarCount(text_, count);
		if (acceptedUnits == text_.size())
		{
			maxScalarCount_ = count;
			return TextEditResult::Changed;
		}

		// 先准备完整候选值并夹紧两个端点；构造失败时原模型保持不变。
		std::u16string candidate(text_.data(), acceptedUnits);
		const std::size_t newCaret = (std::min)(caret_, acceptedUnits);
		const std::size_t newAnchor = (std::min)(selectionAnchor_, acceptedUnits);

		text_ = std::move(candidate);
		caret_ = newCaret;
		selectionAnchor_ = newAnchor;
		maxScalarCount_ = count;
		return TextEditResult::Changed;
	}

	bool TextEditModel::isScalarBoundary(
		std::u16string_view text, std::size_t offset) noexcept
	{
		// caret、选区、删除和容量截断都只接受 scalar 边界，不能拆开代理项对。
		if (offset > text.size())
			return false;
		if (offset == 0 || offset == text.size())
			return true;
		return !(isHighUtf16Surrogate(text[offset - 1]) &&
			isLowUtf16Surrogate(text[offset]));
	}

	std::size_t TextEditModel::previousScalarBoundary(
		std::u16string_view text, std::size_t offset) noexcept
	{
		if (offset == 0)
			return 0;
		if (offset >= 2 && isLowUtf16Surrogate(text[offset - 1]) &&
			isHighUtf16Surrogate(text[offset - 2]))
		{
			return offset - 2;
		}
		return offset - 1;
	}

	std::size_t TextEditModel::nextScalarBoundary(
		std::u16string_view text, std::size_t offset) noexcept
	{
		if (offset >= text.size())
			return text.size();
		if (isHighUtf16Surrogate(text[offset]) && offset + 1 < text.size() &&
			isLowUtf16Surrogate(text[offset + 1]))
		{
			return offset + 2;
		}
		return offset + 1;
	}

	std::size_t TextEditModel::countScalars(std::u16string_view text) noexcept
	{
		std::size_t count = 0;
		for (std::size_t offset = 0; offset < text.size();
			offset = nextScalarBoundary(text, offset))
		{
			++count;
		}
		return count;
	}

	std::size_t TextEditModel::prefixForScalarCount(
		std::u16string_view text, std::size_t count) noexcept
	{
		std::size_t offset = 0;
		while (offset < text.size() && count > 0)
		{
			offset = nextScalarBoundary(text, offset);
			--count;
		}
		return offset;
	}

	std::size_t TextEditModel::countScalarsInRange(
		std::u16string_view text, std::size_t start, std::size_t end) noexcept
	{
		std::size_t count = 0;
		for (std::size_t offset = start; offset < end;
			offset = nextScalarBoundary(text, offset))
		{
			++count;
		}
		return count;
	}

	TextEditResult TextEditModel::replaceRange(std::size_t start, std::size_t end,
		std::u16string_view replacement)
	{
		// 先扣除被替换区占用的 scalar，再按剩余容量接纳新文本。候选字符串
		// 构造完成后才一次性写回，分配失败不会留下半次编辑。
		const std::size_t selectedScalars = countScalarsInRange(text_, start, end);
		const std::size_t preservedScalars = scalarCount() - selectedScalars;
		const std::size_t availableScalars = maxScalarCount_ - preservedScalars;
		const std::size_t acceptedUnits = prefixForScalarCount(
			replacement, availableScalars);

		std::u16string candidate;
		candidate.reserve(text_.size() - (end - start) + acceptedUnits);
		if (start > 0)
			candidate.append(text_.data(), start);
		if (acceptedUnits > 0)
			candidate.append(replacement.data(), acceptedUnits);
		if (end < text_.size())
			candidate.append(text_.data() + end, text_.size() - end);

		const std::size_t newCaret = start + acceptedUnits;
		if (candidate == text_ && caret_ == newCaret && selectionAnchor_ == newCaret)
			return TextEditResult::Unchanged;

		text_ = std::move(candidate);
		caret_ = newCaret;
		selectionAnchor_ = newCaret;
		return TextEditResult::Changed;
	}

	bool TextEditModel::moveTo(std::size_t offset, bool extendSelection) noexcept
	{
		// 扩选只移动 caret；普通移动同时推进 anchor，使选区折叠到新位置。
		const std::size_t oldCaret = caret_;
		const std::size_t oldAnchor = selectionAnchor_;
		caret_ = offset;
		if (!extendSelection)
			selectionAnchor_ = offset;
		return caret_ != oldCaret || selectionAnchor_ != oldAnchor;
	}
}
