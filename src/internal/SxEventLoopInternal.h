#pragma once

#include <windows.h>

#include <cstddef>
#include <cstdint>

class Window;

// 窗口实现和测试共用的内部事件循环工具。
// 本文件不属于 StellarX 公共 API。
namespace StellarX::Internal
{
	// 键盘业务默认使用 Routed。定向测试可在事件循环首次启动前选择
	// LegacyCompatibility；一旦锁定，两条业务路径不能同时启用或相互切换。
	enum class KeyboardPipelineMode : std::uint8_t
	{
		LegacyCompatibility, ///< 由 EasyX EX_KEY 兼容路径交付键盘业务。
		Routed               ///< 由 NativeInputQueue 和 KeyboardRouter 交付业务。
	};

	// Pointer 业务默认使用 Routed。LegacyCompatibility 只供启动前的兼容性测试，
	// 事件循环锁定后不能再切换。
	enum class PointerPipelineMode : std::uint8_t
	{
		LegacyCompatibility, ///< 由 EasyX EX_MOUSE 兼容路径交付 Pointer 业务。
		Routed               ///< 由 NativeInputQueue 交付 Pointer 业务。
	};

	// 仅供测试：在首次事件循环锁定前选择键盘业务管线。
	bool configureKeyboardPipelineForTesting(
		::Window& window, KeyboardPipelineMode mode) noexcept;
	// 查询窗口当前选择的键盘业务管线。
	KeyboardPipelineMode keyboardPipelineMode(
		const ::Window& window) noexcept;
	// 锁定键盘业务管线，防止运行期间发生双投递或临时换轨。
	void lockKeyboardPipeline(::Window& window) noexcept;
	// 仅供测试：在首次事件循环锁定前选择 Pointer 业务管线。
	bool configurePointerPipelineForTesting(
		::Window& window, PointerPipelineMode mode) noexcept;
	// 查询窗口当前选择的 Pointer 业务管线。
	PointerPipelineMode pointerPipelineMode(
		const ::Window& window) noexcept;
	// 锁定 Pointer 业务管线，防止运行期间发生双投递或临时换轨。
	void lockPointerPipeline(::Window& window) noexcept;
	// 判断指定 EasyX 消息是否应由 LegacyCompatibility 交付业务；Routed 下只排空。
	bool shouldDeliverLegacyEasyXMessage(
		const ::Window& window, UINT message) noexcept;
	// WM_GETDLGCODE 是同步查询。宿主线程发布不含指针的原子能力快照，WndProc
	// 只针对本次具体按键查询过滤并返回需要追加的 DLGC_* 位。
	LRESULT dialogCodeAdditionsForQuery(const ::Window& window,
		WPARAM wParam, LPARAM lParam) noexcept;
	// 兼容性测试入口；内部仍走完整的 WM_GETDLGCODE 查询逻辑。
	bool shouldAddTabDialogCode(const ::Window& window,
		WPARAM wParam, LPARAM lParam) noexcept;

	// 判断消息是否属于必须从 EasyX 兼容队列持续排空的键盘消息集合。
	inline bool isEasyXKeyboardMessage(UINT message) noexcept
	{
		constexpr UINT WmUniChar = 0x0109u;
		switch (message)
		{
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_CHAR:
		case WM_SYSCHAR:
		case WM_DEADCHAR:
		case WM_SYSDEADCHAR:
		case WM_IME_CHAR:
		case WmUniChar:
			return true;
		default:
			return false;
		}
	}

	// 判断消息是否属于必须从 EasyX 兼容队列持续排空的鼠标消息集合。
	inline bool isEasyXPointerMessage(UINT message) noexcept
	{
		switch (message)
		{
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
			return true;
		default:
			return false;
		}
	}

	// 限制单轮处理量，避免持续的消息流长期占用循环而导致重绘得不到执行。
	constexpr std::size_t MessageBatchLimit = 128;

	// 单轮消息排空的结果，用于决定是否停止循环或短暂休眠。
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
