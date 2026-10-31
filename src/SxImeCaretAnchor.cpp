#include "SxImeCaretAnchorInternal.h"

#include <imm.h>

#pragma comment(lib, "imm32.lib")

namespace StellarX::Internal
{
	namespace
	{
		constexpr std::size_t WndProcReadAttempts = 8;

		bool validGeometry(const ImeCaretAnchorSnapshot& snapshot) noexcept
		{
			return snapshot.valid && snapshot.target && snapshot.scope &&
				snapshot.layoutGeneration != 0 &&
				snapshot.clientLineRect.right > snapshot.clientLineRect.left &&
				snapshot.clientLineRect.bottom > snapshot.clientLineRect.top &&
				snapshot.clientEditRect.right > snapshot.clientEditRect.left &&
				snapshot.clientEditRect.bottom > snapshot.clientEditRect.top;
		}

		bool pointInRect(const RECT& rect, POINT point) noexcept
		{
			return point.x >= rect.left && point.x < rect.right &&
				point.y >= rect.top && point.y < rect.bottom;
		}

		bool sameAnchorValue(const ImeCaretAnchorSnapshot& left,
			const ImeCaretAnchorSnapshot& right) noexcept
		{
			return left.valid == right.valid && left.target == right.target &&
				left.scope == right.scope &&
				left.clientPoint.x == right.clientPoint.x &&
				left.clientPoint.y == right.clientPoint.y &&
				left.clientLineRect.left == right.clientLineRect.left &&
				left.clientLineRect.top == right.clientLineRect.top &&
				left.clientLineRect.right == right.clientLineRect.right &&
				left.clientLineRect.bottom == right.clientLineRect.bottom &&
				left.clientEditRect.left == right.clientEditRect.left &&
				left.clientEditRect.top == right.clientEditRect.top &&
				left.clientEditRect.right == right.clientEditRect.right &&
				left.clientEditRect.bottom == right.clientEditRect.bottom &&
				left.layoutGeneration == right.layoutGeneration;
		}
	}

	CANDIDATEFORM candidatePositionForm(
		const ImeCaretAnchorSnapshot& snapshot,
		ImeCandidatePositionMode mode) noexcept
	{
		CANDIDATEFORM candidate{};
		candidate.dwIndex = 0;
		candidate.ptCurrentPos = snapshot.clientPoint;
		if (mode == ImeCandidatePositionMode::ExcludeTextLine)
		{
			candidate.dwStyle = CFS_EXCLUDE;
			candidate.rcArea = snapshot.clientLineRect;
		}
		else
		{
			candidate.dwStyle = CFS_CANDIDATEPOS;
		}
		return candidate;
	}

	COMPOSITIONFORM compositionPositionForm(
		const ImeCaretAnchorSnapshot& snapshot) noexcept
	{
		COMPOSITIONFORM composition{};
		composition.dwStyle = CFS_FORCE_POSITION;
		composition.ptCurrentPos = POINT{
			snapshot.clientPoint.x, snapshot.clientLineRect.top };
		return composition;
	}

	void ImeCaretAnchorStore::publish(
		const ImeCaretAnchorSnapshot& snapshot) noexcept
	{
		sequence_.fetch_add(1, std::memory_order_acq_rel);
		targetOwner_.store(snapshot.target.owner.value, std::memory_order_relaxed);
		targetSubNode_.store(snapshot.target.subNode, std::memory_order_relaxed);
		scope_.store(snapshot.scope.value, std::memory_order_relaxed);
		pointX_.store(snapshot.clientPoint.x, std::memory_order_relaxed);
		pointY_.store(snapshot.clientPoint.y, std::memory_order_relaxed);
		lineLeft_.store(snapshot.clientLineRect.left, std::memory_order_relaxed);
		lineTop_.store(snapshot.clientLineRect.top, std::memory_order_relaxed);
		lineRight_.store(snapshot.clientLineRect.right, std::memory_order_relaxed);
		lineBottom_.store(snapshot.clientLineRect.bottom, std::memory_order_relaxed);
		editLeft_.store(snapshot.clientEditRect.left, std::memory_order_relaxed);
		editTop_.store(snapshot.clientEditRect.top, std::memory_order_relaxed);
		editRight_.store(snapshot.clientEditRect.right, std::memory_order_relaxed);
		editBottom_.store(snapshot.clientEditRect.bottom, std::memory_order_relaxed);
		layoutGeneration_.store(snapshot.layoutGeneration, std::memory_order_relaxed);
		publicationGeneration_.store(
			snapshot.publicationGeneration, std::memory_order_relaxed);
		valid_.store(snapshot.valid, std::memory_order_relaxed);
		sequence_.fetch_add(1, std::memory_order_release);
	}

	bool ImeCaretAnchorStore::tryRead(ImeCaretAnchorSnapshot& snapshot,
		std::size_t maxAttempts) const noexcept
	{
		for (std::size_t attempt = 0; attempt < maxAttempts; ++attempt)
		{
			const std::uint64_t before = sequence_.load(std::memory_order_acquire);
			if ((before & 1u) != 0)
				continue;

			ImeCaretAnchorSnapshot candidate;
			candidate.target.owner.value = targetOwner_.load(std::memory_order_relaxed);
			candidate.target.subNode = targetSubNode_.load(std::memory_order_relaxed);
			candidate.scope.value = scope_.load(std::memory_order_relaxed);
			candidate.clientPoint.x = pointX_.load(std::memory_order_relaxed);
			candidate.clientPoint.y = pointY_.load(std::memory_order_relaxed);
			candidate.clientLineRect.left = lineLeft_.load(std::memory_order_relaxed);
			candidate.clientLineRect.top = lineTop_.load(std::memory_order_relaxed);
			candidate.clientLineRect.right = lineRight_.load(std::memory_order_relaxed);
			candidate.clientLineRect.bottom = lineBottom_.load(std::memory_order_relaxed);
			candidate.clientEditRect.left = editLeft_.load(std::memory_order_relaxed);
			candidate.clientEditRect.top = editTop_.load(std::memory_order_relaxed);
			candidate.clientEditRect.right = editRight_.load(std::memory_order_relaxed);
			candidate.clientEditRect.bottom = editBottom_.load(std::memory_order_relaxed);
			candidate.layoutGeneration = layoutGeneration_.load(std::memory_order_relaxed);
			candidate.publicationGeneration =
				publicationGeneration_.load(std::memory_order_relaxed);
			candidate.valid = valid_.load(std::memory_order_relaxed);

			const std::uint64_t after = sequence_.load(std::memory_order_acquire);
			if (before == after && (after & 1u) == 0)
			{
				snapshot = candidate;
				return true;
			}
		}
		return false;
	}

