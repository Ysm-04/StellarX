#pragma once

#include "CoreTypes.h"

#include <Windows.h>
#include <imm.h>

#include <atomic>
#include <cstddef>
#include <cstdint>

class Window;

namespace StellarX::Internal
{
	// 系统候选窗的定位策略。默认同时给出 caret 点和整行排除区。
	enum class ImeCandidatePositionMode : std::uint8_t
	{
		CandidatePosition, ///< 只请求候选窗左上角贴近 caret 底端。
		ExcludeTextLine    ///< 同时排除当前文字行，允许系统选择不遮挡的位置。
	};

	// 取消组合后如何处理原生候选界面。
	enum class ImeCandidateDismissMode : std::uint8_t
	{
		NotifyOnly,                 ///< 只发送取消和关闭候选通知。
		NotifyThenReassociateContext ///< 通知后临时分离并恢复原 HIMC，强制清理残留界面。
	};

	// 原生输入上下文是否随当前逻辑文字输入资格启停。
	enum class ImeInputContextGateMode : std::uint8_t
	{
		LegacyAlwaysAssociated, ///< 始终保留窗口默认 HIMC，供兼容性测试使用。
		TextInputEligibilityGate ///< 仅当焦点叶节点可输入文字时关联 HIMC。
	};

	// 一次取消请求所依据的原生活动证据；没有证据时必须 fail-closed。
	enum class ImeCancelEvidenceKind : std::uint8_t
	{
		None,                ///< 当前没有可安全取消的原生活动证据。
		CompositionLifecycle, ///< 来自 START/COMPOSITION/END 生命周期的轮次证据。
		ProcessKeyActivity   ///< 来自 VK_PROCESSKEY 的短期单键活动证据。
	};

	// IME 锚点管线的隐私安全聚合统计。所有字段都是次数，不保存输入文字、
	// 候选内容或控件指针。
	struct ImeCaretAnchorRuntimeStats
	{
		// 锚点发布与跨线程刷新消息。
		std::uint64_t publications = 0;
		std::uint64_t invalidPublications = 0;
		std::uint64_t refreshPosts = 0;
		std::uint64_t refreshPostFailures = 0;
		std::uint64_t refreshMessages = 0;
		// 组合生命周期、定位请求和定位结果。
		std::uint64_t imeStartMessages = 0;
		std::uint64_t firstCompositionMessages = 0;
		std::uint64_t imeEndMessages = 0;
		std::uint64_t positionRequests = 0;
		std::uint64_t inactiveTargetPositionSkips = 0;
		std::uint64_t invalidAnchorSkips = 0;
		std::uint64_t snapshotReadFailures = 0;
		std::uint64_t missingWindowSkips = 0;
		std::uint64_t missingContextSkips = 0;
		std::uint64_t compositionSuccesses = 0;
		std::uint64_t candidateSuccesses = 0;
		std::uint64_t suppressedCompositionMessages = 0;
		std::uint64_t compositionRequestReplies = 0;
		// 取消凭据、跨线程请求及原生取消/候选关闭结果。
		std::uint64_t cancelRequests = 0;
		std::uint64_t cancelNoEvidenceSkips = 0;
		std::uint64_t lifecycleEvidenceCancels = 0;
		std::uint64_t processKeyEvidenceCancels = 0;
		std::uint64_t cancelRequestPosts = 0;
		std::uint64_t cancelRequestPostFailures = 0;
		std::uint64_t cancelRequestCoalesces = 0;
		std::uint64_t cancelMessages = 0;
		std::uint64_t staleCancelMessages = 0;
		std::uint64_t cancelMissingWindowSkips = 0;
		std::uint64_t cancelMissingContextSkips = 0;
		std::uint64_t compositionCancelSuccesses = 0;
		std::uint64_t compositionCancelFailures = 0;
		std::uint64_t candidateCloseAttempts = 0;
		std::uint64_t candidateCloseSuccesses = 0;
		std::uint64_t candidateCloseFailures = 0;
		std::uint64_t contextReassociateAttempts = 0;
		std::uint64_t contextDetachSuccesses = 0;
		std::uint64_t contextDetachFailures = 0;
		std::uint64_t contextRestoreSuccesses = 0;
		std::uint64_t contextRestoreFailures = 0;
		std::uint64_t cancelSuccesses = 0;
		std::uint64_t cancelFailures = 0;
		// VK_PROCESSKEY 活动和首次输入前的预定位。
		std::uint64_t processKeyActivityUpdates = 0;
		std::uint64_t processKeyActivityClears = 0;
		std::uint64_t processKeyPrepositionAttempts = 0;
		std::uint64_t processKeyPrepositionSuccesses = 0;
		std::uint64_t processKeyPrepositionFailures = 0;
		std::uint64_t idlePrepositionAttempts = 0;
		std::uint64_t idlePrepositionSuccesses = 0;
		std::uint64_t idlePrepositionFailures = 0;
		std::uint64_t idlePrepositionBlocked = 0;
		// Pointer 离开旧编辑器时的取消判定。
		std::uint64_t pointerDepartureChecks = 0;
		std::uint64_t pointerDepartureSnapshotReadFailures = 0;
		std::uint64_t pointerDepartureInvalidAnchorSkips = 0;
		std::uint64_t pointerDepartureInsideSkips = 0;
		std::uint64_t pointerDepartureNoEvidenceSkips = 0;
		std::uint64_t pointerDepartureCancelAttempts = 0;
		std::uint64_t pointerDepartureCancelSuccesses = 0;
		std::uint64_t pointerDepartureCancelFailures = 0;
		// 按文字输入资格分离或恢复 HIMC 的门控结果。
		std::uint64_t inputContextGateTransitions = 0;
		std::uint64_t inputContextDetachAttempts = 0;
		std::uint64_t inputContextDetachSuccesses = 0;
		std::uint64_t inputContextDetachFailures = 0;
		std::uint64_t inputContextRestoreAttempts = 0;
		std::uint64_t inputContextRestoreSuccesses = 0;
		std::uint64_t inputContextRestoreFailures = 0;
		std::uint64_t inputContextAlreadyAbsentSkips = 0;
	};

