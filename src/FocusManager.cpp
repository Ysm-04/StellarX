#include "FocusManager.h"
#include "SxLog.h"
#include "Window.h"

#include <algorithm>

namespace
{
	class WindowMutationGuard
	{
		Window* window = nullptr;
	public:
		explicit WindowMutationGuard(Window* owner) : window(owner)
		{
			if (window) window->beginMutationScope();
		}
		~WindowMutationGuard()
		{
			if (window) window->endMutationScope();
		}
	};

	class FocusCallbackContextGuard
	{
		StellarX::FocusTargetId& slot;
	public:
		FocusCallbackContextGuard(StellarX::FocusTargetId& targetSlot,
			StellarX::FocusTargetId target) : slot(targetSlot)
		{
			slot = target;
		}
		~FocusCallbackContextGuard()
		{
			slot = {};
		}
	};
}

namespace StellarX
{
	FocusManager::FocusManager(::Window& owner) : window(&owner)
	{
		ScopeEntry root;
		root.id.value = 1;
		scopes.push_back(root);
		publishDialogCodeCapabilities();
	}

	void FocusManager::publishDialogCodeCapabilities() const noexcept
	{
		bool wantArrows = false;
		bool wantChars = false;
		bool inlineComposition = false;
		if (window && activeTarget)
		{
			FocusTargetSnapshot snapshot;
			if (validateCurrentTarget(activeTarget, &snapshot) == FocusError::None)
			{
				wantArrows = snapshot.supportsTextSelection;
				wantChars = snapshot.acceptsTextInput;
				inlineComposition = snapshot.supportsInlineComposition;
			}
		}
		if (window)
		{
			window->publishDialogCodeFocusCapabilities(wantArrows, wantChars);
			window->publishImeInlineCompositionOwnership(inlineComposition);
			publishImeCaretAnchor();
			// 先发布几何，再开放文字输入资格。输入上下文门控恢复 HWND 保存的 HIMC 时，
			// 属主线程可以立即用最新锚点定位，避免首个按键短暂显示旧候选窗位置。
			window->publishImeTextInputEligibility(wantChars);
		}
	}

	void FocusManager::publishImeCaretAnchor() const noexcept
	{
		if (!window)
			return;
		ImeCaretAnchorSnapshot anchor;
		if (activeTarget)
		{
			FocusTargetSnapshot snapshot;
			if (validateCurrentTarget(activeTarget, &snapshot) == FocusError::None &&
				snapshot.acceptsTextInput && snapshot.imeCaretAnchor.valid)
			{
				anchor = snapshot.imeCaretAnchor;
				// 身份和作用域必须来自刚完成验证的焦点快照，不能沿用控件缓存或旧布局值。
				anchor.target = snapshot.id;
				anchor.scope = snapshot.scope;
				anchor.publicationGeneration = 0;
				anchor.valid = true;
			}
		}
		window->publishImeCaretAnchor(anchor);
	}

	FocusScopeId FocusManager::rootScope() const noexcept
	{
		return scopes.empty() ? FocusScopeId{} : scopes.front().id;
	}

	FocusScopeId FocusManager::activeScope() const noexcept
	{
		return scopes.empty() ? FocusScopeId{} : scopes.back().id;
	}

	bool FocusManager::querySnapshot(FocusTargetId target, FocusTargetSnapshot& snapshot) const
	{
		if (!window || !target)
			return false;
		if (!window->queryFocusTargetSnapshot(target, snapshot))
			return false;

		snapshot.scope = rootScope();
		for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
		{
			if (it->root && isWithinSubtree(target, it->root))
			{
				snapshot.scope = it->id;
				break;
			}
		}
		return true;
	}

	bool FocusManager::isWithinSubtree(FocusTargetId target, FocusTargetId root) const
	{
		if (!target || !root)
			return false;
		FocusTargetId cursor = target;
		for (std::size_t guard = 0; cursor && guard < MaxReentrantRounds * 4; ++guard)
		{
			if (cursor == root)
				return true;
			FocusTargetSnapshot snapshot;
			if (!window || !window->queryFocusTargetSnapshot(cursor, snapshot))
				return false;
			if (snapshot.logicalParent == cursor)
				return false;
			cursor = snapshot.logicalParent;
		}
		return false;
	}

	bool FocusManager::isUnderInvalidatedSubtree(FocusTargetId target) const
	{
		for (const ControlHandle root : invalidatedSubtrees)
		{
			if (isWithinSubtree(target, FocusTargetId{ root, 0 }))
				return true;
		}
		return false;
	}

