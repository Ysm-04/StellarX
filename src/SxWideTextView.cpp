#include "internal/SxWideTextViewInternal.h"

#include "internal/SxUnicodeInternal.h"

#include <algorithm>
#include <climits>
#include <type_traits>
#include <utility>

namespace
{
	class SavedDc final
	{
	public:
		explicit SavedDc(HDC dc) noexcept : dc_(dc), saved_(dc ? SaveDC(dc) : 0) {}
		~SavedDc() { if (saved_ != 0) RestoreDC(dc_, saved_); }
		SavedDc(const SavedDc&) = delete;
		SavedDc& operator=(const SavedDc&) = delete;
		bool valid() const noexcept { return saved_ != 0; }

	private:
		HDC dc_ = nullptr;
		int saved_ = 0;
	};

	class OwnedBrush final
	{
	public:
		explicit OwnedBrush(COLORREF color) noexcept : brush_(CreateSolidBrush(color)) {}
		~OwnedBrush() { if (brush_) DeleteObject(brush_); }
		OwnedBrush(const OwnedBrush&) = delete;
		OwnedBrush& operator=(const OwnedBrush&) = delete;
		HBRUSH get() const noexcept { return brush_; }

	private:
		HBRUSH brush_ = nullptr;
	};

	bool isScalarBoundary(std::u16string_view text, std::size_t offset) noexcept
	{
		if (offset > text.size())
			return false;
		if (offset == 0 || offset == text.size())
			return true;
		return !(StellarX::Internal::isHighUtf16Surrogate(text[offset - 1]) &&
			StellarX::Internal::isLowUtf16Surrogate(text[offset]));
	}

	std::size_t nextScalarBoundary(
		std::u16string_view text, std::size_t offset) noexcept
	{
		if (offset >= text.size())
			return text.size();
		if (StellarX::Internal::isHighUtf16Surrogate(text[offset]) &&
			offset + 1 < text.size() &&
			StellarX::Internal::isLowUtf16Surrogate(text[offset + 1]))
		{
			return offset + 2;
		}
		return offset + 1;
	}

	bool nonEmptyRect(const RECT& rect) noexcept
	{
		return rect.right > rect.left && rect.bottom > rect.top;
	}

	RECT clippedRect(const RECT& rect, const RECT& clip) noexcept
	{
		RECT result{
			(std::max)(rect.left, clip.left),
			(std::max)(rect.top, clip.top),
			(std::min)(rect.right, clip.right),
			(std::min)(rect.bottom, clip.bottom)
		};
		if (!nonEmptyRect(result))
			return RECT{};
		return result;
	}

	const StellarX::Internal::WideTextBoundaryPlacement* findBoundary(
		const std::vector<StellarX::Internal::WideTextBoundaryPlacement>& boundaries,
		std::size_t offset) noexcept
	{
		const auto found = std::lower_bound(boundaries.begin(), boundaries.end(), offset,
			[](const StellarX::Internal::WideTextBoundaryPlacement& placement,
				std::size_t value)
			{
				return placement.sourceOffset < value;
			});
		if (found == boundaries.end() || found->sourceOffset != offset)
			return nullptr;
		return &*found;
	}
}

namespace StellarX::Internal
{
	// 测量和绘制先写入候选布局；只有整次绘制成功才提交，因此最终交换必须
	// 保证不抛异常，不能让屏幕内容与命中/IME 几何分属两代状态。
	static_assert(std::is_nothrow_move_assignable_v<WideTextViewLayout>,
		"WideTextView requires a no-throw layout commit after drawing succeeds.");

	bool WideTextViewLayout::prefixWidthFor(
		std::size_t sourceOffset, int& width) const noexcept
	{
		const auto* boundary = findBoundary(boundaries, sourceOffset);
		if (!boundary)
			return false;
		width = boundary->prefixWidth;
		return true;
	}