	// 供测试和诊断读取的 IME 锚点完整状态。跨线程版本只填充原子字段与
	// 一致快照，属主线程专用状态由 inspect() 补齐。
	struct ImeCaretAnchorRuntimeSnapshot
	{
		// 最近发布和最近真正送给 IMM32 的锚点。
		ImeCaretAnchorSnapshot published{};
		ImeCaretAnchorSnapshot lastPositioned{};
		ImeCaretAnchorRuntimeStats stats{};
		// 激活、刷新、内联组合和首次按键前预定位状态。
		bool windowActive = false;
		bool refreshPending = false;
		bool inlineCompositionOwned = false;
		bool inlineCompositionRoundActive = false;
		bool idlePrepositionAllowed = true;
		// 当前测试策略与文字输入资格门控状态。
		ImeCandidatePositionMode candidatePositionMode =
			ImeCandidatePositionMode::ExcludeTextLine;
		ImeCandidateDismissMode candidateDismissMode =
			ImeCandidateDismissMode::NotifyThenReassociateContext;
		ImeInputContextGateMode inputContextGateMode =
			ImeInputContextGateMode::TextInputEligibilityGate;
		bool textInputEligible = false;
		bool inputContextDetached = false;
		// 当前生命周期/PROCESSKEY 证据及待处理取消凭据。
		std::uint32_t activeCompositionEpoch = 0;
		std::uint32_t activeProcessKeyToken = 0;
		ImeCancelEvidenceKind cancelPendingKind = ImeCancelEvidenceKind::None;
		std::uint32_t cancelPendingValue = 0;
	};

	// onWndProcMessage() 的同步裁决；handledSynchronously 为 true 时调用方
	// 应直接返回 result，不再转发旧窗口过程。
	struct ImeCaretWndProcResult
	{
		bool handledSynchronously = false;
		LRESULT result = 0;
	};

	// 默认策略把请求点钉在 caret 底端，并排除完整编辑行。微软拼音和搜狗都
	// 通过同一套非输入法特定策略；系统仍可按自身规则微调原生窗口位置。
	CANDIDATEFORM candidatePositionForm(
		const ImeCaretAnchorSnapshot& snapshot,
		ImeCandidatePositionMode mode =
			ImeCandidatePositionMode::ExcludeTextLine) noexcept;

