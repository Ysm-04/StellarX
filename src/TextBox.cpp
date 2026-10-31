// TextBox 的文字编辑、组合显示、焦点与绘制实现。
#include "TextBox.h"
#include "SxLog.h"
#include "Window.h"
#include "internal/SxTextEditControllerInternal.h"
#include "internal/SxUnicodeInternal.h"
#include "internal/SxWideTextViewInternal.h"

#include <algorithm>
#include <atomic>
#include <utility>


struct TextBox::Impl
{
	struct CompositionViewState
	{
		std::uint64_t id = 0;
		std::u16string text;
		std::size_t caretOffset = 0;
		std::size_t replaceStart = 0;
		std::size_t replaceEnd = 0;

		explicit operator bool() const noexcept { return id != 0; }
		void clear() noexcept
		{
			id = 0;
			text.clear();
			caretOffset = 0;
			replaceStart = 0;
			replaceEnd = 0;
		}
	};

	StellarX::Internal::TextEditController editor;
	StellarX::Internal::WideTextView view;
	CompositionViewState composition;
	std::uint64_t pendingImeResultId = 0;
};

namespace
{
	bool SxTextBoxPointInside(const TextBox& box, int x, int y) noexcept
	{
		return x >= box.getX() && x <= box.getX() + box.getWidth() &&
			y >= box.getY() && y <= box.getY() + box.getHeight();
	}

	std::size_t SxNormalizeCompositionCaret(
		std::u16string_view text, std::size_t offset) noexcept
	{
		offset = (std::min)(offset, text.size());
		if (offset > 0 && offset < text.size() &&
			StellarX::Internal::isHighUtf16Surrogate(text[offset - 1]) &&
			StellarX::Internal::isLowUtf16Surrogate(text[offset]))
		{
			--offset;
		}
		return offset;
	}
}

TextBox::TextBox(int x, int y, int width, int height, std::string text,
	StellarX::TextBoxmode mode, StellarX::ControlShape shape)
	: Control(x, y, width, height), impl(std::make_unique<Impl>()),
	mode(mode), shape(shape)
{
	id = "TextBox";
	layoutCapability.allowStretchX = true;
	layoutCapability.allowStretchY = false;
	focusPolicy = StellarX::FocusPolicy::Programmatic |
		StellarX::FocusPolicy::Pointer | StellarX::FocusPolicy::Tab;
	impl->editor.setMode(mode);

	std::u16string decoded;
	if (StellarX::Internal::windowsAnsiToUtf16(text, decoded))
		impl->editor.modelForTesting().assign(std::move(decoded));
	else
		SX_LOGW("TextBox") << "constructor ACP decode rejected: id=" << id;
}

TextBox::~TextBox() = default;

