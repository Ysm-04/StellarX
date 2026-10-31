#include "SxNativeInputInternal.h"
#include "SxUnicodeInternal.h"

#include <imm.h>
#include <windowsx.h>

#include <algorithm>
#include <array>
#include <cwchar>
#include <new>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#pragma comment(lib, "imm32.lib")

namespace StellarX::Internal
{
	namespace
	{
		std::atomic<std::uint64_t> nextNativeInputSequence{ 0 };
		constexpr std::uint64_t ActivationFlagMask = 1;
		constexpr std::uint64_t MaximumActivationGeneration =
			(std::numeric_limits<std::uint64_t>::max)() >> 1;

		std::uint64_t packActivationSnapshot(
			const NativeInputActivationSnapshot& snapshot) noexcept
		{
			return (snapshot.generation << 1) |
				(snapshot.keyboardFocusActive ? ActivationFlagMask : 0);
		}

		NativeInputActivationSnapshot unpackActivationSnapshot(
			std::uint64_t packed) noexcept
		{
			NativeInputActivationSnapshot snapshot;
			snapshot.keyboardFocusActive = (packed & ActivationFlagMask) != 0;
			snapshot.generation = packed >> 1;
			return snapshot;
		}

		std::uint64_t initialActivationState(HWND window) noexcept
		{
			bool active = false;
			if (window && IsWindow(window) != FALSE)
			{
				DWORD processId = 0;
				const DWORD threadId = GetWindowThreadProcessId(window, &processId);
				GUITHREADINFO info{};
				info.cbSize = sizeof(info);
				active = threadId != 0 && GetGUIThreadInfo(threadId, &info) != FALSE &&
					info.hwndFocus == window;
			}
			return packActivationSnapshot({ active, 1 });
		}

		static_assert(std::is_nothrow_move_constructible_v<TextInputEvent>,
			"TextInputEvent must transfer its u16string without allocating during queue publication.");
		static_assert(std::is_nothrow_move_constructible_v<CompositionEvent>,
			"CompositionEvent must transfer its u16string without allocating during queue publication.");
		static_assert(std::is_nothrow_move_constructible_v<NativeInputEvent>,
			"NativeInputEvent must be nothrow move constructible.");
		static_assert(std::is_nothrow_move_assignable_v<NativeInputEvent>,
			"NativeInputEvent must be nothrow move assignable.");

		UINT codePageForLayout(HKL layout) noexcept
		{
			if (layout)
			{
				wchar_t buffer[16]{};
				const LANGID language = LOWORD(reinterpret_cast<ULONG_PTR>(layout));
				const LCID locale = MAKELCID(language, SORT_DEFAULT);
				if (GetLocaleInfoW(locale, LOCALE_IDEFAULTANSICODEPAGE,
					buffer, static_cast<int>(std::size(buffer))) > 0)
				{
					wchar_t* end = nullptr;
					const unsigned long value = std::wcstoul(buffer, &end, 10);
					if (end != buffer && value > 0 && value <= (std::numeric_limits<UINT>::max)())
						return static_cast<UINT>(value);
				}
			}
			return GetACP();
		}

		KeyModifierSnapshot captureKeyModifiers() noexcept
		{
			auto isDown = [](int virtualKey) noexcept
				{
					return (GetKeyState(virtualKey) & 0x8000) != 0;
				};

			KeyModifierSnapshot snapshot;
			snapshot.leftControl = isDown(VK_LCONTROL);
			snapshot.rightControl = isDown(VK_RCONTROL);
			snapshot.leftShift = isDown(VK_LSHIFT);
			snapshot.rightShift = isDown(VK_RSHIFT);
			snapshot.leftAlt = isDown(VK_LMENU);
			snapshot.rightAlt = isDown(VK_RMENU);
			return snapshot;
		}

		UINT physicalVirtualKey(UINT virtualKey, UINT scanCode, bool extended) noexcept
		{
			switch (virtualKey)
			{
			case VK_SHIFT:
				return scanCode == 0x36 ? VK_RSHIFT : VK_LSHIFT;
			case VK_CONTROL:
				return extended ? VK_RCONTROL : VK_LCONTROL;
			case VK_MENU:
				return extended ? VK_RMENU : VK_LMENU;
			default:
				return virtualKey;
			}
		}

		bool queryCompositionString(HIMC context, DWORD index, std::u16string& text)
		{
			const LONG byteCount = ImmGetCompositionStringW(context, index, nullptr, 0);
			if (byteCount < 0 || (byteCount % static_cast<LONG>(sizeof(wchar_t))) != 0)
				return false;

			const std::size_t unitCount = static_cast<std::size_t>(byteCount) / sizeof(wchar_t);
			std::wstring wide(unitCount, L'\0');
			if (byteCount == 0)
			{
				text.clear();
				return true;
			}

			const LONG copied = ImmGetCompositionStringW(context, index, wide.data(), byteCount);
			return copied == byteCount && windowsWideToUtf16(wide, text);
		}