	std::size_t WideTextViewLayout::nearestSourceOffset(int clientX) const noexcept
	{
		if (boundaries.empty())
			return 0;
		const int contentX = clientX - contentRect.left + horizontalScroll;
		if (contentX <= boundaries.front().prefixWidth)
			return boundaries.front().sourceOffset;

		// 以相邻字形边界的中点决定 caret 落点，拖选时向左、向右都保持一致手感。
		for (std::size_t index = 1; index < boundaries.size(); ++index)
		{
			const int left = boundaries[index - 1].prefixWidth;
			const int right = boundaries[index].prefixWidth;
			const int midpoint = left + (right - left) / 2;
			if (contentX < midpoint)
				return boundaries[index - 1].sourceOffset;
			if (contentX < right)
				return boundaries[index].sourceOffset;
		}
		return boundaries.back().sourceOffset;
	}

	RECT textLineRect(const WideTextViewLayout& layout) noexcept
	{
		return RECT{ layout.contentRect.left, layout.textTop,
			layout.contentRect.right, layout.textTop + layout.lineHeight };
	}

	WideTextViewPaintResult WideTextView::paint(
		HDC destination, const WideTextViewPaintRequest& request)
	{
		WideTextViewPaintResult result;
		if (!destination || !nonEmptyRect(request.contentRect) ||
			!isValidUtf16(request.text) ||
			!isScalarBoundary(request.text, request.selectionAnchor) ||
			!isScalarBoundary(request.text, request.caret) ||
			(request.compositionEligible &&
				(!isScalarBoundary(request.text, request.compositionStart) ||
				 !isScalarBoundary(request.text, request.compositionEnd) ||
				 request.compositionStart > request.compositionEnd)) ||
			request.text.size() > static_cast<std::size_t>(INT_MAX))
		{
			return result;
		}

		// sourceToDisplay 保留正式文本 code unit 边界到显示串的映射。密码模式
		// 每个 scalar 只显示一个圆点，但 caret 和选区仍使用原文本偏移。
		std::u16string displayText;
		std::vector<std::pair<std::size_t, std::size_t>> sourceToDisplay;
		displayText.reserve(request.text.size());
		sourceToDisplay.reserve(request.text.size() + 1);
		sourceToDisplay.push_back({ 0, 0 });
		std::size_t sourceOffset = 0;
		while (sourceOffset < request.text.size())
		{
			const std::size_t nextSource = nextScalarBoundary(request.text, sourceOffset);
			if (request.password)
				displayText.push_back(u'\u2022');
			else
				displayText.append(request.text.data() + sourceOffset,
					nextSource - sourceOffset);
			sourceOffset = nextSource;
			sourceToDisplay.push_back({ sourceOffset, displayText.size() });
		}

		std::wstring displayWide;
		if (!utf16ToWindowsWide(displayText, displayWide) ||
			displayWide.size() > static_cast<std::size_t>(INT_MAX))
		{
			return result;
		}

		SavedDc saved(destination);
		if (!saved.valid())
		{
			result.status = WideTextViewPaintStatus::MeasurementFailed;
			return result;
		}
		if (request.font)
		{
			const HGDIOBJ selected = SelectObject(destination, request.font);
			if (!selected || selected == HGDI_ERROR)
			{
				result.status = WideTextViewPaintStatus::MeasurementFailed;
				return result;
			}
		}

		TEXTMETRICW metrics{};
		if (!GetTextMetricsW(destination, &metrics))
		{
			result.status = WideTextViewPaintStatus::MeasurementFailed;
			return result;
		}

		WideTextViewLayout candidate;
		candidate.valid = true;
		candidate.contentRect = request.contentRect;
		candidate.lineHeight = (std::max)(1L, metrics.tmHeight);
		candidate.textTop = request.contentRect.top +
			((request.contentRect.bottom - request.contentRect.top -
				candidate.lineHeight) / 2);
		candidate.sourceCodeUnitCount = request.text.size();
		candidate.displayScalarCount = sourceToDisplay.size() - 1;
		candidate.boundaries.reserve(sourceToDisplay.size());

		for (const auto& mapping : sourceToDisplay)
		{
			SIZE extent{};
			const int displayUnits = static_cast<int>(mapping.second);
			if (displayUnits > 0 && !GetTextExtentPoint32W(destination,
				displayWide.data(), displayUnits, &extent))
			{
				result.status = WideTextViewPaintStatus::MeasurementFailed;
				return result;
			}
			candidate.boundaries.push_back({ mapping.first, extent.cx });
		}
		candidate.totalWidth = candidate.boundaries.back().prefixWidth;

		const auto* caretBoundary = findBoundary(candidate.boundaries, request.caret);
		const std::size_t selectionStart =
			(std::min)(request.selectionAnchor, request.caret);
		const std::size_t selectionEnd =
			(std::max)(request.selectionAnchor, request.caret);
		const auto* selectionStartBoundary = findBoundary(
			candidate.boundaries, selectionStart);
		const auto* selectionEndBoundary = findBoundary(
			candidate.boundaries, selectionEnd);
		const auto* compositionStartBoundary = request.compositionEligible ?
			findBoundary(candidate.boundaries, request.compositionStart) : nullptr;
		const auto* compositionEndBoundary = request.compositionEligible ?
			findBoundary(candidate.boundaries, request.compositionEnd) : nullptr;
		if (!caretBoundary || !selectionStartBoundary || !selectionEndBoundary)
			return result;
		if (request.compositionEligible &&
			(!compositionStartBoundary || !compositionEndBoundary))
		{
			return result;
		}

		const int contentWidth = request.contentRect.right - request.contentRect.left;
		constexpr int caretWidth = 2;
		const int visibleCaretRight = (std::max)(0, contentWidth - caretWidth);
		// 仅活动 caret 驱动水平滚动；失焦绘制不会擅自把长文本拉回另一端。
		int candidateScroll = request.caretEligible ?
			(std::max)(0, horizontalScroll_) : 0;
		if (request.caretEligible)
		{
			if (caretBoundary->prefixWidth < candidateScroll)
				candidateScroll = caretBoundary->prefixWidth;
			else if (caretBoundary->prefixWidth > candidateScroll + visibleCaretRight)
				candidateScroll = caretBoundary->prefixWidth - visibleCaretRight;
		}
		const int maximumScroll = (std::max)(0,
			candidate.totalWidth - visibleCaretRight);
		candidateScroll = (std::clamp)(candidateScroll, 0, maximumScroll);
		candidate.horizontalScroll = candidateScroll;

		const int caretLeft = request.contentRect.left +
			caretBoundary->prefixWidth - candidateScroll;
		candidate.caretRect = clippedRect(
			RECT{ caretLeft, candidate.textTop, caretLeft + caretWidth,
				candidate.textTop + candidate.lineHeight }, request.contentRect);
		const int selectionLeft = request.contentRect.left +
			selectionStartBoundary->prefixWidth - candidateScroll;
		const int selectionRight = request.contentRect.left +
			selectionEndBoundary->prefixWidth - candidateScroll;
		candidate.selectionRect = clippedRect(
			RECT{ selectionLeft, candidate.textTop, selectionRight,
				candidate.textTop + candidate.lineHeight }, request.contentRect);
		if (request.compositionEligible)
		{
			const int compositionLeft = request.contentRect.left +
				compositionStartBoundary->prefixWidth - candidateScroll;
			const int compositionRight = request.contentRect.left +
				compositionEndBoundary->prefixWidth - candidateScroll;
			const int underlineBottom = (std::min)(
				static_cast<int>(request.contentRect.bottom),
				candidate.textTop + candidate.lineHeight);
			candidate.compositionRect = clippedRect(
				RECT{ compositionLeft, (std::max)(candidate.textTop,
					underlineBottom - 1), compositionRight, underlineBottom },
				request.contentRect);
		}

		if (IntersectClipRect(destination, request.contentRect.left,
			request.contentRect.top, request.contentRect.right,
			request.contentRect.bottom) == ERROR)
		{
			result.status = WideTextViewPaintStatus::DrawFailed;
			return result;
		}
		if (SetBkMode(destination, TRANSPARENT) == 0 ||
			SetTextColor(destination, request.textColor) == CLR_INVALID)
		{
			result.status = WideTextViewPaintStatus::DrawFailed;
			return result;
		}

		if (request.selectionEligible && selectionStart != selectionEnd &&
			nonEmptyRect(candidate.selectionRect))
		{
			OwnedBrush selectionBrush(request.selectionColor);
			if (!selectionBrush.get() || !FillRect(destination,
				&candidate.selectionRect, selectionBrush.get()))
			{
				result.status = WideTextViewPaintStatus::DrawFailed;
				return result;
			}
			result.selectionPainted = true;
		}

		if (!displayWide.empty())
		{
			const int textX = request.contentRect.left - candidateScroll;
			if (!TextOutW(destination, textX, candidate.textTop,
				displayWide.data(), static_cast<int>(displayWide.size())))
			{
				result.status = WideTextViewPaintStatus::DrawFailed;
				return result;
			}
			result.textPainted = true;
		}

		if (request.compositionEligible &&
			request.compositionStart != request.compositionEnd &&
			nonEmptyRect(candidate.compositionRect))
		{
			OwnedBrush compositionBrush(request.compositionUnderlineColor);
			if (!compositionBrush.get() || !FillRect(destination,
				&candidate.compositionRect, compositionBrush.get()))
			{
				result.status = WideTextViewPaintStatus::DrawFailed;
				return result;
			}
			result.compositionUnderlinePainted = true;
		}

		if (request.caretEligible && caretVisible() &&
			nonEmptyRect(candidate.caretRect))
		{
			OwnedBrush caretBrush(request.caretColor);
			if (!caretBrush.get() || !FillRect(destination,
				&candidate.caretRect, caretBrush.get()))
			{
				result.status = WideTextViewPaintStatus::DrawFailed;
				return result;
			}
			result.caretPainted = true;
		}

		// 到这里像素与全部几何均已成功生成，再原子发布新一代布局。
		candidate.generation = nextGeneration_++;
		layout_ = std::move(candidate);
		horizontalScroll_ = candidateScroll;
		result.status = WideTextViewPaintStatus::Drawn;
		return result;
	}

