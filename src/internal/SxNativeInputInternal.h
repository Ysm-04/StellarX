#pragma once

#include "InputEvents.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <mutex>

class Window;

namespace StellarX::Internal
{
	// 入队结果必须显式区分内存耗尽和其它异常，供宿主进入 fail-stop。
	enum class NativeInputQueuePushResult : std::uint8_t
	{
		Queued,            ///< 已成功取得事件所有权并入队。
		AllocationFailure, ///< 队列扩容或事件移动时内存分配失败。
		UnexpectedFailure  ///< 非内存类异常，队列状态不再可信。
	};

	// 出队结果；异常与空队列分开报告，避免把故障误认为暂时没有输入。
	enum class NativeInputQueuePopResult : std::uint8_t
	{
		Popped,           ///< 成功弹出一个事件。
		Empty,            ///< 当前没有待处理事件。
		UnexpectedFailure ///< 出队过程发生异常，队列状态不再可信。
	};

	// 队列的隐私安全水位统计，不含事件内容。
	struct NativeInputQueueStats
	{
		std::uint64_t pushed = 0;
		std::uint64_t popped = 0;
		std::size_t currentDepth = 0;
		std::size_t maximumDepth = 0;
	};

	// 正确性优先的跨线程单生产者、单消费者队列。mutex 同时提供明确的
	// happens-before 关系；在换成 lock-free 实现前，不以吞吐量换取丢输入风险。
	class NativeInputQueue
	{
	public:
		// 创建空队列。
		NativeInputQueue() = default;
		// 队列拥有事件与同步原语，不允许复制。
		NativeInputQueue(const NativeInputQueue&) = delete;
		// 队列拥有事件与同步原语，不允许复制赋值。
		NativeInputQueue& operator=(const NativeInputQueue&) = delete;

		// 移动接管 event 并入队；失败时通过返回值显式报告。
		NativeInputQueuePushResult push(NativeInputEvent&& event) noexcept;
		// 尝试弹出队首事件；成功时覆盖 event。
		NativeInputQueuePopResult tryPop(NativeInputEvent& event) noexcept;
		// 返回在同一把锁下采集的一致统计快照。
		NativeInputQueueStats stats() const noexcept;

		// 仅供单元测试：让下一次 push 确定性返回指定结果，以验证 fail-stop。
		void forceNextPushResultForTesting(NativeInputQueuePushResult result) noexcept;

	private:
		mutable std::mutex mutex_;
		std::deque<NativeInputEvent> events_;
		std::uint64_t pushed_ = 0;
		std::uint64_t popped_ = 0;
		std::size_t maximumDepth_ = 0;
		std::atomic<NativeInputQueuePushResult> forcedPushResult_{ NativeInputQueuePushResult::Queued };
	};

	// 原生输入管线的不可恢复故障类型；任一非 None 值都会要求宿主安全停止。
	enum class NativeInputFaultCode : std::uint8_t
	{
		None,                ///< 管线尚未记录不可恢复故障。
		OutOfMemory,         ///< 采集、入队或宿主处理期间发生内存分配失败。
		QueueFailure,        ///< 队列操作抛出非内存类异常，队列状态不再可信。
		SequenceViolation,   ///< 消费端观察到重复或倒退的事件序号。
		UnexpectedException  ///< 管线边界捕获到无法进一步分类的异常。
	};

	// 首个故障的最小诊断快照；不保存文字、控件指针或其它敏感负载。
	struct NativeInputFaultSnapshot
	{
		NativeInputFaultCode code = NativeInputFaultCode::None;
		std::uint64_t sequence = 0;
		UINT message = 0;
		HWND hwnd = nullptr;
	};

	// 宿主排空事件时维护的顺序与类型统计。
	struct NativeInputObservationStats
	{
		std::uint64_t observed = 0;
		std::uint64_t keyEvents = 0;
		std::uint64_t textEvents = 0;
		std::uint64_t compositionEvents = 0;
		std::uint64_t activationEvents = 0;
		std::uint64_t pointerEvents = 0;
		std::uint64_t lastSequence = 0;
		bool sequenceStrict = true;
	};

	// 生产者侧规范化异常必须进入显式诊断：不会生成替代文字，也绝不静默计为成功。
	struct NativeInputNormalizationStats
	{
		std::uint64_t rejectedTextSequences = 0;
		std::uint64_t imeQueryFailures = 0;
		std::uint64_t discardedDecoderFragments = 0;
		std::uint64_t pointerCoordinateFailures = 0;
	};