void TextBox::draw()
{
	if (!dirty || !show)
		return;

	if (impl->view.invalidateLayout())
		notifyFocusSubtreeStateChanged();

	saveStyle();
	setfillcolor(textBoxBkClor);
	setlinecolor(textBoxBorderClor);
	StellarX::ControlText drawStyle = textStyle;
	if (drawStyle.nHeight > height) drawStyle.nHeight = height;
	if (drawStyle.nWidth > width) drawStyle.nWidth = width;
	settextstyle(drawStyle.nHeight, drawStyle.nWidth, drawStyle.faceName.c_str(),
		drawStyle.nEscapement, drawStyle.nOrientation, drawStyle.nWeight,
		drawStyle.bItalic, drawStyle.bUnderline, drawStyle.bStrikeOut);
	settextcolor(drawStyle.color);
	setbkmode(TRANSPARENT);

	if (saveBkX != x || saveBkY != y || !hasSnap || saveWidth != width ||
		saveHeight != height || !saveBkImage)
	{
		saveBackground(x, y, width, height);
	}
	restBackground();
	switch (shape)
	{
	case StellarX::ControlShape::RECTANGLE:
		fillrectangle(x, y, x + width, y + height);
		break;
	case StellarX::ControlShape::B_RECTANGLE:
		solidrectangle(x, y, x + width, y + height);
		break;
	case StellarX::ControlShape::ROUND_RECTANGLE:
		fillroundrect(x, y, x + width, y + height,
			rouRectangleSize.ROUND_RECTANGLEwidth,
			rouRectangleSize.ROUND_RECTANGLEheight);
		break;
	case StellarX::ControlShape::B_ROUND_RECTANGLE:
		solidroundrect(x, y, x + width, y + height,
			rouRectangleSize.ROUND_RECTANGLEwidth,
			rouRectangleSize.ROUND_RECTANGLEheight);
		break;
	default:
		break;
	}

	const RECT contentRect{ x + 10, y + 2,
		(std::max)(x + 10, x + width - 10),
		(std::max)(y + 2, y + height - 2) };
	HDC destination = GetImageHDC(GetWorkingImage());
	HFONT font = destination ? static_cast<HFONT>(
		GetCurrentObject(destination, OBJ_FONT)) : nullptr;
	StellarX::Internal::WideTextViewPaintRequest request;
	const auto& editModel = impl->editor.model();
	std::u16string composedText;
	std::u16string_view paintText = editModel.text();
	std::size_t paintSelectionAnchor = editModel.selectionAnchor();
	std::size_t paintCaret = editModel.caret();
	std::size_t compositionStart = 0;
	std::size_t compositionEnd = 0;
	if (impl->composition)
	{
		// 组合串只拼进本次显示文本，不提前写入编辑模型；候选取消后正式文本
		// 因而能够原样恢复。
		const std::size_t replaceStart = (std::min)(
			impl->composition.replaceStart, editModel.text().size());
		const std::size_t replaceEnd = (std::clamp)(
			impl->composition.replaceEnd, replaceStart, editModel.text().size());
		composedText.reserve(editModel.text().size() -
			(replaceEnd - replaceStart) + impl->composition.text.size());
		composedText.append(editModel.text().data(), replaceStart);
		composedText.append(impl->composition.text);
		composedText.append(editModel.text().data() + replaceEnd,
			editModel.text().size() - replaceEnd);
		paintText = composedText;
		paintSelectionAnchor = replaceStart + impl->composition.caretOffset;
		paintCaret = paintSelectionAnchor;
		compositionStart = replaceStart;
		compositionEnd = replaceStart + impl->composition.text.size();
	}
	request.text = paintText;
	request.selectionAnchor = paintSelectionAnchor;
	request.caret = paintCaret;
	request.contentRect = contentRect;
	request.font = font;
	request.textColor = drawStyle.color;
	request.password = mode == StellarX::TextBoxmode::PASSWORD_MODE;
	request.selectionEligible = logicalFocused && !impl->composition;
	request.caretEligible = logicalFocused;
	if (impl->composition)
	{
		request.compositionEligible = true;
		request.compositionStart = compositionStart;
		request.compositionEnd = compositionEnd;
		request.compositionUnderlineColor = drawStyle.color;
	}
	const auto paintResult = impl->view.paint(destination, request);
	if (!paintResult)
	{
		SX_LOGW("TextBox") << "wide paint failed: id=" << id
			<< " status=" << static_cast<int>(paintResult.status);
	}

	if (logicalFocused && width > 6 && height > 6)
	{
		setlinecolor(RGB(32, 96, 192));
		setlinestyle(PS_DOT, 1);
		rectangle(x + 3, y + 3, x + width - 3, y + height - 3);
	}
	restoreStyle();
	dirty = false;
	if (paintResult)
		notifyFocusSubtreeStateChanged();
}

bool TextBox::handleEvent(const ExMessage& msg)
{
	if (!show)
		return false;
	auto mutationScope = enterMutationScope();
	resetEventVisualChanged();
	const bool inside = SxTextBoxPointInside(*this, msg.x, msg.y);
	if (inside && msg.message == WM_LBUTTONDOWN)
	{
		requestFocus(StellarX::FocusReason::Pointer);
		return true;
	}
	return inside && msg.message == WM_LBUTTONUP;
}

