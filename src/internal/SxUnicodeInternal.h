#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace StellarX::Internal
{
	// 判断一个 UTF-16 code unit 是否为高代理项。
	bool isHighUtf16Surrogate(char16_t value) noexcept;
	// 判断一个 UTF-16 code unit 是否为低代理项。
	bool isLowUtf16Surrogate(char16_t value) noexcept;
	// 验证整段 UTF-16 是否由完整 BMP 字符或正确代理项对组成。
	bool isValidUtf16(std::u16string_view text) noexcept;

	// 把一个 Unicode scalar 以合法 UTF-16 追加到 destination。scalar 无效时
	// 返回 false，并保持 destination 完全不变。
	bool appendUtf16Scalar(std::uint32_t scalar, std::u16string& destination);

	// Windows 边界适配器。在当前支持的 Win32 ABI 中 wchar_t 承载 UTF-16，
	// 但它仍是独立 C++ 类型，因此这里显式复制并验证内容。
	bool windowsWideToUtf16(std::wstring_view source, std::u16string& destination);
	bool utf16ToWindowsWide(std::u16string_view source, std::wstring& destination);

	// Windows ACP 适配器只服务 TextBox 的 std::string 兼容边界。两者失败时
	// 都保持 destination 不变。严格输出拒绝最佳匹配和默认字符替换；宽松输出
	// 只允许系统默认字符（通常为 '?'），仍不接受最佳匹配映射。
	bool windowsAnsiToUtf16(std::string_view source, std::u16string& destination);
	bool utf16ToWindowsAnsi(std::u16string_view source, std::string& destination,
		bool allowDefaultCharacter);
}
