#pragma once

#include "InputEvents.h"
#include "TextSelectionRouting.h"

#include <cstdint>

class Window;

namespace StellarX::Internal
{
	// 路由一次 Pointer 选区手势时使用的宿主状态快照。
	struct TextSelectionGestureEnvironment
	{
		// 宿主当前已处理到的激活状态和代次。
		NativeInputActivationSnapshot currentActivation{};
		// 模态作用域是否抑制底层窗口的 Pointer 业务。
		bool modalBusinessSuppressed = false;
	};

	// 手势未开始、被中止或无法继续投递的原因。
	enum class TextSelectionGestureDropReason : std::uint8_t
	{
		None,                    ///< 事件不是 Pointer，或手势已正常投递。
		InactiveWindow,          ///< 采集或路由时窗口没有键盘激活资格。
		StaleActivation,         ///< 事件或活动手势跨越了失活/恢复代次。
		ModalSuppressed,         ///< 活动模态作用域抑制底层 Pointer 业务。
		InvalidCoordinates,      ///< 原生坐标无法可靠换算到客户区。
		NoHitTarget,             ///< LeftDown 没有命中可绑定的焦点目标。
		InvalidTarget,           ///< 目标身份已失效或位于待删除子树。
		NotVisible,              ///< 目标或其祖先当前不可见。
		Disabled,                ///< 目标当前禁用或不可聚焦。
		OutsideActiveScope,      ///< 目标不在当前焦点作用域中。
		FocusRejected,           ///< LeftDown 申请焦点后，目标未成为实际焦点。
		FocusChanged,            ///< 拖选期间逻辑焦点已转移到其它目标。
		NotTextSelectionCapable, ///< 目标没有实现文字选区手势协议。
		DispatchTargetMissing,   ///< 递归解析时找不到身份匹配的投递叶节点。
		GestureRejected,         ///< 目标收到手势事件，但明确拒绝接受或继续。
		LeftButtonReleased,      ///< Move 到达时左键已松开，活动手势被取消。
		CaptureChanged           ///< 原生 Pointer 捕获已转移，活动手势被取消。
	};

	// 单条 Pointer 事件经过选区状态机后的可断言结果。
	struct TextSelectionGestureDispatchResult
	{
		// 输入资格和是否尝试建立手势。
		bool pointerEvent = false;
		bool beginAttempted = false;
		bool delivered = false;
		bool began = false;
		// 本事件对既有手势造成的状态变化。
		bool updated = false;
		bool ended = false;
		bool cancelled = false;
		TextSelectionGestureDropReason reason = TextSelectionGestureDropReason::None;
		FocusTargetId target{};
	};

	// 每个 Window 事件流拥有一个管理器。跨事件只保存稳定身份与激活代次；
	// Control*、布局指针和借用快照都不得逃出单次 dispatch 调用。
	class TextSelectionGestureManager final
	{
		FocusTargetId target_{};
		FocusScopeId startScope_{};
		std::uint64_t activationGeneration_ = 0;
		bool active_ = false;

		void clear() noexcept;
		TextSelectionGestureDropReason validateActiveTarget(
			::Window& window,
			const TextSelectionGestureEnvironment& environment) const;
		bool dispatch(::Window& window, const NativeInputEvent& nativeEvent,
			const PointerEvent& pointer, TextSelectionGesturePhase phase,
			bool& accepted);

	public:
		// 查询当前是否存在已开始且尚未结束的拖选手势。
		bool active() const noexcept { return active_; }
		// 返回活动手势最初绑定的稳定目标身份。
		FocusTargetId target() const noexcept { return target_; }
		// 返回手势开始时的焦点作用域。
		FocusScopeId startScope() const noexcept { return startScope_; }
		// 返回手势开始时冻结的窗口激活代次。
		std::uint64_t activationGeneration() const noexcept
		{
			return activationGeneration_;
		}

		// hitTarget 只在 LeftDown 时参与建立绑定。后续 Move/Up 即使越过其它控件，
		// 仍定向投递给原始目标；资格失效时显式发送 Cancel 并清空状态。
		TextSelectionGestureDispatchResult route(
			::Window& window, const NativeInputEvent& nativeEvent,
			const TextSelectionGestureEnvironment& environment,
			FocusTargetId hitTarget = {});
	};
}
