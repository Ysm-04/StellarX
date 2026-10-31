#include "internal/SxTextSelectionGestureInternal.h"

#include "FocusManager.h"
#include "Window.h"

#include <variant>

namespace
{
	class ManagedTextSelectionDispatchGuard
	{
		Window& window_;
		bool& managedDispatchActive_;
		bool previousManagedDispatchActive_ = false;
	public:
		ManagedTextSelectionDispatchGuard(Window& window, bool& dispatchFlag)
			: window_(window), managedDispatchActive_(dispatchFlag),
			previousManagedDispatchActive_(dispatchFlag)
		{
			// 目标回调可以修改控件树；把整次投递纳入同一个事务，避免遍历中
			// 立即销毁控件，并保留外层嵌套分发状态。
			window_.beginMutationScope();
			managedDispatchActive_ = true;
		}

		~ManagedTextSelectionDispatchGuard()
		{
			managedDispatchActive_ = previousManagedDispatchActive_;
			window_.endMutationScope();
		}
	};

	StellarX::Internal::TextSelectionGestureDropReason validateActivation(
		const StellarX::NativeInputEvent& nativeEvent,
		const StellarX::Internal::TextSelectionGestureEnvironment& environment,
		std::uint64_t expectedGeneration = 0) noexcept
	{
		// 事件只属于采集时的那一代激活窗口。窗口失活又恢复后，旧队列中的
		// 指针事件也不能被当作新会话输入继续使用。
		using Reason = StellarX::Internal::TextSelectionGestureDropReason;
		if (!nativeEvent.activationAtCapture.keyboardFocusActive ||
			!environment.currentActivation.keyboardFocusActive)
			return Reason::InactiveWindow;
		if (nativeEvent.activationAtCapture.generation == 0 ||
			environment.currentActivation.generation == 0 ||
			nativeEvent.activationAtCapture.generation !=
				environment.currentActivation.generation ||
			(expectedGeneration != 0 && expectedGeneration !=
				environment.currentActivation.generation))
			return Reason::StaleActivation;
		if (environment.modalBusinessSuppressed)
			return Reason::ModalSuppressed;
		return Reason::None;
	}
}

namespace StellarX::Internal
{
	void TextSelectionGestureManager::clear() noexcept
	{
		target_ = {};
		startScope_ = {};
		activationGeneration_ = 0;
		active_ = false;
	}

	TextSelectionGestureDropReason
		TextSelectionGestureManager::validateActiveTarget(
			::Window& window,
			const TextSelectionGestureEnvironment& environment) const
	{
		// 跨事件只保存稳定身份；每次继续拖选前都重新解析当前快照，不能沿用
		// 上一次回调前取得的 Control 指针或可见、可用状态。
		if (!active_ || !target_)
			return TextSelectionGestureDropReason::InvalidTarget;
		if (!environment.currentActivation.keyboardFocusActive)
			return TextSelectionGestureDropReason::InactiveWindow;
		if (environment.currentActivation.generation == 0 ||
			environment.currentActivation.generation != activationGeneration_)
			return TextSelectionGestureDropReason::StaleActivation;
		if (environment.modalBusinessSuppressed)
			return TextSelectionGestureDropReason::ModalSuppressed;

		FocusManager& focus = window.getFocusManager();
		if (focus.activeScope() != startScope_)
			return TextSelectionGestureDropReason::OutsideActiveScope;

		FocusTargetSnapshot snapshot;
		if (!focus.querySnapshot(target_, snapshot) ||
			focus.isUnderInvalidatedSubtree(target_))
			return TextSelectionGestureDropReason::InvalidTarget;
		if (focus.focusedTarget() != target_)
			return TextSelectionGestureDropReason::FocusChanged;
		if (!snapshot.visible)
			return TextSelectionGestureDropReason::NotVisible;
		if (!snapshot.enabled || !snapshot.focusable)
			return TextSelectionGestureDropReason::Disabled;
		if (snapshot.scope != startScope_ || !focus.isWithinActiveScope(target_))
			return TextSelectionGestureDropReason::OutsideActiveScope;
		if (!snapshot.supportsTextSelection)
			return TextSelectionGestureDropReason::NotTextSelectionCapable;
		return TextSelectionGestureDropReason::None;
	}

