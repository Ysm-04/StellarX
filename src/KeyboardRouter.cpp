#include "internal/SxKeyboardRouterInternal.h"

#include "FocusManager.h"
#include "Window.h"

#include <algorithm>
#include <array>
#include <variant>

namespace
{
	constexpr std::size_t MaxKeyboardRouteDepth = 256;
	using RoutePath = std::array<StellarX::FocusTargetId, MaxKeyboardRouteDepth>;

	class ManagedKeyboardDispatchGuard
	{
		Window& window;
		bool& managedDispatchActive;
		bool previousManagedDispatchActive = false;
	public:
		ManagedKeyboardDispatchGuard(Window& owner, bool& dispatchFlag)
			: window(owner), managedDispatchActive(dispatchFlag),
			previousManagedDispatchActive(dispatchFlag)
		{
			window.beginMutationScope();
			managedDispatchActive = true;
		}

		~ManagedKeyboardDispatchGuard()
		{
			// 离开 mutation 作用域前恢复外层状态；只有真正的最外层事件可以提交挂起变更。
			managedDispatchActive = previousManagedDispatchActive;
			window.endMutationScope();
		}
	};

}

namespace StellarX::Internal
{
	bool KeyboardRouter::buildOriginalPath(FocusManager& focus,
		FocusTargetId originalTarget, FocusTargetId* path,
		std::size_t capacity, std::size_t& pathLength)
	{
		pathLength = 0;
		FocusTargetId cursor = originalTarget;
		while (cursor && pathLength < capacity)
		{
			if (std::find(path, path + pathLength, cursor) != path + pathLength)
				return false;

			FocusTargetSnapshot snapshot;
			if (!focus.querySnapshot(cursor, snapshot) ||
				focus.isUnderInvalidatedSubtree(cursor))
				return false;

			path[pathLength++] = cursor;
			cursor = snapshot.logicalParent;
		}
		return !cursor;
	}

	KeyboardRouteDispatchResult KeyboardRouter::suspendFocusedPath(::Window& window)
	{
		KeyboardRouteDispatchResult outcome;
		ManagedKeyboardDispatchGuard dispatchGuard(window, window.managedDispatchActive);
		FocusManager& focus = window.getFocusManager();
		const FocusTargetId originalTarget = focus.focusedTarget();
		outcome.originalTarget = originalTarget;
		outcome.hadFocusedTarget = static_cast<bool>(originalTarget);
		if (!originalTarget)
		{
			outcome.pathComplete = true;
			return outcome;
		}

		RoutePath path{};
		std::size_t pathLength = 0;
		if (!buildOriginalPath(focus, originalTarget, path.data(), path.size(), pathLength))
			return outcome;

		outcome.pathComplete = true;
		outcome.pathLength = pathLength;
		for (std::size_t index = 0; index < pathLength; ++index)
		{
			const FocusTargetId target = path[index];
			FocusTargetSnapshot freshSnapshot;
			if (!focus.querySnapshot(target, freshSnapshot) ||
				focus.isUnderInvalidatedSubtree(target))
			{
				++outcome.skippedTargets;
				continue;
			}
			if (!window.dispatchKeyboardSuspensionToTarget(target))
			{
				++outcome.skippedTargets;
				continue;
			}
			++outcome.deliveredTargets;
			outcome.focusedPathSuspended = true;
		}
		return outcome;
	}

	KeyboardRouteDispatchResult KeyboardRouter::resumeFocusedPath(::Window& window)
	{
		KeyboardRouteDispatchResult outcome;
		ManagedKeyboardDispatchGuard dispatchGuard(window, window.managedDispatchActive);
		FocusManager& focus = window.getFocusManager();
		const FocusTargetId originalTarget = focus.focusedTarget();
		outcome.originalTarget = originalTarget;
		outcome.hadFocusedTarget = static_cast<bool>(originalTarget);
		if (!originalTarget)
		{
			outcome.pathComplete = true;
			return outcome;
		}

		RoutePath path{};
		std::size_t pathLength = 0;
		if (!buildOriginalPath(focus, originalTarget, path.data(), path.size(), pathLength))
			return outcome;

		outcome.pathComplete = true;
		outcome.pathLength = pathLength;
		for (std::size_t index = 0; index < pathLength; ++index)
		{
			const FocusTargetId target = path[index];
			FocusTargetSnapshot freshSnapshot;
			if (!focus.querySnapshot(target, freshSnapshot) ||
				focus.isUnderInvalidatedSubtree(target))
			{
				++outcome.skippedTargets;
				continue;
			}
			if (!window.dispatchKeyboardResumptionToTarget(target))
			{
				++outcome.skippedTargets;
				continue;
			}
			++outcome.deliveredTargets;
			outcome.focusedPathResumed = true;
		}
		return outcome;
	}

