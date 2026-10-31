#pragma once

#include "InputEvents.h"
#include "TextInputRouting.h"

#include <cstdint>

class Window;

namespace StellarX::Internal
{
	// 路由文字或组合事件时使用的宿主状态快照。
	struct TextInputRoutingEnvironment
	{
		// 宿主当前已处理到的激活状态和代次。
		NativeInputActivationSnapshot currentActivation{};
		// 模态作用域是否抑制底层窗口的文字业务。
		bool modalBusinessSuppressed = false;
	};

	// 正式文字与临时组合使用相同的激活、模态环境口径。
	using CompositionRoutingEnvironment = TextInputRoutingEnvironment;

	// 正式文字未投递的原因；用于计数和测试，不包含文字内容。
	enum class TextInputDropReason : std::uint8_t
	{
		None,                   ///< 已投递，或当前负载并非文字事件。
		InactiveWindow,         ///< 采集时窗口未激活。
		StaleActivation,        ///< 事件跨越了失活/恢复代次边界。
		ModalSuppressed,        ///< 活动模态作用域抑制底层文字业务。
		InvalidImeOwnership,    ///< IME 结果与当前组合轮次所有权不匹配。
		RetiredImeSession,      ///< 组合绑定目标已离开焦点，轮次只允许收口。
		InvalidText,            ///< 负载为空或不是合法 UTF-16。
		NoFocusedTarget,        ///< 当前没有逻辑焦点目标。
		InvalidTarget,          ///< 原始焦点身份已失效。
		NotVisible,             ///< 目标或祖先当前不可见。
		Disabled,               ///< 目标当前禁用。
		OutsideActiveScope,     ///< 目标不在活动焦点作用域中。
		NotTextInputCapable,    ///< 目标当前模式不接受正式文字。
		DispatchTargetMissing   ///< 递归解析时找不到身份完全匹配的叶节点。
	};

	// 一条正式文字事件的路由裁决；只保留身份和原因，不保留文字。
	struct TextInputRouteDispatchResult
	{
		bool textEvent = false;
		bool delivered = false;
		TextInputDropReason dropReason = TextInputDropReason::None;
		FocusTargetId originalTarget{};
		FocusScopeId originalScope{};
	};

	// 正式文字的单目标路由器：不冒泡，也不执行键盘默认动作。
	class TextInputRouter
	{
	public:
		// 把 NativeInputEvent 中的正式文字投递给采集轮次绑定的焦点叶节点；
		// 框架事件循环从 NativeInputQueue 排空事件时调用该入口。
		static TextInputRouteDispatchResult route(
			::Window& window, const NativeInputEvent& nativeEvent,
			const TextInputRoutingEnvironment& environment);
		// 返回窗口累计的隐私安全文字路由统计。
		static TextInputRoutingStats stats(const ::Window& window) noexcept;
	};

	// IME 临时组合事件未投递的原因。
	enum class CompositionDropReason : std::uint8_t
	{
		None,                  ///< 已投递，或当前负载并非组合事件。
		InactiveWindow,        ///< 采集时窗口未激活。
		StaleActivation,       ///< 事件跨越了失活/恢复代次边界。
		ModalSuppressed,       ///< 活动模态作用域抑制底层组合业务。
		InvalidPayload,        ///< 阶段、编号或 UTF-16 组合串不合法。
		InvalidSession,        ///< 组合事件与当前轮次编号不匹配。
		RetiredSession,        ///< 轮次绑定目标已退休，只接受收口事件。
		NoFocusedTarget,       ///< 建立组合绑定时没有焦点目标。
		InvalidTarget,         ///< 轮次绑定的焦点身份已失效。
		OutsideActiveScope,    ///< 绑定目标不在活动作用域中。
		NotCompositionCapable, ///< 目标不支持框架内联组合显示。
		DispatchTargetMissing  ///< 递归解析时找不到绑定的叶节点。
	};

	// 一条临时组合事件的路由裁决。
	struct CompositionRouteDispatchResult
	{
		bool compositionEvent = false;
		bool delivered = false;
		bool lazyBinding = false;
		CompositionDropReason dropReason = CompositionDropReason::None;
		FocusTargetId originalTarget{};
		FocusScopeId originalScope{};
	};

	// 维护 compositionId 与稳定焦点身份绑定的组合路由器。
	class CompositionRouter
	{
	public:
		// 投递组合生命周期事件；缺少 Start 时允许首个 Update 惰性建立绑定。
		static CompositionRouteDispatchResult route(
			::Window& window, const NativeInputEvent& nativeEvent,
			const CompositionRoutingEnvironment& environment);
		// 窗口失活时退休活动组合轮次，阻止延迟事件投向后来获得焦点的控件。
		static void cancelForDeactivation(
			::Window& window, const NativeInputEvent& nativeEvent);
		// 返回窗口累计的隐私安全组合路由统计。
		static CompositionRoutingStats stats(const ::Window& window) noexcept;
	};
}