	ImeCaretAnchorContext::ImeCaretAnchorContext(HWND hwnd) noexcept : hwnd_(hwnd)
	{
	}

	UINT ImeCaretAnchorContext::refreshMessage() noexcept
	{
		static const UINT message = RegisterWindowMessageW(
			L"StellarX.ImeCaretAnchor.Refresh.v1");
		return message;
	}

	UINT ImeCaretAnchorContext::compositionCancelMessage() noexcept
	{
		static const UINT message = RegisterWindowMessageW(
			L"StellarX.ImeComposition.Cancel.v1");
		return message;
	}

	void ImeCaretAnchorContext::attachWindow(HWND hwnd) noexcept
	{
		hwnd_.store(hwnd, std::memory_order_release);
		requestRefresh();
	}

	void ImeCaretAnchorContext::setWindowActive(bool active) noexcept
	{
		windowActive_.store(active, std::memory_order_release);
		if (active)
			requestRefresh();
	}

	void ImeCaretAnchorContext::setInlineCompositionOwned(bool owned) noexcept
	{
		inlineCompositionOwned_.store(owned, std::memory_order_release);
	}

	bool ImeCaretAnchorContext::inlineCompositionOwned() const noexcept
	{
		return inlineCompositionOwned_.load(std::memory_order_acquire);
	}

	void ImeCaretAnchorContext::setCandidatePositionModeForTesting(
		ImeCandidatePositionMode mode) noexcept
	{
		candidatePositionMode_.store(mode, std::memory_order_release);
		requestRefresh();
	}

	void ImeCaretAnchorContext::setCandidateDismissModeForTesting(
		ImeCandidateDismissMode mode) noexcept
	{
		candidateDismissMode_.store(mode, std::memory_order_release);
	}

	void ImeCaretAnchorContext::setInputContextGateModeForTesting(
		ImeInputContextGateMode mode) noexcept
	{
		const auto previous = inputContextGateMode_.exchange(
			mode, std::memory_order_acq_rel);
		if (previous == mode)
			return;
		inputContextGateGeneration_.fetch_add(1, std::memory_order_acq_rel);
		inputContextGateTransitions_.fetch_add(1, std::memory_order_relaxed);
		requestRefresh();
	}

	void ImeCaretAnchorContext::setTextInputEligible(bool eligible) noexcept
	{
		const bool previous = textInputEligible_.exchange(
			eligible, std::memory_order_acq_rel);
		if (previous == eligible)
			return;
		inputContextGateGeneration_.fetch_add(1, std::memory_order_acq_rel);
		inputContextGateTransitions_.fetch_add(1, std::memory_order_relaxed);
		requestRefresh();
	}

	LPARAM ImeCaretAnchorContext::filterSetContextLParam(
		WPARAM wParam, LPARAM lParam) const noexcept
	{
		if (wParam == FALSE || !inlineCompositionOwned())
			return lParam;
		const ULONG_PTR flags = static_cast<ULONG_PTR>(lParam);
		return static_cast<LPARAM>(flags &
			~static_cast<ULONG_PTR>(ISC_SHOWUICOMPOSITIONWINDOW));
	}

	void ImeCaretAnchorContext::publish(ImeCaretAnchorSnapshot snapshot) noexcept
	{
		if (!validGeometry(snapshot))
		{
			snapshot = {};
		}
		ImeCaretAnchorSnapshot current;
		if (published_.tryRead(current) && sameAnchorValue(current, snapshot))
			return;

		snapshot.publicationGeneration =
			nextPublicationGeneration_.fetch_add(1, std::memory_order_relaxed) + 1;
		if (!snapshot.valid)
			invalidPublications_.fetch_add(1, std::memory_order_relaxed);
		published_.publish(snapshot);
		publications_.fetch_add(1, std::memory_order_relaxed);
		requestRefresh();
	}

	bool ImeCaretAnchorContext::requestCompositionCancel() noexcept
	{
		cancelRequests_.fetch_add(1, std::memory_order_relaxed);
		const CancelCredential credential = currentCancelCredential();
		if (!credential)
		{
			cancelNoEvidenceSkips_.fetch_add(1, std::memory_order_relaxed);
			return false;
		}
		if (credential.kind == ImeCancelEvidenceKind::CompositionLifecycle)
			lifecycleEvidenceCancels_.fetch_add(1, std::memory_order_relaxed);
		else
			processKeyEvidenceCancels_.fetch_add(1, std::memory_order_relaxed);

		const std::uint64_t packed = packCancelCredential(credential);
		std::uint64_t pending = cancelPendingCredential_.load(
			std::memory_order_acquire);
		for (;;)
		{
			if (pending == packed)
			{
				cancelRequestCoalesces_.fetch_add(1, std::memory_order_relaxed);
				return true;
			}
			if (cancelPendingCredential_.compare_exchange_weak(
				pending, packed, std::memory_order_acq_rel,
				std::memory_order_acquire))
			{
				break;
			}
		}

		const HWND hwnd = hwnd_.load(std::memory_order_acquire);
		const UINT message = compositionCancelMessage();
		if (hwnd && message != 0 &&
			PostMessage(hwnd, message, static_cast<WPARAM>(credential.value),
				static_cast<LPARAM>(credential.kind)) != FALSE)
		{
			cancelRequestPosts_.fetch_add(1, std::memory_order_relaxed);
			return true;
		}

		std::uint64_t expected = packed;
		(void)cancelPendingCredential_.compare_exchange_strong(
			expected, 0, std::memory_order_acq_rel,
			std::memory_order_acquire);
		cancelRequestPostFailures_.fetch_add(1, std::memory_order_relaxed);
		return false;
	}