	KeyboardRouteDispatchResult KeyboardRouter::route(
		::Window& window, const NativeInputEvent& nativeEvent,
		const KeyboardRoutingEnvironment& environment)
	{
		if (std::holds_alternative<WindowActivationEvent>(nativeEvent.payload))
		{
			KeyboardRouteDispatchResult outcome;
			outcome.activationEvent = true;
			// 以生产者侧最新激活快照为准。较早入队的失活或恢复事件不能越过更新的
			// 原生焦点变化，重新启用或清除同步 Dialog 能力。
			window.publishDialogCodeActivation(
				environment.currentActivation.keyboardFocusActive);
			if (!environment.currentActivation.keyboardFocusActive)
			{
				outcome = suspendFocusedPath(window);
				outcome.activationEvent = true;
			}
			else
			{
				outcome = resumeFocusedPath(window);
				outcome.activationEvent = true;
				FocusManager& focus = window.getFocusManager();
				const FocusTargetId focused = focus.focusedTarget();
				outcome.originalTarget = focused;
				outcome.hadFocusedTarget = static_cast<bool>(focused);
				if (focused)
					focus.notifySubtreeStateChanged(focused.owner);
			}
			window.refreshDialogCodeFocusCapabilities();
			return outcome;
		}

		KeyboardRouteDispatchResult outcome;
		const auto* key = std::get_if<KeyEvent>(&nativeEvent.payload);
		if (!key)
			return outcome;
		outcome.keyEvent = true;

		outcome.suppressedByModal = environment.modalBusinessSuppressed;
		outcome.suppressedByInactiveWindow =
			!nativeEvent.activationAtCapture.keyboardFocusActive ||
			!environment.currentActivation.keyboardFocusActive;
		outcome.suppressedByStaleActivation =
			nativeEvent.activationAtCapture.generation == 0 ||
			environment.currentActivation.generation == 0 ||
			nativeEvent.activationAtCapture.generation !=
				environment.currentActivation.generation;
		outcome.suppressedByIme =
			key->isImeOwnedAtCapture() || key->imeComposingAtCapture;
		outcome.suppressedBySystemKey =
			key->systemKey || key->altContext || key->modifiers.alt();
		if (outcome.suppressedByModal || outcome.suppressedByInactiveWindow ||
			outcome.suppressedByStaleActivation || outcome.suppressedByIme ||
			outcome.suppressedBySystemKey)
			return outcome;

		const bool isTabTraversal = key->action == KeyAction::Down &&
			key->virtualKey == VK_TAB && !key->modifiers.control();
		const bool isDialogDefaultKey = key->action == KeyAction::Down &&
			!key->isAutoRepeat() &&
			(key->virtualKey == VK_ESCAPE ||
				(key->virtualKey == VK_RETURN && !key->modifiers.control()));

		ManagedKeyboardDispatchGuard dispatchGuard(window, window.managedDispatchActive);
		FocusManager& focus = window.getFocusManager();
		const FocusTargetId originalTarget = focus.focusedTarget();
		outcome.originalTarget = originalTarget;
		outcome.hadFocusedTarget = static_cast<bool>(originalTarget);
		if (!originalTarget)
		{
			outcome.pathComplete = true;
			if (isTabTraversal)
			{
				outcome.tabTraversalAttempted = true;
				const FocusChangeResult focusResult =
					focus.advanceTabFocus(key->modifiers.shift());
				outcome.tabFocusChanged = focusResult.changed;
				outcome.tabFocusedTarget = focusResult.focused;
			}
			return outcome;
		}

		// 路径只保存稳定身份。回调造成的焦点或控件树变化从下一事件生效，
		// 不改变本事件已经冻结的原始路径。
		RoutePath path{};
		std::size_t pathLength = 0;
		if (!buildOriginalPath(focus, originalTarget, path.data(), path.size(), pathLength))
			return outcome;

		outcome.pathComplete = true;
		outcome.pathLength = pathLength;
		outcome.originalDialogTarget =
			window.dialogRootOnFocusPath(path.data(), pathLength);
		RoutedKeyEvent event;
		event.sequence = nativeEvent.sequence;
		event.messageTime = nativeEvent.messageTime;
		event.hwnd = nativeEvent.hwnd;
		event.key = *key;
		event.originalTarget = originalTarget;

		for (std::size_t index = 0; index < pathLength; ++index)
		{
			const FocusTargetId target = path[index];
			// 前面的回调可能删除路径后方节点，因此每次都重新解析身份，并拒绝待删除子树；
			// 任何借用指针都不能跨过一次回调。
			FocusTargetSnapshot freshSnapshot;
			if (!focus.querySnapshot(target, freshSnapshot) ||
				focus.isUnderInvalidatedSubtree(target))
			{
				++outcome.skippedTargets;
				continue;
			}

			KeyRouteResult result;
			if (!window.dispatchKeyEventToTarget(target, event, result))
			{
				++outcome.skippedTargets;
				continue;
			}

			++outcome.deliveredTargets;
			outcome.frameworkDefaultPrevented =
				outcome.frameworkDefaultPrevented || result.preventFrameworkDefault;
			if (result.stopBubble)
			{
				outcome.bubbleStopped = true;
				break;
			}
		}

		if (isDialogDefaultKey && outcome.originalDialogTarget &&
			!outcome.frameworkDefaultPrevented)
		{
			outcome.dialogDefaultAttempted = true;
			outcome.dialogDefaultExecuted = window.dispatchDialogFrameworkDefault(
				outcome.originalDialogTarget, *key);
		}

		// Tab 是框架默认动作，不是叶控件命令。它只在原始焦点路径完成后执行，
		// 控件和容器可以分别停止冒泡或阻止遍历。这里重新获取焦点树，不复用
		// 回调前的快照。
		if (isTabTraversal && !outcome.frameworkDefaultPrevented)
		{
			outcome.tabTraversalAttempted = true;
			const FocusChangeResult focusResult =
				focus.advanceTabFocus(key->modifiers.shift());
			outcome.tabFocusChanged = focusResult.changed;
			outcome.tabFocusedTarget = focusResult.focused;
		}
		return outcome;
	}
}