	bool WideTextView::setCaretActive(bool active, std::uint64_t nowMs) noexcept
	{
		const bool oldVisible = caretVisible();
		if (active)
		{
			if (!caretActive_)
			{
				caretActive_ = true;
				caretVisible_ = true;
				caretEpochMs_ = nowMs;
			}
		}
		else
		{
			caretActive_ = false;
			caretVisible_ = false;
			caretEpochMs_ = nowMs;
		}
		return oldVisible != caretVisible();
	}

	bool WideTextView::resetCaretBlink(std::uint64_t nowMs) noexcept
	{
		const bool oldVisible = caretVisible();
		caretEpochMs_ = nowMs;
		if (caretActive_)
			caretVisible_ = true;
		return oldVisible != caretVisible();
	}

	bool WideTextView::advanceCaretBlink(std::uint64_t nowMs) noexcept
	{
		if (!caretActive_)
			return false;
		const bool oldVisible = caretVisible_;
		const std::uint64_t elapsed = nowMs >= caretEpochMs_ ?
			nowMs - caretEpochMs_ : 0;
		caretVisible_ = ((elapsed / CaretBlinkIntervalMs) % 2u) == 0u;
		return oldVisible != caretVisible_;
	}

	bool WideTextView::invalidateLayout() noexcept
	{
		if (!layout_.valid)
			return false;
		layout_.valid = false;
		return true;
	}
}