StellarX::KeyRouteResult TextBox::handleKeyEvent(
	StellarX::RoutedKeyEvent& event)
{
	auto mutationScope = enterMutationScope();
	if (!show || !logicalFocused)
		return {};
	Window* host = getHostWindow();
	StellarX::Internal::WindowsTextClipboard clipboard(
		host ? host->getHwnd() : nullptr);
	const auto result = impl->editor.handleKey(event.key, clipboard);
	if (!result.handled())
		return {};
	if (result.changed())
	{
		impl->composition.clear();
		impl->pendingImeResultId = 0;
		const ULONGLONG now = GetTickCount64();
		impl->view.setCaretActive(true, now);
		impl->view.resetCaretBlink(now);
		invalidateEditingLayout(true);
	}
	return { false, true };
}

void TextBox::handleTextInputEvent(
	const StellarX::RoutedTextInputEvent& event)
{
	auto mutationScope = enterMutationScope();
	if (!show || !logicalFocused)
		return;
	if (event.source == StellarX::TextSource::ImeResult &&
		impl->pendingImeResultId != event.compositionId)
	{
		// 只接收刚完成 Commit 的同一组合结果，迟到或串到其它 TextBox 的结果丢弃。
		return;
	}
	const bool clearedComposition =
		event.source == StellarX::TextSource::ImeResult &&
		static_cast<bool>(impl->composition);
	if (clearedComposition)
		impl->composition.clear();
	if (event.source == StellarX::TextSource::ImeResult)
		impl->pendingImeResultId = 0;
	const auto result = impl->editor.handleTextInput(event.text);
	if (result.changed() || clearedComposition)
	{
		const ULONGLONG now = GetTickCount64();
		impl->view.setCaretActive(true, now);
		impl->view.resetCaretBlink(now);
		invalidateEditingLayout(true);
	}
}

void TextBox::handleCompositionEvent(
	const StellarX::RoutedCompositionEvent& event)
{
	auto mutationScope = enterMutationScope();
	if (!show || !logicalFocused || !supportsInlineComposition())
		return;

	auto beginComposition = [&]()
	{
		const auto& model = impl->editor.model();
		impl->composition.id = event.compositionId;
		impl->composition.text.clear();
		impl->composition.caretOffset = 0;
		impl->composition.replaceStart =
			(std::min)(model.selectionAnchor(), model.caret());
		impl->composition.replaceEnd =
			(std::max)(model.selectionAnchor(), model.caret());
		impl->pendingImeResultId = 0;
	};

	switch (event.stage)
	{
	case StellarX::CompositionStage::Start:
		beginComposition();
		break;
	case StellarX::CompositionStage::Update:
		if (!impl->composition || impl->composition.id != event.compositionId)
			beginComposition();
		impl->composition.text.assign(event.compositionText);
		impl->composition.caretOffset = SxNormalizeCompositionCaret(
			impl->composition.text, event.caretOffset);
		break;
	case StellarX::CompositionStage::Commit:
		if (impl->composition.id == event.compositionId)
		{
			// Commit 只授权随后到达的结果文本；实际内容仍由 TextInputEvent
			// 统一写入，避免组合事件与字符回声重复插入。
			impl->pendingImeResultId = event.compositionId;
			impl->composition.clear();
		}
		break;
	case StellarX::CompositionStage::Cancel:
	case StellarX::CompositionStage::End:
		if (impl->composition.id == event.compositionId)
			impl->composition.clear();
		if (impl->pendingImeResultId == event.compositionId)
			impl->pendingImeResultId = 0;
		break;
	default:
		return;
	}

	const ULONGLONG now = GetTickCount64();
	impl->view.setCaretActive(true, now);
	impl->view.resetCaretBlink(now);
	invalidateEditingLayout(true);
}

