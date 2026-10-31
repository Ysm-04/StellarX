#pragma once

#include <cstddef>
#include <cstdint>
#include <windows.h>

struct ExMessage;

namespace StellarX::Internal
{
	struct ImeCaretAnchorRuntimeSnapshot;

	// 一次 WndProc 调用的探针令牌。令牌只保存序号、规则裁决和层级关系，
	// 不记录原始文字参数；必须原样传回 inputProbeWndProcLeave()。
	struct InputProbeWndProcToken
	{
		// 当前调用、父调用和最外层调用的关联编号。
		std::uint64_t callId = 0;
		std::uint64_t parentCallId = 0;
		std::uint64_t rootCallId = 0;
		std::uint64_t enterSequence = 0;
		std::uint64_t compositionId = 0;
		std::uint32_t callDepth = 0;
		UINT message = 0;
		// 记录探针是否生效，以及消息规则在入口处作出的裁决。
		bool depthCounted = false;
		bool recorded = false;
		bool experimentTagged = false;
		bool ruleMatched = false;
		bool observeAndConsume = false;
		bool failOpenApplied = false;
		std::uint8_t failOpenReason = 0;
		std::uint32_t matchOrdinal = 0;
	};

	// 宿主循环一次排空批次的探针令牌，用来关联 EasyX 消息与安全点快照。
	struct InputProbeBatchToken
	{
		std::uint64_t batchId = 0;
		std::uint64_t startedQpcTicks = 0;
		std::uint32_t nextMessageIndex = 0;
		std::uint32_t dispatchDepth = 0;
		std::uint32_t modalDepth = 0;
		bool active = false;
		bool modal = false;
		bool materialized = false;
	};

#if defined(STELLARX_ENABLE_INPUT_PROBE)
	// 为 hwnd 建立被动探针会话；不改变窗口过程的业务裁决。
	void inputProbeAttach(HWND hwnd) noexcept;
	// 结束 hwnd 的探针会话，并在安全范围内提交剩余记录。
	void inputProbeDetach(HWND hwnd) noexcept;

	// 在 WndProc 最前端记录调用关系和诊断规则匹配结果。
	InputProbeWndProcToken inputProbeWndProcEnter(
		HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept;
	// 与 enter 成对记录返回结果；返回值保持 result 不变，便于直接用于 WndProc 返回。
	LRESULT inputProbeWndProcLeave(
		const InputProbeWndProcToken& token, LRESULT result, bool forwardedToOldProc) noexcept;

	// 开始一次宿主消息排空批次；modal 标明是否处于模态循环。
	InputProbeBatchToken inputProbeHostBatchBegin(HWND hwnd, bool modal) noexcept;
	// 记录本批次观察到的一条 EasyX 消息，不复制其中的敏感文字。
	void inputProbeObserveEasyXMessage(
		HWND hwnd, InputProbeBatchToken& batch, const ExMessage& message) noexcept;
	// 结束批次并记录处理数量和提前停止状态。
	void inputProbeHostBatchEnd(
		HWND hwnd, InputProbeBatchToken& batch, std::size_t processed, bool stopped) noexcept;
	// 查询本会话是否启用了几何安全点记录。
	bool inputProbeGeometrySafePointEnabled(HWND hwnd) noexcept;
	// 在宿主安全点记录窗口几何与 IME 运行态的隐私安全快照。
	void inputProbeObserveGeometrySafePoint(
		HWND hwnd, std::uint64_t batchId,
		const ImeCaretAnchorRuntimeSnapshot& imeRuntime) noexcept;
	// 在宿主安全点把当前线程缓冲提交到诊断文件。
	void inputProbeFlushAtSafePoint(HWND hwnd) noexcept;
#else
	// 未编译探针时保留同形空实现，使正常调用点无需条件编译。
	inline void inputProbeAttach(HWND) noexcept {}
	inline void inputProbeDetach(HWND) noexcept {}

	inline InputProbeWndProcToken inputProbeWndProcEnter(
		HWND, UINT, WPARAM, LPARAM) noexcept
	{
		return {};
	}

	inline LRESULT inputProbeWndProcLeave(
		const InputProbeWndProcToken&, LRESULT result, bool) noexcept
	{
		return result;
	}

	inline InputProbeBatchToken inputProbeHostBatchBegin(HWND, bool) noexcept
	{
		return {};
	}

	inline void inputProbeObserveEasyXMessage(
		HWND, InputProbeBatchToken&, const ExMessage&) noexcept {}

	inline void inputProbeHostBatchEnd(
		HWND, InputProbeBatchToken&, std::size_t, bool) noexcept {}

	inline bool inputProbeGeometrySafePointEnabled(HWND) noexcept { return false; }

	inline void inputProbeObserveGeometrySafePoint(
		HWND, std::uint64_t, const ImeCaretAnchorRuntimeSnapshot&) noexcept {}

	inline void inputProbeFlushAtSafePoint(HWND) noexcept {}
#endif
}