	bool ImeCaretAnchorContext::observeProcessKeyActivityForCollection(
		HWND hwnd, UINT message, WPARAM wParam) noexcept
	{
		if ((message != WM_KEYDOWN && message != WM_SYSKEYDOWN) ||
			static_cast<UINT>(wParam) != VK_PROCESSKEY)
		{
			return false;
		}

		++nextProcessKeyToken_;
		if (nextProcessKeyToken_ == 0)
			++nextProcessKeyToken_;
		closeActiveImePositioning();
		activeProcessKeyToken_.store(nextProcessKeyToken_,
			std::memory_order_release);
		captureActiveImeTarget();
		processKeyActivityUpdates_.fetch_add(1, std::memory_order_relaxed);
		// 在 EasyX 原窗口过程允许输入法创建候选窗前，先用最新锚点预热共享 HIMC。
		// 重新关联会保留上下文身份，也会保留上次候选窗位置；若只异步刷新，新候选窗
		// 可能先在已失焦 TextBox 闪现，随后才移动到新目标。
		processKeyPrepositionAttempts_.fetch_add(1, std::memory_order_relaxed);
		if (positionLatest(hwnd))
		{
			processKeyPrepositionSuccesses_.fetch_add(
				1, std::memory_order_relaxed);
		}
		else
		{
			processKeyPrepositionFailures_.fetch_add(
				1, std::memory_order_relaxed);
			// 快照竞争、HIMC 短暂缺失或 IMM32 失败，可能在按键进入原窗口过程后恢复；
			// 保留属主线程刷新，作为可观测的尽力重试。
			requestRefresh();
		}
		return true;
	}

	bool ImeCaretAnchorContext::cancelCompositionBeforePointerCollection(
		HWND hwnd, LPARAM lParam) noexcept
	{
		pointerDepartureChecks_.fetch_add(1, std::memory_order_relaxed);
		ImeCaretAnchorSnapshot snapshot;
		if (!tryReadPublished(snapshot, WndProcReadAttempts))
		{
			pointerDepartureSnapshotReadFailures_.fetch_add(
				1, std::memory_order_relaxed);
			return false;
		}
		if (!snapshot.valid)
		{
			pointerDepartureInvalidAnchorSkips_.fetch_add(
				1, std::memory_order_relaxed);
			return false;
		}

		const auto raw = static_cast<std::uint32_t>(
			static_cast<ULONG_PTR>(lParam));
		const POINT point{
			static_cast<SHORT>(raw & 0xFFFFu),
			static_cast<SHORT>((raw >> 16) & 0xFFFFu) };
		if (pointInRect(snapshot.clientEditRect, point))
		{
			pointerDepartureInsideSkips_.fetch_add(1, std::memory_order_relaxed);
			return false;
		}

		const CancelCredential credential = currentCancelCredential();
		if (!credential)
		{
			pointerDepartureNoEvidenceSkips_.fetch_add(
				1, std::memory_order_relaxed);
			return false;
		}
		if (credential.kind == ImeCancelEvidenceKind::CompositionLifecycle)
			lifecycleEvidenceCancels_.fetch_add(1, std::memory_order_relaxed);
		else
			processKeyEvidenceCancels_.fetch_add(1, std::memory_order_relaxed);
		pointerDepartureCancelAttempts_.fetch_add(1, std::memory_order_relaxed);
		return performCompositionCancel(hwnd, credential, true);
	}

	bool ImeCaretAnchorContext::prepareCompositionEpochForCollection(
		UINT message) noexcept
	{
		if (message == WM_IME_STARTCOMPOSITION)
		{
			compositionRoundActive_ = true;
			firstCompositionHandled_ = false;
			beginCompositionEpoch();
			return true;
		}
		if (message == WM_IME_COMPOSITION && !compositionRoundActive_)
		{
			compositionRoundActive_ = true;
			beginCompositionEpoch();
			return true;
		}
		return false;
	}

	bool ImeCaretAnchorContext::tryReadPublished(
		ImeCaretAnchorSnapshot& snapshot, std::size_t maxAttempts) const noexcept
	{
		if (!published_.tryRead(snapshot, maxAttempts))
			return false;
		if (!windowActive_.load(std::memory_order_acquire))
			snapshot.valid = false;
		return true;
	}

	void ImeCaretAnchorContext::requestRefresh() noexcept
	{
		const HWND hwnd = hwnd_.load(std::memory_order_acquire);
		const UINT message = refreshMessage();
		if (!hwnd || message == 0)
			return;
		if (refreshPending_.exchange(true, std::memory_order_acq_rel))
			return;
		if (PostMessage(hwnd, message, 0, 0) != FALSE)
			refreshPosts_.fetch_add(1, std::memory_order_relaxed);
		else
		{
			refreshPending_.store(false, std::memory_order_release);
			refreshPostFailures_.fetch_add(1, std::memory_order_relaxed);
		}
	}

	ImeCaretAnchorContext::CancelCredential
		ImeCaretAnchorContext::currentCancelCredential() const noexcept
	{
		const std::uint32_t epoch = activeCompositionEpoch_.load(
			std::memory_order_acquire);
		if (epoch != 0)
		{
			return CancelCredential{
				ImeCancelEvidenceKind::CompositionLifecycle, epoch };
		}
		const std::uint32_t token = activeProcessKeyToken_.load(
			std::memory_order_acquire);
		if (token != 0)
		{
			return CancelCredential{
				ImeCancelEvidenceKind::ProcessKeyActivity, token };
		}
		return {};
	}

	bool ImeCaretAnchorContext::isCurrentCancelCredential(
		const CancelCredential& credential) const noexcept
	{
		switch (credential.kind)
		{
		case ImeCancelEvidenceKind::CompositionLifecycle:
			return credential.value != 0 && compositionRoundActive_ &&
				activeCompositionEpoch_.load(std::memory_order_acquire) ==
				credential.value;
		case ImeCancelEvidenceKind::ProcessKeyActivity:
			return credential.value != 0 &&
				activeProcessKeyToken_.load(std::memory_order_acquire) ==
				credential.value;
		default:
			return false;
		}
	}