	// 组合串属于当前字形行，候选列表属于其下方。部分输入法把二者画在同一表面，
	// 因此组合窗请求行顶，而候选窗仍用独立发布的 caret 底端，避免重复叠加行高。
	COMPOSITIONFORM compositionPositionForm(
		const ImeCaretAnchorSnapshot& snapshot) noexcept;

	// 跨线程标量快照存储。每个字段都是原子量：序列校验阻止混合不同代次，
	// 原子字段本身则消除 C++ 数据竞争。
	class ImeCaretAnchorStore final
	{
	public:
		// 发布一份完整快照；写入期间读者只会重试，不会看到半代数据。
		void publish(const ImeCaretAnchorSnapshot& snapshot) noexcept;
		// 在有限次数内尝试读取同一代快照；竞争持续时返回 false。
		bool tryRead(ImeCaretAnchorSnapshot& snapshot,
			std::size_t maxAttempts = 8) const noexcept;

	private:
		std::atomic<std::uint64_t> sequence_{ 0 };
		std::atomic<std::uint64_t> targetOwner_{ 0 };
		std::atomic<std::uint32_t> targetSubNode_{ 0 };
		std::atomic<std::uint64_t> scope_{ 0 };
		std::atomic<LONG> pointX_{ 0 };
		std::atomic<LONG> pointY_{ 0 };
		std::atomic<LONG> lineLeft_{ 0 };
		std::atomic<LONG> lineTop_{ 0 };
		std::atomic<LONG> lineRight_{ 0 };
		std::atomic<LONG> lineBottom_{ 0 };
		std::atomic<LONG> editLeft_{ 0 };
		std::atomic<LONG> editTop_{ 0 };
		std::atomic<LONG> editRight_{ 0 };
		std::atomic<LONG> editBottom_{ 0 };
		std::atomic<std::uint64_t> layoutGeneration_{ 0 };
		std::atomic<std::uint64_t> publicationGeneration_{ 0 };
		std::atomic<bool> valid_{ false };
	};

	// 宿主线程只发布几何，EasyX 窗口属主线程负责读取并且是唯一可调用 IMM32
	// 的线程。上下文不允许 Control*、HDC 或借用视图跨线程逃逸。
	class ImeCaretAnchorContext final
	{
	public:
		// 创建上下文；hwnd 可稍后由 attachWindow() 绑定。
		explicit ImeCaretAnchorContext(HWND hwnd = nullptr) noexcept;

