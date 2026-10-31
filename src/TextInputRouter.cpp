#include "internal/SxTextInputRouterInternal.h"

#include "FocusManager.h"
#include "Window.h"
#include "internal/SxUnicodeInternal.h"

#include <variant>

namespace
{
	class ManagedTextInputDispatchGuard
	{
		Window& window;
		bool& managedDispatchActive;
		bool previousManagedDispatchActive = false;
	public:
		ManagedTextInputDispatchGuard(Window& owner, bool& dispatchFlag)
			: window(owner), managedDispatchActive(dispatchFlag),
			previousManagedDispatchActive(dispatchFlag)
		{
			// 文本回调可以修改控件树；投递期间延迟所有权变更，并保留可能存在的
			// 外层托管分发状态。
			window.beginMutationScope();
			managedDispatchActive = true;
		}

		~ManagedTextInputDispatchGuard()
		{
			managedDispatchActive = previousManagedDispatchActive;
			window.endMutationScope();
		}
	};
}

namespace StellarX::Internal
{
	TextInputRouteDispatchResult TextInputRouter::route(
		::Window& window, const NativeInputEvent& nativeEvent,
		const TextInputRoutingEnvironment& environment)
	{
		TextInputRouteDispatchResult outcome;
		const auto* text = std::get_if<TextInputEvent>(&nativeEvent.payload);
		if (!text)
			return outcome;

		outcome.textEvent = true;
		++window.textInputRoutingStats.observedEvents;
		auto drop = [&](TextInputDropReason reason)
		{
			outcome.dropReason = reason;
			++window.textInputRoutingStats.droppedEvents;
			window.textInputRoutingStats.droppedCodeUnits +=
				static_cast<std::uint64_t>(text->text.size());
			return outcome;
		};

		if (!nativeEvent.activationAtCapture.keyboardFocusActive ||
			!environment.currentActivation.keyboardFocusActive)
			return drop(TextInputDropReason::InactiveWindow);
		if (nativeEvent.activationAtCapture.generation == 0 ||
			environment.currentActivation.generation == 0 ||
			nativeEvent.activationAtCapture.generation !=
				environment.currentActivation.generation)
			return drop(TextInputDropReason::StaleActivation);
		if (environment.modalBusinessSuppressed)
			return drop(TextInputDropReason::ModalSuppressed);
		// GCS_RESULTSTR 必须携带采集器分配的非零组合身份。WM_IME_CHAR 回退路径
		// 可以合法地用非 ImeResult 来源携带 compositionId，因此不能一并拒绝。
		if (text->source == TextSource::ImeResult && text->compositionId == 0)
			return drop(TextInputDropReason::InvalidImeOwnership);
		if (text->source == TextSource::Clipboard || text->text.empty() ||
			!isValidUtf16(text->text))
			return drop(TextInputDropReason::InvalidText);
		FocusManager& focus = window.getFocusManager();
		const bool imeResult = text->source == TextSource::ImeResult;
		if (imeResult)
		{
			const auto& session = window.compositionRouteSession;
			if (session && session.compositionId == text->compositionId &&
				session.focusRetired)
			{
				return drop(TextInputDropReason::RetiredImeSession);
			}
			if (!session || session.compositionId != text->compositionId ||
				!session.committed || session.resultDelivered ||
				session.activationGeneration != nativeEvent.activationAtCapture.generation)
			{
				return drop(TextInputDropReason::InvalidImeOwnership);
			}
			outcome.originalTarget = session.target;
			outcome.originalScope = session.scope;
		}
		else
		{
			// 在任何用户回调前冻结唯一合格的身份和作用域；回调造成的变化只影响下一事件。
			outcome.originalTarget = focus.focusedTarget();
			outcome.originalScope = focus.activeScope();
		}
		if (!outcome.originalTarget)
			return drop(TextInputDropReason::NoFocusedTarget);

		FocusTargetSnapshot snapshot;
		if (!focus.querySnapshot(outcome.originalTarget, snapshot) ||
			focus.isUnderInvalidatedSubtree(outcome.originalTarget) ||
			focus.focusedTarget() != outcome.originalTarget)
			return drop(TextInputDropReason::InvalidTarget);
		if (!snapshot.visible)
			return drop(TextInputDropReason::NotVisible);
		if (!snapshot.enabled || !snapshot.focusable)
			return drop(TextInputDropReason::Disabled);
		if (focus.activeScope() != outcome.originalScope ||
			snapshot.scope != outcome.originalScope ||
			!focus.isWithinActiveScope(outcome.originalTarget))
			return drop(TextInputDropReason::OutsideActiveScope);
		if (!snapshot.acceptsTextInput)
			return drop(TextInputDropReason::NotTextInputCapable);

		ManagedTextInputDispatchGuard dispatchGuard(
			window, window.managedDispatchActive);
		RoutedTextInputEvent routed;
		routed.sequence = nativeEvent.sequence;
		routed.messageTime = nativeEvent.messageTime;
		routed.hwnd = nativeEvent.hwnd;
		routed.text = text->text;
		routed.source = text->source;
		routed.compositionId = text->compositionId;
		routed.target = outcome.originalTarget;
		if (!window.dispatchTextInputEventToTarget(outcome.originalTarget, routed))
			return drop(TextInputDropReason::DispatchTargetMissing);

		outcome.delivered = true;
		if (imeResult && window.compositionRouteSession.compositionId ==
			text->compositionId)
		{
			window.compositionRouteSession.resultDelivered = true;
		}
		++window.textInputRoutingStats.deliveredEvents;
		return outcome;
	}