	bool ImeCaretAnchorContext::performCompositionCancel(HWND hwnd,
		const CancelCredential& credential, bool pointerDeparture) noexcept
	{
		if (!hwnd)
		{
			cancelMissingWindowSkips_.fetch_add(1, std::memory_order_relaxed);
			if (pointerDeparture)
				pointerDepartureCancelFailures_.fetch_add(1,
					std::memory_order_relaxed);
			return false;
		}
		HIMC context = ImmGetContext(hwnd);
		if (!context)
		{
			cancelMissingContextSkips_.fetch_add(1, std::memory_order_relaxed);
			if (pointerDeparture)
				pointerDepartureCancelFailures_.fetch_add(1,
					std::memory_order_relaxed);
			return false;
		}

		const bool compositionCancelled = ImmNotifyIME(
			context, NI_COMPOSITIONSTR, CPS_CANCEL, 0) != FALSE;
		candidateCloseAttempts_.fetch_add(1, std::memory_order_relaxed);
		// 取消组合串和关闭候选列表是两个独立的 IMM32 操作；当前定位的候选列表索引为 0。
		const bool candidateClosed = ImmNotifyIME(
			context, NI_CLOSECANDIDATE, 0, 0) != FALSE;
		ImmReleaseContext(hwnd, context);
		if (compositionCancelled)
			compositionCancelSuccesses_.fetch_add(1, std::memory_order_relaxed);
		else
			compositionCancelFailures_.fetch_add(1, std::memory_order_relaxed);
		if (candidateClosed)
			candidateCloseSuccesses_.fetch_add(1, std::memory_order_relaxed);
		else
			candidateCloseFailures_.fetch_add(1, std::memory_order_relaxed);

		bool contextReassociated = false;
		if (compositionCancelled && !candidateClosed &&
			candidateDismissMode_.load(std::memory_order_acquire) ==
				ImeCandidateDismissMode::NotifyThenReassociateContext)
		{
			contextReassociated = reassociateInputContext(hwnd);
		}

		const bool cancelled = compositionCancelled &&
			(candidateClosed || contextReassociated);
		if (cancelled)
		{
			cancelSuccesses_.fetch_add(1, std::memory_order_relaxed);
			if (pointerDeparture)
				pointerDepartureCancelSuccesses_.fetch_add(
					1, std::memory_order_relaxed);
			if (credential.kind == ImeCancelEvidenceKind::CompositionLifecycle)
			{
				// 原生组合串和候选界面都已确认关闭。这里只退休匹配的属主线程 epoch；
				// 稍后到达的真实 END 仍可安全处理，路由生命周期事件也会独立采集。
				compositionRoundActive_ = false;
				firstCompositionHandled_ = false;
				activeCompositionEpoch_.store(0, std::memory_order_release);
				clearProcessKeyActivity();
			}
			else
			{
				clearProcessKeyActivity(credential.value);
			}
			reopenIdlePrepositionIfInactive();
		}
		else
		{
			cancelFailures_.fetch_add(1, std::memory_order_relaxed);
			if (pointerDeparture)
				pointerDepartureCancelFailures_.fetch_add(
					1, std::memory_order_relaxed);
		}
		return cancelled;
	}

	bool ImeCaretAnchorContext::reassociateInputContext(HWND hwnd) noexcept
	{
		contextReassociateAttempts_.fetch_add(1, std::memory_order_relaxed);
		// ImmAssociateContext 返回窗口此前关联的上下文。非空返回值既表示分离成功，
		// 也是之后必须原样恢复的身份；此过程不创建或销毁上下文。
		HIMC previous = ImmAssociateContext(hwnd, nullptr);
		if (!previous)
		{
			contextDetachFailures_.fetch_add(1, std::memory_order_relaxed);
			contextRestoreFailures_.fetch_add(1, std::memory_order_relaxed);
			return false;
		}
		contextDetachSuccesses_.fetch_add(1, std::memory_order_relaxed);

		auto verifyRestored = [hwnd, previous]() noexcept
		{
			HIMC restored = ImmGetContext(hwnd);
			const bool matches = restored == previous;
			if (restored)
				ImmReleaseContext(hwnd, restored);
			return matches;
		};
		(void)ImmAssociateContext(hwnd, previous);
		bool restoredExactly = verifyRestored();
		if (!restoredExactly)
		{
			// 恢复失败会让 EasyX 窗口失去文字输入；使用同一个已保存上下文重试一次，
			// 再失败则显式报告。
			(void)ImmAssociateContext(hwnd, previous);
			restoredExactly = verifyRestored();
		}
		if (restoredExactly)
			contextRestoreSuccesses_.fetch_add(1, std::memory_order_relaxed);
		else
			contextRestoreFailures_.fetch_add(1, std::memory_order_relaxed);
		return restoredExactly;
	}

	bool ImeCaretAnchorContext::detachInputContext(HWND hwnd) noexcept
	{
		if (inputContextDetached_.load(std::memory_order_acquire))
			return true;
		inputContextDetachAttempts_.fetch_add(1, std::memory_order_relaxed);
		if (!hwnd)
		{
			inputContextDetachFailures_.fetch_add(1, std::memory_order_relaxed);
			return false;
		}

		// 返回句柄就是该 HWND 此前关联的上下文。这里只保存、不销毁，并在同一属主线程恢复。
		HIMC previous = ImmAssociateContext(hwnd, nullptr);
		HIMC observed = ImmGetContext(hwnd);
		const bool detached = observed == nullptr;
		if (observed)
			ImmReleaseContext(hwnd, observed);
		if (!detached)
		{
			// 关联未按预期改变。若 IMM 返回了先前上下文，立即恢复，不能留下半修改状态。
			if (previous)
				(void)ImmAssociateContext(hwnd, previous);
			inputContextDetachFailures_.fetch_add(1, std::memory_order_relaxed);
			return false;
		}

		detachedInputContext_ = previous;
		inputContextDetached_.store(true, std::memory_order_release);
		if (!previous)
			inputContextAlreadyAbsentSkips_.fetch_add(1,
				std::memory_order_relaxed);
		inputContextDetachSuccesses_.fetch_add(1, std::memory_order_relaxed);
		return true;
	}

