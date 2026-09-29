#pragma once

#include <cstddef>
#include <string>

// 文本控件使用的内部 MBCS 辅助工具。
// 按项目约定的 GBK 风格字节规则处理，并且不作为公共 API 暴露。
namespace StellarX::Internal
{
	// 返回从 `index` 开始的逻辑字符所占的字节数。
	// ASCII 字符占一个字节，合法的首字节和尾字节组合占两个字节。
	// 非法或不完整的输入按一个字节处理，确保调用方始终能够向前推进。
	inline std::size_t mbcCharLength(const std::string& text, std::size_t index) noexcept
	{
		if (index >= text.size())
			return 0;

		const auto first = static_cast<unsigned char>(text[index]);
		if (first <= 0x7F)
			return 1;

		if (first >= 0x81 && first <= 0xFE && index + 1 < text.size())
		{
			const auto second = static_cast<unsigned char>(text[index + 1]);
			if (second >= 0x40 && second <= 0xFE && second != 0x7F)
				return 2;
		}

		// 非法或不完整的输入消耗一个字节，确保调用方始终能够向前推进。
		return 1;
	}

	// 为每个逻辑 MBCS 字符生成一个掩码字符，同时保留原始字节数语义，
	// 用原文本的字节长度为结果预留容量。
	inline std::string makePasswordMask(const std::string& text)
	{
		std::string result;
		result.reserve(text.size());
		for (std::size_t index = 0; index < text.size();)
		{
			result.push_back('*');
			index += mbcCharLength(text, index);
		}
		return result;
	}
}
