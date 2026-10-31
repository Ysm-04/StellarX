#include "internal/SxTextEditControllerInternal.h"

#include "internal/SxUnicodeInternal.h"

#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>

namespace
{
	class ClipboardCloseGuard final
	{
	public:
		ClipboardCloseGuard() = default;
		~ClipboardCloseGuard() { CloseClipboard(); }
		ClipboardCloseGuard(const ClipboardCloseGuard&) = delete;
		ClipboardCloseGuard& operator=(const ClipboardCloseGuard&) = delete;
	};

	class GlobalUnlockGuard final
	{
		HGLOBAL handle_ = nullptr;
	public:
		explicit GlobalUnlockGuard(HGLOBAL handle) noexcept : handle_(handle) {}
		~GlobalUnlockGuard() { if (handle_) GlobalUnlock(handle_); }
		GlobalUnlockGuard(const GlobalUnlockGuard&) = delete;
		GlobalUnlockGuard& operator=(const GlobalUnlockGuard&) = delete;
	};

	StellarX::Internal::TextEditCommandStatus statusFor(
		StellarX::Internal::TextEditResult result) noexcept
	{
		using StellarX::Internal::TextEditCommandStatus;
		using StellarX::Internal::TextEditResult;
		switch (result)
		{
		case TextEditResult::Changed:
			return TextEditCommandStatus::HandledChanged;
		case TextEditResult::Unchanged:
			return TextEditCommandStatus::HandledUnchanged;
		case TextEditResult::Rejected:
		default:
			return TextEditCommandStatus::Rejected;
		}
	}

	StellarX::Internal::TextEditCommandStatus statusFor(bool changed) noexcept
	{
		return changed ?
			StellarX::Internal::TextEditCommandStatus::HandledChanged :
			StellarX::Internal::TextEditCommandStatus::HandledUnchanged;
	}
}

namespace StellarX::Internal
{
	// 剪切先写剪贴板，成功后才用候选模型覆盖当前状态；最终提交必须不抛异常，
	// 否则外部文本与控件内容可能只完成其中一边。
	static_assert(std::is_nothrow_move_assignable_v<TextEditModel>,
		"Cut requires a no-throw model commit after the clipboard write succeeds.");

	bool normalizeTextInput(std::u16string_view source,
		std::u16string& destination)
	{
		if (!isValidUtf16(source))
			return false;

		// 单行输入忽略控制字符，但代理项对必须整体保留。
		std::u16string candidate;
		candidate.reserve(source.size());
		for (std::size_t index = 0; index < source.size(); ++index)
		{
			const char16_t current = source[index];
			if (current < 0x20 || current == 0x7F)
				continue;
			candidate.push_back(current);
			if (isHighUtf16Surrogate(current))
				candidate.push_back(source[++index]);
		}
		destination = std::move(candidate);
		return true;
	}

	bool normalizeSingleLinePaste(std::u16string_view source,
		std::u16string& destination)
	{
		if (!isValidUtf16(source))
			return false;

		// 粘贴时把换行和制表符折成空格，其余控制字符丢弃，避免单行控件
		// 保存不可见的多行结构。
		std::u16string candidate;
		candidate.reserve(source.size());
		for (std::size_t index = 0; index < source.size(); ++index)
		{
			const char16_t current = source[index];
			if (current == u'\r')
			{
				if (index + 1 < source.size() && source[index + 1] == u'\n')
					++index;
				candidate.push_back(u' ');
				continue;
			}
			if (current == u'\n' || current == u'\t')
			{
				candidate.push_back(u' ');
				continue;
			}
			if (current < 0x20 || current == 0x7F)
				continue;
			candidate.push_back(current);
			if (isHighUtf16Surrogate(current))
				candidate.push_back(source[++index]);
		}
		destination = std::move(candidate);
		return true;
	}

	bool WindowsTextClipboard::readUnicode(std::u16string& text)
	{
		if (!OpenClipboard(owner_))
			return false;
		ClipboardCloseGuard closeGuard;

		const HGLOBAL handle = static_cast<HGLOBAL>(GetClipboardData(CF_UNICODETEXT));
		if (!handle)
			return false;
		const SIZE_T byteCount = GlobalSize(handle);
		if (byteCount < sizeof(wchar_t) || byteCount % sizeof(wchar_t) != 0)
			return false;

		const auto* data = static_cast<const wchar_t*>(GlobalLock(handle));
		if (!data)
			return false;
		GlobalUnlockGuard unlockGuard(handle);

		const std::size_t capacity = byteCount / sizeof(wchar_t);
		std::size_t length = 0;
		while (length < capacity && data[length] != L'\0')
			++length;
		if (length == capacity)
			return false;

		// 先完整读取并校验，再替换输出；剪贴板内容损坏时调用方仍保留原值。
		std::u16string candidate;
		if (!windowsWideToUtf16(std::wstring_view(data, length), candidate))
			return false;
		text = std::move(candidate);
		return true;
	}

	bool WindowsTextClipboard::writeUnicode(std::u16string_view text)
	{
		if (text.find(u'\0') != std::u16string_view::npos)
			return false;

		std::wstring wide;
		if (!utf16ToWindowsWide(text, wide))
			return false;
		if (wide.size() > ((std::numeric_limits<SIZE_T>::max)() /
			sizeof(wchar_t)) - 1)
			return false;

		const SIZE_T byteCount = (wide.size() + 1) * sizeof(wchar_t);
		HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, byteCount);
		if (!handle)
			return false;
		void* data = GlobalLock(handle);
		if (!data)
		{
			GlobalFree(handle);
			return false;
		}
		std::memcpy(data, wide.c_str(), byteCount);
		GlobalUnlock(handle);