bool TextBox::handleTextSelectionGesture(
	const StellarX::RoutedTextSelectionGestureEvent& event)
{
	auto mutationScope = enterMutationScope();
	if (!show || !logicalFocused || !event.clientCoordinatesValid)
		return false;
	if (impl->composition)
		return false;
	const auto& layout = impl->view.layout();
	const auto& currentModel = impl->editor.model();
	// 仅选区变化会使已发布锚点失效，但测量数据可保留到托管重绘；只有正式文本
	// code unit 数量未变时才允许暂时复用。
	if (layout.boundaries.empty() ||
		layout.sourceCodeUnitCount != currentModel.text().size())
	{
		return false;
	}
	if (event.phase == StellarX::TextSelectionGesturePhase::Cancel)
		return true;
	if (event.phase == StellarX::TextSelectionGesturePhase::Begin &&
		!SxTextBoxPointInside(*this, event.clientX, event.clientY))
	{
		return false;
	}

	auto& mutableModel = impl->editor.modelForTesting();
	const std::size_t offset = layout.nearestSourceOffset(event.clientX);
	const std::size_t anchor =
		event.phase == StellarX::TextSelectionGesturePhase::Begin && !event.shift ?
			offset : mutableModel.selectionAnchor();
	if (!mutableModel.setSelection(anchor, offset))
		return false;
	const ULONGLONG now = GetTickCount64();
	impl->view.setCaretActive(true, now);
	impl->view.resetCaretBlink(now);
	invalidateEditingLayout(true);
	return true;
}

void TextBox::tick(ULONGLONG nowMs)
{
	if (!show || !logicalFocused)
		return;
	if (impl->view.advanceCaretBlink(nowMs))
	{
		dirty = true;
		markEventVisualChanged(true);
		if (hasSnap) requestRepaint(parent);
	}
}

void TextBox::onWindowResize()
{
	impl->view.invalidateLayout();
	notifyFocusSubtreeStateChanged();
	Control::onWindowResize();
}

void TextBox::setMode(StellarX::TextBoxmode value)
{
	if (mode == value)
		return;
	mode = value;
	impl->composition.clear();
	impl->pendingImeResultId = 0;
	impl->editor.setMode(value);
	invalidateEditingLayout(true);
}

void TextBox::setMaxCharLen(size_t len)
{
	if (len == 0)
		return;
	const auto result = impl->editor.modelForTesting().setMaxScalarCount(len);
	if (result == StellarX::Internal::TextEditResult::Changed)
	{
		impl->composition.clear();
		impl->pendingImeResultId = 0;
		invalidateEditingLayout(true);
	}
}

void TextBox::setTextBoxshape(StellarX::ControlShape value)
{
	switch (value)
	{
	case StellarX::ControlShape::RECTANGLE:
	case StellarX::ControlShape::B_RECTANGLE:
	case StellarX::ControlShape::ROUND_RECTANGLE:
	case StellarX::ControlShape::B_ROUND_RECTANGLE:
		shape = value;
		break;
	default:
		shape = StellarX::ControlShape::RECTANGLE;
		break;
	}
	dirty = true;
	if (hasSnap) requestRepaint(parent);
}

void TextBox::setTextBoxBorder(COLORREF color)
{
	textBoxBorderClor = color;
	dirty = true;
	if (hasSnap) requestRepaint(parent);
}

void TextBox::setTextBoxBk(COLORREF color)
{
	textBoxBkClor = color;
	dirty = true;
	if (hasSnap) requestRepaint(parent);
}

bool TextBox::setTextUtf16(std::u16string text)
{
	const auto result = impl->editor.modelForTesting().assign(std::move(text));
	if (result == StellarX::Internal::TextEditResult::Rejected)
		return false;
	const bool clearedComposition = static_cast<bool>(impl->composition);
	impl->composition.clear();
	impl->pendingImeResultId = 0;
	if (result == StellarX::Internal::TextEditResult::Changed ||
		clearedComposition)
		invalidateEditingLayout(true);
	return true;
}

const std::u16string& TextBox::getTextUtf16() const noexcept
{
	return impl->editor.model().text();
}

void TextBox::setText(std::string text)
{
	std::u16string decoded;
	if (!StellarX::Internal::windowsAnsiToUtf16(text, decoded))
	{
		SX_LOGW("TextBox") << "setText ACP decode rejected: id=" << id;
		return;
	}
	(void)setTextUtf16(std::move(decoded));
}