	bool FocusManager::isWithinActiveScope(FocusTargetId target) const
	{
		if (scopes.empty() || !scopes.back().root)
			return true;
		return isWithinSubtree(target, scopes.back().root);
	}

	FocusPolicy FocusManager::requiredPolicy(FocusReason reason) const noexcept
	{
		switch (reason)
		{
		case FocusReason::Pointer:
			return FocusPolicy::Pointer;
		case FocusReason::TabTraversal:
		case FocusReason::InvalidationFallback:
			return FocusPolicy::Tab;
		case FocusReason::TabRoving:
			return FocusPolicy::Programmatic;
		case FocusReason::Programmatic:
		case FocusReason::ScopeActivation:
		case FocusReason::ScopeRestore:
		default:
			return FocusPolicy::Programmatic;
		}
	}

	FocusError FocusManager::validateCurrentTarget(FocusTargetId target,
		FocusTargetSnapshot* result) const
	{
		if (!target)
			return FocusError::InvalidTarget;
		FocusTargetSnapshot snapshot;
		if (!querySnapshot(target, snapshot) || isUnderInvalidatedSubtree(target))
			return FocusError::InvalidTarget;
		if (!snapshot.focusable)
			return FocusError::NotFocusable;
		if (!snapshot.visible)
			return FocusError::NotVisible;
		if (!snapshot.enabled)
			return FocusError::Disabled;
		if (!isWithinActiveScope(target))
			return FocusError::OutsideActiveScope;
		if (result) *result = snapshot;
		return FocusError::None;
	}

	FocusError FocusManager::validateTarget(FocusTargetId target, FocusReason reason,
		FocusTargetSnapshot* result) const
	{
		const FocusError currentError = validateCurrentTarget(target, result);
		if (currentError != FocusError::None)
			return currentError;
		FocusTargetSnapshot snapshot;
		if (!result)
		{
			if (!querySnapshot(target, snapshot))
				return FocusError::InvalidTarget;
			result = &snapshot;
		}
		if (!hasFocusPolicy(result->policy, requiredPolicy(reason)))
			return FocusError::NotFocusable;
		return FocusError::None;
	}

	void FocusManager::queueReentrantRequest(FocusTargetId target, FocusReason reason) noexcept
	{
		pending.target = target;
		pending.reason = reason;
		pendingPresent = true;
	}

	FocusChangeResult FocusManager::performTransition(FocusTargetId target, FocusReason reason)
	{
		FocusChangeResult result;
		result.focused = activeTarget;

		if (target)
		{
			result.error = validateTarget(target, reason);
			if (result.error != FocusError::None)
				return result;
		}
		if (target == activeTarget)
			return result;

		const FocusTargetId oldTarget = activeTarget;
		if (oldTarget)
			window->retireImeCompositionForLogicalFocusChange(oldTarget);
		activeTarget = {};
		// FocusLost 回调可以修改控件树或打开新作用域，因此先撤销旧叶节点的
		// 原生 IME 坐标，不能让候选窗在回调期间继续停留在旧位置。
		publishImeCaretAnchor();
		window->publishImeTextInputEligibility(false);
		if (oldTarget)
		{
			FocusCallbackContextGuard callbackContext(callbackFocusContext, oldTarget);
			window->applyFocusState(oldTarget, false, reason);
		}

		// onFocusLost 可能删除、隐藏或禁用新目标；回调后必须重新获取快照。
		if (target)
		{
			result.error = validateTarget(target, reason);
			if (result.error != FocusError::None)
			{
				result.changed = static_cast<bool>(oldTarget);
				result.focused = activeTarget;
				return result;
			}
			activeTarget = target;
			FocusCallbackContextGuard callbackContext(callbackFocusContext, target);
			window->applyFocusState(target, true, reason);
		}

		result.changed = oldTarget != activeTarget;
		result.focused = activeTarget;
		return result;
	}

	std::vector<FocusTargetSnapshot> FocusManager::collectOrderedTargets(bool tabStopsOnly) const
	{
		std::vector<FocusTargetSnapshot> snapshots;
		if (!window)
			return snapshots;
		window->collectFocusTargetSnapshots(snapshots);

		std::vector<FocusTargetSnapshot> filtered;
		filtered.reserve(snapshots.size());
		for (auto snapshot : snapshots)
		{
			FocusTargetSnapshot current;
			if (validateCurrentTarget(snapshot.id, &current) != FocusError::None)
				continue;
			if (tabStopsOnly && !current.tabStop)
				continue;
			filtered.push_back(current);
		}
		std::stable_sort(filtered.begin(), filtered.end(), [](const auto& left, const auto& right)
			{ return left.tabIndex < right.tabIndex; });
		return filtered;
	}

