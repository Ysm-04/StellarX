/*******************************************************************************
 * StellarX 定向文字选区手势契约。
 * Pointer 路由使用稳定身份把选区手势交给焦点文字叶节点；事件不拥有控件，
 * 也不允许接收方跨越同步回调保存借用指针。
 ******************************************************************************/
#pragma once

#include "CoreTypes.h"

#include <cstdint>

namespace StellarX
{
	/// 一次文字选区手势的生命周期阶段。
	enum class TextSelectionGesturePhase : std::uint8_t
	{
		Begin,  ///< 按下并开始新的选区手势。
		Update, ///< 指针移动并更新当前选区。
		End,    ///< 正常释放并结束手势。
		Cancel  ///< 因失活、目标失效或捕获取消而终止手势。
	};

	/// 投递给焦点文字控件的一次选区手势事件。
	/// 坐标是 NativeInputQueue 采集的 HWND 客户区坐标。事件仅在同步回调
	/// 期间有效；控件可以保留稳定的 scalar 偏移，但不得保留事件或借用指针。
	struct RoutedTextSelectionGestureEvent
	{
		/// NativeInputQueue 中的全局严格递增序号。
		std::uint64_t sequence = 0;
		/// 原生消息采集时的 Win32 消息时间。
		DWORD messageTime = 0;
		/// 产生本事件的 EasyX 原生窗口句柄。
		HWND hwnd = nullptr;
		/// 当前选区手势阶段。
		TextSelectionGesturePhase phase = TextSelectionGesturePhase::Begin;
		/// 指针在 HWND 客户区中的 X 坐标。
		int clientX = 0;
		/// 指针在 HWND 客户区中的 Y 坐标。
		int clientY = 0;
		/// 坐标转换是否成功；为 false 时接收方不得使用 clientX/clientY。
		bool clientCoordinatesValid = false;
		/// 采集时 Control 修饰键是否按下。
		bool control = false;
		/// 采集时 Shift 修饰键是否按下。
		bool shift = false;
		/// 本次手势绑定的稳定焦点目标。
		FocusTargetId target{};
	};
}