	bool TextSelectionGestureManager::dispatch(
		::Window& window, const NativeInputEvent& nativeEvent,
		const PointerEvent& pointer, TextSelectionGesturePhase phase,
		bool& accepted)
	{
		RoutedTextSelectionGestureEvent routed;
		routed.sequence = nativeEvent.sequence;
		routed.messageTime = nativeEvent.messageTime;
		routed.hwnd = nativeEvent.hwnd;
		routed.phase = phase;
		routed.clientX = pointer.clientX;
		routed.clientY = pointer.clientY;
		routed.clientCoordinatesValid = pointer.clientCoordinatesValid;
		routed.control = pointer.control;
		routed.shift = pointer.shift;
		routed.target = target_;
		return window.dispatchTextSelectionGestureToTarget(
			target_, routed, accepted);
	}

	TextSelectionGestureDispatchResult TextSelectionGestureManager::route(
		::Window& window, const NativeInputEvent& nativeEvent,
		const TextSelectionGestureEnvironment& environment,
		FocusTargetId hitTarget)
	{
		TextSelectionGestureDispatchResult outcome;
		outcome.target = target_;

		if (active_)
		{
			const auto activeReason = validateActiveTarget(window, environment);
			if (activeReason != TextSelectionGestureDropReason::None)
			{
				outcome.cancelled = true;
				outcome.reason = activeReason;
				clear();
				return outcome;
			}
		}

		const auto* pointer = std::get_if<PointerEvent>(&nativeEvent.payload);
		if (!pointer)
			return outcome;
		outcome.pointerEvent = true;

		if (!active_)
		{
			if (pointer->action != PointerAction::LeftDown)
				return outcome;
			outcome.beginAttempted = true;
			outcome.target = hitTarget;

			const auto activationReason = validateActivation(nativeEvent, environment);
			if (activationReason != TextSelectionGestureDropReason::None)
			{
				outcome.reason = activationReason;
				return outcome;
			}
			if (!pointer->clientCoordinatesValid)
			{
				outcome.reason = TextSelectionGestureDropReason::InvalidCoordinates;
				return outcome;
			}
			if (!hitTarget)
			{
				outcome.reason = TextSelectionGestureDropReason::NoHitTarget;
				return outcome;
			}

			FocusManager& focus = window.getFocusManager();
			FocusTargetSnapshot snapshot;
			if (!focus.querySnapshot(hitTarget, snapshot) ||
				focus.isUnderInvalidatedSubtree(hitTarget))
			{
				outcome.reason = TextSelectionGestureDropReason::InvalidTarget;
				return outcome;
			}
			if (!snapshot.visible)
			{
				outcome.reason = TextSelectionGestureDropReason::NotVisible;
				return outcome;
			}
			if (!snapshot.enabled || !snapshot.focusable)
			{
				outcome.reason = TextSelectionGestureDropReason::Disabled;
				return outcome;
			}
			if (!focus.isWithinActiveScope(hitTarget))
			{
				outcome.reason = TextSelectionGestureDropReason::OutsideActiveScope;
				return outcome;
			}
			if (!snapshot.supportsTextSelection)
			{
				outcome.reason =
					TextSelectionGestureDropReason::NotTextSelectionCapable;
				return outcome;
			}

			// 先完成焦点切换，再把 Begin 定向交给命中的文本控件。只有目标明确
			// 接受且回调后仍然有效，手势才进入跨事件的活动状态。
			ManagedTextSelectionDispatchGuard dispatchGuard(
				window, window.managedDispatchActive);
			const auto focusResult = focus.requestFocus(
				hitTarget, FocusReason::Pointer);
			if (focusResult.error != FocusError::None ||
				focus.focusedTarget() != hitTarget)
			{
				outcome.reason = TextSelectionGestureDropReason::FocusRejected;
				return outcome;
			}

			target_ = hitTarget;
			startScope_ = focus.activeScope();
			activationGeneration_ = environment.currentActivation.generation;
			bool accepted = false;
			if (!dispatch(window, nativeEvent, *pointer,
				TextSelectionGesturePhase::Begin, accepted))
			{
				outcome.reason =
					TextSelectionGestureDropReason::DispatchTargetMissing;
				clear();
				return outcome;
			}
			outcome.delivered = true;
			if (!accepted)
			{
				outcome.reason = TextSelectionGestureDropReason::GestureRejected;
				clear();
				return outcome;
			}

			active_ = true;
			const auto freshReason = validateActiveTarget(window, environment);
			if (freshReason != TextSelectionGestureDropReason::None)
			{
				outcome.cancelled = true;
				outcome.reason = freshReason;
				clear();
				return outcome;
			}
			outcome.began = true;
			outcome.target = target_;
			return outcome;
		}

		outcome.target = target_;
		const auto activationReason = validateActivation(
			nativeEvent, environment, activationGeneration_);
		if (activationReason != TextSelectionGestureDropReason::None)
		{
			outcome.cancelled = true;
			outcome.reason = activationReason;
			clear();
			return outcome;
		}

		TextSelectionGesturePhase phase;
		TextSelectionGestureDropReason terminalReason =
			TextSelectionGestureDropReason::None;
		switch (pointer->action)
		{
		case PointerAction::Move:
			if (!pointer->clientCoordinatesValid)
			{
				outcome.cancelled = true;
				outcome.reason = TextSelectionGestureDropReason::InvalidCoordinates;
				clear();
				return outcome;
			}
			if (!pointer->leftButton)
			{
				phase = TextSelectionGesturePhase::Cancel;
				terminalReason = TextSelectionGestureDropReason::LeftButtonReleased;
			}
			else
			{
				phase = TextSelectionGesturePhase::Update;
			}
			break;
		case PointerAction::LeftUp:
			if (!pointer->clientCoordinatesValid)
			{
				outcome.cancelled = true;
				outcome.reason = TextSelectionGestureDropReason::InvalidCoordinates;
				clear();
				return outcome;
			}
			phase = TextSelectionGesturePhase::End;
			break;
		case PointerAction::CaptureChanged:
			phase = TextSelectionGesturePhase::Cancel;
			terminalReason = TextSelectionGestureDropReason::CaptureChanged;
			break;
		default:
			return outcome;
		}

		ManagedTextSelectionDispatchGuard dispatchGuard(
			window, window.managedDispatchActive);
		const FocusTargetId dispatchTarget = target_;
		const bool terminal = phase == TextSelectionGesturePhase::End ||
			phase == TextSelectionGesturePhase::Cancel;
		// 终止事件在调用目标前先撤销活动标记，防止回调中的嵌套输入继续驱动
		// 已结束的拖选；target_ 仍保留到本次定向投递完成。
		if (terminal)
			active_ = false;

		bool accepted = false;
		if (!dispatch(window, nativeEvent, *pointer, phase, accepted))
		{
			outcome.reason = TextSelectionGestureDropReason::DispatchTargetMissing;
			outcome.cancelled = true;
			clear();
			return outcome;
		}
		outcome.delivered = true;
		if (!accepted)
		{
			outcome.reason = TextSelectionGestureDropReason::GestureRejected;
			outcome.cancelled = true;
			clear();
			return outcome;
		}

		if (phase == TextSelectionGesturePhase::Update)
		{
			outcome.updated = true;
			const auto freshReason = validateActiveTarget(window, environment);
			if (freshReason != TextSelectionGestureDropReason::None)
			{
				outcome.cancelled = true;
				outcome.reason = freshReason;
				clear();
			}
		}
		else if (phase == TextSelectionGesturePhase::End)
		{
			outcome.ended = true;
			clear();
		}
		else
		{
			outcome.cancelled = true;
			outcome.reason = terminalReason;
			clear();
		}
		outcome.target = dispatchTarget;
		return outcome;
	}
}
