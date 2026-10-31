#pragma once

#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace StellarX::Internal
{
	// 一个合法 UTF-16 边界及其对应的已测量前缀宽度。
	struct WideTextBoundaryPlacement
	{
		// 正式文本中的 UTF-16 code unit 偏移。
		std::size_t sourceOffset = 0;
		// 从显示行起点到该边界的像素宽度。
		int prefixWidth = 0;
	};

	// 最近一次成功绘制得到的单行文字布局；所有矩形均为目标 HDC 的客户区坐标。
	struct WideTextViewLayout
	{
		// valid 为 false 时，其余几何只可用于诊断，不得参与命中或 IME 定位。
		bool valid = false;
		RECT contentRect{};
		RECT selectionRect{};
		RECT compositionRect{};
		RECT caretRect{};
		int textTop = 0;
		int lineHeight = 0;
		int totalWidth = 0;
		int horizontalScroll = 0;
		std::size_t sourceCodeUnitCount = 0;
		std::size_t displayScalarCount = 0;
		std::uint64_t generation = 0;
		std::vector<WideTextBoundaryPlacement> boundaries;

		// 查询指定 UTF-16 边界的已测量前缀宽度；偏移不是合法边界时返回 false。
		bool prefixWidthFor(std::size_t sourceOffset, int& width) const noexcept;
		// 把客户区 X 坐标映射到最近的合法 UTF-16 scalar 边界。
		std::size_t nearestSourceOffset(int clientX) const noexcept;
	};

	// 候选窗排除区应跟随实际测量的字形行，而不是整个 TextBox 内容矩形；
	// 使用更大的矩形会让部分输入法在单行编辑器下方额外避让。
	RECT textLineRect(const WideTextViewLayout& layout) noexcept;

	// 一次单行文字绘制的完整输入。所有 string_view、HFONT 和 HDC 都只借用到
	// paint() 返回为止，WideTextView 不会保存或接管它们。
	struct WideTextViewPaintRequest
	{
		// 正式 UTF-16 文本及其选区位置。
		std::u16string_view text;
		std::size_t selectionAnchor = 0;
		std::size_t caret = 0;
		RECT contentRect{};
		HFONT font = nullptr; // 非拥有；调用方保证本次 paint() 期间字体有效。
		// 正常文字、选区、caret 和组合下划线的绘制颜色。
		COLORREF textColor = RGB(0, 0, 0);
		COLORREF selectionColor = RGB(51, 153, 255);
		COLORREF caretColor = RGB(0, 0, 0);
		COLORREF compositionUnderlineColor = RGB(0, 0, 0);
		bool password = false; // 密码模式按 scalar 数量绘制掩码，不修改正式文本。
		// 选区仍保留在编辑模型中；该开关只允许失焦视图隐藏高亮，重新聚焦后
		// 可以再次显示原选区。
		bool selectionEligible = true;
		// 是否绘制 caret；闪烁相位仍由 WideTextView 自身管理。
		bool caretEligible = false;
		// text 内可选的 IME 临时组合范围，只参与视图绘制，不代表已提交模型内容。
		bool compositionEligible = false;
		std::size_t compositionStart = 0;
		std::size_t compositionEnd = 0;
	};

	// paint() 的整体结果，区分调用参数、测量和真正绘制失败。
	enum class WideTextViewPaintStatus
	{
		Drawn,             ///< 已完成绘制并发布新布局。
		InvalidArgument,   ///< HDC、字体、范围或 UTF-16 输入无效。
		MeasurementFailed, ///< GDI 字形测量失败，旧布局继续保留。
		DrawFailed         ///< 测量成功但绘制阶段失败。
	};

	// 本次绘制实际完成了哪些层，便于测试局部失败与可见性门控。
	struct WideTextViewPaintResult
	{
		WideTextViewPaintStatus status = WideTextViewPaintStatus::InvalidArgument;
		bool selectionPainted = false;
		bool textPainted = false;
		bool caretPainted = false;
		bool compositionUnderlinePainted = false;

		// 仅完整 Drawn 状态可按成功结果使用。
		explicit operator bool() const noexcept
		{
			return status == WideTextViewPaintStatus::Drawn;
		}
	};

	// 框架 TextBox 与定向测试共用的 UTF-16 单行视图。调用方提供目标 HDC 和
	// 借用 HFONT；视图只拥有布局、闪烁与滚动状态，绝不自行获取 EasyX 表面。
	class WideTextView final
	{
	public:
		// Windows 常用的 caret 闪烁间隔；测试据此用确定性时钟推进。
		static constexpr std::uint64_t CaretBlinkIntervalMs = 530;

		// 返回最近一次成功 paint() 的布局快照。
		const WideTextViewLayout& layout() const noexcept { return layout_; }
		// 返回当前水平滚动像素量。
		int horizontalScroll() const noexcept { return horizontalScroll_; }
		// 只有 caret 处于活动且当前闪烁相位可见时才返回 true。
		bool caretVisible() const noexcept { return caretActive_ && caretVisible_; }

		// 在 destination 上绘制并更新布局、滚动和命中边界；失败时不发布半成品布局。
		WideTextViewPaintResult paint(
			HDC destination, const WideTextViewPaintRequest& request);

		// 新激活的 caret 从可见相位开始；停用只隐藏 caret，不丢弃上次成功布局
		// 或水平滚动位置。
		bool setCaretActive(bool active, std::uint64_t nowMs) noexcept;
		// 以 nowMs 为新纪元重置闪烁，并让活动 caret 立即可见。
		bool resetCaretBlink(std::uint64_t nowMs) noexcept;
		// 推进到 nowMs 对应的闪烁相位；返回可见状态是否改变。
		bool advanceCaretBlink(std::uint64_t nowMs) noexcept;
		// 只使上次绘制派生的几何失效；滚动与闪烁状态仍由视图保留，并在下次
		// paint() 时重新校准。
		bool invalidateLayout() noexcept;

	private:
		WideTextViewLayout layout_;
		int horizontalScroll_ = 0;
		std::uint64_t nextGeneration_ = 1;
		std::uint64_t caretEpochMs_ = 0;
		bool caretActive_ = false;
		bool caretVisible_ = false;
	};
}
