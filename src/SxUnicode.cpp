#include "SxUnicodeInternal.h"

#include <Windows.h>

#include <limits>
#include <utility>

namespace StellarX::Internal
{
	static_assert(sizeof(wchar_t) == 2,
		"The Windows text adapter requires UTF-16 wchar_t.");

	bool isHighUtf16Surrogate(char16_t value) noexcept
	{
		return value >= 0xD800 && value <= 0xDBFF;
	}

	bool isLowUtf16Surrogate(char16_t value) noexcept
	{
		return value >= 0xDC00 && value <= 0xDFFF;
	}

	bool isValidUtf16(std::u16string_view text) noexcept
	{
		// 高代理项必须紧跟低代理项；孤立的任一半都不是有效 UTF-16。
		for (std::size_t index = 0; index < text.size(); ++index)
		{
			const char16_t current = text[index];
			if (isHighUtf16Surrogate(current))
			{
				if (index + 1 >= text.size() ||
					!isLowUtf16Surrogate(text[index + 1]))
				{
					return false;
				}
				++index;
			}
			else if (isLowUtf16Surrogate(current))
			{
				return false;
			}
		}
		return true;
	}

	bool appendUtf16Scalar(std::uint32_t scalar, std::u16string& destination)
	{
		if (scalar > 0x10FFFFu || (scalar >= 0xD800u && scalar <= 0xDFFFu))
			return false;

		// 先在栈上形成完整的 1～2 个 code unit，再统一追加；非法 scalar
		// 会在触碰目标字符串前返回。
		char16_t units[2]{};
		std::size_t count = 1;
		if (scalar <= 0xFFFFu)
		{
			units[0] = static_cast<char16_t>(scalar);
		}
		else
		{
			const std::uint32_t value = scalar - 0x10000u;
			units[0] = static_cast<char16_t>(0xD800u + (value >> 10));
			units[1] = static_cast<char16_t>(0xDC00u + (value & 0x3FFu));
			count = 2;
		}

		destination.append(units, count);
		return true;
	}

	bool windowsWideToUtf16(
		std::wstring_view source, std::u16string& destination)
	{
		// 转换结果验证通过后才提交，失败时不改写调用方原有内容。
		std::u16string converted;
		converted.reserve(source.size());
		for (const wchar_t unit : source)
			converted.push_back(static_cast<char16_t>(unit));

		if (!isValidUtf16(converted))
			return false;

		destination = std::move(converted);
		return true;
	}

	bool utf16ToWindowsWide(
		std::u16string_view source, std::wstring& destination)
	{
		if (!isValidUtf16(source))
			return false;

		std::wstring converted;
		converted.reserve(source.size());
		for (const char16_t unit : source)
			converted.push_back(static_cast<wchar_t>(unit));

		destination = std::move(converted);
		return true;
	}

	bool windowsAnsiToUtf16(
		std::string_view source, std::u16string& destination)
	{
		if (source.empty())
		{
			destination.clear();
			return true;
		}
		if (source.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
			return false;

		const int sourceLength = static_cast<int>(source.size());
		const int required = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
			source.data(), sourceLength, nullptr, 0);
		if (required <= 0)
			return false;

		std::wstring wide(static_cast<std::size_t>(required), L'\0');
		if (MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, source.data(),
			sourceLength, wide.data(), required) != required)
		{
			return false;
		}

		std::u16string converted;
		if (!windowsWideToUtf16(wide, converted))
			return false;
		destination = std::move(converted);
		return true;
	}

	bool utf16ToWindowsAnsi(std::u16string_view source,
		std::string& destination, bool allowDefaultCharacter)
	{
		std::wstring wide;
		if (!utf16ToWindowsWide(source, wide))
			return false;
		if (wide.empty())
		{
			destination.clear();
			return true;
		}
		if (wide.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
			return false;

		// 非 UTF 代码页可报告替代字符；调用方不允许有损转换时，探测到替代
		// 即视为失败，不能把已经变形的文本交给旧式 ANSI 接口。
		const UINT codePage = GetACP();
		const bool supportsDefaultTracking =
			codePage != CP_UTF7 && codePage != CP_UTF8;
		const DWORD flags = supportsDefaultTracking ? WC_NO_BEST_FIT_CHARS : 0;
		BOOL usedDefault = FALSE;
		BOOL* usedDefaultPointer = supportsDefaultTracking ? &usedDefault : nullptr;
		const int sourceLength = static_cast<int>(wide.size());
		const int required = WideCharToMultiByte(codePage, flags, wide.data(),
			sourceLength, nullptr, 0, nullptr, usedDefaultPointer);
		if (required <= 0 || (!allowDefaultCharacter && usedDefault != FALSE))
			return false;

		std::string converted(static_cast<std::size_t>(required), '\0');
		usedDefault = FALSE;
		if (WideCharToMultiByte(codePage, flags, wide.data(), sourceLength,
			converted.data(), required, nullptr, usedDefaultPointer) != required ||
			(!allowDefaultCharacter && usedDefault != FALSE))
		{
			return false;
		}

		destination = std::move(converted);
		return true;
	}
}