	TextInputRoutingStats TextInputRouter::stats(const ::Window& window) noexcept
	{
		return window.textInputRoutingStats;
	}

	CompositionRouteDispatchResult CompositionRouter::route(
		::Window& window, const NativeInputEvent& nativeEvent,
		const CompositionRoutingEnvironment& environment)
	{
		CompositionRouteDispatchResult outcome;
		const auto* composition = std::get_if<CompositionEvent>(&nativeEvent.payload);
		if (!composition)
			return outcome;
		outcome.compositionEvent = true;
		++window.compositionRoutingStats.observedEvents;

		auto drop = [&](CompositionDropReason reason)
		{
			outcome.dropReason = reason;
			++window.compositionRoutingStats.droppedEvents;
			return outcome;
		};
		if (composition->compositionId == 0 ||
			!isValidUtf16(composition->compositionText))
		{
			return drop(CompositionDropReason::InvalidPayload);
		}

		FocusManager& focus = window.getFocusManager();
		auto dispatchBound = [&](const CompositionEvent& payload)
		{
			RoutedCompositionEvent routed;
			routed.sequence = nativeEvent.sequence;
			routed.messageTime = nativeEvent.messageTime;
			routed.hwnd = nativeEvent.hwnd;
			routed.stage = payload.stage;
			routed.compositionId = payload.compositionId;
			routed.compositionText = payload.compositionText;
			routed.caretOffset = payload.caretOffset;
			routed.target = window.compositionRouteSession.target;
			ManagedTextInputDispatchGuard dispatchGuard(
				window, window.managedDispatchActive);
			return window.dispatchCompositionEventToTarget(routed.target, routed);
		};
		auto resetSession = [&]() noexcept
		{
			window.compositionRouteSession = {};
		};

		const bool terminal = composition->stage == CompositionStage::Cancel ||
			composition->stage == CompositionStage::End;
		if (terminal)
		{
			// Cancel/End 只收口已经绑定的同一会话，即使焦点资格随后变化，
			// 也不能把终止事件改投给新获得焦点的控件。
			if (!window.compositionRouteSession ||
				window.compositionRouteSession.compositionId !=
					composition->compositionId)
			{
				return drop(CompositionDropReason::InvalidSession);
			}
			if (window.compositionRouteSession.focusRetired)
			{
				resetSession();
				return drop(CompositionDropReason::RetiredSession);
			}
			outcome.originalTarget = window.compositionRouteSession.target;
			outcome.originalScope = window.compositionRouteSession.scope;
			outcome.delivered = dispatchBound(*composition);
			resetSession();
			if (!outcome.delivered)
				return drop(CompositionDropReason::DispatchTargetMissing);
			++window.compositionRoutingStats.deliveredEvents;
			return outcome;
		}

		if (!nativeEvent.activationAtCapture.keyboardFocusActive ||
			!environment.currentActivation.keyboardFocusActive)
			return drop(CompositionDropReason::InactiveWindow);
		if (nativeEvent.activationAtCapture.generation == 0 ||
			environment.currentActivation.generation == 0 ||
			nativeEvent.activationAtCapture.generation !=
				environment.currentActivation.generation)
		{
			return drop(CompositionDropReason::StaleActivation);
		}
		if (environment.modalBusinessSuppressed)
			return drop(CompositionDropReason::ModalSuppressed);

		const bool matchingSession = window.compositionRouteSession &&
			window.compositionRouteSession.compositionId == composition->compositionId;
		if (matchingSession && window.compositionRouteSession.focusRetired)
			return drop(CompositionDropReason::RetiredSession);
		const bool needsBinding = composition->stage == CompositionStage::Start ||
			(composition->stage == CompositionStage::Update && !matchingSession);
		if (needsBinding)
		{
			if (window.compositionRouteSession && !matchingSession)
			{
				// 新组合身份到来前先显式取消旧会话，避免两个控件同时保留组合外观。
				if (!window.compositionRouteSession.focusRetired)
				{
					CompositionEvent cancel;
					cancel.stage = CompositionStage::Cancel;
					cancel.compositionId = window.compositionRouteSession.compositionId;
					(void)dispatchBound(cancel);
				}
				resetSession();
			}

			const FocusTargetId target = focus.focusedTarget();
			if (!target)
				return drop(CompositionDropReason::NoFocusedTarget);
			FocusTargetSnapshot snapshot;
			if (!focus.querySnapshot(target, snapshot) ||
				focus.isUnderInvalidatedSubtree(target) ||
				focus.focusedTarget() != target || !snapshot.visible ||
				!snapshot.enabled || !snapshot.focusable)
			{
				return drop(CompositionDropReason::InvalidTarget);
			}
			const FocusScopeId scope = focus.activeScope();
			if (snapshot.scope != scope || !focus.isWithinActiveScope(target))
				return drop(CompositionDropReason::OutsideActiveScope);
			if (!snapshot.acceptsTextInput || !snapshot.supportsInlineComposition)
				return drop(CompositionDropReason::NotCompositionCapable);

			window.compositionRouteSession.compositionId = composition->compositionId;
			window.compositionRouteSession.target = target;
			window.compositionRouteSession.scope = scope;
			window.compositionRouteSession.activationGeneration =
				nativeEvent.activationAtCapture.generation;
			window.compositionRouteSession.committed = false;
			window.compositionRouteSession.resultDelivered = false;
			window.compositionRouteSession.focusRetired = false;
			outcome.lazyBinding = composition->stage == CompositionStage::Update;
			if (outcome.lazyBinding)
			{
				// 部分输入法不会发送 START；首次 UPDATE 需补一条 Start，
				// 让控件仍能看到完整的组合生命周期。
				++window.compositionRoutingStats.lazyBindings;
				CompositionEvent start;
				start.stage = CompositionStage::Start;
				start.compositionId = composition->compositionId;
				if (!dispatchBound(start))
				{
					resetSession();
					return drop(CompositionDropReason::DispatchTargetMissing);
				}
			}
		}

		if (!window.compositionRouteSession ||
			window.compositionRouteSession.compositionId != composition->compositionId)
		{
			return drop(CompositionDropReason::InvalidSession);
		}
		if (window.compositionRouteSession.focusRetired)
			return drop(CompositionDropReason::RetiredSession);
		outcome.originalTarget = window.compositionRouteSession.target;
		outcome.originalScope = window.compositionRouteSession.scope;
		FocusTargetSnapshot snapshot;
		if (focus.focusedTarget() != outcome.originalTarget ||
			!focus.querySnapshot(outcome.originalTarget, snapshot) ||
			focus.isUnderInvalidatedSubtree(outcome.originalTarget) ||
			!snapshot.visible || !snapshot.enabled || !snapshot.focusable)
		{
			resetSession();
			return drop(CompositionDropReason::InvalidTarget);
		}
		if (focus.activeScope() != outcome.originalScope ||
			snapshot.scope != outcome.originalScope ||
			!focus.isWithinActiveScope(outcome.originalTarget))
		{
			resetSession();
			return drop(CompositionDropReason::OutsideActiveScope);
		}
		if (!snapshot.acceptsTextInput || !snapshot.supportsInlineComposition)
		{
			resetSession();
			return drop(CompositionDropReason::NotCompositionCapable);
		}

		// 先标记已提交，再调用目标；若回调期间同步到达结果文本，所有权校验
		// 已能识别这次提交。Commit 本身不写入文本，结果由 TextInputEvent 交付。
		if (composition->stage == CompositionStage::Commit)
			window.compositionRouteSession.committed = true;
		outcome.delivered = dispatchBound(*composition);
		if (!outcome.delivered)
		{
			resetSession();
			return drop(CompositionDropReason::DispatchTargetMissing);
		}
		++window.compositionRoutingStats.deliveredEvents;
		return outcome;
	}

	void CompositionRouter::cancelForDeactivation(
		::Window& window, const NativeInputEvent& nativeEvent)
	{
		if (!window.compositionRouteSession)
			return;
		RoutedCompositionEvent routed;
		routed.sequence = nativeEvent.sequence;
		routed.messageTime = nativeEvent.messageTime;
		routed.hwnd = nativeEvent.hwnd;
		routed.stage = CompositionStage::Cancel;
		routed.compositionId = window.compositionRouteSession.compositionId;
		routed.target = window.compositionRouteSession.target;
		const FocusTargetId target = routed.target;
		// 回调前先退休会话，防止重入路径把失活前的组合继续当作活动状态。
		window.compositionRouteSession = {};
		ManagedTextInputDispatchGuard dispatchGuard(
			window, window.managedDispatchActive);
		(void)window.dispatchCompositionEventToTarget(target, routed);
	}

	CompositionRoutingStats CompositionRouter::stats(const ::Window& window) noexcept
	{
		return window.compositionRoutingStats;
	}
}