	bool ImeCaretAnchorContext::restoreInputContext(HWND hwnd) noexcept
	{
		if (!inputContextDetached_.load(std::memory_order_acquire))
			return true;
		inputContextRestoreAttempts_.fetch_add(1, std::memory_order_relaxed);
		if (!hwnd)
		{
			inputContextRestoreFailures_.fetch_add(1, std::memory_order_relaxed);
			return false;
		}

		const HIMC saved = detachedInputContext_;
		if (!saved)
		{
			// 资格门控执行前窗口就没有上下文；不要凭空创建默认上下文而改变调用方策略。
			inputContextDetached_.store(false, std::memory_order_release);
			inputContextRestoreSuccesses_.fetch_add(1, std::memory_order_relaxed);
			return true;
		}

		auto verifySaved = [hwnd, saved]() noexcept
		{
			HIMC current = ImmGetContext(hwnd);
			const bool matches = current == saved;
			if (current)
				ImmReleaseContext(hwnd, current);
			return matches;
		};

		HIMC displaced = ImmAssociateContext(hwnd, saved);
		if (displaced && displaced != saved)
		{
			// 门控分离期间出现了另一个上下文；保留现有关联，不能抢占其所有权或泄漏状态。
			(void)ImmAssociateContext(hwnd, displaced);
			inputContextRestoreFailures_.fetch_add(1, std::memory_order_relaxed);
			return false;
		}
		bool restored = verifySaved();
		if (!restored)
		{
			// 与候选窗关闭的回退策略一致：用同一个已保存上下文重试一次，
			// 仍失败则让诊断层明确观察到。
			(void)ImmAssociateContext(hwnd, saved);
			restored = verifySaved();
		}
		if (!restored)
		{
			inputContextRestoreFailures_.fetch_add(1, std::memory_order_relaxed);
			return false;
		}

		detachedInputContext_ = nullptr;
		inputContextDetached_.store(false, std::memory_order_release);
		inputContextRestoreSuccesses_.fetch_add(1, std::memory_order_relaxed);
		return true;
	}

	bool ImeCaretAnchorContext::synchronizeInputContextAssociation(
		HWND hwnd, bool forceRestore) noexcept
	{
		const std::uint64_t generation = inputContextGateGeneration_.load(
			std::memory_order_acquire);
		if (!forceRestore && generation == appliedInputContextGateGeneration_)
			return true;

		const bool gateEnabled = inputContextGateMode_.load(
			std::memory_order_acquire) ==
			ImeInputContextGateMode::TextInputEligibilityGate;
		const bool shouldAssociate = forceRestore || !gateEnabled ||
			textInputEligible_.load(std::memory_order_acquire);
		const bool succeeded = shouldAssociate
			? restoreInputContext(hwnd)
			: detachInputContext(hwnd);
		if (succeeded)
			appliedInputContextGateGeneration_ = generation;
		return succeeded;
	}

	bool ImeCaretAnchorContext::restoreInputContextBeforeWindowTeardown(
		HWND hwnd) noexcept
	{
		if (!inputContextDetached_.load(std::memory_order_acquire))
			return true;
		if (!hwnd)
			return false;

		// 销毁过程必须退出文字输入资格门控。这里不能调用测试 setter：它会投递
		// 异步刷新，而刷新消息可能晚于即将移除的子类化过程。
		const auto previousMode = inputContextGateMode_.exchange(
			ImeInputContextGateMode::LegacyAlwaysAssociated,
			std::memory_order_acq_rel);
		if (previousMode != ImeInputContextGateMode::LegacyAlwaysAssociated)
		{
			inputContextGateGeneration_.fetch_add(1,
				std::memory_order_acq_rel);
			inputContextGateTransitions_.fetch_add(1,
				std::memory_order_relaxed);
		}

		DWORD processId = 0;
		const DWORD ownerThread = GetWindowThreadProcessId(hwnd, &processId);
		if (ownerThread == 0)
			return false;
		if (ownerThread == GetCurrentThreadId())
			return synchronizeInputContextAssociation(hwnd, true);

		// WM_NULL 已足够：WndProcThunk 会在分发任何消息前应用待处理的关联状态。
		// 跨线程等待必须有上限，避免 EasyX 属主线程已不可用时销毁永久死锁。
		DWORD_PTR ignored = 0;
		const LRESULT sent = SendMessageTimeoutW(
			hwnd, WM_NULL, 0, 0,
			SMTO_ABORTIFHUNG | SMTO_BLOCK,
			1000, &ignored);
		return sent != 0 &&
			!inputContextDetached_.load(std::memory_order_acquire);
	}

	void ImeCaretAnchorContext::clearProcessKeyActivity(
		std::uint32_t expected) noexcept
	{
		if (expected == 0)
		{
			expected = activeProcessKeyToken_.exchange(
				0, std::memory_order_acq_rel);
			if (expected != 0)
				processKeyActivityClears_.fetch_add(1,
					std::memory_order_relaxed);
			if (activeCompositionEpoch_.load(std::memory_order_acquire) == 0)
			{
				activeImeTarget_ = {};
				reopenIdlePrepositionIfInactive();
			}
			return;
		}
		if (activeProcessKeyToken_.compare_exchange_strong(
			expected, 0, std::memory_order_acq_rel,
			std::memory_order_acquire))
		{
			processKeyActivityClears_.fetch_add(1, std::memory_order_relaxed);
			if (activeCompositionEpoch_.load(std::memory_order_acquire) == 0)
			{
				activeImeTarget_ = {};
				reopenIdlePrepositionIfInactive();
			}
		}
	}

	void ImeCaretAnchorContext::closeActiveImePositioning() noexcept
	{
		idlePrepositionAllowed_.store(false, std::memory_order_release);
	}

	void ImeCaretAnchorContext::reopenIdlePrepositionIfInactive() noexcept
	{
		if (activeCompositionEpoch_.load(std::memory_order_acquire) == 0 &&
			activeProcessKeyToken_.load(std::memory_order_acquire) == 0)
		{
			activeImeTarget_ = {};
			idlePrepositionAllowed_.store(true, std::memory_order_release);
		}
	}

	void ImeCaretAnchorContext::captureActiveImeTarget() noexcept
	{
		ImeCaretAnchorSnapshot snapshot;
		if (published_.tryRead(snapshot, WndProcReadAttempts) && snapshot.valid)
			activeImeTarget_ = snapshot.target;
		else
			activeImeTarget_ = {};
	}

	std::uint64_t ImeCaretAnchorContext::packCancelCredential(
		const CancelCredential& credential) noexcept
	{
		return (static_cast<std::uint64_t>(credential.kind) << 32) |
			credential.value;
	}

