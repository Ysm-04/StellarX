/*******************************************************************************
 * StellarX 原生键盘、文字、IME、Pointer 与窗口激活事件模型。
 * WndProc 负责采集并规范化事件，NativeInputQueue 按序跨线程交付；
 * EasyX EX_KEY/EX_MOUSE 仅继续排空和观察，不再进入业务路由。
 ******************************************************************************/
#pragma once

#include <windows.h>

#include <cstdint>
#include <string>
#include <variant>

namespace StellarX
{
	/// 按键动作类型。
	enum class KeyAction : std::uint8_t
	{
		Down, ///< 按键按下。
		Up    ///< 按键抬起。
	};

	/// WndProc 采集时冻结的左右修饰键快照；延迟消费者不得重新读取全局键态。
	struct KeyModifiers
	{
		/// 左 Control 键是否按下。
		bool leftControl = false;
		/// 右 Control 键是否按下。
		bool rightControl = false;
		/// 左 Shift 键是否按下。
		bool leftShift = false;
		/// 右 Shift 键是否按下。
		bool rightShift = false;
		/// 左 Alt 键是否按下。
		bool leftAlt = false;
		/// 右 Alt 键是否按下。
		bool rightAlt = false;

		/// 任一 Control 键是否按下。
		bool control() const noexcept { return leftControl || rightControl; }
		/// 任一 Shift 键是否按下。
		bool shift() const noexcept { return leftShift || rightShift; }
		/// 任一 Alt 键是否按下。
		bool alt() const noexcept { return leftAlt || rightAlt; }
	};

	/// WndProc 采集并规范化的一次键盘事件。
	struct KeyEvent
	{
		/// 按下或抬起动作。
		KeyAction action = KeyAction::Down;

		UINT virtualKey = 0;          // WndProc 原始 VK；VK_PROCESSKEY 永不改写。
		UINT physicalVirtualKey = 0;  // 已区分左右修饰键的物理 VK。
		/// 原生 lParam 中的扫描码。
		UINT scanCode = 0;

		/// 原生消息携带的重复计数。
		std::uint16_t repeatCount = 0;
		/// 原生 lParam 是否标记为扩展键。
		bool extended = false;
		/// 本次消息到来前该键是否已经处于按下状态。
		bool wasDown = false;
		bool altContext = false; // 原始 lParam 第 29 位；modifiers.alt() 是语义快照。
		/// 本事件是否来自 WM_SYSKEYDOWN/WM_SYSKEYUP 系统键消息。
		bool systemKey = false;

		// 采集时的标准 IME 生命周期轴快照；不得覆盖 VK_PROCESSKEY 表达的单键所有权。
		bool imeComposingAtCapture = false;

		/// 采集时冻结的左右修饰键状态。
		KeyModifiers modifiers{};

		/// 判断本事件是否为系统自动重复产生的按下事件。
		bool isAutoRepeat() const noexcept
		{
			return action == KeyAction::Down && wasDown;
		}

		/// 判断采集时该单键是否由 IME 持有；该判断不等同于持久组合状态。
		bool isImeOwnedAtCapture() const noexcept
		{
			// 单键所有权与 IME 生命周期是两条独立轴。标准组合期间采集的普通键
			// 仍是普通键；路由可因生命周期快照抑制它，但不得改写其所有权。
			return virtualKey == VK_PROCESSKEY;
		}
	};

	/// 正式文字输入的来源。
	enum class TextSource : std::uint8_t
	{
		ImeResult,        ///< 来自 IME 的正式结果串。
		UnicodeChar,      ///< 来自 WM_CHAR 的 Unicode code unit。
		UnicodeScalar,    ///< 来自 WM_UNICHAR 的 Unicode scalar。
		AnsiCharFallback, ///< 来自 ANSI 窗口字符消息的兼容转换。
		Clipboard         ///< 来自剪贴板粘贴。
	};

	/// 一次正式 UTF-16 文字提交事件。
	struct TextInputEvent
	{
		/// 待提交的 UTF-16 文字。
		std::u16string text;
		/// 文字来源。
		TextSource source = TextSource::UnicodeChar;
		/// 所属组合轮次；非 IME 输入为 0。
		std::uint64_t compositionId = 0;
	};

	/// IME 组合生命周期阶段。
	enum class CompositionStage : std::uint8_t
	{
		Start,  ///< 新组合轮次开始。
		Update, ///< 临时组合串或 caret 更新。
		Commit, ///< IME 宣告提交；正式文本仍由 TextInputEvent(ImeResult) 写入。
		Cancel, ///< 组合被取消。
		End     ///< 组合生命周期结束。
	};