bool TextBox::tryGetText(std::string& output) const
{
	std::string converted;
	if (!StellarX::Internal::utf16ToWindowsAnsi(
		getTextUtf16(), converted, false))
	{
		return false;
	}
	output = std::move(converted);
	return true;
}

std::string TextBox::getText() const
{
	std::string converted;
	if (!StellarX::Internal::utf16ToWindowsAnsi(
		getTextUtf16(), converted, true))
	{
		SX_LOGW("TextBox") << "getText ACP conversion failed: id=" << id;
		return {};
	}
	static std::atomic<bool> replacementLogged{ false };
	std::string strict;
	if (!replacementLogged.load(std::memory_order_relaxed) &&
		!StellarX::Internal::utf16ToWindowsAnsi(getTextUtf16(), strict, false))
	{
		bool expected = false;
		if (replacementLogged.compare_exchange_strong(expected, true,
			std::memory_order_relaxed))
		{
			SX_LOGW("TextBox") << "getText used ACP replacement; use getTextUtf16";
		}
	}
	return converted;
}

bool TextBox::queryFocusTargetSnapshot(
	const StellarX::FocusTargetId& target,
	StellarX::FocusTargetSnapshot& snapshot) const
{
	if (!Control::queryFocusTargetSnapshot(target, snapshot))
		return false;
	const auto& layout = impl->view.layout();
	if (logicalFocused && layout.valid)
	{
		// 只有当前焦点且测量成功的布局才能发布 IME 锚点；失焦控件的旧几何
		// 不得继续影响候选窗定位。
		snapshot.imeCaretAnchor.target = target;
		snapshot.imeCaretAnchor.clientPoint =
			POINT{ layout.caretRect.left, layout.caretRect.bottom };
		snapshot.imeCaretAnchor.clientLineRect =
			StellarX::Internal::textLineRect(layout);
		snapshot.imeCaretAnchor.clientEditRect =
			RECT{ x, y, x + width, y + height };
		snapshot.imeCaretAnchor.layoutGeneration = layout.generation;
		snapshot.imeCaretAnchor.valid = true;
	}
	return true;
}

bool TextBox::acceptsTextInput() const
{
	return mode == StellarX::TextBoxmode::INPUT_MODE ||
		mode == StellarX::TextBoxmode::PASSWORD_MODE;
}

bool TextBox::supportsInlineComposition() const
{
	return acceptsTextInput();
}

void TextBox::onLogicalFocusChanged(bool focused, StellarX::FocusReason)
{
	if (!focused)
	{
		// 焦点离开即退休未提交组合及其结果许可，不能把旧候选状态带到下次聚焦。
		impl->composition.clear();
		impl->pendingImeResultId = 0;
	}
	if (impl->view.setCaretActive(focused, GetTickCount64()))
	{
		dirty = true;
		markEventVisualChanged(true);
		if (hasSnap) requestRepaint(parent);
	}
}

void TextBox::onKeyboardRoutingSuspended()
{
	const bool clearedComposition = static_cast<bool>(impl->composition);
	impl->composition.clear();
	impl->pendingImeResultId = 0;
	if (impl->view.setCaretActive(false, GetTickCount64()) ||
		clearedComposition)
	{
		dirty = true;
		markEventVisualChanged(true);
		if (hasSnap) requestRepaint(parent);
	}
}

void TextBox::onKeyboardRoutingResumed()
{
	if (!logicalFocused)
		return;
	if (impl->view.setCaretActive(true, GetTickCount64()))
	{
		dirty = true;
		markEventVisualChanged(true);
		if (hasSnap) requestRepaint(parent);
	}
}

void TextBox::invalidateEditingLayout(bool requestVisual)
{
	impl->view.invalidateLayout();
	dirty = true;
	notifyFocusSubtreeStateChanged();
	if (requestVisual && hasSnap)
		requestRepaint(parent);
}