	ImeCaretAnchorContext::CancelCredential
		ImeCaretAnchorContext::unpackCancelCredential(
			std::uint64_t packed) noexcept
	{
		CancelCredential credential;
		credential.kind = static_cast<ImeCancelEvidenceKind>(packed >> 32);
		credential.value = static_cast<std::uint32_t>(packed & 0xFFFFFFFFull);
		return credential;
	}

	std::uint32_t ImeCaretAnchorContext::beginCompositionEpoch() noexcept
	{
		++nextCompositionEpoch_;
		if (nextCompositionEpoch_ == 0)
			++nextCompositionEpoch_;
		closeActiveImePositioning();
		activeCompositionEpoch_.store(nextCompositionEpoch_,
			std::memory_order_release);
		captureActiveImeTarget();
		return nextCompositionEpoch_;
	}

	bool ImeCaretAnchorContext::positionLatest(HWND hwnd,
		bool allowIdlePreposition) noexcept
	{
		positionRequests_.fetch_add(1, std::memory_order_relaxed);
		ImeCaretAnchorSnapshot snapshot;
		if (!tryReadPublished(snapshot, WndProcReadAttempts))
		{
			snapshotReadFailures_.fetch_add(1, std::memory_order_relaxed);
			return false;
		}
		if (!snapshot.valid)
		{
			invalidAnchorSkips_.fetch_add(1, std::memory_order_relaxed);
			return false;
		}
		const CancelCredential credential = currentCancelCredential();
		bool idlePreposition = false;
		if (credential)
		{
			if (!activeImeTarget_ || snapshot.target != activeImeTarget_)
			{
				inactiveTargetPositionSkips_.fetch_add(1,
					std::memory_order_relaxed);
				return false;
			}
		}
		else if (allowIdlePreposition)
		{
			if (!idlePrepositionAllowed_.load(std::memory_order_acquire))
			{
				idlePrepositionBlocked_.fetch_add(1, std::memory_order_relaxed);
				return false;
			}
			idlePreposition = true;
			idlePrepositionAttempts_.fetch_add(1, std::memory_order_relaxed);
		}
		else
		{
			inactiveTargetPositionSkips_.fetch_add(1,
				std::memory_order_relaxed);
			return false;
		}
		auto finishIdlePreposition = [this, idlePreposition](bool success) noexcept
		{
			if (idlePreposition)
			{
				if (success)
					idlePrepositionSuccesses_.fetch_add(
						1, std::memory_order_relaxed);
				else
					idlePrepositionFailures_.fetch_add(
						1, std::memory_order_relaxed);
			}
			return success;
		};
		if (!hwnd)
		{
			missingWindowSkips_.fetch_add(1, std::memory_order_relaxed);
			return finishIdlePreposition(false);
		}

		HIMC context = ImmGetContext(hwnd);
		if (!context)
		{
			missingContextSkips_.fetch_add(1, std::memory_order_relaxed);
			return finishIdlePreposition(false);
		}

		COMPOSITIONFORM composition = compositionPositionForm(snapshot);
		const bool compositionSucceeded =
			ImmSetCompositionWindow(context, &composition) != FALSE;

		CANDIDATEFORM candidate = candidatePositionForm(snapshot,
			candidatePositionMode_.load(std::memory_order_acquire));
		const bool candidateSucceeded =
			ImmSetCandidateWindow(context, &candidate) != FALSE;
		ImmReleaseContext(hwnd, context);

		if (compositionSucceeded)
			compositionSuccesses_.fetch_add(1, std::memory_order_relaxed);
		if (candidateSucceeded)
			candidateSuccesses_.fetch_add(1, std::memory_order_relaxed);
		if (compositionSucceeded || candidateSucceeded)
			lastPositioned_.publish(snapshot);
		return finishIdlePreposition(compositionSucceeded && candidateSucceeded);
	}

