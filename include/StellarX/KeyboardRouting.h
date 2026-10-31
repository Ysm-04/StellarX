/*******************************************************************************
 * StellarX 定向键盘路由契约。
 * 规范化按键通过稳定焦点身份沿逻辑父链冒泡，控件可以分别停止冒泡和
 * 阻止框架默认动作；该结果不会回溯改变已经完成的 WndProc 返回值。
 ******************************************************************************/
#pragma once

#include "InputEvents.h"
#include "CoreTypes.h"

namespace StellarX
{
	/// 规范化按键及其在焦点路径中冒泡时使用的稳定身份。
	/// 路由器会在每次调用控件前更新 currentTarget；控件不得把任何身份
	/// 解析成裸指针后跨越当前同步调用保存。
	struct RoutedKeyEvent
	{
		/// NativeInputQueue 中的全局严格递增序号。
		std::uint64_t sequence = 0;
		/// 原生消息采集时的 Win32 消息时间。
		DWORD messageTime = 0;
		/// 产生本事件的 EasyX 原生窗口句柄；仅用于本次同步观察。
		HWND hwnd = nullptr;
		/// WndProc 采集并规范化后的按键快照。
		KeyEvent key{};
		/// 路由开始时冻结的原始焦点目标。
		FocusTargetId originalTarget{};
		/// 当前即将接收事件的焦点路径节点。
		FocusTargetId currentTarget{};
	};

	/// 单个控件处理键盘事件后的路由裁决。
	/// 停止控件冒泡和阻止后续框架默认动作彼此独立；两者都不能改变
	/// 已经完成的 WndProc 返回值。
	struct KeyRouteResult
	{
		/// 为 true 时，不再把当前按键交给后续逻辑父节点。
		bool stopBubble = false;
		/// 为 true 时，跳过 Tab 遍历等框架默认动作。
		bool preventFrameworkDefault = false;
	};
}