		// 绑定或更新目标 EasyX HWND，不接管窗口生命周期。
		void attachWindow(HWND hwnd) noexcept;
		// 发布窗口键盘激活状态；失活会阻止新的原生定位。
		void setWindowActive(bool active) noexcept;
		// 声明当前焦点叶节点是否由框架内联绘制临时组合串。
		void setInlineCompositionOwned(bool owned) noexcept;
		// 查询最近发布的内联组合所有权。
		bool inlineCompositionOwned() const noexcept;
		// 仅供测试：切换候选窗定位表单策略。
		void setCandidatePositionModeForTesting(
			ImeCandidatePositionMode mode) noexcept;
		// 仅供测试：切换候选窗取消后的清理策略。
		void setCandidateDismissModeForTesting(
			ImeCandidateDismissMode mode) noexcept;
		// 仅供测试：切换 HIMC 是否跟随文字输入资格。
		void setInputContextGateModeForTesting(
			ImeInputContextGateMode mode) noexcept;
		// 发布当前逻辑焦点叶节点是否允许文字输入，并请求属主线程同步 HIMC。
		void setTextInputEligible(bool eligible) noexcept;
		// 在 WM_IME_SETCONTEXT 转发前按内联组合所有权过滤默认 UI 标志。
		LPARAM filterSetContextLParam(WPARAM wParam, LPARAM lParam) const noexcept;
		// 从宿主线程发布客户区锚点，并为有效锚点分配新的发布代次。
		void publish(ImeCaretAnchorSnapshot snapshot) noexcept;
		// WndProc 属主线程边界：在转发当前消息前应用待处理的文字输入资格门控。
		// WM_NCDESTROY 会强制恢复原 HIMC，让系统按窗口初始上下文完成销毁。
		bool synchronizeInputContextAssociation(
			HWND hwnd, bool forceRestore = false) noexcept;
		// 窗口销毁可能先从宿主线程开始，而子类化过程尚未观察到 WM_NCDESTROY。
		// 该入口执行有界属主线程握手，确保移除子类前恢复当初保存的同一个 HIMC。
		bool restoreInputContextBeforeWindowTeardown(HWND hwnd) noexcept;
		// 宿主线程只负责提交取消请求；真正的 IMM32 调用由注册消息在 HWND
		// 属主线程执行。没有可靠活动证据时返回 false。
		bool requestCompositionCancel() noexcept;
		// 仅属主线程观察。原始 VK_PROCESSKEY 是“当前单键归 IME”的证据，
		// 绝不能据此开启持久 composition epoch。
		bool observeProcessKeyActivityForCollection(
			HWND hwnd, UINT message, WPARAM wParam) noexcept;
		// 在 NativeInputContext 采集对应 LeftDown 前由 HWND 属主线程调用。
		// 此时取消可保证同步产生的 Cancel/End 先于 Pointer 进入统一原生队列。
		bool cancelCompositionBeforePointerCollection(
			HWND hwnd, LPARAM lParam) noexcept;
		// WndProc 在 NativeInputContext 发布对应生命周期事件前调用，先让属主
		// epoch 可见，避免宿主在线程竞态中提前退休焦点绑定。
		bool prepareCompositionEpochForCollection(UINT message) noexcept;
		// 有界读取最近发布锚点；持续并发写入时返回 false。
		bool tryReadPublished(ImeCaretAnchorSnapshot& snapshot,
			std::size_t maxAttempts = 8) const noexcept;
		// 处理本组件关心的原生窗口消息，并返回是否需要同步截断默认处理。
		ImeCaretWndProcResult onWndProcMessage(
			HWND hwnd, UINT message, WPARAM wParam = 0,
			LPARAM lParam = 0,
			bool compositionEpochPrepared = false) noexcept;
		// 跨线程诊断只能读取原子量和快照存储；属主线程独占的组合轮次细节
		// 会被有意省略。
		ImeCaretAnchorRuntimeSnapshot inspectCrossThread() const noexcept;
		// 返回完整运行态快照；调用方必须已经位于 HWND 属主线程。
		ImeCaretAnchorRuntimeSnapshot inspect() const noexcept;

		// 返回用于请求重新定位的进程内注册窗口消息编号。
		static UINT refreshMessage() noexcept;
		// 返回用于执行组合取消的进程内注册窗口消息编号。
		static UINT compositionCancelMessage() noexcept;

	private:
		// 以下操作只在 HWND 属主线程真正接触 IMM32；宿主线程最多投递注册消息。
		bool positionLatest(HWND hwnd,
			bool allowIdlePreposition = false) noexcept;
		void requestRefresh() noexcept;
		void captureActiveImeTarget() noexcept;
		void closeActiveImePositioning() noexcept;
		void reopenIdlePrepositionIfInactive() noexcept;
		std::uint32_t beginCompositionEpoch() noexcept;
		// 取消凭据把证据类型和单调编号绑定在一起，防止延迟消息误取消新轮次。
		struct CancelCredential
		{
			ImeCancelEvidenceKind kind = ImeCancelEvidenceKind::None;
			std::uint32_t value = 0;
			explicit operator bool() const noexcept
			{
				return kind != ImeCancelEvidenceKind::None && value != 0;
			}
		};
		// 取消与 HIMC 重新关联都要求凭据在执行时仍匹配当前活动证据。
		CancelCredential currentCancelCredential() const noexcept;
		bool isCurrentCancelCredential(const CancelCredential& credential) const noexcept;
		bool performCompositionCancel(HWND hwnd,
			const CancelCredential& credential, bool pointerDeparture) noexcept;
		bool reassociateInputContext(HWND hwnd) noexcept;
		bool detachInputContext(HWND hwnd) noexcept;
		bool restoreInputContext(HWND hwnd) noexcept;
		void clearProcessKeyActivity(std::uint32_t expected = 0) noexcept;
		static std::uint64_t packCancelCredential(
			const CancelCredential& credential) noexcept;
		static CancelCredential unpackCancelCredential(std::uint64_t packed) noexcept;

		// 跨线程发布状态和双快照存储。
		std::atomic<HWND> hwnd_{ nullptr };
		std::atomic<bool> windowActive_{ true };
		std::atomic<bool> refreshPending_{ false };
		std::atomic<std::uint64_t> nextPublicationGeneration_{ 0 };
		ImeCaretAnchorStore published_;
		ImeCaretAnchorStore lastPositioned_;