	ImeCaretWndProcResult ImeCaretAnchorContext::onWndProcMessage(
		HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
		bool compositionEpochPrepared) noexcept
	{
		ImeCaretWndProcResult result;
		if (message == WM_KILLFOCUS ||
			(message == WM_IME_SETCONTEXT && wParam == FALSE))
		{
			compositionRoundActive_ = false;
			firstCompositionHandled_ = false;
			inlineCompositionRoundActive_ = false;
			activeCompositionEpoch_.store(0, std::memory_order_release);
			clearProcessKeyActivity();
		}
		else if (message == WM_CHAR || message == WM_IME_CHAR ||
			(message == WM_UNICHAR && wParam != UNICODE_NOCHAR))
		{
			// 一个已交付字符会关闭无生命周期的单键证据；标准组合轮次仍由独立 epoch 管理。
			clearProcessKeyActivity();
		}
		const UINT cancelMessage = compositionCancelMessage();
		if (cancelMessage != 0 && message == cancelMessage)
		{
			result.handledSynchronously = true;
			cancelMessages_.fetch_add(1, std::memory_order_relaxed);
			CancelCredential credential;
			credential.kind = static_cast<ImeCancelEvidenceKind>(lParam);
			credential.value = static_cast<std::uint32_t>(wParam);
			const std::uint64_t packed = packCancelCredential(credential);
			std::uint64_t pending = packed;
			(void)cancelPendingCredential_.compare_exchange_strong(
				pending, 0, std::memory_order_acq_rel,
				std::memory_order_acquire);
			if (!isCurrentCancelCredential(credential))
			{
				staleCancelMessages_.fetch_add(1, std::memory_order_relaxed);
				return result;
			}
			(void)performCompositionCancel(hwnd, credential, false);
			return result;
		}
		if (message == WM_IME_REQUEST && wParam == IMR_COMPOSITIONWINDOW &&
			lParam != 0 && (inlineCompositionRoundActive_ || inlineCompositionOwned()))
		{
			ImeCaretAnchorSnapshot snapshot;
			if (tryReadPublished(snapshot, WndProcReadAttempts) && snapshot.valid)
			{
				*reinterpret_cast<COMPOSITIONFORM*>(lParam) =
					compositionPositionForm(snapshot);
				compositionRequestReplies_.fetch_add(1, std::memory_order_relaxed);
				result.handledSynchronously = true;
				result.result = TRUE;
				return result;
			}
		}
		const UINT refresh = refreshMessage();
		if (refresh != 0 && message == refresh)
		{
			// 读取前先清除 pending：并发宿主发布要么被本次读取看到，要么观察到 false
			// 并再投递一条刷新消息。
			refreshPending_.store(false, std::memory_order_release);
			refreshMessages_.fetch_add(1, std::memory_order_relaxed);
			positionLatest(hwnd, true);
			result.handledSynchronously = true;
			return result;
		}

		if (message == WM_IME_STARTCOMPOSITION)
		{
			compositionRoundActive_ = true;
			firstCompositionHandled_ = false;
			if (!compositionEpochPrepared)
				beginCompositionEpoch();
			imeStartMessages_.fetch_add(1, std::memory_order_relaxed);
			positionLatest(hwnd);
			if (inlineCompositionOwned())
			{
				inlineCompositionRoundActive_ = true;
				suppressedCompositionMessages_.fetch_add(1,
					std::memory_order_relaxed);
				result.handledSynchronously = true;
			}
		}
		else if (message == WM_IME_COMPOSITION && !firstCompositionHandled_)
		{
			// 没有 START 的 COMPOSITION 会建立惰性轮次，以兼容省略或重排生命周期通知的输入法。
			if (!compositionRoundActive_)
				beginCompositionEpoch();
			compositionRoundActive_ = true;
			firstCompositionHandled_ = true;
			firstCompositionMessages_.fetch_add(1, std::memory_order_relaxed);
			positionLatest(hwnd);
			if (inlineCompositionRoundActive_ || inlineCompositionOwned())
			{
				inlineCompositionRoundActive_ = true;
				suppressedCompositionMessages_.fetch_add(1,
					std::memory_order_relaxed);
				result.handledSynchronously = true;
			}
		}
		else if (message == WM_IME_COMPOSITION && inlineCompositionRoundActive_)
		{
			suppressedCompositionMessages_.fetch_add(1,
				std::memory_order_relaxed);
			result.handledSynchronously = true;
		}
		else if (message == WM_IME_ENDCOMPOSITION)
		{
			const bool suppress = inlineCompositionRoundActive_ ||
				inlineCompositionOwned();
			compositionRoundActive_ = false;
			firstCompositionHandled_ = false;
			inlineCompositionRoundActive_ = false;
			activeCompositionEpoch_.store(0, std::memory_order_release);
			clearProcessKeyActivity();
			imeEndMessages_.fetch_add(1, std::memory_order_relaxed);
			if (suppress)
			{
				suppressedCompositionMessages_.fetch_add(1,
					std::memory_order_relaxed);
				result.handledSynchronously = true;
			}
		}
		return result;
	}