	// 测试夹具读取的完整运行态快照，只包含计数、状态和故障元数据。
	struct NativeInputRuntimeSnapshot
	{
		NativeInputQueueStats queue{};
		NativeInputObservationStats observations{};
		NativeInputNormalizationStats normalization{};
		NativeInputActivationSnapshot activation{};
		NativeInputFaultSnapshot fault{};
		std::uint64_t easyXKeyboardMessagesDrained = 0;
		std::uint64_t legacyEasyXKeyboardBusinessDeliveries = 0;
		std::uint64_t easyXPointerMessagesDrained = 0;
		std::uint64_t legacyEasyXPointerBusinessDeliveries = 0;
		std::uint64_t nativePointerBusinessDeliveries = 0;
		bool faulted = false;
	};

	// WndProc 采集结果；仅同步处理的消息需要直接返回 result。
	struct NativeInputCollectResult
	{
		bool handledSynchronously = false;
		LRESULT result = 0;
	};

	// WndProc 采集时冻结的左右修饰键状态，随后写入 KeyEvent。
	struct KeyModifierSnapshot
	{
		bool leftControl = false;
		bool rightControl = false;
		bool leftShift = false;
		bool rightShift = false;
		bool leftAlt = false;
		bool rightAlt = false;
	};

	// 根据原生键盘消息和已冻结修饰键生成不依赖全局键态的 KeyEvent。
	KeyEvent makeKeyEvent(UINT message, WPARAM wParam, LPARAM lParam,
		const KeyModifierSnapshot& modifiers, bool imeLifecycleActive) noexcept;
	// 把支持的原生鼠标消息规范化为 PointerEvent；坐标转换失败会显式反映在事件中。
	bool makePointerEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
		PointerEvent& event) noexcept;

	// 每个 Window 对应一个上下文：WndProc 是生产者，最内层宿主循环是消费者。
	// Impl 隔离解码器、回声抑制与队列细节，析构前必须先停止相关窗口消息来源。
	class NativeInputContext
	{
	public:
		// 创建绑定到 hwnd 的采集上下文。
		explicit NativeInputContext(HWND hwnd);
		// 释放队列和解码状态；不负责销毁 hwnd。
		~NativeInputContext();
		// 一个 Window 只能拥有自己的上下文，不允许复制。
		NativeInputContext(const NativeInputContext&) = delete;
		// 一个 Window 只能拥有自己的上下文，不允许复制赋值。
		NativeInputContext& operator=(const NativeInputContext&) = delete;

		// 在 WndProc 线程采集并规范化一条消息；需要同步答复时返回 handledSynchronously。
		NativeInputCollectResult collect(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept;
		// 弹出一个事件，并同步更新严格序号和事件类型观察统计。
		NativeInputQueuePopResult tryPopObserved(NativeInputEvent& event) noexcept;
		// 最多排空 limit 个事件而不进入业务路由，返回实际排空数量。
		std::size_t drainObserved(std::size_t limit) noexcept;

		// 查询上下文是否已经进入 fail-stop。
		bool hasFault() const noexcept;
		// 返回首个故障的最小快照。
		NativeInputFaultSnapshot fault() const noexcept;
		// 返回队列深度与吞吐统计。
		NativeInputQueueStats queueStats() const noexcept;
		// 返回宿主观察到的事件类型和顺序统计。
		NativeInputObservationStats observationStats() const noexcept;
		// 返回生产者侧规范化异常统计。
		NativeInputNormalizationStats normalizationStats() const noexcept;
		// 返回当前原生激活状态及其代次。
		NativeInputActivationSnapshot activationSnapshot() const noexcept;
		// 宿主路由或编辑阶段的分配失败与生产者、队列故障共用同一条 fail-stop
		// 通道，确保文字输入不会在异常路径静默丢失。
		void reportHostFault(NativeInputFaultCode code, std::uint64_t sequence,
			UINT message, HWND hwnd) noexcept;

		// 以下入口只服务测试观察，不参与实际业务路由。
		// 绕过业务路由弹出一个事件，用于断言规范化结果。
		bool tryPopForTesting(NativeInputEvent& event) noexcept;
		// 指定下一次入队的确定性结果，用于覆盖 fail-stop 分支。
		void forceNextQueuePushResultForTesting(NativeInputQueuePushResult result) noexcept;
		// 替换 ANSI 回退解码使用的代码页，不修改系统全局设置。
		void setAnsiCodePageForTesting(UINT codePage) noexcept;
		// 预置一次 IME 结果回声，用固定文本验证重复通道抑制。
		void seedEchoForTesting(const std::u16string& text, std::uint64_t compositionId);

	private:
		struct Impl;
		std::unique_ptr<Impl> impl_;
	};

	// 内部夹具入口：只暴露计数和状态，绝不返回队列中的文字内容。
	NativeInputRuntimeSnapshot inspectWindowNativeInputForTesting(const ::Window& window) noexcept;

	// 宿主发现输入管线 fail-stop 时使用的稳定进程退出码。
	constexpr int NativeInputFailureExitCode = -2;
}