	std::vector<FocusTargetSnapshot> FocusManager::collectTabStops() const
	{
		return collectOrderedTargets(true);
	}

	FocusChangeResult FocusManager::advanceTabFocus(bool reverse)
	{
		// 每次按 Tab 都重新构建焦点树。Table 等控件可能在首次绘制后才物化真实
		// Button 子节点，回调也可能在相邻事件之间隐藏、禁用、删除或新增停靠点。
		const auto stops = collectTabStops();
		if (stops.empty())
			return { false, FocusError::None, activeTarget };

		const auto current = std::find_if(stops.begin(), stops.end(),
			[&](const FocusTargetSnapshot& item) { return item.id == activeTarget; });
		FocusTargetId target;
		if (current == stops.end())
		{
			target = reverse ? stops.back().id : stops.front().id;
		}
		else if (reverse)
		{
			target = current == stops.begin() ? stops.back().id : (current - 1)->id;
		}
		else
		{
			const auto next = current + 1;
			target = next == stops.end() ? stops.front().id : next->id;
		}
		return requestFocus(target, FocusReason::TabTraversal);
	}

	FocusTargetId FocusManager::findInvalidationFallback(FocusTargetId invalidTarget) const
	{
		std::vector<FocusTargetSnapshot> all;
		if (window) window->collectFocusTargetSnapshots(all);
		std::stable_sort(all.begin(), all.end(), [](const auto& left, const auto& right)
			{ return left.tabIndex < right.tabIndex; });

		auto invalidIt = std::find_if(all.begin(), all.end(), [&](const auto& item)
			{ return item.id == invalidTarget; });
		const std::size_t invalidIndex = invalidIt == all.end()
			? 0 : static_cast<std::size_t>(std::distance(all.begin(), invalidIt));

		for (std::size_t i = invalidIndex + (invalidIt == all.end() ? 0 : 1); i < all.size(); ++i)
		{
			if (validateTarget(all[i].id, FocusReason::InvalidationFallback) == FocusError::None)
				return all[i].id;
		}
		for (std::size_t i = invalidIndex; i > 0; --i)
		{
			if (validateTarget(all[i - 1].id, FocusReason::InvalidationFallback) == FocusError::None)
				return all[i - 1].id;
		}

		if (!scopes.empty() && scopes.back().initialFocus &&
			validateTarget(scopes.back().initialFocus, FocusReason::ScopeActivation) == FocusError::None)
			return scopes.back().initialFocus;
		return {};
	}

	FocusChangeResult FocusManager::requestFocus(FocusTargetId target, FocusReason reason)
	{
		if (transitionActive)
		{
			queueReentrantRequest(target, reason);
			return { false, FocusError::None, activeTarget };
		}

		WindowMutationGuard mutation(window);
		transitionActive = true;
		lastError = FocusError::None;
		FocusRequest request{ target, reason };
		FocusChangeResult aggregate{ false, FocusError::None, activeTarget };
		bool continueTransition = true;
		std::size_t round = 0;

		while (continueTransition && round < MaxReentrantRounds)
		{
			++round;
			pendingPresent = false;
			const FocusChangeResult step = performTransition(request.target, request.reason);
			aggregate.changed = aggregate.changed || step.changed;
			if (round == 1 && step.error != FocusError::None)
				aggregate.error = step.error;

			const bool activeStillValid = !activeTarget ||
				validateCurrentTarget(activeTarget) == FocusError::None;
			bool selectedNext = false;

			if (pendingPresent)
			{
				const FocusRequest latest = pending;
				pendingPresent = false;
				// pending 只有一个槽位，最后请求胜出；末项失效时也不复活更早的请求。
				if (!latest.target || validateTarget(latest.target, latest.reason) == FocusError::None)
				{
					request = latest;
					selectedNext = true;
				}
			}

			if (!selectedNext && !activeStillValid)
			{
				const FocusTargetId invalid = activeTarget;
				request = { findInvalidationFallback(invalid), FocusReason::InvalidationFallback };
				selectedNext = true;
			}
			continueTransition = selectedNext;
		}

		if (continueTransition)
		{
			pendingPresent = false;
			lastError = FocusError::ReentrancyLimit;
			aggregate.error = FocusError::ReentrancyLimit;
			SX_LOGE("Focus") << "focus reentrancy limit reached: rounds=" << MaxReentrantRounds;
		}
		else if (aggregate.error != FocusError::None)
		{
			lastError = aggregate.error;
		}

		transitionActive = false;
		aggregate.focused = activeTarget;
		publishDialogCodeCapabilities();
		return aggregate;
	}

