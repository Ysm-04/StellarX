#pragma once

#include <cstddef>

// 窗口实现和测试共用的内部事件循环工具。
// 本文件不属于 StellarX 公共 API。
namespace StellarX::Internal
{
	// 限制单轮处理量，避免持续的消息流长期占用循环而导致重绘得不到执行。
	constexpr std::size_t MessageBatchLimit = 128;

	struct MessageBatchResult
	{
		// 本批次成功获取并交给处理器的消息数量。
		std::size_t processed = 0;
		// 处理器请求提前停止时为 true，例如处理 WM_CLOSE 后结束循环。
		bool stopped = false;

		// 空批次表示当前轮次空闲，可以进行短暂休眠。
		bool shouldSleep() const noexcept { return processed == 0; }
	};

	// 获取并处理最多 `limit` 条消息；处理器返回 false 时提前停止。
	// 没有可用消息时，获取器必须返回 false。每次获取到消息都复用同一个
	// message 对象，从而不要求本工具绑定某一种具体的消息队列类型。
	template <typename Message, typename Fetch, typename Handle>
	MessageBatchResult drainMessageBatch(Message& message, Fetch&& fetch, Handle&& handle,
		std::size_t limit = MessageBatchLimit)
	{
		MessageBatchResult result;
		while (result.processed < limit && fetch(message))
		{
			++result.processed;
			if (!handle(message))
			{
				result.stopped = true;
				break;
			}
		}
		return result;
	}
}