	ImeCaretAnchorRuntimeSnapshot ImeCaretAnchorContext::inspectCrossThread() const noexcept
	{
		ImeCaretAnchorRuntimeSnapshot snapshot;
		published_.tryRead(snapshot.published);
		lastPositioned_.tryRead(snapshot.lastPositioned);
		snapshot.windowActive = windowActive_.load(std::memory_order_acquire);
		snapshot.refreshPending = refreshPending_.load(std::memory_order_acquire);
		snapshot.inlineCompositionOwned = inlineCompositionOwned();
		snapshot.idlePrepositionAllowed = idlePrepositionAllowed_.load(
			std::memory_order_acquire);
		snapshot.candidatePositionMode = candidatePositionMode_.load(
			std::memory_order_acquire);
		snapshot.candidateDismissMode = candidateDismissMode_.load(
			std::memory_order_acquire);
		snapshot.inputContextGateMode = inputContextGateMode_.load(
			std::memory_order_acquire);
		snapshot.textInputEligible = textInputEligible_.load(
			std::memory_order_acquire);
		snapshot.inputContextDetached = inputContextDetached_.load(
			std::memory_order_acquire);
		snapshot.activeCompositionEpoch = activeCompositionEpoch_.load(
			std::memory_order_acquire);
		snapshot.activeProcessKeyToken = activeProcessKeyToken_.load(
			std::memory_order_acquire);
		const CancelCredential pending = unpackCancelCredential(
			cancelPendingCredential_.load(std::memory_order_acquire));
		snapshot.cancelPendingKind = pending.kind;
		snapshot.cancelPendingValue = pending.value;
		snapshot.stats.publications = publications_.load(std::memory_order_relaxed);
		snapshot.stats.invalidPublications = invalidPublications_.load(std::memory_order_relaxed);
		snapshot.stats.refreshPosts = refreshPosts_.load(std::memory_order_relaxed);
		snapshot.stats.refreshPostFailures = refreshPostFailures_.load(std::memory_order_relaxed);
		snapshot.stats.refreshMessages = refreshMessages_.load(std::memory_order_relaxed);
		snapshot.stats.imeStartMessages = imeStartMessages_.load(std::memory_order_relaxed);
		snapshot.stats.firstCompositionMessages = firstCompositionMessages_.load(std::memory_order_relaxed);
		snapshot.stats.imeEndMessages = imeEndMessages_.load(std::memory_order_relaxed);
		snapshot.stats.positionRequests = positionRequests_.load(std::memory_order_relaxed);
		snapshot.stats.inactiveTargetPositionSkips =
			inactiveTargetPositionSkips_.load(std::memory_order_relaxed);
		snapshot.stats.invalidAnchorSkips = invalidAnchorSkips_.load(std::memory_order_relaxed);
		snapshot.stats.snapshotReadFailures = snapshotReadFailures_.load(std::memory_order_relaxed);
		snapshot.stats.missingWindowSkips = missingWindowSkips_.load(std::memory_order_relaxed);
		snapshot.stats.missingContextSkips = missingContextSkips_.load(std::memory_order_relaxed);
		snapshot.stats.compositionSuccesses = compositionSuccesses_.load(std::memory_order_relaxed);
		snapshot.stats.candidateSuccesses = candidateSuccesses_.load(std::memory_order_relaxed);
		snapshot.stats.suppressedCompositionMessages =
			suppressedCompositionMessages_.load(std::memory_order_relaxed);
		snapshot.stats.compositionRequestReplies =
			compositionRequestReplies_.load(std::memory_order_relaxed);
		snapshot.stats.cancelRequests = cancelRequests_.load(std::memory_order_relaxed);
		snapshot.stats.cancelNoEvidenceSkips =
			cancelNoEvidenceSkips_.load(std::memory_order_relaxed);
		snapshot.stats.lifecycleEvidenceCancels =
			lifecycleEvidenceCancels_.load(std::memory_order_relaxed);
		snapshot.stats.processKeyEvidenceCancels =
			processKeyEvidenceCancels_.load(std::memory_order_relaxed);
		snapshot.stats.cancelRequestPosts = cancelRequestPosts_.load(std::memory_order_relaxed);
		snapshot.stats.cancelRequestPostFailures =
			cancelRequestPostFailures_.load(std::memory_order_relaxed);
		snapshot.stats.cancelRequestCoalesces =
			cancelRequestCoalesces_.load(std::memory_order_relaxed);
		snapshot.stats.cancelMessages = cancelMessages_.load(std::memory_order_relaxed);
		snapshot.stats.staleCancelMessages =
			staleCancelMessages_.load(std::memory_order_relaxed);
		snapshot.stats.cancelMissingWindowSkips =
			cancelMissingWindowSkips_.load(std::memory_order_relaxed);
		snapshot.stats.cancelMissingContextSkips =
			cancelMissingContextSkips_.load(std::memory_order_relaxed);
		snapshot.stats.compositionCancelSuccesses =
			compositionCancelSuccesses_.load(std::memory_order_relaxed);
		snapshot.stats.compositionCancelFailures =
			compositionCancelFailures_.load(std::memory_order_relaxed);
		snapshot.stats.candidateCloseAttempts =
			candidateCloseAttempts_.load(std::memory_order_relaxed);
		snapshot.stats.candidateCloseSuccesses =
			candidateCloseSuccesses_.load(std::memory_order_relaxed);
		snapshot.stats.candidateCloseFailures =
			candidateCloseFailures_.load(std::memory_order_relaxed);
		snapshot.stats.contextReassociateAttempts =
			contextReassociateAttempts_.load(std::memory_order_relaxed);
		snapshot.stats.contextDetachSuccesses =
			contextDetachSuccesses_.load(std::memory_order_relaxed);
		snapshot.stats.contextDetachFailures =
			contextDetachFailures_.load(std::memory_order_relaxed);
		snapshot.stats.contextRestoreSuccesses =
			contextRestoreSuccesses_.load(std::memory_order_relaxed);
		snapshot.stats.contextRestoreFailures =
			contextRestoreFailures_.load(std::memory_order_relaxed);
		snapshot.stats.cancelSuccesses = cancelSuccesses_.load(std::memory_order_relaxed);
		snapshot.stats.cancelFailures = cancelFailures_.load(std::memory_order_relaxed);
		snapshot.stats.processKeyActivityUpdates =
			processKeyActivityUpdates_.load(std::memory_order_relaxed);
		snapshot.stats.processKeyActivityClears =
			processKeyActivityClears_.load(std::memory_order_relaxed);
		snapshot.stats.processKeyPrepositionAttempts =
			processKeyPrepositionAttempts_.load(std::memory_order_relaxed);
		snapshot.stats.processKeyPrepositionSuccesses =
			processKeyPrepositionSuccesses_.load(std::memory_order_relaxed);
		snapshot.stats.processKeyPrepositionFailures =
			processKeyPrepositionFailures_.load(std::memory_order_relaxed);
		snapshot.stats.idlePrepositionAttempts =
			idlePrepositionAttempts_.load(std::memory_order_relaxed);
		snapshot.stats.idlePrepositionSuccesses =
			idlePrepositionSuccesses_.load(std::memory_order_relaxed);
		snapshot.stats.idlePrepositionFailures =
			idlePrepositionFailures_.load(std::memory_order_relaxed);
		snapshot.stats.idlePrepositionBlocked =
			idlePrepositionBlocked_.load(std::memory_order_relaxed);
		snapshot.stats.pointerDepartureChecks =
			pointerDepartureChecks_.load(std::memory_order_relaxed);
		snapshot.stats.pointerDepartureSnapshotReadFailures =
			pointerDepartureSnapshotReadFailures_.load(std::memory_order_relaxed);
		snapshot.stats.pointerDepartureInvalidAnchorSkips =
			pointerDepartureInvalidAnchorSkips_.load(std::memory_order_relaxed);
		snapshot.stats.pointerDepartureInsideSkips =
			pointerDepartureInsideSkips_.load(std::memory_order_relaxed);
		snapshot.stats.pointerDepartureNoEvidenceSkips =
			pointerDepartureNoEvidenceSkips_.load(std::memory_order_relaxed);
		snapshot.stats.pointerDepartureCancelAttempts =
			pointerDepartureCancelAttempts_.load(std::memory_order_relaxed);
		snapshot.stats.pointerDepartureCancelSuccesses =
			pointerDepartureCancelSuccesses_.load(std::memory_order_relaxed);
		snapshot.stats.pointerDepartureCancelFailures =
			pointerDepartureCancelFailures_.load(std::memory_order_relaxed);
		snapshot.stats.inputContextGateTransitions =
			inputContextGateTransitions_.load(std::memory_order_relaxed);
		snapshot.stats.inputContextDetachAttempts =
			inputContextDetachAttempts_.load(std::memory_order_relaxed);
		snapshot.stats.inputContextDetachSuccesses =
			inputContextDetachSuccesses_.load(std::memory_order_relaxed);
		snapshot.stats.inputContextDetachFailures =
			inputContextDetachFailures_.load(std::memory_order_relaxed);
		snapshot.stats.inputContextRestoreAttempts =
			inputContextRestoreAttempts_.load(std::memory_order_relaxed);
		snapshot.stats.inputContextRestoreSuccesses =
			inputContextRestoreSuccesses_.load(std::memory_order_relaxed);
		snapshot.stats.inputContextRestoreFailures =
			inputContextRestoreFailures_.load(std::memory_order_relaxed);
		snapshot.stats.inputContextAlreadyAbsentSkips =
			inputContextAlreadyAbsentSkips_.load(std::memory_order_relaxed);
		return snapshot;
	}

	ImeCaretAnchorRuntimeSnapshot ImeCaretAnchorContext::inspect() const noexcept
	{
		auto snapshot = inspectCrossThread();
		snapshot.inlineCompositionRoundActive = inlineCompositionRoundActive_;
		return snapshot;
	}
}