		if (!OpenClipboard(owner_))
		{
			GlobalFree(handle);
			return false;
		}
		ClipboardCloseGuard closeGuard;
		if (!EmptyClipboard())
		{
			GlobalFree(handle);
			return false;
		}
		if (!SetClipboardData(CF_UNICODETEXT, handle))
		{
			GlobalFree(handle);
			return false;
		}
		// 只有 SetClipboardData 成功后，内存所有权才转移给系统。
		return true;
	}

	bool TextEditController::allowsInput() const noexcept
	{
		return mode_ == TextBoxmode::INPUT_MODE ||
			mode_ == TextBoxmode::PASSWORD_MODE;
	}

	bool TextEditController::allowsCopy() const noexcept
	{
		return mode_ == TextBoxmode::INPUT_MODE ||
			mode_ == TextBoxmode::READONLY_MODE;
	}

	bool TextEditController::allowsCut() const noexcept
	{
		return mode_ == TextBoxmode::INPUT_MODE;
	}

	std::u16string TextEditController::selectedText() const
	{
		const TextSelectionRange range = model_.selection();
		return std::u16string(model_.text().data() + range.start,
			range.end - range.start);
	}

	TextEditCommandResult TextEditController::handleTextInput(
		std::u16string_view text)
	{
		TextEditCommandResult result;
		if (!isValidUtf16(text))
		{
			result.status = TextEditCommandStatus::Rejected;
			return result;
		}
		if (!allowsInput())
		{
			result.status = TextEditCommandStatus::HandledUnchanged;
			return result;
		}

		std::u16string normalized;
		if (!normalizeTextInput(text, normalized))
		{
			result.status = TextEditCommandStatus::Rejected;
			return result;
		}
		if (normalized.empty())
		{
			result.status = TextEditCommandStatus::HandledUnchanged;
			return result;
		}
		result.status = statusFor(model_.replaceSelection(normalized));
		return result;
	}

	TextEditCommandResult TextEditController::handleKey(
		const KeyEvent& key, TextClipboard& clipboard)
	{
		TextEditCommandResult result;
		if (key.action != KeyAction::Down || key.systemKey ||
			key.altContext || key.modifiers.alt())
			return result;

		if (key.modifiers.control())
		{
			switch (key.virtualKey)
			{
			case 'A':
				result.status = statusFor(model_.selectAll());
				return result;
			case 'C':
				result.status = TextEditCommandStatus::HandledUnchanged;
				if (!allowsCopy() || !model_.hasSelection())
					return result;
				result.clipboardOperation = TextClipboardOperation::Write;
				result.clipboardSucceeded = clipboard.writeUnicode(selectedText());
				return result;
			case 'X':
				result.status = TextEditCommandStatus::HandledUnchanged;
				if (!allowsCut() || !model_.hasSelection())
					return result;
				{
					// 删除先作用于副本。只有剪贴板接管所选文本后才提交副本，
					// 写入失败不会让用户丢失原内容。
					const std::u16string selected = selectedText();
					TextEditModel candidate = model_;
					const TextEditResult edit = candidate.replaceSelection(u"");
					if (edit == TextEditResult::Rejected)
					{
						result.status = TextEditCommandStatus::Rejected;
						return result;
					}
					result.clipboardOperation = TextClipboardOperation::Write;
					result.clipboardSucceeded = clipboard.writeUnicode(selected);
					if (result.clipboardSucceeded)
					{
						model_ = std::move(candidate);
						result.status = TextEditCommandStatus::HandledChanged;
					}
					return result;
				}
			case 'V':
				result.status = TextEditCommandStatus::HandledUnchanged;
				if (!allowsInput())
					return result;
				{
					std::u16string clipboardText;
					result.clipboardOperation = TextClipboardOperation::Read;
					result.clipboardSucceeded = clipboard.readUnicode(clipboardText);
					if (!result.clipboardSucceeded)
						return result;
					std::u16string normalized;
					if (!normalizeSingleLinePaste(clipboardText, normalized))
					{
						result.status = TextEditCommandStatus::Rejected;
						return result;
					}
					if (normalized.empty())
						return result;
					result.status = statusFor(model_.replaceSelection(normalized));
					return result;
				}
			default:
				break;
			}
		}

		switch (key.virtualKey)
		{
		case VK_LEFT:
			result.status = statusFor(model_.moveLeft(key.modifiers.shift()));
			break;
		case VK_RIGHT:
			result.status = statusFor(model_.moveRight(key.modifiers.shift()));
			break;
		case VK_HOME:
			result.status = statusFor(model_.moveHome(key.modifiers.shift()));
			break;
		case VK_END:
			result.status = statusFor(model_.moveEnd(key.modifiers.shift()));
			break;
		case VK_BACK:
			result.status = !allowsInput() ?
				TextEditCommandStatus::HandledUnchanged :
				statusFor(model_.eraseBackward());
			break;
		case VK_DELETE:
			result.status = !allowsInput() ?
				TextEditCommandStatus::HandledUnchanged :
				statusFor(model_.eraseForward());
			break;
		default:
			break;
		}
		return result;
	}
}