	FocusScopeId FocusManager::pushScope(FocusTargetId scopeRoot, FocusTargetId initialFocus)
	{
		FocusTargetSnapshot rootSnapshot;
		if (!scopeRoot || !querySnapshot(scopeRoot, rootSnapshot) || isUnderInvalidatedSubtree(scopeRoot))
		{
			lastError = FocusError::InvalidScope;
			return {};
		}

		ScopeEntry entry;
		entry.id.value = nextScopeValue++;
		entry.root = scopeRoot;
		entry.initialFocus = initialFocus;
		entry.restoreFocus = callbackFocusContext ? callbackFocusContext : activeTarget;
		scopes.push_back(entry);

		FocusTargetId target = initialFocus;
		if (!target || validateTarget(target, FocusReason::ScopeActivation) != FocusError::None)
		{
			const auto stops = collectTabStops();
			target = stops.empty() ? FocusTargetId{} : stops.front().id;
		}
		requestFocus(target, FocusReason::ScopeActivation);
		return entry.id;
	}

	bool FocusManager::popScope(FocusScopeId scope)
	{
		if (scopes.size() <= 1 || scopes.back().id != scope)
		{
			lastError = FocusError::InvalidScope;
			return false;
		}
		const FocusTargetId restore = scopes.back().restoreFocus;
		scopes.pop_back();
		if (restore && validateTarget(restore, FocusReason::ScopeRestore) == FocusError::None)
			requestFocus(restore, FocusReason::ScopeRestore);
		else
		{
			const auto stops = collectTabStops();
			requestFocus(stops.empty() ? FocusTargetId{} : stops.front().id,
				FocusReason::ScopeRestore);
		}
		return true;
	}

	void FocusManager::normalizeInvalidScopes()
	{
		while (scopes.size() > 1)
		{
			FocusTargetSnapshot snapshot;
			if (querySnapshot(scopes.back().root, snapshot) &&
				!isUnderInvalidatedSubtree(scopes.back().root))
				break;
			scopes.pop_back();
		}
	}

	void FocusManager::notifySubtreePendingRemoval(ControlHandle rootHandle)
	{
		if (!rootHandle)
			return;
		notifySubtreesPendingRemoval({ rootHandle });
	}

	void FocusManager::notifySubtreesPendingRemoval(const std::vector<ControlHandle>& rootHandles)
	{
		for (const ControlHandle rootHandle : rootHandles)
		{
			if (!rootHandle)
				continue;
			if (std::find(invalidatedSubtrees.begin(), invalidatedSubtrees.end(), rootHandle) == invalidatedSubtrees.end())
				invalidatedSubtrees.push_back(rootHandle);
		}
		normalizeInvalidScopes();
		publishDialogCodeCapabilities();

		bool activeRemoved = false;
		for (const ControlHandle rootHandle : rootHandles)
		{
			if (rootHandle && activeTarget &&
				isWithinSubtree(activeTarget, FocusTargetId{ rootHandle, 0 }))
			{
				activeRemoved = true;
				break;
			}
		}
		if (!activeRemoved)
			return;
		if (transitionActive)
			return;
		const FocusTargetId invalid = activeTarget;
		requestFocus(findInvalidationFallback(invalid), FocusReason::InvalidationFallback);
	}

	void FocusManager::notifySubtreeStateChanged(ControlHandle rootHandle)
	{
		if (!rootHandle || !activeTarget ||
			!isWithinSubtree(activeTarget, FocusTargetId{ rootHandle, 0 }))
			return;
		publishDialogCodeCapabilities();
		if (validateCurrentTarget(activeTarget) == FocusError::None || transitionActive)
			return;
		const FocusTargetId invalid = activeTarget;
		requestFocus(findInvalidationFallback(invalid), FocusReason::InvalidationFallback);
	}

	void FocusManager::onMutationCommitted()
	{
		invalidatedSubtrees.clear();
		normalizeInvalidScopes();
		publishDialogCodeCapabilities();
	}
}
