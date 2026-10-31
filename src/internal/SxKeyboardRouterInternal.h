#pragma once

#include "KeyboardRouting.h"

#include <cstddef>

class Window;
namespace StellarX { class FocusManager; }

namespace StellarX::Internal
{
	// 路由一条原生事件时使用的宿主状态快照；不得在路由过程中重新读取全局键态。
	struct KeyboardRoutingEnvironment
	{
		// 当前宿主已处理到的激活代次和激活状态。
		NativeInputActivationSnapshot currentActivation{};
		// 模态作用域是否要求暂停底层窗口的键盘业务。
		bool modalBusinessSuppressed = false;
	};

	// 一条原生事件的可观测路由结果，覆盖资格门控、冒泡、框架默认动作，
	// 以及焦点路径的暂停和恢复。这里只保留状态与计数，不保存控件指针。
	struct KeyboardRouteDispatchResult
	{
		// 事件类型与焦点路径处理结果。
		bool keyEvent = false;
		bool activationEvent = false;
		bool hadFocusedTarget = false;
		bool pathComplete = false;
		bool bubbleStopped = false;
		bool frameworkDefaultPrevented = false;
		bool focusedPathSuspended = false;
		bool focusedPathResumed = false;
		// 未进入业务路由的具体门控原因。
		bool suppressedByInactiveWindow = false;
		bool suppressedByStaleActivation = false;
		bool suppressedByIme = false;
		bool suppressedBySystemKey = false;
		bool suppressedByModal = false;
		// 框架默认动作的尝试与执行结果。
		bool tabTraversalAttempted = false;
		bool tabFocusChanged = false;
		bool dialogDefaultAttempted = false;
		bool dialogDefaultExecuted = false;
		// 原始焦点路径的长度及实际投递、跳过数量。
		std::size_t pathLength = 0;
		std::size_t deliveredTargets = 0;
		std::size_t skippedTargets = 0;
		FocusTargetId originalTarget{};
		FocusTargetId originalDialogTarget{};
		FocusTargetId tabFocusedTarget{};
	};

	// 把 NativeInputQueue 中的键盘与激活事件交给当前窗口焦点路径。
	class KeyboardRouter
	{
		static bool buildOriginalPath(FocusManager& focus,
			FocusTargetId originalTarget, FocusTargetId* path,
			std::size_t capacity, std::size_t& pathLength);
	public:
		// 激活事件只维护瞬时状态；满足资格的 KeyEvent 在同一个可嵌套托管分发
		// 与 mutation 事务中沿原始焦点路径冒泡。
		static KeyboardRouteDispatchResult route(
			::Window& window, const NativeInputEvent& nativeEvent,
			const KeyboardRoutingEnvironment& environment);
		// 进入模态作用域时暂停当前焦点路径并清理 armed 等瞬时状态；即使没有
		// 原生激活消息也会执行，但逻辑焦点本身仍然保留。
		static KeyboardRouteDispatchResult suspendFocusedPath(::Window& window);
		// 离开模态或恢复激活后，通知仍然有效的当前焦点路径恢复键盘路由。
		static KeyboardRouteDispatchResult resumeFocusedPath(::Window& window);
	};
}
