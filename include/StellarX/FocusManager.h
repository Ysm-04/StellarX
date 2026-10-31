/*******************************************************************************
 * FocusManager.h - 每个 Window 独立持有的逻辑焦点管理器。
 ******************************************************************************/
#pragma once

#include "CoreTypes.h"
#include <cstddef>
#include <cstdint>
#include <vector>

class Window;

namespace StellarX
{
	namespace Internal
	{
		class KeyboardRouter;
		class TextInputRouter;
		class CompositionRouter;
		class TextSelectionGestureManager;
	}

	/// 管理单个 Window 中的逻辑焦点、Tab 遍历和焦点作用域。
	class FocusManager
	{
		friend class ::Window;
		friend class Internal::KeyboardRouter;
		friend class Internal::TextInputRouter;
		friend class Internal::CompositionRouter;
		friend class Internal::TextSelectionGestureManager;
		struct FocusRequest
		{
			FocusTargetId target{};
			FocusReason reason = FocusReason::Programmatic;
		};

		struct ScopeEntry
		{
			FocusScopeId id{};
			FocusTargetId root{};
			FocusTargetId initialFocus{};
			FocusTargetId restoreFocus{};
		};

		::Window* window = nullptr;
		FocusTargetId activeTarget{};
		// 当前正在执行焦点回调的目标稳定身份。FocusLost 期间 activeTarget 会被
		// 有意清空，因此 pushScope() 必须通过该字段识别回调上下文。
		FocusTargetId callbackFocusContext{};
		bool transitionActive = false;
		bool pendingPresent = false;
		FocusRequest pending{};
		FocusError lastError = FocusError::None;
		std::vector<ScopeEntry> scopes;
		std::vector<ControlHandle> invalidatedSubtrees;
		std::uint64_t nextScopeValue = 2;

		// 限制焦点回调连续重入次数，避免错误控件形成无限切换循环。
		static constexpr std::size_t MaxReentrantRounds = 64;

		// 从当前控件树重新解析目标快照，不缓存裸指针或旧状态。
		bool querySnapshot(FocusTargetId target, FocusTargetSnapshot& snapshot) const;
		// 判断目标是否位于已请求删除、但尚未物理提交的子树中。
		bool isUnderInvalidatedSubtree(FocusTargetId target) const;
		// 判断 target 是否属于 root 表示的逻辑子树。
		bool isWithinSubtree(FocusTargetId target, FocusTargetId root) const;
		// 判断目标是否处于当前活动焦点作用域内。
		bool isWithinActiveScope(FocusTargetId target) const;
		// 按焦点原因和最新快照验证候选目标是否可接受请求。
		FocusError validateTarget(FocusTargetId target, FocusReason reason,
			FocusTargetSnapshot* snapshot = nullptr) const;
		// 重新验证已提交焦点；不沿用回调前取得的可见性或能力值。
		FocusError validateCurrentTarget(FocusTargetId target,
			FocusTargetSnapshot* snapshot = nullptr) const;
		// 返回指定焦点原因要求目标具备的能力位。
		FocusPolicy requiredPolicy(FocusReason reason) const noexcept;
		// 执行一次非递归焦点事务，并在每个回调后重新解析参与者。
		FocusChangeResult performTransition(FocusTargetId target, FocusReason reason);
		// 活动焦点失效时，从当前作用域寻找可用的遍历回退目标。
		FocusTargetId findInvalidationFallback(FocusTargetId invalidTarget) const;
		// 按 tabIndex 与控件树顺序收集当前作用域内的有效目标。
		std::vector<FocusTargetSnapshot> collectOrderedTargets(bool tabStopsOnly) const;
		// 焦点回调重入时只保留最新请求，等待当前事务回合结束后处理。
		void queueReentrantRequest(FocusTargetId target, FocusReason reason) noexcept;
		// 移除已经失效的作用域，并恢复仍然有效的焦点关系。
		void normalizeInvalidScopes();
		// 把当前焦点能力发布给原生窗口的 WM_GETDLGCODE 查询路径。
		void publishDialogCodeCapabilities() const noexcept;
		// 把当前文本焦点的 IME caret 锚点发布给窗口属主线程。
		void publishImeCaretAnchor() const noexcept;

	public:
		/// 为指定 Window 创建唯一逻辑焦点管理器；owner 必须比本对象存活更久。
		explicit FocusManager(::Window& owner);
		/// 销毁焦点管理器；不拥有 Window 或任何 Control。
		~FocusManager() = default;

		/// 焦点管理器与所属 Window 一一对应，不允许复制。
		FocusManager(const FocusManager&) = delete;
		/// 焦点管理器与所属 Window 一一对应，不允许复制赋值。
		FocusManager& operator=(const FocusManager&) = delete;

		/// 请求把逻辑焦点切换到 target；空 target 表示清除焦点。
		/// 普通调用立即切换；焦点回调中的重入调用只覆盖当前回合的单槽 pending 请求。
		FocusChangeResult requestFocus(FocusTargetId target,
			FocusReason reason = FocusReason::Programmatic);
		/// 清除当前逻辑焦点，语义等价于 requestFocus({}, reason)。
		FocusChangeResult clearFocus(FocusReason reason = FocusReason::Programmatic)
		{
			return requestFocus({}, reason);
		}

		/// 返回当前焦点目标的稳定身份；无焦点时返回空身份。
		FocusTargetId focusedTarget() const noexcept { return activeTarget; }
		/// 判断 target 是否为当前焦点；空身份始终返回 false。
		bool hasFocus(FocusTargetId target) const noexcept
		{
			return static_cast<bool>(target) && activeTarget == target;
		}
		/// 返回最近一次焦点事务的错误码。
		FocusError getLastError() const noexcept { return lastError; }
		/// 返回始终存在的根焦点作用域编号。
		FocusScopeId rootScope() const noexcept;
		/// 返回当前栈顶焦点作用域编号。
		FocusScopeId activeScope() const noexcept;

		/// 重新收集活动作用域内的有效 Tab stop，并按 tabIndex 与稳定树顺序排序。
		std::vector<FocusTargetSnapshot> collectTabStops() const;
		/// 每次调用都重新建立当前作用域的 Tab 顺序，然后正向或反向循环移动焦点。
		/// 不会跨按键保留遍历快照。
		FocusChangeResult advanceTabFocus(bool reverse);

		/// 压入一个焦点作用域，并优先聚焦 initialFocus；返回新作用域编号。
		/// scopeRoot 必须在当前有效作用域内，失败时返回空编号。
		FocusScopeId pushScope(FocusTargetId scopeRoot, FocusTargetId initialFocus = {});
		/// 弹出指定活动作用域并按保存身份恢复焦点；作用域无效时返回 false。
		bool popScope(FocusScopeId scope);

		/// 在控件子树进入待删除队列时立即使其焦点身份失效，早于物理析构。
		void notifySubtreePendingRemoval(ControlHandle rootHandle);
		/// 批量标记即将删除的控件子树，避免逐项触发重复焦点回退。
		void notifySubtreesPendingRemoval(const std::vector<ControlHandle>& rootHandles);
		/// 在可见性、启用状态或焦点策略变化后重新验证当前焦点。
		void notifySubtreeStateChanged(ControlHandle rootHandle);
		/// 在最外层 mutation 提交后清理本回合的逻辑失效标记。
		void onMutationCommitted();
	};
}