		// 组合轮次细节只由 HWND 属主线程访问；对外只发布必要的原子 epoch/token。
		bool compositionRoundActive_ = false;
		bool firstCompositionHandled_ = false;
		bool inlineCompositionRoundActive_ = false;
		std::uint32_t nextCompositionEpoch_ = 0;
		std::atomic<std::uint32_t> activeCompositionEpoch_{ 0 };
		std::uint32_t nextProcessKeyToken_ = 0;
		std::atomic<std::uint32_t> activeProcessKeyToken_{ 0 };
		// 有效焦点锚点可在首次按键前预热休眠 HIMC。原生活动会关闭该门；只有
		// 明确终止事件或完整成功的清理才重新开启，避免取消失败后把仍可见的
		// 旧候选界面移动到新的逻辑 TextBox。
		std::atomic<bool> idlePrepositionAllowed_{ true };
		// 启动当前原生 IME 活动的文字叶节点身份，仅由属主线程维护；后来获得
		// 逻辑焦点的 TextBox 不得继承旧原生界面。
		FocusTargetId activeImeTarget_{};
		std::atomic<std::uint64_t> cancelPendingCredential_{ 0 };

		// 以下原子计数与 ImeCaretAnchorRuntimeStats 分组一一对应。
		std::atomic<std::uint64_t> publications_{ 0 };
		std::atomic<std::uint64_t> invalidPublications_{ 0 };
		std::atomic<std::uint64_t> refreshPosts_{ 0 };
		std::atomic<std::uint64_t> refreshPostFailures_{ 0 };
		std::atomic<std::uint64_t> refreshMessages_{ 0 };
		std::atomic<std::uint64_t> imeStartMessages_{ 0 };
		std::atomic<std::uint64_t> firstCompositionMessages_{ 0 };
		std::atomic<std::uint64_t> imeEndMessages_{ 0 };
		std::atomic<std::uint64_t> positionRequests_{ 0 };
		std::atomic<std::uint64_t> inactiveTargetPositionSkips_{ 0 };
		std::atomic<std::uint64_t> invalidAnchorSkips_{ 0 };
		std::atomic<std::uint64_t> snapshotReadFailures_{ 0 };
		std::atomic<std::uint64_t> missingWindowSkips_{ 0 };
		std::atomic<std::uint64_t> missingContextSkips_{ 0 };
		std::atomic<std::uint64_t> compositionSuccesses_{ 0 };
		std::atomic<std::uint64_t> candidateSuccesses_{ 0 };
		std::atomic<bool> inlineCompositionOwned_{ false };
		std::atomic<ImeCandidatePositionMode> candidatePositionMode_{
			ImeCandidatePositionMode::ExcludeTextLine };
		std::atomic<ImeCandidateDismissMode> candidateDismissMode_{
			ImeCandidateDismissMode::NotifyThenReassociateContext };
		std::atomic<ImeInputContextGateMode> inputContextGateMode_{
			ImeInputContextGateMode::TextInputEligibilityGate };
		std::atomic<bool> textInputEligible_{ false };
		std::atomic<std::uint64_t> inputContextGateGeneration_{ 1 };
		// 代次和保存的 HIMC 只由 HWND 属主线程写；分离状态通过原子标志暴露给
		// 只读运行态快照。
		std::uint64_t appliedInputContextGateGeneration_ = 0;
		HIMC detachedInputContext_ = nullptr;
		std::atomic<bool> inputContextDetached_{ false };
		std::atomic<std::uint64_t> suppressedCompositionMessages_{ 0 };
		std::atomic<std::uint64_t> compositionRequestReplies_{ 0 };
		std::atomic<std::uint64_t> cancelRequests_{ 0 };
		std::atomic<std::uint64_t> cancelNoEvidenceSkips_{ 0 };
		std::atomic<std::uint64_t> lifecycleEvidenceCancels_{ 0 };
		std::atomic<std::uint64_t> processKeyEvidenceCancels_{ 0 };
		std::atomic<std::uint64_t> cancelRequestPosts_{ 0 };
		std::atomic<std::uint64_t> cancelRequestPostFailures_{ 0 };
		std::atomic<std::uint64_t> cancelRequestCoalesces_{ 0 };
		std::atomic<std::uint64_t> cancelMessages_{ 0 };
		std::atomic<std::uint64_t> staleCancelMessages_{ 0 };
		std::atomic<std::uint64_t> cancelMissingWindowSkips_{ 0 };
		std::atomic<std::uint64_t> cancelMissingContextSkips_{ 0 };
		std::atomic<std::uint64_t> compositionCancelSuccesses_{ 0 };
		std::atomic<std::uint64_t> compositionCancelFailures_{ 0 };
		std::atomic<std::uint64_t> candidateCloseAttempts_{ 0 };
		std::atomic<std::uint64_t> candidateCloseSuccesses_{ 0 };
		std::atomic<std::uint64_t> candidateCloseFailures_{ 0 };
		std::atomic<std::uint64_t> contextReassociateAttempts_{ 0 };
		std::atomic<std::uint64_t> contextDetachSuccesses_{ 0 };
		std::atomic<std::uint64_t> contextDetachFailures_{ 0 };
		std::atomic<std::uint64_t> contextRestoreSuccesses_{ 0 };
		std::atomic<std::uint64_t> contextRestoreFailures_{ 0 };
		std::atomic<std::uint64_t> cancelSuccesses_{ 0 };
		std::atomic<std::uint64_t> cancelFailures_{ 0 };
		std::atomic<std::uint64_t> processKeyActivityUpdates_{ 0 };
		std::atomic<std::uint64_t> processKeyActivityClears_{ 0 };
		std::atomic<std::uint64_t> processKeyPrepositionAttempts_{ 0 };
		std::atomic<std::uint64_t> processKeyPrepositionSuccesses_{ 0 };
		std::atomic<std::uint64_t> processKeyPrepositionFailures_{ 0 };
		std::atomic<std::uint64_t> idlePrepositionAttempts_{ 0 };
		std::atomic<std::uint64_t> idlePrepositionSuccesses_{ 0 };
		std::atomic<std::uint64_t> idlePrepositionFailures_{ 0 };
		std::atomic<std::uint64_t> idlePrepositionBlocked_{ 0 };
		std::atomic<std::uint64_t> pointerDepartureChecks_{ 0 };
		std::atomic<std::uint64_t> pointerDepartureSnapshotReadFailures_{ 0 };
		std::atomic<std::uint64_t> pointerDepartureInvalidAnchorSkips_{ 0 };
		std::atomic<std::uint64_t> pointerDepartureInsideSkips_{ 0 };
		std::atomic<std::uint64_t> pointerDepartureNoEvidenceSkips_{ 0 };
		std::atomic<std::uint64_t> pointerDepartureCancelAttempts_{ 0 };
		std::atomic<std::uint64_t> pointerDepartureCancelSuccesses_{ 0 };
		std::atomic<std::uint64_t> pointerDepartureCancelFailures_{ 0 };
		std::atomic<std::uint64_t> inputContextGateTransitions_{ 0 };
		std::atomic<std::uint64_t> inputContextDetachAttempts_{ 0 };
		std::atomic<std::uint64_t> inputContextDetachSuccesses_{ 0 };
		std::atomic<std::uint64_t> inputContextDetachFailures_{ 0 };
		std::atomic<std::uint64_t> inputContextRestoreAttempts_{ 0 };
		std::atomic<std::uint64_t> inputContextRestoreSuccesses_{ 0 };
		std::atomic<std::uint64_t> inputContextRestoreFailures_{ 0 };
		std::atomic<std::uint64_t> inputContextAlreadyAbsentSkips_{ 0 };
	};

	// 仅供测试：读取指定 Window 的 IME 锚点运行态，不暴露输入内容。
	ImeCaretAnchorRuntimeSnapshot inspectWindowImeCaretAnchorForTesting(
		const ::Window& window) noexcept;
	// 仅供测试：在事件循环锁定前切换候选窗定位策略。
	bool configureImeCandidatePositionModeForTesting(
		::Window& window, ImeCandidatePositionMode mode) noexcept;
	// 仅供测试：切换取消后的候选界面清理策略。
	bool configureImeCandidateDismissModeForTesting(
		::Window& window, ImeCandidateDismissMode mode) noexcept;
	// 仅供测试：切换 HIMC 文字输入资格门控策略。
	bool configureImeInputContextGateModeForTesting(
		::Window& window, ImeInputContextGateMode mode) noexcept;
}