	/// 一次 IME 临时组合生命周期事件。
	struct CompositionEvent
	{
		/// 当前生命周期阶段。
		CompositionStage stage = CompositionStage::Start;
		/// 当前组合轮次的稳定编号。
		std::uint64_t compositionId = 0;
		/// 当前 UTF-16 临时组合串；不得直接写入正式编辑模型。
		std::u16string compositionText;
		/// IME caret 在组合串中的 UTF-16 code unit 偏移。
		std::uint32_t caretOffset = 0;
	};

	/// 原生窗口键盘激活状态变化事件。
	struct WindowActivationEvent
	{
		/// 当前窗口是否拥有键盘焦点。
		bool keyboardFocusActive = false;
		/// 获得或失去焦点时涉及的另一个原生窗口。
		HWND otherWindow = nullptr;
	};

	/// 原生 Pointer 动作类型。
	enum class PointerAction : std::uint8_t
	{
		Move,                  ///< 指针移动。
		LeftDown,              ///< 左键按下。
		LeftUp,                ///< 左键抬起。
		LeftDoubleClick,       ///< 左键双击。
		RightDown,             ///< 右键按下。
		RightUp,               ///< 右键抬起。
		RightDoubleClick,      ///< 右键双击。
		MiddleDown,            ///< 中键按下。
		MiddleUp,              ///< 中键抬起。
		MiddleDoubleClick,     ///< 中键双击。
		VerticalWheel,         ///< 垂直滚轮。
		HorizontalWheel,       ///< 水平滚轮。
		CaptureChanged         ///< 原生鼠标捕获目标发生变化。
	};

	/// EasyX HWND 属主线程采集的 Pointer 事实。
	/// 其中不保存控件或焦点身份；宿主线程只在更早的 NativeInputEvent 全部
	/// 处理完毕后，才根据当前控件树解析目标。
	struct PointerEvent
	{
		/// 当前 Pointer 动作。
		PointerAction action = PointerAction::Move;
		/// 采集时的 HWND 客户区 X 坐标。
		int clientX = 0;
		/// 采集时的 HWND 客户区 Y 坐标。
		int clientY = 0;
		/// 滚轮增量；非滚轮事件为 0。
		int wheelDelta = 0;
		/// 采集时左键是否按下。
		bool leftButton = false;
		/// 采集时中键是否按下。
		bool middleButton = false;
		/// 采集时右键是否按下。
		bool rightButton = false;
		/// 采集时 Control 键是否按下。
		bool control = false;
		/// 采集时 Shift 键是否按下。
		bool shift = false;
		// 滚轮坐标在采集时通过 ScreenToClient 转换。CaptureChanged 没有指针
		// 坐标；转换失败会通过该字段显式报告。
		bool clientCoordinatesValid = true;
		/// CaptureChanged 后的新捕获窗口；无捕获时为 nullptr。
		HWND captureChangedTo = nullptr;
	};

	/// 复制到每个原生输入事件中的生产者激活快照。
	/// 键盘激活状态变化时 generation 必须递增，从而阻止延迟事件仅因宿主
	/// 排空时窗口已再次激活而跨越失活/恢复边界。
	struct NativeInputActivationSnapshot
	{
		/// 采集时窗口是否拥有键盘焦点。
		bool keyboardFocusActive = false;
		/// 每次原生键盘激活状态变化时递增的代次。
		std::uint64_t generation = 0;
	};

	/// NativeInputQueue 可承载的规范化事件负载联合。
	using NativeInputPayload = std::variant<
		KeyEvent,
		TextInputEvent,
		CompositionEvent,
		WindowActivationEvent,
		PointerEvent>;

	/// 跨线程队列中的一个完整原生输入事件。
	struct NativeInputEvent
	{
		/// 全局严格递增序号，用于保持不同输入类型之间的原生顺序。
		std::uint64_t sequence = 0;
		/// 原生消息采集时的 Win32 消息时间。
		DWORD messageTime = 0;
		/// 产生事件的 EasyX 原生窗口句柄。
		HWND hwnd = nullptr;
		/// 采集时冻结的窗口激活状态。
		NativeInputActivationSnapshot activationAtCapture{};
		/// 本事件的具体规范化负载。
		NativeInputPayload payload{};
	};
}
