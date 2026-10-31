/*******************************************************************************
 * StellarX 定向文字输入与 IME 组合路由契约。
 * 文字只投递给一个焦点叶节点，不沿逻辑父链冒泡，也没有框架默认动作阶段。
 ******************************************************************************/
#pragma once

#include "CoreTypes.h"
#include "InputEvents.h"

#include <cstdint>
#include <string_view>

namespace StellarX
{
	/// 投递给焦点文字控件的一次正式文字输入。
	/// text 视图只在同步调用 handleTextInputEvent() 期间有效；控件若需在
	/// 回调返回后保留文字，必须自行复制。
	struct RoutedTextInputEvent
	{
		/// NativeInputQueue 中的全局严格递增序号。
		std::uint64_t sequence = 0;
		/// 原生消息采集时的 Win32 消息时间。
		DWORD messageTime = 0;
		/// 产生本事件的 EasyX 原生窗口句柄。
		HWND hwnd = nullptr;
		/// 本次正式提交的 UTF-16 文字借用视图。
		std::u16string_view text{};
		/// 文字来源，用于区分 IME 结果、Unicode 输入和兼容回退。
		TextSource source = TextSource::UnicodeChar;
		/// 所属组合轮次；非 IME 输入为 0。
		std::uint64_t compositionId = 0;
		/// 本次文字输入唯一允许投递的稳定焦点目标。
		FocusTargetId target{};
	};

	/// 定向投递的 IME 临时组合生命周期事件。
	/// compositionText 视图仅在同步回调期间有效，并且只能作为临时 preedit
	/// 显示；控件不得把它写入正式编辑模型。随后同一组合轮次的
	/// TextInputEvent(ImeResult) 才是唯一正式提交来源。
	struct RoutedCompositionEvent
	{
		/// NativeInputQueue 中的全局严格递增序号。
		std::uint64_t sequence = 0;
		/// 原生消息采集时的 Win32 消息时间。
		DWORD messageTime = 0;
		/// 产生本事件的 EasyX 原生窗口句柄。
		HWND hwnd = nullptr;
		/// 当前组合生命周期阶段。
		CompositionStage stage = CompositionStage::Start;
		/// 当前组合轮次的稳定编号。
		std::uint64_t compositionId = 0;
		/// 当前 UTF-16 临时组合串的借用视图。
		std::u16string_view compositionText{};
		/// IME caret 在组合串中的 UTF-16 code unit 偏移。
		std::uint32_t caretOffset = 0;
		/// 当前组合轮次绑定的稳定焦点目标。
		FocusTargetId target{};
	};

	/// 文字路由的隐私安全聚合计数。
	/// 路由器不保留文字、哈希、前缀或控件指针。
	struct TextInputRoutingStats
	{
		/// 被路由器观察到的文字事件总数。
		std::uint64_t observedEvents = 0;
		/// 成功投递到目标控件的事件总数。
		std::uint64_t deliveredEvents = 0;
		/// 因目标、作用域或激活资格不满足而丢弃的事件总数。
		std::uint64_t droppedEvents = 0;
		/// 被丢弃事件所包含的 UTF-16 code unit 总数。
		std::uint64_t droppedCodeUnits = 0;
	};

	/// IME 组合路由的隐私安全聚合计数。
	struct CompositionRoutingStats
	{
		/// 被路由器观察到的组合事件总数。
		std::uint64_t observedEvents = 0;
		/// 成功投递到绑定目标的组合事件总数。
		std::uint64_t deliveredEvents = 0;
		/// 因轮次或目标资格不匹配而丢弃的组合事件总数。
		std::uint64_t droppedEvents = 0;
		/// 缺少 Start 时由首个 Update 建立绑定的次数。
		std::uint64_t lazyBindings = 0;
	};
}