		bool convertAnsiStrict(UINT codePage, const unsigned char* bytes,
			std::size_t byteCount, std::u16string& text)
		{
			if (!bytes || byteCount == 0 || byteCount > 2)
				return false;

			wchar_t buffer[2]{};
			const int converted = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS,
				reinterpret_cast<const char*>(bytes), static_cast<int>(byteCount),
				buffer, static_cast<int>(std::size(buffer)));
			if (converted <= 0)
				return false;
			return windowsWideToUtf16(std::wstring_view(buffer,
				static_cast<std::size_t>(converted)), text);
		}
	}

	NativeInputQueuePushResult NativeInputQueue::push(NativeInputEvent&& event) noexcept
	{
		const NativeInputQueuePushResult forced = forcedPushResult_.exchange(
			NativeInputQueuePushResult::Queued, std::memory_order_acq_rel);
		if (forced != NativeInputQueuePushResult::Queued)
			return forced;

		// 队列正常运行时不丢事件；分配或容器异常通过返回值上报，由上下文
		// 进入显式故障状态，而不是悄悄跳过一次输入。
		try
		{
			std::lock_guard<std::mutex> lock(mutex_);
			events_.emplace_back(std::move(event));
			++pushed_;
			maximumDepth_ = (std::max)(maximumDepth_, events_.size());
			return NativeInputQueuePushResult::Queued;
		}
		catch (const std::bad_alloc&)
		{
			return NativeInputQueuePushResult::AllocationFailure;
		}
		catch (...)
		{
			return NativeInputQueuePushResult::UnexpectedFailure;
		}
	}

	NativeInputQueuePopResult NativeInputQueue::tryPop(NativeInputEvent& event) noexcept
	{
		try
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (events_.empty())
				return NativeInputQueuePopResult::Empty;
			event = std::move(events_.front());
			events_.pop_front();
			++popped_;
			return NativeInputQueuePopResult::Popped;
		}
		catch (...)
		{
			return NativeInputQueuePopResult::UnexpectedFailure;
		}
	}

	NativeInputQueueStats NativeInputQueue::stats() const noexcept
	{
		try
		{
			std::lock_guard<std::mutex> lock(mutex_);
			NativeInputQueueStats snapshot;
			snapshot.pushed = pushed_;
			snapshot.popped = popped_;
			snapshot.currentDepth = events_.size();
			snapshot.maximumDepth = maximumDepth_;
			return snapshot;
		}
		catch (...)
		{
			return {};
		}
	}

	void NativeInputQueue::forceNextPushResultForTesting(NativeInputQueuePushResult result) noexcept
	{
		forcedPushResult_.store(result, std::memory_order_release);
	}

	KeyEvent makeKeyEvent(UINT message, WPARAM wParam, LPARAM lParam,
		const KeyModifierSnapshot& modifierSnapshot, bool imeLifecycleActive) noexcept
	{
		const auto raw = static_cast<std::uint32_t>(static_cast<ULONG_PTR>(lParam));
		KeyEvent event;
		event.action = message == WM_KEYUP || message == WM_SYSKEYUP
			? KeyAction::Up : KeyAction::Down;
		event.virtualKey = static_cast<UINT>(wParam);
		event.repeatCount = static_cast<std::uint16_t>(raw & 0xFFFFu);
		event.scanCode = (raw >> 16) & 0xFFu;
		event.extended = (raw & (1u << 24)) != 0;
		event.altContext = (raw & (1u << 29)) != 0;
		event.wasDown = (raw & (1u << 30)) != 0;
		event.systemKey = message == WM_SYSKEYDOWN || message == WM_SYSKEYUP;
		event.imeComposingAtCapture = imeLifecycleActive;
		event.physicalVirtualKey = physicalVirtualKey(event.virtualKey, event.scanCode, event.extended);
		event.modifiers.leftControl = modifierSnapshot.leftControl;
		event.modifiers.rightControl = modifierSnapshot.rightControl;
		event.modifiers.leftShift = modifierSnapshot.leftShift;
		event.modifiers.rightShift = modifierSnapshot.rightShift;
		event.modifiers.leftAlt = modifierSnapshot.leftAlt;
		event.modifiers.rightAlt = modifierSnapshot.rightAlt;
		return event;
	}

	bool makePointerEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
		PointerEvent& event) noexcept
	{
		event = {};
		switch (message)
		{
		case WM_MOUSEMOVE: event.action = PointerAction::Move; break;
		case WM_LBUTTONDOWN: event.action = PointerAction::LeftDown; break;
		case WM_LBUTTONUP: event.action = PointerAction::LeftUp; break;
		case WM_LBUTTONDBLCLK: event.action = PointerAction::LeftDoubleClick; break;
		case WM_RBUTTONDOWN: event.action = PointerAction::RightDown; break;
		case WM_RBUTTONUP: event.action = PointerAction::RightUp; break;
		case WM_RBUTTONDBLCLK: event.action = PointerAction::RightDoubleClick; break;
		case WM_MBUTTONDOWN: event.action = PointerAction::MiddleDown; break;
		case WM_MBUTTONUP: event.action = PointerAction::MiddleUp; break;
		case WM_MBUTTONDBLCLK: event.action = PointerAction::MiddleDoubleClick; break;
		case WM_MOUSEWHEEL: event.action = PointerAction::VerticalWheel; break;
		case WM_MOUSEHWHEEL: event.action = PointerAction::HorizontalWheel; break;
		case WM_CAPTURECHANGED:
			event.action = PointerAction::CaptureChanged;
			event.clientCoordinatesValid = false;
			event.captureChangedTo = reinterpret_cast<HWND>(lParam);
			return true;
		default:
			return false;
		}

		const UINT keyState = GET_KEYSTATE_WPARAM(wParam);
		event.leftButton = (keyState & MK_LBUTTON) != 0;
		event.middleButton = (keyState & MK_MBUTTON) != 0;
		event.rightButton = (keyState & MK_RBUTTON) != 0;
		event.control = (keyState & MK_CONTROL) != 0;
		event.shift = (keyState & MK_SHIFT) != 0;

		if (message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL)
		{
			event.wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			if (!window || ScreenToClient(window, &point) == FALSE ||
				point.x < (std::numeric_limits<short>::min)() ||
				point.x > (std::numeric_limits<short>::max)() ||
				point.y < (std::numeric_limits<short>::min)() ||
				point.y > (std::numeric_limits<short>::max)())
			{
				event.clientCoordinatesValid = false;
				return true;
			}
			event.clientX = point.x;
			event.clientY = point.y;
			return true;
		}

		event.clientX = GET_X_LPARAM(lParam);
		event.clientY = GET_Y_LPARAM(lParam);
		return true;
	}

	struct NativeInputContext::Impl
	{
		enum class EchoDisposition : std::uint8_t
		{
			NotEcho,
			Suppress
		};

		struct EchoState
		{
			bool active = false;
			bool windowUnicode = false;
			std::uint64_t compositionId = 0;
			std::u16string utf16;
			std::string ansi;
			std::size_t imeCharMatched = 0;
			std::size_t charMatched = 0;
		};

		explicit Impl(HWND window)
			: hwnd(window), windowUnicode(window && IsWindowUnicode(window) != FALSE),
			activationState(initialActivationState(window))
		{
			DWORD processId = 0;
			windowThreadId = window ? GetWindowThreadProcessId(window, &processId) : GetCurrentThreadId();
			keyboardLayout = GetKeyboardLayout(windowThreadId);
			ansiCodePage = codePageForLayout(keyboardLayout);
		}

		HWND hwnd = nullptr;
		bool windowUnicode = false;
		DWORD windowThreadId = 0;
		HKL keyboardLayout = nullptr;
		UINT ansiCodePage = CP_ACP;
		std::optional<unsigned char> pendingLead;
		std::optional<char16_t> pendingHighSurrogate;

		bool imeLifecycleActive = false;
		bool compositionCommitted = false;
		bool compositionCancelled = false;
		std::uint64_t nextCompositionId = 0;
		std::uint64_t activeCompositionId = 0;
		EchoState echo;
		std::atomic<std::uint64_t> activationState;

		NativeInputQueue queue;
		NativeInputObservationStats observations;

		std::atomic<bool> faultClaimed{ false };
		std::atomic<bool> faultReady{ false };
		NativeInputFaultSnapshot faultSnapshot{};
		std::atomic<std::uint64_t> rejectedTextSequences{ 0 };
		std::atomic<std::uint64_t> imeQueryFailures{ 0 };
		std::atomic<std::uint64_t> discardedDecoderFragments{ 0 };
		std::atomic<std::uint64_t> pointerCoordinateFailures{ 0 };

		bool hasFault() const noexcept
		{
			return faultReady.load(std::memory_order_acquire);
		}

		void publishFault(NativeInputFaultCode code, std::uint64_t sequence,
			UINT message, HWND window) noexcept
		{
			// 只保留首个故障。发布过程不分配内存，ready 的 release 写保证宿主
			// 看到标志后也能读到完整快照。
			bool expected = false;
			if (!faultClaimed.compare_exchange_strong(expected, true,
				std::memory_order_acq_rel, std::memory_order_acquire))
				return;

			faultSnapshot.code = code;
			faultSnapshot.sequence = sequence;
			faultSnapshot.message = message;
			faultSnapshot.hwnd = window;
			faultReady.store(true, std::memory_order_release);
		}

		void clearTextDecoderState() noexcept
		{
			if (pendingLead || pendingHighSurrogate)
				discardedDecoderFragments.fetch_add(1, std::memory_order_relaxed);
			pendingLead.reset();
			pendingHighSurrogate.reset();
		}

		void clearEcho() noexcept
		{
			echo = {};
		}

		NativeInputActivationSnapshot activationSnapshot() const noexcept
		{
			return unpackActivationSnapshot(
				activationState.load(std::memory_order_acquire));
		}

		NativeInputActivationSnapshot publishActivation(bool active) noexcept
		{
			std::uint64_t current = activationState.load(std::memory_order_acquire);
			for (;;)
			{
				const NativeInputActivationSnapshot old = unpackActivationSnapshot(current);
				if (old.keyboardFocusActive == active)
					return old;
				const std::uint64_t nextGeneration =
					old.generation >= MaximumActivationGeneration ? 1 : old.generation + 1;
				const NativeInputActivationSnapshot next{ active, nextGeneration };
				const std::uint64_t desired = packActivationSnapshot(next);
				if (activationState.compare_exchange_weak(current, desired,
					std::memory_order_acq_rel, std::memory_order_acquire))
					return next;
			}
		}

		void snapshotKeyboardLayout(HKL layout) noexcept
		{
			const HKL actual = layout ? layout : GetKeyboardLayout(0);
			const UINT actualCodePage = codePageForLayout(actual);
			if (actual != keyboardLayout || actualCodePage != ansiCodePage)
			{
				if (pendingLead)
					discardedDecoderFragments.fetch_add(1, std::memory_order_relaxed);
				pendingLead.reset();
				keyboardLayout = actual;
				ansiCodePage = actualCodePage;
			}
		}

		std::uint64_t allocateCompositionId() noexcept
		{
			do
			{
				++nextCompositionId;
			} while (nextCompositionId == 0);
			activeCompositionId = nextCompositionId;
			return activeCompositionId;
		}

		template <typename Payload>
		bool enqueue(HWND window, UINT message, Payload&& payload) noexcept
		{
			if (hasFault())
				return false;

			NativeInputEvent event;
			event.sequence = nextNativeInputSequence.fetch_add(1, std::memory_order_relaxed) + 1;
			event.messageTime = static_cast<DWORD>(GetMessageTime());
			event.hwnd = window;
			event.activationAtCapture = activationSnapshot();
			event.payload = std::forward<Payload>(payload);
			const std::uint64_t sequence = event.sequence;
			const NativeInputQueuePushResult result = queue.push(std::move(event));
			if (result == NativeInputQueuePushResult::Queued)
				return true;

			publishFault(result == NativeInputQueuePushResult::AllocationFailure
				? NativeInputFaultCode::OutOfMemory : NativeInputFaultCode::QueueFailure,
				sequence, message, window);
			return false;
		}

		bool ensureComposition(HWND window, UINT message)
		{
			if (imeLifecycleActive)
				return true;

			// 输入法可能省略 START；首次 COMPOSITION 也要建立身份并入队 Start，
			// 让下游始终按同一会话模型处理。
			imeLifecycleActive = true;
			compositionCommitted = false;
			compositionCancelled = false;
			allocateCompositionId();
			CompositionEvent start;
			start.stage = CompositionStage::Start;
			start.compositionId = activeCompositionId;
			return enqueue(window, message, std::move(start));
		}

		void beginEcho(const std::u16string& text, std::uint64_t compositionId)
		{
			clearEcho();
			if (text.empty() || !isValidUtf16(text))
				return;

			echo.active = true;
			echo.windowUnicode = windowUnicode;
			echo.compositionId = compositionId;
			echo.utf16 = text;
			if (windowUnicode)
				return;

			std::wstring wide;
			if (!utf16ToWindowsWide(text, wide))
			{
				clearEcho();
				return;
			}

			const bool supportsDefaultTracking = ansiCodePage != CP_UTF7 && ansiCodePage != CP_UTF8;
			const DWORD conversionFlags = supportsDefaultTracking ? WC_NO_BEST_FIT_CHARS : 0;
			BOOL usedDefault = FALSE;
			BOOL* usedDefaultPointer = supportsDefaultTracking ? &usedDefault : nullptr;
			const int required = WideCharToMultiByte(ansiCodePage, conversionFlags,
				wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, usedDefaultPointer);
			if (required <= 0)
			{
				// 没有确定的预期字节序列，就无法安全地按内容抑制；遇到首个回声即结束关联。
				echo.ansi.clear();
				return;
			}

			echo.ansi.assign(static_cast<std::size_t>(required), '\0');
			usedDefault = FALSE;
			const int converted = WideCharToMultiByte(ansiCodePage, conversionFlags,
				wide.data(), static_cast<int>(wide.size()), echo.ansi.data(), required,
				nullptr, usedDefaultPointer);
			if (converted != required)
				echo.ansi.clear();
		}

		bool matchBytes(const unsigned char* bytes, std::size_t count,
			std::size_t& matched) const noexcept
		{
			if (matched + count > echo.ansi.size())
				return false;
			for (std::size_t index = 0; index < count; ++index)
			{
				if (static_cast<unsigned char>(echo.ansi[matched + index]) != bytes[index])
					return false;
			}
			matched += count;
			return true;
		}

		bool matchUtf16(char16_t value, std::size_t& matched) const noexcept
		{
			if (matched >= echo.utf16.size() || echo.utf16[matched] != value)
				return false;
			++matched;
			return true;
		}

		EchoDisposition correlateEcho(UINT message, WPARAM wParam) noexcept
		{
			if (!echo.active || (message != WM_IME_CHAR && message != WM_CHAR))
				return EchoDisposition::NotEcho;

			std::size_t& matched = message == WM_IME_CHAR
				? echo.imeCharMatched : echo.charMatched;
			bool matches = false;
			if (echo.windowUnicode)
			{
				matches = matchUtf16(static_cast<char16_t>(wParam), matched);
			}
			else if (message == WM_IME_CHAR)
			{
				unsigned char bytes[2]{};
				std::size_t count = 1;
				if ((wParam & 0xFF00u) != 0)
				{
					bytes[0] = static_cast<unsigned char>((wParam >> 8) & 0xFFu);
					bytes[1] = static_cast<unsigned char>(wParam & 0xFFu);
					count = 2;
				}
				else
				{
					bytes[0] = static_cast<unsigned char>(wParam & 0xFFu);
				}
				matches = matchBytes(bytes, count, matched);
			}
			else
			{
				const unsigned char byte = static_cast<unsigned char>(wParam & 0xFFu);
				matches = matchBytes(&byte, 1, matched);
			}

			if (matches)
				return EchoDisposition::Suppress;

			// 只有紧邻且逐 code unit 相同的内容才算回声。首次不匹配即结束关联，
			// 防止稍后出现的同文用户输入仅因内容相同而被删除。
			clearEcho();
			return EchoDisposition::NotEcho;
		}

		std::optional<std::u16string> decodeUnicodeUnit(char16_t value)
		{
			// 高代理项跨消息暂存；只有收到匹配低代理项后才交付完整 scalar。
			if (pendingHighSurrogate)
			{
				const char16_t high = *pendingHighSurrogate;
				pendingHighSurrogate.reset();
				if (isLowUtf16Surrogate(value))
				{
					std::u16string text;
					text.push_back(high);
					text.push_back(value);
					return text;
				}
				rejectedTextSequences.fetch_add(1, std::memory_order_relaxed);
			}

			if (isHighUtf16Surrogate(value))
			{
				pendingHighSurrogate = value;
				return std::nullopt;
			}
			if (isLowUtf16Surrogate(value))
			{
				rejectedTextSequences.fetch_add(1, std::memory_order_relaxed);
				return std::nullopt;
			}
			return std::u16string(1, value);
		}

		std::optional<std::u16string> decodeAnsiChar(unsigned char byte)
		{
			// DBCS 前导字节同样跨消息暂存，输入语言变化或焦点切换会显式清理残片。
			if (pendingLead)
			{
				const unsigned char bytes[2]{ *pendingLead, byte };
				pendingLead.reset();
				std::u16string text;
				if (!convertAnsiStrict(ansiCodePage, bytes, 2, text))
				{
					rejectedTextSequences.fetch_add(1, std::memory_order_relaxed);
					return std::nullopt;
				}
				return text;
			}

			if (IsDBCSLeadByteEx(ansiCodePage, byte) != FALSE)
			{
				pendingLead = byte;
				return std::nullopt;
			}

			std::u16string text;
			if (!convertAnsiStrict(ansiCodePage, &byte, 1, text))
			{
				rejectedTextSequences.fetch_add(1, std::memory_order_relaxed);
				return std::nullopt;
			}
			return text;
		}

		std::optional<std::u16string> decodeImeChar(WPARAM wParam)
		{
			if (windowUnicode)
				return decodeUnicodeUnit(static_cast<char16_t>(wParam));

			unsigned char bytes[2]{};
			std::size_t count = 1;
			if ((wParam & 0xFF00u) != 0)
			{
				bytes[0] = static_cast<unsigned char>((wParam >> 8) & 0xFFu);
				bytes[1] = static_cast<unsigned char>(wParam & 0xFFu);
				count = 2;
			}
			else
			{
				bytes[0] = static_cast<unsigned char>(wParam & 0xFFu);
			}

			std::u16string text;
			if (!convertAnsiStrict(ansiCodePage, bytes, count, text))
			{
				rejectedTextSequences.fetch_add(1, std::memory_order_relaxed);
				return std::nullopt;
			}
			return text;
		}

		bool collectKey(HWND window, UINT message, WPARAM wParam, LPARAM lParam) noexcept
		{
			if (echo.active)
				clearEcho();
			KeyEvent event = makeKeyEvent(message, wParam, lParam,
				captureKeyModifiers(), imeLifecycleActive);
			return enqueue(window, message, std::move(event));
		}

		bool collectText(HWND window, UINT message, WPARAM wParam)
		{
			if (correlateEcho(message, wParam) == EchoDisposition::Suppress)
				return true;

			std::optional<std::u16string> decoded;
			TextSource source = TextSource::AnsiCharFallback;
			if (message == WM_IME_CHAR)
			{
				decoded = decodeImeChar(wParam);
				source = windowUnicode ? TextSource::UnicodeChar : TextSource::AnsiCharFallback;
			}
			else if (windowUnicode)
			{
				decoded = decodeUnicodeUnit(static_cast<char16_t>(wParam));
				source = TextSource::UnicodeChar;
			}
			else
			{
				if (wParam > 0xFFu)
				{
					rejectedTextSequences.fetch_add(1, std::memory_order_relaxed);
					return true;
				}
				decoded = decodeAnsiChar(static_cast<unsigned char>(wParam & 0xFFu));
			}

			if (!decoded || decoded->empty())
				return true;
			TextInputEvent event;
			event.text = std::move(*decoded);
			event.source = source;
			event.compositionId = message == WM_IME_CHAR ? activeCompositionId : 0;
			return enqueue(window, message, std::move(event));
		}

		bool collectPointer(HWND window, UINT message, WPARAM wParam, LPARAM lParam) noexcept
		{
			PointerEvent event;
			if (!makePointerEvent(window, message, wParam, lParam, event))
				return false;
			if (!event.clientCoordinatesValid && event.action != PointerAction::CaptureChanged)
				pointerCoordinateFailures.fetch_add(1, std::memory_order_relaxed);
			return enqueue(window, message, std::move(event));
		}

		bool collectImeComposition(HWND window, LPARAM lParam)
		{
			if (!ensureComposition(window, WM_IME_COMPOSITION))
				return false;

			const DWORD flags = static_cast<DWORD>(lParam);
			const bool hasCompositionString = (flags & GCS_COMPSTR) != 0;
			const bool hasResultString = (flags & GCS_RESULTSTR) != 0;
			const DWORD anyGcsMask = GCS_COMPREADSTR | GCS_COMPREADATTR | GCS_COMPREADCLAUSE |
				GCS_COMPSTR | GCS_COMPATTR | GCS_COMPCLAUSE | GCS_CURSORPOS | GCS_DELTASTART |
				GCS_RESULTREADSTR | GCS_RESULTREADCLAUSE | GCS_RESULTSTR | GCS_RESULTCLAUSE;

			HIMC context = nullptr;
			if (hasCompositionString || hasResultString)
				context = ImmGetContext(window);

			bool success = true;
			if ((hasCompositionString || hasResultString) && !context)
				imeQueryFailures.fetch_add(1, std::memory_order_relaxed);

			if (hasCompositionString && context)
			{
				std::u16string composition;
				if (!queryCompositionString(context, GCS_COMPSTR, composition))
				{
					imeQueryFailures.fetch_add(1, std::memory_order_relaxed);
				}
				else if (!isValidUtf16(composition))
				{
					rejectedTextSequences.fetch_add(1, std::memory_order_relaxed);
				}
				else
				{
					CompositionEvent update;
					update.stage = CompositionStage::Update;
					update.compositionId = activeCompositionId;
					update.compositionText = std::move(composition);
					const LONG caret = ImmGetCompositionStringW(context, GCS_CURSORPOS, nullptr, 0);
					update.caretOffset = caret >= 0 ? static_cast<std::uint32_t>(caret) : 0;
					success = enqueue(window, WM_IME_COMPOSITION, std::move(update)) && success;
				}
			}

			if (hasResultString && context)
			{
				std::u16string result;
				if (!queryCompositionString(context, GCS_RESULTSTR, result))
				{
					imeQueryFailures.fetch_add(1, std::memory_order_relaxed);
				}
				else if (!isValidUtf16(result))
				{
					rejectedTextSequences.fetch_add(1, std::memory_order_relaxed);
				}
				else
				{
					// 先交付 Commit，再交付结果文本；同时建立短期回声关联，抑制
					// 原窗口过程随后生成的等价 WM_IME_CHAR/WM_CHAR。
					compositionCommitted = true;
					CompositionEvent commit;
					commit.stage = CompositionStage::Commit;
					commit.compositionId = activeCompositionId;
					success = enqueue(window, WM_IME_COMPOSITION, std::move(commit)) && success;
					beginEcho(result, activeCompositionId);
					if (!result.empty())
					{
						TextInputEvent text;
						text.text = std::move(result);
						text.source = TextSource::ImeResult;
						text.compositionId = activeCompositionId;
						success = enqueue(window, WM_IME_COMPOSITION, std::move(text)) && success;
					}
				}
			}

			if (context)
				ImmReleaseContext(window, context);

			if ((flags & anyGcsMask) == 0 && !compositionCommitted && !compositionCancelled)
			{
				compositionCancelled = true;
				CompositionEvent cancel;
				cancel.stage = CompositionStage::Cancel;
				cancel.compositionId = activeCompositionId;
				success = enqueue(window, WM_IME_COMPOSITION, std::move(cancel)) && success;
			}
			return success;
		}

		NativeInputCollectResult collect(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
		{
			NativeInputCollectResult result;
			if (message == WM_UNICHAR && wParam == UNICODE_NOCHAR)
			{
				result.handledSynchronously = true;
				result.result = TRUE;
				return result;
			}
			if (hasFault())
				return result;

			switch (message)
			{
			case WM_MOUSEMOVE:
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_RBUTTONDBLCLK:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_MBUTTONDBLCLK:
			case WM_MOUSEWHEEL:
			case WM_MOUSEHWHEEL:
			case WM_CAPTURECHANGED:
				collectPointer(window, message, wParam, lParam);
				break;

			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
				collectKey(window, message, wParam, lParam);
				break;

			case WM_CHAR:
			case WM_IME_CHAR:
				collectText(window, message, wParam);
				break;

			case WM_UNICHAR:
			{
				const std::uint32_t scalar = static_cast<std::uint32_t>(wParam);
				TextInputEvent text;
				if (!appendUtf16Scalar(scalar, text.text))
				{
					rejectedTextSequences.fetch_add(1, std::memory_order_relaxed);
					break;
				}
				text.source = TextSource::UnicodeScalar;
				if (enqueue(window, message, std::move(text)))
				{
					result.handledSynchronously = true;
					result.result = 0;
				}
				break;
			}

			case WM_IME_STARTCOMPOSITION:
				clearTextDecoderState();
				clearEcho();
				if (imeLifecycleActive)
				{
					// 新 START 到来时旧轮次仍未结束，先按 Cancel/End 收口旧身份，
					// 再建立新会话，不能复用旧 compositionId。
					if (!compositionCommitted && !compositionCancelled)
					{
						CompositionEvent cancel;
						cancel.stage = CompositionStage::Cancel;
						cancel.compositionId = activeCompositionId;
						enqueue(window, message, std::move(cancel));
					}
					CompositionEvent end;
					end.stage = CompositionStage::End;
					end.compositionId = activeCompositionId;
					enqueue(window, message, std::move(end));
				}
				imeLifecycleActive = false;
				ensureComposition(window, message);
				break;

			case WM_IME_COMPOSITION:
				collectImeComposition(window, lParam);
				break;

			case WM_IME_ENDCOMPOSITION:
				if (imeLifecycleActive)
				{
					if (!compositionCommitted && !compositionCancelled)
					{
						compositionCancelled = true;
						CompositionEvent cancel;
						cancel.stage = CompositionStage::Cancel;
						cancel.compositionId = activeCompositionId;
						enqueue(window, message, std::move(cancel));
					}
					CompositionEvent end;
					end.stage = CompositionStage::End;
					end.compositionId = activeCompositionId;
					enqueue(window, message, std::move(end));
				}
				imeLifecycleActive = false;
				compositionCommitted = false;
				compositionCancelled = false;
				activeCompositionId = 0;
				clearTextDecoderState();
				// END 转发给原窗口过程前不能清除结果关联。部分输入法会在处理 END 时
				// 嵌套或紧接着产生 WM_IME_CHAR/WM_CHAR 回声。下一次物理按键、组合、
				// 焦点变化、输入语言变化或不匹配字符会关闭这个短期回声窗口。
				break;

			case WM_SETFOCUS:
				publishActivation(true);
				clearTextDecoderState();
				clearEcho();
				snapshotKeyboardLayout(nullptr);
				{
					WindowActivationEvent activation;
					activation.keyboardFocusActive = true;
					activation.otherWindow = reinterpret_cast<HWND>(wParam);
					enqueue(window, message, std::move(activation));
				}
				break;

			case WM_KILLFOCUS:
				publishActivation(false);
				clearTextDecoderState();
				clearEcho();
				{
					WindowActivationEvent activation;
					activation.keyboardFocusActive = false;
					activation.otherWindow = reinterpret_cast<HWND>(wParam);
					enqueue(window, message, std::move(activation));
				}
				break;

			case WM_INPUTLANGCHANGE:
				if (pendingLead)
					discardedDecoderFragments.fetch_add(1, std::memory_order_relaxed);
				pendingLead.reset();
				clearEcho();
				snapshotKeyboardLayout(reinterpret_cast<HKL>(lParam));
				break;

			default:
				break;
			}
			return result;
		}
	};

	NativeInputContext::NativeInputContext(HWND hwnd)
		: impl_(std::make_unique<Impl>(hwnd))
	{
	}

	NativeInputContext::~NativeInputContext() = default;

	NativeInputCollectResult NativeInputContext::collect(
		HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
	{
		if (!impl_)
			return {};
		// WndProc 边界不允许异常逃逸；任何异常都转成可由宿主安全点观察的
		// fail-stop 故障。
		try
		{
			return impl_->collect(hwnd, message, wParam, lParam);
		}
		catch (const std::bad_alloc&)
		{
			impl_->publishFault(NativeInputFaultCode::OutOfMemory, 0, message, hwnd);
		}
		catch (...)
		{
			impl_->publishFault(NativeInputFaultCode::UnexpectedException, 0, message, hwnd);
		}
		return {};
	}

	NativeInputQueuePopResult NativeInputContext::tryPopObserved(
		NativeInputEvent& event) noexcept
	{
		if (!impl_)
			return NativeInputQueuePopResult::Empty;

		const NativeInputQueuePopResult result = impl_->queue.tryPop(event);
		if (result == NativeInputQueuePopResult::UnexpectedFailure)
		{
			impl_->publishFault(NativeInputFaultCode::QueueFailure, 0, 0, impl_->hwnd);
			return result;
		}
		if (result == NativeInputQueuePopResult::Empty)
			return result;

		if (impl_->observations.lastSequence != 0 &&
			event.sequence <= impl_->observations.lastSequence)
		{
			impl_->observations.sequenceStrict = false;
			impl_->publishFault(NativeInputFaultCode::SequenceViolation,
				event.sequence, 0, event.hwnd);
			return NativeInputQueuePopResult::UnexpectedFailure;
		}
		impl_->observations.lastSequence = event.sequence;
		++impl_->observations.observed;
		if (std::holds_alternative<KeyEvent>(event.payload))
			++impl_->observations.keyEvents;
		else if (std::holds_alternative<TextInputEvent>(event.payload))
			++impl_->observations.textEvents;
		else if (std::holds_alternative<CompositionEvent>(event.payload))
			++impl_->observations.compositionEvents;
		else if (std::holds_alternative<WindowActivationEvent>(event.payload))
			++impl_->observations.activationEvents;
		else if (std::holds_alternative<PointerEvent>(event.payload))
			++impl_->observations.pointerEvents;
		return NativeInputQueuePopResult::Popped;
	}

	std::size_t NativeInputContext::drainObserved(std::size_t limit) noexcept
	{
		if (!impl_ || limit == 0)
			return 0;

		std::size_t drained = 0;
		while (drained < limit)
		{
			NativeInputEvent event;
			const NativeInputQueuePopResult result = tryPopObserved(event);
			if (result == NativeInputQueuePopResult::Empty)
				break;
			if (result == NativeInputQueuePopResult::UnexpectedFailure)
				break;
			++drained;
		}
		return drained;
	}

	bool NativeInputContext::hasFault() const noexcept
	{
		return impl_ && impl_->hasFault();
	}

	NativeInputFaultSnapshot NativeInputContext::fault() const noexcept
	{
		if (!impl_ || !impl_->faultReady.load(std::memory_order_acquire))
			return {};
		return impl_->faultSnapshot;
	}

	NativeInputQueueStats NativeInputContext::queueStats() const noexcept
	{
		return impl_ ? impl_->queue.stats() : NativeInputQueueStats{};
	}

	NativeInputObservationStats NativeInputContext::observationStats() const noexcept
	{
		return impl_ ? impl_->observations : NativeInputObservationStats{};
	}

	NativeInputActivationSnapshot NativeInputContext::activationSnapshot() const noexcept
	{
		return impl_ ? impl_->activationSnapshot() : NativeInputActivationSnapshot{};
	}

	void NativeInputContext::reportHostFault(NativeInputFaultCode code,
		std::uint64_t sequence, UINT message, HWND hwnd) noexcept
	{
		if (impl_)
			impl_->publishFault(code, sequence, message, hwnd);
	}

	NativeInputNormalizationStats NativeInputContext::normalizationStats() const noexcept
	{
		NativeInputNormalizationStats snapshot;
		if (!impl_)
			return snapshot;
		snapshot.rejectedTextSequences = impl_->rejectedTextSequences.load(std::memory_order_relaxed);
		snapshot.imeQueryFailures = impl_->imeQueryFailures.load(std::memory_order_relaxed);
		snapshot.discardedDecoderFragments = impl_->discardedDecoderFragments.load(std::memory_order_relaxed);
		snapshot.pointerCoordinateFailures =
			impl_->pointerCoordinateFailures.load(std::memory_order_relaxed);
		return snapshot;
	}

	bool NativeInputContext::tryPopForTesting(NativeInputEvent& event) noexcept
	{
		return impl_ && impl_->queue.tryPop(event) == NativeInputQueuePopResult::Popped;
	}

	void NativeInputContext::forceNextQueuePushResultForTesting(
		NativeInputQueuePushResult result) noexcept
	{
		if (impl_)
			impl_->queue.forceNextPushResultForTesting(result);
	}

	void NativeInputContext::setAnsiCodePageForTesting(UINT codePage) noexcept
	{
		if (impl_)
		{
			impl_->pendingLead.reset();
			impl_->ansiCodePage = codePage;
			impl_->windowUnicode = false;
		}
	}

	void NativeInputContext::seedEchoForTesting(
		const std::u16string& text, std::uint64_t compositionId)
	{
		if (impl_)
			impl_->beginEcho(text, compositionId);
	}
}
