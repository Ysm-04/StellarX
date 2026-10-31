#include "SxInputProbeInternal.h"

#if defined(STELLARX_ENABLE_INPUT_PROBE)

#include "SxImeCaretAnchorInternal.h"

#include <easyx.h>
#include <bcrypt.h>
#include <imm.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cwchar>
#include <filesystem>
#include <limits>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "imm32.lib")

namespace StellarX::Internal
{
	namespace
	{
		constexpr std::size_t MaxBufferedRecords = 4096;
		constexpr std::size_t DefaultSoftLimit = 3072;
		constexpr std::size_t DefaultHardLimit = 4096;
		constexpr std::size_t MaxImeResultBytes = 64 * 1024;
		constexpr std::size_t MaxEchoUtf16Units = 1024;
		constexpr std::size_t MaxEchoAnsiBytes = 2048;
		constexpr std::uint32_t MaxConsumeRuleMatches = 16;
		constexpr const char* Current0CBaselineId =
			"sha256:5C544D9FEC9C1A2324A7AF3F593A1A34110100644BAF5071E777302387E3C6C5";

		enum class RecordType : std::uint8_t
		{
			SessionStart,
			SessionEnd,
			WindowAttach,
			ExperimentConfig,
			WindowDetach,
			WndProcEnter,
			WndProcLeave,
			ImePosition,
			EasyXKey,
			EasyXMouse,
			EasyXWindow,
			GeometrySafePoint,
			ImmQuery,
			EchoSummary,
			HostBatchBegin,
			HostBatchEnd,
			FlushBoundary,
			RecordsLost
		};

		enum class ValuePolicy : std::uint8_t
		{
			NotApplicable,
			Raw,
			RawAndParsed,
			ParsedOnly,
			Redacted
		};

		enum class MessageCategory : std::uint8_t
		{
			Key,
			Text,
			Ime,
			Focus,
			Window,
			Mouse,
			Other
		};

		enum class CompositionAllocation : std::uint8_t
		{
			None,
			StartComposition,
			LazyMissingStart
		};

		enum class TextSource : std::uint8_t
		{
			None,
			GcsResultStr,
			WmImeChar,
			WmChar
		};

		enum class ImmQueryStatus : std::uint8_t
		{
			NotRequested,
			Success,
			Empty,
			NoContext,
			SizeError,
			OddByteLength,
			TooLarge,
			ReadError,
			DigestError
		};

		enum class CorrelationStatus : std::uint8_t
		{
			None,
			Candidate,
			Matched,
			Partial,
			Mismatched,
			Expired,
			Unrepresentable
		};

		enum class EchoChannel : std::uint8_t
		{
			None,
			ImeChar,
			WmChar
		};

		enum class ProbeAction : std::uint8_t
		{
			ObserveAndForward,
			ObserveAndConsume
		};

		enum class ImeStateCondition : std::uint8_t
		{
			Any,
			IdleOnly,
			ComposingOnly
		};

		enum class ProbeInputSource : std::uint8_t
		{
			Unknown,
			SendInput,
			Manual
		};

		enum class FailOpenReason : std::uint8_t
		{
			None,
			InvalidConfiguration,
			BaselineIdMismatch,
			WindowUnicodeMismatch,
			AnsiCodePageMismatch,
			DpiMismatch,
			DpiAwarenessMismatch,
			ImeIdentityUnconfirmed,
			ImeConditionNotMet,
			ImePositionUnavailable,
			CaptureLossObserved,
			MatchLimitReached
		};

		struct ConsumeRuleState
		{
			bool requested = false;
			bool configurationValid = false;
			bool baselineMatched = false;
			bool eligible = false;
			std::string experimentId;
			std::string ruleId;
			std::string baselineId;
			std::string imeName;
			UINT message = 0;
			UINT virtualKey = 0;
			bool scanCodeSpecified = false;
			std::uint8_t scanCode = 0;
			ImeStateCondition imeCondition = ImeStateCondition::Any;
			ProbeAction requestedAction = ProbeAction::ObserveAndForward;
			ProbeInputSource inputSource = ProbeInputSource::Unknown;
			std::uint32_t maxMatches = 0;
			std::uint32_t matchCount = 0;
			bool expectedWindowUnicode = false;
			std::uint32_t expectedAnsiCodePage = 0;
			std::uint32_t expectedDpiForWindow = 0;
			int expectedDpiAwareness = -1;
			bool actualWindowUnicode = false;
			std::uint32_t actualAnsiCodePage = 0;
			std::uint32_t actualDpiForWindow = 0;
			int actualDpiAwareness = -1;
			bool imePositionConfigured = false;
			bool imePositionReady = false;
			POINT imeCaretPoint{};
			RECT imeLineRect{};
			std::uint32_t imePositionAttemptCount = 0;
			std::uint32_t imePositionSuccessCount = 0;
			FailOpenReason failOpenReason = FailOpenReason::None;
		};

		struct ProbeRecord
		{
			RecordType type = RecordType::SessionStart;
			std::uint64_t sequence = 0;
			std::uint64_t qpcTicks = 0;
			DWORD threadId = 0;
			DWORD processId = 0;
			HWND hwnd = nullptr;
			std::uint64_t hwndInstanceId = 0;
			std::uint64_t wndProcCallId = 0;
			std::uint64_t parentWndProcCallId = 0;
			std::uint64_t rootWndProcCallId = 0;
			std::uint64_t parentSequence = 0;
			std::uint64_t compositionId = 0;
			std::uint32_t callDepth = 0;
			std::uint32_t dispatchDepth = 0;
			std::uint32_t modalDepth = 0;

			UINT messageId = 0;
			MessageCategory messageCategory = MessageCategory::Other;
			ValuePolicy wParamPolicy = ValuePolicy::NotApplicable;
			ValuePolicy lParamPolicy = ValuePolicy::NotApplicable;
			std::uint64_t wParamValue = 0;
			std::uint64_t lParamValue = 0;
			std::uint64_t forwardResult = 0;
			std::uint32_t messageTime = 0;
			bool messageTimeAvailable = false;
			HWND captureHwnd = nullptr;
			HWND captureChangedTo = nullptr;
			bool forwarded = false;
			bool experimentTagged = false;
			bool configurationValid = false;
			bool baselineMatched = false;
			bool ruleEligible = false;
			bool ruleMatched = false;
			bool failOpenApplied = false;
			std::uint32_t matchOrdinal = 0;
			ProbeAction requestedAction = ProbeAction::ObserveAndForward;
			ProbeAction actualAction = ProbeAction::ObserveAndForward;
			FailOpenReason failOpenReason = FailOpenReason::None;

			std::uint16_t repeatCount = 0;
			std::uint8_t scanCode = 0;
			bool extended = false;
			bool altContext = false;
			bool wasDown = false;
			bool transition = false;
			bool systemKey = false;
			bool leftShift = false;
			bool rightShift = false;
			bool leftCtrl = false;
			bool rightCtrl = false;
			bool leftAlt = false;
			bool rightAlt = false;
			bool capsLock = false;
			bool numLock = false;
			bool scrollLock = false;
			bool imePositionAttempted = false;
			bool imePositionContextAvailable = false;
			bool compositionPositionSucceeded = false;
			bool candidatePositionSucceeded = false;
			POINT imeCaretPoint{};
			RECT imeLineRect{};

			std::uint32_t imeFlags = 0;
			bool imeStateBefore = false;
			bool imeStateAfter = false;
			CompositionAllocation compositionAllocation = CompositionAllocation::None;
			bool unicodeNoCharQuery = false;
			bool windowUnicode = false;

			TextSource textSource = TextSource::None;
			ImmQueryStatus immQueryStatus = ImmQueryStatus::NotRequested;
			std::uint32_t immByteLength = 0;
			std::uint32_t utf16Length = 0;
			std::uint32_t scalarCount = 0;
			std::uint32_t ansiCodePage = 0;
			bool inputLanguageSysCharset = false;
			bool inputLanguageForward = false;
			bool inputLanguageBackward = false;
			bool textMetadataPresent = false;
			bool validUtf16 = false;
			bool digestPresent = false;
			bool ansiConversionUsedDefault = false;
			bool correlationTruncated = false;
			std::array<char, 33> sessionDigest{};
			CorrelationStatus correlationStatus = CorrelationStatus::None;
			EchoChannel echoChannel = EchoChannel::None;
			std::uint64_t correlationTargetSequence = 0;
			std::uint32_t matchedUnitCount = 0;
			std::uint32_t expectedUnitCount = 0;

			std::uint64_t batchId = 0;
			std::uint32_t indexInBatch = 0;
			std::uint32_t processedCount = 0;
			bool modalBatch = false;
			bool batchStopped = false;

			std::uint8_t easyxVkCode = 0;
			std::uint8_t easyxScanCode = 0;
			bool easyxDown = false;
			bool easyxExtended = false;
			bool easyxPreviousDown = false;
			int easyxMouseX = 0;
			int easyxMouseY = 0;
			int easyxMouseWheel = 0;
			bool easyxLeftButton = false;
			bool easyxMiddleButton = false;
			bool easyxRightButton = false;
			bool easyxCtrl = false;
			bool easyxShift = false;

			bool geometrySnapshotPresent = false;
			bool windowRectAvailable = false;
			bool clientRectAvailable = false;
			bool clientOriginAvailable = false;
			RECT windowRect{};
			RECT clientRect{};
			POINT clientOriginScreen{};
			bool monitorAvailable = false;
			HMONITOR monitor = nullptr;
			std::array<char, 64> monitorDevice{};
			RECT monitorRect{};
			RECT monitorWorkRect{};
			bool primaryMonitor = false;
			bool windowVisible = false;
			bool windowMinimized = false;
			bool windowMaximized = false;
			std::uint32_t geometryDpiForWindow = 0;

			bool parsedPointPresent = false;
			POINT parsedPoint{};
			bool parsedSizePresent = false;
			SIZE parsedSize{};
			std::uint32_t sizeType = 0;
			bool windowPosPresent = false;
			WINDOWPOS windowPos{};
			bool dpiChangePresent = false;
			std::uint32_t dpiX = 0;
			std::uint32_t dpiY = 0;
			RECT suggestedWindowRect{};
			std::uint32_t displayBitsPerPixel = 0;

			ImeCaretAnchorSnapshot publishedAnchor{};
			ImeCaretAnchorSnapshot positionedAnchor{};
			bool imeWindowActive = false;
			bool imeRefreshPending = false;
			bool imeTextInputEligible = false;
			bool imeInputContextDetached = false;
			std::uint64_t imePositionRequests = 0;
			std::uint64_t imeCompositionSuccesses = 0;
			std::uint64_t imeCandidateSuccesses = 0;
			std::uint64_t imeSnapshotReadFailures = 0;
			std::uint64_t imeMissingContextSkips = 0;
			std::uint64_t imeCancelRequests = 0;
			std::uint64_t imeCancelSuccesses = 0;
			std::uint64_t imeCancelFailures = 0;
			std::uint64_t imeInputContextGateTransitions = 0;
			std::uint64_t imeInputContextDetachSuccesses = 0;
			std::uint64_t imeInputContextDetachFailures = 0;
			std::uint64_t imeInputContextRestoreSuccesses = 0;
			std::uint64_t imeInputContextRestoreFailures = 0;

			std::uint32_t dpiForWindow = 0;
			int dpiAwareness = -1;
			std::uint32_t bufferedRecordCount = 0;
			std::uint64_t lostCountValue = 0;
			std::uint64_t lostFirstSequence = 0;
			std::uint64_t lostLastSequence = 0;
			bool lossFromContention = false;
			bool durableFlush = false;
		};

		struct ImePositionRequest
		{
			bool active = false;
			HWND hwnd = nullptr;
			std::uint64_t hwndInstanceId = 0;
			std::uint64_t parentSequence = 0;
			std::uint64_t wndProcCallId = 0;
			std::uint64_t compositionId = 0;
			UINT triggerMessage = 0;
			POINT caretPoint{};
			RECT lineRect{};
		};

		struct ImePositionResult
		{
			bool contextAvailable = false;
			bool compositionSucceeded = false;
			bool candidateSucceeded = false;
		};

		struct EchoCorrelationState
		{
			bool active = false;
			bool windowUnicode = false;
			bool ansiRepresentable = false;
			bool truncated = false;
			bool imeMismatched = false;
			bool charMismatched = false;
			std::uint64_t targetSequence = 0;
			std::uint64_t compositionId = 0;
			std::uint32_t ansiCodePage = 0;
			std::size_t utf16Count = 0;
			std::size_t ansiCount = 0;
			std::size_t imeMatched = 0;
			std::size_t charMatched = 0;
			std::array<wchar_t, MaxEchoUtf16Units> utf16{};
			std::array<unsigned char, MaxEchoAnsiBytes> ansi{};
		};

		struct FlushSnapshot
		{
			std::size_t bufferIndex = 0;
			std::size_t head = 0;
			std::size_t count = 0;
			std::size_t hardLimit = 0;
			std::vector<ProbeRecord> trailingRecords;
			HWND hwnd = nullptr;
			std::uint64_t hwndInstanceId = 0;
			bool durable = false;
		};

		struct ProbeState
		{
			std::atomic<bool> enabled{ false };
			SRWLOCK lock = SRWLOCK_INIT;
			std::uint32_t schemaVersion = 1;
			bool recordNonInputMessages = false;
			bool recordPointerMessages = false;
			bool geometryProbeEnabled = false;
			bool durableRequested = false;
			HANDLE file = INVALID_HANDLE_VALUE;
			HWND attachedHwnd = nullptr;
			std::uint64_t hwndInstanceId = 0;
			DWORD processId = 0;
			DWORD uiThreadId = 0;
			DWORD hostThreadId = 0;
			std::uint32_t ansiCodePage = 0;
			std::uint64_t qpcFrequency = 0;
			std::array<char, 37> sessionId{};
			std::array<char, 32> sessionKey{};
			bool sessionKeyReady = false;
			std::array<char, 32> startedAtUtc{};

			std::atomic<std::uint64_t> nextSequence{ 0 };
			std::uint64_t nextWndProcCallId = 0;
			std::uint64_t nextBatchId = 0;
			std::uint64_t nextCompositionId = 0;
			std::uint64_t latestGeometryRootCallId = 0;
			std::uint64_t activeCompositionId = 0;
			std::uint32_t dispatchDepth = 0;
			std::uint32_t modalDepth = 0;
			bool imeComposing = false;
			EchoCorrelationState echo{};
			ConsumeRuleState consumeRule{};
			bool geometrySafePointRecorded = false;
			ProbeRecord lastGeometrySafePoint{};

			std::array<std::array<ProbeRecord, MaxBufferedRecords>, 2> recordBuffers{};
			std::size_t activeRecordBuffer = 0;
			std::size_t head = 0;
			std::size_t count = 0;
			std::size_t softLimit = DefaultSoftLimit;
			std::size_t hardLimit = DefaultHardLimit;
			std::uint64_t overflowLostFirstSequence = 0;
			std::uint64_t overflowLostLastSequence = 0;
			std::uint64_t overflowLostCount = 0;
			std::atomic<std::uint64_t> contentionLostCount{ 0 };
			std::atomic<std::uint64_t> totalLostCount{ 0 };
		};

		constexpr std::size_t MaxTrackedWndProcDepth = 64;
		thread_local std::uint32_t wndProcDepth = 0;
		thread_local std::array<std::uint64_t, MaxTrackedWndProcDepth> wndProcCallStack{};
		thread_local std::array<std::uint64_t, MaxTrackedWndProcDepth> wndProcRootCallStack{};
		thread_local bool imePositionApiCallActive = false;
		// 所有持久化路径都先取得此锁，再取得 ProbeState::lock，保证宿主线程与
		// WndProc 线程的快照按全局序号顺序落盘。
		SRWLOCK probeFileWriteLock = SRWLOCK_INIT;

		ProbeState& probeState() noexcept
		{
			static ProbeState state;
			return state;
		}

		class ExclusiveStateLock
		{
		public:
			explicit ExclusiveStateLock(SRWLOCK& lock) noexcept : lock_(&lock)
			{
				AcquireSRWLockExclusive(lock_);
			}
			~ExclusiveStateLock() noexcept
			{
				ReleaseSRWLockExclusive(lock_);
			}
			ExclusiveStateLock(const ExclusiveStateLock&) = delete;
			ExclusiveStateLock& operator=(const ExclusiveStateLock&) = delete;

		private:
			SRWLOCK* lock_;
		};

		std::uint64_t qpcNow() noexcept
		{
			LARGE_INTEGER value{};
			QueryPerformanceCounter(&value);
			return static_cast<std::uint64_t>(value.QuadPart);
		}

		bool environmentFlagEnabled(const wchar_t* name) noexcept
		{
			wchar_t value[16]{};
			const DWORD length = GetEnvironmentVariableW(name, value, static_cast<DWORD>(std::size(value)));
			if (length == 0 || length >= std::size(value))
				return false;
			return _wcsicmp(value, L"1") == 0 || _wcsicmp(value, L"true") == 0 ||
				_wcsicmp(value, L"yes") == 0 || _wcsicmp(value, L"on") == 0;
		}

		std::size_t environmentSize(const wchar_t* name, std::size_t fallback) noexcept
		{
			wchar_t value[32]{};
			const DWORD length = GetEnvironmentVariableW(name, value, static_cast<DWORD>(std::size(value)));
			if (length == 0 || length >= std::size(value))
				return fallback;
			wchar_t* end = nullptr;
			const unsigned long parsed = std::wcstoul(value, &end, 10);
			if (end == value || *end != L'\0')
				return fallback;
			return static_cast<std::size_t>(parsed);
		}

		bool environmentText(const wchar_t* name, std::wstring& output) noexcept
		{
			try
			{
				const DWORD required = GetEnvironmentVariableW(name, nullptr, 0);
				if (required <= 1 || required > 512)
					return false;
				std::wstring value(required, L'\0');
				const DWORD copied = GetEnvironmentVariableW(name, value.data(), required);
				if (copied == 0 || copied >= required)
					return false;
				value.resize(copied);
				output = std::move(value);
				return true;
			}
			catch (...)
			{
				return false;
			}
		}

		bool toSafeAscii(const std::wstring& value, std::string& output,
			bool identifierOnly) noexcept
		{
			try
			{
				if (value.empty() || value.size() > 128)
					return false;
				std::string converted;
				converted.reserve(value.size());
				for (const wchar_t unit : value)
				{
					if (unit < 0x20 || unit > 0x7E)
						return false;
					const char character = static_cast<char>(unit);
					if (identifierOnly && !((character >= 'a' && character <= 'z') ||
						(character >= 'A' && character <= 'Z') ||
						(character >= '0' && character <= '9') || character == '.' ||
						character == '_' || character == '-'))
					{
						return false;
					}
					converted.push_back(character);
				}
				output = std::move(converted);
				return true;
			}
			catch (...)
			{
				return false;
			}
		}

		bool environmentAscii(const wchar_t* name, std::string& output,
			bool identifierOnly = false) noexcept
		{
			std::wstring value;
			return environmentText(name, value) && toSafeAscii(value, output, identifierOnly);
		}

		bool environmentUnsigned(const wchar_t* name, std::uint32_t& output) noexcept
		{
			std::wstring value;
			if (!environmentText(name, value))
				return false;
			wchar_t* end = nullptr;
			const unsigned long parsed = std::wcstoul(value.c_str(), &end, 0);
			if (end == value.c_str() || *end != L'\0')
				return false;
			output = static_cast<std::uint32_t>(parsed);
			return true;
		}

		bool environmentSigned(const wchar_t* name, int& output) noexcept
		{
			std::wstring value;
			if (!environmentText(name, value))
				return false;
			wchar_t* end = nullptr;
			const long parsed = std::wcstol(value.c_str(), &end, 10);
			if (end == value.c_str() || *end != L'\0' ||
				parsed < (std::numeric_limits<int>::min)() || parsed > (std::numeric_limits<int>::max)())
			{
				return false;
			}
			output = static_cast<int>(parsed);
			return true;
		}

		bool environmentBoolean(const wchar_t* name, bool& output) noexcept
		{
			std::wstring value;
			if (!environmentText(name, value))
				return false;
			if (value == L"1" || _wcsicmp(value.c_str(), L"true") == 0)
			{
				output = true;
				return true;
			}
			if (value == L"0" || _wcsicmp(value.c_str(), L"false") == 0)
			{
				output = false;
				return true;
			}
			return false;
		}

		std::filesystem::path outputDirectory() noexcept
		{
			try
			{
				const DWORD required = GetEnvironmentVariableW(L"STELLARX_INPUT_PROBE_DIR", nullptr, 0);
				if (required > 1)
				{
					std::wstring value(required, L'\0');
					const DWORD copied = GetEnvironmentVariableW(
						L"STELLARX_INPUT_PROBE_DIR", value.data(), required);
					if (copied > 0 && copied < required)
					{
						value.resize(copied);
						return std::filesystem::path(value);
					}
				}
				return std::filesystem::path(L"TestResults") / L"InputProbe";
			}
			catch (...)
			{
				return {};
			}
		}

		bool makeSessionId(std::array<char, 37>& output, std::array<char, 32>& key) noexcept
		{
			std::array<unsigned char, 16> bytes{};
			const NTSTATUS idStatus = BCryptGenRandom(
				nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
			const NTSTATUS keyStatus = BCryptGenRandom(
				nullptr, reinterpret_cast<PUCHAR>(key.data()), static_cast<ULONG>(key.size()),
				BCRYPT_USE_SYSTEM_PREFERRED_RNG);
			if (idStatus < 0)
			{
				const std::uint64_t seed = qpcNow() ^
					(static_cast<std::uint64_t>(GetCurrentProcessId()) << 32) ^ GetCurrentThreadId();
				for (std::size_t index = 0; index < bytes.size(); ++index)
					bytes[index] = static_cast<unsigned char>((seed >> ((index % 8) * 8)) ^ index);
			}
			if (keyStatus < 0)
				std::fill(key.begin(), key.end(), '\0');

			bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x40);
			bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80);
			std::snprintf(output.data(), output.size(),
				"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
				bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
				bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
			return keyStatus >= 0;
		}

		void makeUtcTimestamp(std::array<char, 32>& output, SYSTEMTIME& utc) noexcept
		{
			GetSystemTime(&utc);
			std::snprintf(output.data(), output.size(), "%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
				utc.wYear, utc.wMonth, utc.wDay, utc.wHour, utc.wMinute, utc.wSecond, utc.wMilliseconds);
		}

		const char* recordTypeName(RecordType type) noexcept
		{
			switch (type)
			{
			case RecordType::SessionStart: return "session_start";
			case RecordType::SessionEnd: return "session_end";
			case RecordType::WindowAttach: return "window_attach";
			case RecordType::ExperimentConfig: return "experiment_config";
			case RecordType::WindowDetach: return "window_detach";
			case RecordType::WndProcEnter: return "wndproc_enter";
			case RecordType::WndProcLeave: return "wndproc_leave";
			case RecordType::ImePosition: return "ime_position";
			case RecordType::EasyXKey: return "easyx_key";
			case RecordType::EasyXMouse: return "easyx_mouse";
			case RecordType::EasyXWindow: return "easyx_window";
			case RecordType::GeometrySafePoint: return "geometry_safe_point";
			case RecordType::ImmQuery: return "imm_query";
			case RecordType::EchoSummary: return "echo_correlation";
			case RecordType::HostBatchBegin: return "host_batch_begin";
			case RecordType::HostBatchEnd: return "host_batch_end";
			case RecordType::FlushBoundary: return "flush_boundary";
			case RecordType::RecordsLost: return "records_lost";
			default: return "probe_error";
			}
		}

		const char* sourceBackendName(RecordType type) noexcept
		{
			switch (type)
			{
			case RecordType::WndProcEnter:
			case RecordType::WndProcLeave:
				return "Win32WndProc";
			case RecordType::ImePosition:
				return "Imm32Position";
			case RecordType::EasyXKey:
				return "EasyXExKey";
			case RecordType::EasyXMouse:
				return "EasyXExMouse";
			case RecordType::EasyXWindow:
				return "EasyXExWindow";
			case RecordType::GeometrySafePoint:
				return "HostGeometrySafePoint";
			case RecordType::ImmQuery:
				return "Imm32Query";
			case RecordType::EchoSummary:
				return "InputProbeCorrelation";
			default:
				return "HostEventLoop";
			}
		}

		const char* observationPointName(RecordType type) noexcept
		{
			switch (type)
			{
			case RecordType::SessionStart: return "probe_start";
			case RecordType::SessionEnd: return "probe_stop";
			case RecordType::WindowAttach: return "window_attach";
			case RecordType::ExperimentConfig: return "experiment_config";
			case RecordType::WindowDetach: return "window_detach";
			case RecordType::WndProcEnter: return "enter";
			case RecordType::WndProcLeave: return "after_dispatch";
			case RecordType::ImePosition: return "ime_position_result";
			case RecordType::EasyXKey: return "existing_peekmessage_result";
			case RecordType::EasyXMouse: return "existing_peekmessage_result";
			case RecordType::EasyXWindow: return "existing_peekmessage_result";
			case RecordType::GeometrySafePoint: return "post_settle_snapshot";
			case RecordType::ImmQuery: return "gcs_result";
			case RecordType::EchoSummary: return "echo_summary";
			case RecordType::HostBatchBegin: return "input_batch_begin";
			case RecordType::HostBatchEnd: return "input_batch_end";
			case RecordType::FlushBoundary: return "safe_point";
			case RecordType::RecordsLost: return "capture_loss";
			default: return "unknown";
			}
		}

		const char* valuePolicyName(ValuePolicy policy) noexcept
		{
			switch (policy)
			{
			case ValuePolicy::Raw: return "raw";
			case ValuePolicy::RawAndParsed: return "raw_and_parsed";
			case ValuePolicy::ParsedOnly: return "parsed_only";
			case ValuePolicy::Redacted: return "redacted";
			default: return "not_applicable";
			}
		}

		const char* categoryName(MessageCategory category) noexcept
		{
			switch (category)
			{
			case MessageCategory::Key: return "key";
			case MessageCategory::Text: return "text";
			case MessageCategory::Ime: return "ime";
			case MessageCategory::Focus: return "focus";
			case MessageCategory::Window: return "window";
			case MessageCategory::Mouse: return "mouse";
			default: return "other";
			}
		}

		const char* textSourceName(TextSource source) noexcept
		{
			switch (source)
			{
			case TextSource::GcsResultStr: return "GCS_RESULTSTR";
			case TextSource::WmImeChar: return "WM_IME_CHAR";
			case TextSource::WmChar: return "WM_CHAR";
			default: return "none";
			}
		}

		const char* immQueryStatusName(ImmQueryStatus status) noexcept
		{
			switch (status)
			{
			case ImmQueryStatus::Success: return "success";
			case ImmQueryStatus::Empty: return "empty";
			case ImmQueryStatus::NoContext: return "no_context";
			case ImmQueryStatus::SizeError: return "size_error";
			case ImmQueryStatus::OddByteLength: return "odd_byte_length";
			case ImmQueryStatus::TooLarge: return "too_large";
			case ImmQueryStatus::ReadError: return "read_error";
			case ImmQueryStatus::DigestError: return "digest_error";
			default: return "not_requested";
			}
		}

		const char* correlationStatusName(CorrelationStatus status) noexcept
		{
			switch (status)
			{
			case CorrelationStatus::Candidate: return "candidate";
			case CorrelationStatus::Matched: return "matched";
			case CorrelationStatus::Partial: return "partial";
			case CorrelationStatus::Mismatched: return "mismatched";
			case CorrelationStatus::Expired: return "expired";
			case CorrelationStatus::Unrepresentable: return "unrepresentable";
			default: return "none";
			}
		}

		const char* echoChannelName(EchoChannel channel) noexcept
		{
			switch (channel)
			{
			case EchoChannel::ImeChar: return "WM_IME_CHAR";
			case EchoChannel::WmChar: return "WM_CHAR";
			default: return "none";
			}
		}

		const char* probeActionName(ProbeAction action) noexcept
		{
			return action == ProbeAction::ObserveAndConsume
				? "ObserveAndConsume" : "ObserveAndForward";
		}

		const char* imeConditionName(ImeStateCondition condition) noexcept
		{
			switch (condition)
			{
			case ImeStateCondition::IdleOnly: return "idle_only";
			case ImeStateCondition::ComposingOnly: return "composing_only";
			default: return "any";
			}
		}

		const char* inputSourceName(ProbeInputSource source) noexcept
		{
			switch (source)
			{
			case ProbeInputSource::SendInput: return "SendInput";
			case ProbeInputSource::Manual: return "Manual";
			default: return "unknown";
			}
		}

		const char* failOpenReasonName(FailOpenReason reason) noexcept
		{
			switch (reason)
			{
			case FailOpenReason::InvalidConfiguration: return "invalid_configuration";
			case FailOpenReason::BaselineIdMismatch: return "baseline_id_mismatch";
			case FailOpenReason::WindowUnicodeMismatch: return "window_unicode_mismatch";
			case FailOpenReason::AnsiCodePageMismatch: return "ansi_code_page_mismatch";
			case FailOpenReason::DpiMismatch: return "dpi_mismatch";
			case FailOpenReason::DpiAwarenessMismatch: return "dpi_awareness_mismatch";
			case FailOpenReason::ImeIdentityUnconfirmed: return "ime_identity_unconfirmed";
			case FailOpenReason::ImeConditionNotMet: return "ime_condition_not_met";
			case FailOpenReason::ImePositionUnavailable: return "ime_position_unavailable";
			case FailOpenReason::CaptureLossObserved: return "capture_loss_observed";
			case FailOpenReason::MatchLimitReached: return "match_limit_reached";
			default: return "none";
			}
		}

		const char* messageName(UINT message) noexcept
		{
			switch (message)
			{
			case WM_KEYDOWN: return "WM_KEYDOWN";
			case WM_KEYUP: return "WM_KEYUP";
			case WM_SYSKEYDOWN: return "WM_SYSKEYDOWN";
			case WM_SYSKEYUP: return "WM_SYSKEYUP";
			case WM_CHAR: return "WM_CHAR";
			case WM_SYSCHAR: return "WM_SYSCHAR";
			case WM_DEADCHAR: return "WM_DEADCHAR";
			case WM_SYSDEADCHAR: return "WM_SYSDEADCHAR";
			case WM_UNICHAR: return "WM_UNICHAR";
			case WM_IME_CHAR: return "WM_IME_CHAR";
			case WM_IME_STARTCOMPOSITION: return "WM_IME_STARTCOMPOSITION";
			case WM_IME_COMPOSITION: return "WM_IME_COMPOSITION";
			case WM_IME_ENDCOMPOSITION: return "WM_IME_ENDCOMPOSITION";
			case WM_IME_SETCONTEXT: return "WM_IME_SETCONTEXT";
			case WM_IME_NOTIFY: return "WM_IME_NOTIFY";
			case WM_IME_REQUEST: return "WM_IME_REQUEST";
			case WM_IME_CONTROL: return "WM_IME_CONTROL";
			case WM_IME_SELECT: return "WM_IME_SELECT";
			case WM_INPUTLANGCHANGEREQUEST: return "WM_INPUTLANGCHANGEREQUEST";
			case WM_INPUTLANGCHANGE: return "WM_INPUTLANGCHANGE";
			case WM_SETFOCUS: return "WM_SETFOCUS";
			case WM_KILLFOCUS: return "WM_KILLFOCUS";
			case WM_ACTIVATE: return "WM_ACTIVATE";
			case WM_ACTIVATEAPP: return "WM_ACTIVATEAPP";
			case WM_MOVE: return "WM_MOVE";
			case WM_SIZE: return "WM_SIZE";
			case WM_WINDOWPOSCHANGING: return "WM_WINDOWPOSCHANGING";
			case WM_WINDOWPOSCHANGED: return "WM_WINDOWPOSCHANGED";
			case WM_DPICHANGED: return "WM_DPICHANGED";
			case WM_DISPLAYCHANGE: return "WM_DISPLAYCHANGE";
			case WM_ENTERSIZEMOVE: return "WM_ENTERSIZEMOVE";
			case WM_EXITSIZEMOVE: return "WM_EXITSIZEMOVE";
			case WM_GETDLGCODE: return "WM_GETDLGCODE";
			case WM_SETCURSOR: return "WM_SETCURSOR";
			case WM_MOUSEACTIVATE: return "WM_MOUSEACTIVATE";
			case WM_MOUSEMOVE: return "WM_MOUSEMOVE";
			case WM_LBUTTONDOWN: return "WM_LBUTTONDOWN";
			case WM_LBUTTONUP: return "WM_LBUTTONUP";
			case WM_LBUTTONDBLCLK: return "WM_LBUTTONDBLCLK";
			case WM_RBUTTONDOWN: return "WM_RBUTTONDOWN";
			case WM_RBUTTONUP: return "WM_RBUTTONUP";
			case WM_RBUTTONDBLCLK: return "WM_RBUTTONDBLCLK";
			case WM_MBUTTONDOWN: return "WM_MBUTTONDOWN";
			case WM_MBUTTONUP: return "WM_MBUTTONUP";
			case WM_MBUTTONDBLCLK: return "WM_MBUTTONDBLCLK";
			case WM_MOUSEWHEEL: return "WM_MOUSEWHEEL";
			case WM_MOUSEHWHEEL: return "WM_MOUSEHWHEEL";
			case WM_CAPTURECHANGED: return "WM_CAPTURECHANGED";
			case WM_CLOSE: return "WM_CLOSE";
			case WM_DESTROY: return "WM_DESTROY";
			case WM_NCDESTROY: return "WM_NCDESTROY";
			default: return nullptr;
			}
		}

		MessageCategory classifyMessage(UINT message) noexcept
		{
			switch (message)
			{
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
				return MessageCategory::Key;
			case WM_CHAR:
			case WM_SYSCHAR:
			case WM_DEADCHAR:
			case WM_SYSDEADCHAR:
			case WM_UNICHAR:
			case WM_IME_CHAR:
				return MessageCategory::Text;
			case WM_IME_STARTCOMPOSITION:
			case WM_IME_COMPOSITION:
			case WM_IME_ENDCOMPOSITION:
			case WM_IME_SETCONTEXT:
			case WM_IME_NOTIFY:
			case WM_IME_REQUEST:
			case WM_IME_CONTROL:
			case WM_IME_SELECT:
				return MessageCategory::Ime;
			case WM_SETFOCUS:
			case WM_KILLFOCUS:
			case WM_ACTIVATE:
			case WM_ACTIVATEAPP:
				return MessageCategory::Focus;
			case WM_SETCURSOR:
			case WM_MOUSEACTIVATE:
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
				return MessageCategory::Mouse;
			case WM_INPUTLANGCHANGEREQUEST:
			case WM_INPUTLANGCHANGE:
			case WM_MOVE:
			case WM_SIZE:
			case WM_WINDOWPOSCHANGING:
			case WM_WINDOWPOSCHANGED:
			case WM_DPICHANGED:
			case WM_DISPLAYCHANGE:
			case WM_ENTERSIZEMOVE:
			case WM_EXITSIZEMOVE:
			case WM_GETDLGCODE:
			case WM_CLOSE:
			case WM_DESTROY:
			case WM_NCDESTROY:
				return MessageCategory::Window;
			default:
				return MessageCategory::Other;
			}
		}

		bool isGeometryMessage(UINT message) noexcept
		{
			switch (message)
			{
			case WM_MOVE:
			case WM_SIZE:
			case WM_WINDOWPOSCHANGING:
			case WM_WINDOWPOSCHANGED:
			case WM_DPICHANGED:
			case WM_DISPLAYCHANGE:
			case WM_ENTERSIZEMOVE:
			case WM_EXITSIZEMOVE:
			case WM_ACTIVATE:
			case WM_ACTIVATEAPP:
			case WM_SETFOCUS:
			case WM_KILLFOCUS:
				return true;
			default:
				return false;
			}
		}

		bool isPointerMessage(UINT message) noexcept
		{
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
				return true;
			default:
				return false;
			}
		}

		bool shouldObserveMessage(UINT message, bool recordNonInput,
			bool recordPointer, bool geometryProbe) noexcept
		{
			if (geometryProbe && isGeometryMessage(message))
				return true;
			if (isPointerMessage(message) && !recordPointer && !recordNonInput)
				return false;
			return recordNonInput || classifyMessage(message) != MessageCategory::Other;
		}

		bool isKeyMessage(UINT message) noexcept
		{
			return message == WM_KEYDOWN || message == WM_KEYUP ||
				message == WM_SYSKEYDOWN || message == WM_SYSKEYUP;
		}

		bool isTextMessage(UINT message) noexcept
		{
			return classifyMessage(message) == MessageCategory::Text;
		}

		bool keyStateDown(int virtualKey) noexcept
		{
			return (GetKeyState(virtualKey) & 0x8000) != 0;
		}

		bool toggleStateOn(int virtualKey) noexcept
		{
			return (GetKeyState(virtualKey) & 0x0001) != 0;
		}

		std::uint32_t dpiForWindow(HWND hwnd) noexcept
		{
			using Function = UINT(WINAPI*)(HWND);
			const HMODULE user32 = GetModuleHandleW(L"user32.dll");
			const auto function = user32
				? reinterpret_cast<Function>(GetProcAddress(user32, "GetDpiForWindow")) : nullptr;
			return function ? function(hwnd) : 0;
		}

		int awarenessForWindow(HWND hwnd) noexcept
		{
			using GetContextFunction = HANDLE(WINAPI*)(HWND);
			using GetAwarenessFunction = int(WINAPI*)(HANDLE);
			const HMODULE user32 = GetModuleHandleW(L"user32.dll");
			if (!user32)
				return -1;
			const auto getContext = reinterpret_cast<GetContextFunction>(
				GetProcAddress(user32, "GetWindowDpiAwarenessContext"));
			const auto getAwareness = reinterpret_cast<GetAwarenessFunction>(
				GetProcAddress(user32, "GetAwarenessFromDpiAwarenessContext"));
			if (!getContext || !getAwareness)
				return -1;
			return getAwareness(getContext(hwnd));
		}

		bool samePoint(const POINT& left, const POINT& right) noexcept
		{
			return left.x == right.x && left.y == right.y;
		}

		bool sameRect(const RECT& left, const RECT& right) noexcept
		{
			return left.left == right.left && left.top == right.top &&
				left.right == right.right && left.bottom == right.bottom;
		}

		bool sameAnchorForGeometryProbe(
			const ImeCaretAnchorSnapshot& left,
			const ImeCaretAnchorSnapshot& right) noexcept
		{
			return left.valid == right.valid && left.target == right.target &&
				left.scope == right.scope && samePoint(left.clientPoint, right.clientPoint) &&
				sameRect(left.clientLineRect, right.clientLineRect) &&
				sameRect(left.clientEditRect, right.clientEditRect) &&
				left.layoutGeneration == right.layoutGeneration &&
				left.publicationGeneration == right.publicationGeneration;
		}

		void captureWindowGeometry(HWND hwnd, ProbeRecord& record) noexcept
		{
			record.geometrySnapshotPresent = hwnd && IsWindow(hwnd) != FALSE;
			if (!record.geometrySnapshotPresent)
				return;

			record.windowRectAvailable = GetWindowRect(hwnd, &record.windowRect) != FALSE;
			record.clientRectAvailable = GetClientRect(hwnd, &record.clientRect) != FALSE;
			POINT origin{};
			record.clientOriginAvailable = ClientToScreen(hwnd, &origin) != FALSE;
			if (record.clientOriginAvailable)
				record.clientOriginScreen = origin;
			record.windowVisible = IsWindowVisible(hwnd) != FALSE;
			record.windowMinimized = IsIconic(hwnd) != FALSE;
			record.windowMaximized = IsZoomed(hwnd) != FALSE;
			record.geometryDpiForWindow = dpiForWindow(hwnd);

			record.monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
			if (!record.monitor)
				return;
			MONITORINFOEXW info{};
			info.cbSize = sizeof(info);
			if (!GetMonitorInfoW(record.monitor, &info))
				return;
			record.monitorAvailable = true;
			record.monitorRect = info.rcMonitor;
			record.monitorWorkRect = info.rcWork;
			record.primaryMonitor = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
			const int converted = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
				info.szDevice, -1, record.monitorDevice.data(),
				static_cast<int>(record.monitorDevice.size()), nullptr, nullptr);
			if (converted <= 0)
				record.monitorDevice[0] = '\0';
		}

		void parseGeometryMessage(
			ProbeRecord& record, UINT message, WPARAM wParam, LPARAM lParam) noexcept
		{
			record.wParamValue = static_cast<std::uint64_t>(wParam);
			record.lParamValue = static_cast<std::uint64_t>(static_cast<ULONG_PTR>(lParam));
			switch (message)
			{
			case WM_MOVE:
				record.wParamPolicy = ValuePolicy::Raw;
				record.lParamPolicy = ValuePolicy::RawAndParsed;
				record.parsedPointPresent = true;
				record.parsedPoint.x = static_cast<short>(LOWORD(lParam));
				record.parsedPoint.y = static_cast<short>(HIWORD(lParam));
				break;
			case WM_SIZE:
				record.wParamPolicy = ValuePolicy::RawAndParsed;
				record.lParamPolicy = ValuePolicy::RawAndParsed;
				record.sizeType = static_cast<std::uint32_t>(wParam);
				record.parsedSizePresent = true;
				record.parsedSize.cx = static_cast<LONG>(LOWORD(lParam));
				record.parsedSize.cy = static_cast<LONG>(HIWORD(lParam));
				break;
			case WM_WINDOWPOSCHANGING:
			case WM_WINDOWPOSCHANGED:
				record.wParamPolicy = ValuePolicy::Raw;
				record.lParamPolicy = ValuePolicy::ParsedOnly;
				if (lParam)
				{
					record.windowPos = *reinterpret_cast<const WINDOWPOS*>(lParam);
					record.windowPosPresent = true;
				}
				break;
			case WM_DPICHANGED:
				record.wParamPolicy = ValuePolicy::RawAndParsed;
				record.lParamPolicy = ValuePolicy::ParsedOnly;
				record.dpiX = LOWORD(wParam);
				record.dpiY = HIWORD(wParam);
				if (lParam)
				{
					record.suggestedWindowRect = *reinterpret_cast<const RECT*>(lParam);
					record.dpiChangePresent = true;
				}
				break;
			case WM_DISPLAYCHANGE:
				record.wParamPolicy = ValuePolicy::RawAndParsed;
				record.lParamPolicy = ValuePolicy::RawAndParsed;
				record.displayBitsPerPixel = static_cast<std::uint32_t>(wParam);
				record.parsedSizePresent = true;
				record.parsedSize.cx = static_cast<LONG>(LOWORD(lParam));
				record.parsedSize.cy = static_cast<LONG>(HIWORD(lParam));
				break;
			case WM_ACTIVATE:
			case WM_ACTIVATEAPP:
				record.wParamPolicy = ValuePolicy::RawAndParsed;
				record.lParamPolicy = ValuePolicy::RawAndParsed;
				break;
			default:
				record.wParamPolicy = ValuePolicy::ParsedOnly;
				record.lParamPolicy = ValuePolicy::ParsedOnly;
				break;
			}
		}

		void copyImeRuntimeToRecord(
			const ImeCaretAnchorRuntimeSnapshot& runtime, ProbeRecord& record) noexcept
		{
			record.publishedAnchor = runtime.published;
			record.positionedAnchor = runtime.lastPositioned;
			record.imeWindowActive = runtime.windowActive;
			record.imeRefreshPending = runtime.refreshPending;
			record.imeTextInputEligible = runtime.textInputEligible;
			record.imeInputContextDetached = runtime.inputContextDetached;
			record.imePositionRequests = runtime.stats.positionRequests;
			record.imeCompositionSuccesses = runtime.stats.compositionSuccesses;
			record.imeCandidateSuccesses = runtime.stats.candidateSuccesses;
			record.imeSnapshotReadFailures = runtime.stats.snapshotReadFailures;
			record.imeMissingContextSkips = runtime.stats.missingContextSkips;
			record.imeCancelRequests = runtime.stats.cancelRequests;
			record.imeCancelSuccesses = runtime.stats.cancelSuccesses;
			record.imeCancelFailures = runtime.stats.cancelFailures;
			record.imeInputContextGateTransitions = runtime.stats.inputContextGateTransitions;
			record.imeInputContextDetachSuccesses = runtime.stats.inputContextDetachSuccesses;
			record.imeInputContextDetachFailures = runtime.stats.inputContextDetachFailures;
			record.imeInputContextRestoreSuccesses = runtime.stats.inputContextRestoreSuccesses;
			record.imeInputContextRestoreFailures = runtime.stats.inputContextRestoreFailures;
		}

		bool sameGeometrySafePoint(
			const ProbeRecord& left, const ProbeRecord& right) noexcept
		{
			return left.geometrySnapshotPresent == right.geometrySnapshotPresent &&
				left.windowRectAvailable == right.windowRectAvailable &&
				sameRect(left.windowRect, right.windowRect) &&
				left.clientRectAvailable == right.clientRectAvailable &&
				sameRect(left.clientRect, right.clientRect) &&
				left.clientOriginAvailable == right.clientOriginAvailable &&
				samePoint(left.clientOriginScreen, right.clientOriginScreen) &&
				left.monitor == right.monitor &&
				left.monitorAvailable == right.monitorAvailable &&
				sameRect(left.monitorRect, right.monitorRect) &&
				sameRect(left.monitorWorkRect, right.monitorWorkRect) &&
				left.geometryDpiForWindow == right.geometryDpiForWindow &&
				left.windowVisible == right.windowVisible &&
				left.windowMinimized == right.windowMinimized &&
				left.windowMaximized == right.windowMaximized &&
				sameAnchorForGeometryProbe(left.publishedAnchor, right.publishedAnchor) &&
				sameAnchorForGeometryProbe(left.positionedAnchor, right.positionedAnchor) &&
				left.imeWindowActive == right.imeWindowActive &&
				left.imeRefreshPending == right.imeRefreshPending &&
				left.imeTextInputEligible == right.imeTextInputEligible &&
				left.imeInputContextDetached == right.imeInputContextDetached &&
				left.imePositionRequests == right.imePositionRequests &&
				left.imeCompositionSuccesses == right.imeCompositionSuccesses &&
				left.imeCandidateSuccesses == right.imeCandidateSuccesses &&
				left.imeSnapshotReadFailures == right.imeSnapshotReadFailures &&
				left.imeMissingContextSkips == right.imeMissingContextSkips &&
				left.imeCancelRequests == right.imeCancelRequests &&
				left.imeCancelSuccesses == right.imeCancelSuccesses &&
				left.imeCancelFailures == right.imeCancelFailures &&
				left.imeInputContextGateTransitions == right.imeInputContextGateTransitions &&
				left.imeInputContextDetachSuccesses == right.imeInputContextDetachSuccesses &&
				left.imeInputContextDetachFailures == right.imeInputContextDetachFailures &&
				left.imeInputContextRestoreSuccesses == right.imeInputContextRestoreSuccesses &&
				left.imeInputContextRestoreFailures == right.imeInputContextRestoreFailures;
		}

		std::uint32_t ansiCodePageForLayout(HKL layout) noexcept
		{
			if (layout)
			{
				DWORD codePage = 0;
				const LCID locale = MAKELCID(LOWORD(reinterpret_cast<ULONG_PTR>(layout)), SORT_DEFAULT);
				if (GetLocaleInfoW(locale, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
					reinterpret_cast<LPWSTR>(&codePage), sizeof(codePage) / sizeof(wchar_t)) != 0 &&
					codePage != 0)
				{
					return codePage;
				}
			}
			return GetACP();
		}

		bool parseProbeAction(const std::string& value, ProbeAction& action) noexcept
		{
			if (value == "ObserveAndForward")
			{
				action = ProbeAction::ObserveAndForward;
				return true;
			}
			if (value == "ObserveAndConsume")
			{
				action = ProbeAction::ObserveAndConsume;
				return true;
			}
			return false;
		}

		bool parseImeCondition(const std::string& value, ImeStateCondition& condition) noexcept
		{
			if (value == "Any") condition = ImeStateCondition::Any;
			else if (value == "IdleOnly") condition = ImeStateCondition::IdleOnly;
			else if (value == "ComposingOnly") condition = ImeStateCondition::ComposingOnly;
			else return false;
			return true;
		}

		bool parseInputSource(const std::string& value, ProbeInputSource& source) noexcept
		{
			if (value == "SendInput") source = ProbeInputSource::SendInput;
			else if (value == "Manual") source = ProbeInputSource::Manual;
			else return false;
			return true;
		}

		bool parseRuleMessage(const std::string& value, UINT& message) noexcept
		{
			if (value == "WM_KEYDOWN") message = WM_KEYDOWN;
			else if (value == "WM_KEYUP") message = WM_KEYUP;
			else if (value == "WM_SYSKEYDOWN") message = WM_SYSKEYDOWN;
			else if (value == "WM_SYSKEYUP") message = WM_SYSKEYUP;
			else return false;
			return true;
		}

		bool isSha256BaselineId(const std::string& value) noexcept
		{
			constexpr char Prefix[] = "sha256:";
			if (value.size() != (std::size(Prefix) - 1u) + 64u ||
				value.compare(0, std::size(Prefix) - 1u, Prefix) != 0)
			{
				return false;
			}
			return std::all_of(value.begin() + (std::size(Prefix) - 1u), value.end(),
				[](char character) noexcept {
					return (character >= '0' && character <= '9') ||
						(character >= 'a' && character <= 'f') ||
						(character >= 'A' && character <= 'F');
				});
		}

		ConsumeRuleState loadConsumeRule(HWND hwnd, std::uint32_t ansiCodePage) noexcept
		{
			ConsumeRuleState rule{};
			rule.requested = environmentFlagEnabled(L"STELLARX_INPUT_PROBE_0B");
			if (!rule.requested)
				return rule;

			rule.actualWindowUnicode = IsWindowUnicode(hwnd) != FALSE;
			rule.actualAnsiCodePage = ansiCodePage;
			rule.actualDpiForWindow = dpiForWindow(hwnd);
			rule.actualDpiAwareness = awarenessForWindow(hwnd);

			std::string action;
			std::string message;
			std::string condition;
			std::string inputSource;
			std::string baselineId;
			std::uint32_t virtualKey = 0;
			std::uint32_t scanCode = 0;
			std::uint32_t maxMatches = 0;
			bool configurationValid =
				environmentAscii(L"STELLARX_INPUT_PROBE_0B_EXPERIMENT_ID", rule.experimentId, true) &&
				environmentAscii(L"STELLARX_INPUT_PROBE_0B_RULE_ID", rule.ruleId, true) &&
				environmentAscii(L"STELLARX_INPUT_PROBE_0B_ACTION", action) &&
				environmentAscii(L"STELLARX_INPUT_PROBE_0B_MESSAGE", message) &&
				environmentAscii(L"STELLARX_INPUT_PROBE_0B_IME_CONDITION", condition) &&
				environmentAscii(L"STELLARX_INPUT_PROBE_0B_INPUT_SOURCE", inputSource) &&
				environmentAscii(L"STELLARX_INPUT_PROBE_0B_BASELINE_ID", baselineId) &&
				environmentUnsigned(L"STELLARX_INPUT_PROBE_0B_VIRTUAL_KEY", virtualKey) &&
				environmentUnsigned(L"STELLARX_INPUT_PROBE_0B_SCAN_CODE", scanCode) &&
				environmentUnsigned(L"STELLARX_INPUT_PROBE_0B_MAX_MATCHES", maxMatches) &&
				environmentBoolean(L"STELLARX_INPUT_PROBE_0B_EXPECT_WINDOW_UNICODE",
					rule.expectedWindowUnicode) &&
				environmentUnsigned(L"STELLARX_INPUT_PROBE_0B_EXPECT_ANSI_CODE_PAGE",
					rule.expectedAnsiCodePage) &&
				environmentUnsigned(L"STELLARX_INPUT_PROBE_0B_EXPECT_DPI_FOR_WINDOW",
					rule.expectedDpiForWindow) &&
				environmentSigned(L"STELLARX_INPUT_PROBE_0B_EXPECT_DPI_AWARENESS",
					rule.expectedDpiAwareness);

			const bool baselineIdValid = isSha256BaselineId(baselineId);
			if (baselineIdValid)
				rule.baselineId = std::move(baselineId);
			configurationValid = configurationValid && baselineIdValid &&
				parseProbeAction(action, rule.requestedAction) &&
				parseRuleMessage(message, rule.message) &&
				parseImeCondition(condition, rule.imeCondition) &&
				parseInputSource(inputSource, rule.inputSource) &&
				virtualKey > 0 && virtualKey <= 0xFFu && scanCode <= 0x100u && maxMatches > 0 &&
				maxMatches <= MaxConsumeRuleMatches && rule.expectedAnsiCodePage > 0 &&
				rule.expectedDpiForWindow > 0;
			rule.virtualKey = virtualKey;
			rule.scanCodeSpecified = scanCode <= 0xFFu;
			rule.scanCode = rule.scanCodeSpecified ? static_cast<std::uint8_t>(scanCode) : 0;
			rule.maxMatches = maxMatches;
			rule.configurationValid = configurationValid;
			if (!configurationValid)
			{
				rule.failOpenReason = FailOpenReason::InvalidConfiguration;
				return rule;
			}

			if (rule.baselineId != Current0CBaselineId)
				rule.failOpenReason = FailOpenReason::BaselineIdMismatch;
			else if (rule.actualWindowUnicode != rule.expectedWindowUnicode)
				rule.failOpenReason = FailOpenReason::WindowUnicodeMismatch;
			else if (rule.actualAnsiCodePage != rule.expectedAnsiCodePage)
				rule.failOpenReason = FailOpenReason::AnsiCodePageMismatch;
			else if (rule.actualDpiForWindow != rule.expectedDpiForWindow)
				rule.failOpenReason = FailOpenReason::DpiMismatch;
			else if (rule.actualDpiAwareness != rule.expectedDpiAwareness)
				rule.failOpenReason = FailOpenReason::DpiAwarenessMismatch;
			else
				rule.baselineMatched = true;

			if (!rule.baselineMatched)
				return rule;

			if (rule.imeCondition == ImeStateCondition::ComposingOnly)
			{
				const bool identityConfirmed =
					rule.inputSource == ProbeInputSource::Manual &&
					environmentFlagEnabled(L"STELLARX_INPUT_PROBE_0B_IME_IDENTITY_CONFIRMED") &&
					environmentAscii(L"STELLARX_INPUT_PROBE_0B_IME_NAME", rule.imeName, true);
				if (!identityConfirmed)
				{
					rule.failOpenReason = FailOpenReason::ImeIdentityUnconfirmed;
					return rule;
				}

				int caretX = 0;
				int caretY = 0;
				int lineLeft = 0;
				int lineTop = 0;
				int lineRight = 0;
				int lineBottom = 0;
				rule.imePositionConfigured =
					environmentSigned(L"STELLARX_INPUT_PROBE_0B_IME_CARET_X", caretX) &&
					environmentSigned(L"STELLARX_INPUT_PROBE_0B_IME_CARET_Y", caretY) &&
					environmentSigned(L"STELLARX_INPUT_PROBE_0B_IME_LINE_LEFT", lineLeft) &&
					environmentSigned(L"STELLARX_INPUT_PROBE_0B_IME_LINE_TOP", lineTop) &&
					environmentSigned(L"STELLARX_INPUT_PROBE_0B_IME_LINE_RIGHT", lineRight) &&
					environmentSigned(L"STELLARX_INPUT_PROBE_0B_IME_LINE_BOTTOM", lineBottom) &&
					caretX >= 0 && caretY >= 0 && lineLeft >= 0 && lineTop >= 0 &&
					lineRight > lineLeft && lineBottom > lineTop &&
					caretX >= lineLeft && caretX <= lineRight &&
					caretY >= lineTop && caretY <= lineBottom;
				if (!rule.imePositionConfigured)
				{
					rule.failOpenReason = FailOpenReason::ImePositionUnavailable;
					return rule;
				}
				rule.imeCaretPoint = POINT{ caretX, caretY };
				rule.imeLineRect = RECT{ lineLeft, lineTop, lineRight, lineBottom };
			}

			rule.eligible = true;
			rule.failOpenReason = FailOpenReason::None;
			return rule;
		}

		bool analyzeUtf16(const wchar_t* text, std::size_t length, std::uint32_t& scalarCount) noexcept
		{
			scalarCount = 0;
			for (std::size_t index = 0; index < length; ++index)
			{
				const std::uint16_t unit = static_cast<std::uint16_t>(text[index]);
				if (unit >= 0xD800 && unit <= 0xDBFF)
				{
					if (index + 1 >= length)
						return false;
					const std::uint16_t next = static_cast<std::uint16_t>(text[index + 1]);
					if (next < 0xDC00 || next > 0xDFFF)
						return false;
					++index;
				}
				else if (unit >= 0xDC00 && unit <= 0xDFFF)
				{
					return false;
				}
				++scalarCount;
			}
			return true;
		}

		bool computeSessionDigest(const wchar_t* text, std::size_t length,
			std::array<char, 33>& output) noexcept
		{
			auto& state = probeState();
			if (!state.sessionKeyReady || length > (std::numeric_limits<ULONG>::max)() / sizeof(wchar_t))
				return false;

			BCRYPT_ALG_HANDLE algorithm = nullptr;
			BCRYPT_HASH_HANDLE hash = nullptr;
			try
			{
				if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr,
					BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0)
					return false;
				DWORD objectLength = 0;
				DWORD returned = 0;
				if (BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
					reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength), &returned, 0) < 0)
				{
					BCryptCloseAlgorithmProvider(algorithm, 0);
					return false;
				}
				std::vector<unsigned char> hashObject(objectLength);
				std::array<unsigned char, 32> digest{};
				if (BCryptCreateHash(algorithm, &hash, hashObject.data(), objectLength,
					reinterpret_cast<PUCHAR>(state.sessionKey.data()),
					static_cast<ULONG>(state.sessionKey.size()), 0) < 0 ||
					BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(text)),
						static_cast<ULONG>(length * sizeof(wchar_t)), 0) < 0 ||
					BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) < 0)
				{
					if (hash) BCryptDestroyHash(hash);
					BCryptCloseAlgorithmProvider(algorithm, 0);
					return false;
				}
				BCryptDestroyHash(hash);
				BCryptCloseAlgorithmProvider(algorithm, 0);
				for (std::size_t index = 0; index < 16; ++index)
					std::snprintf(output.data() + index * 2, 3, "%02x", digest[index]);
				output[32] = '\0';
				return true;
			}
			catch (...)
			{
				if (hash) BCryptDestroyHash(hash);
				if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
				return false;
			}
		}

		ProbeRecord makeRecord(RecordType type, HWND hwnd) noexcept
		{
			auto& state = probeState();
			ProbeRecord record{};
			record.type = type;
			record.sequence = state.nextSequence.fetch_add(1, std::memory_order_relaxed) + 1;
			record.qpcTicks = qpcNow();
			record.threadId = GetCurrentThreadId();
			record.processId = state.processId;
			record.hwnd = hwnd;
			record.hwndInstanceId = hwnd ? state.hwndInstanceId : 0;
			record.dispatchDepth = state.dispatchDepth;
			record.modalDepth = state.modalDepth;
			record.compositionId = state.activeCompositionId;
			record.imeStateBefore = state.imeComposing;
			record.imeStateAfter = state.imeComposing;
			return record;
		}

		void noteOverflowLostSequence(std::uint64_t sequence) noexcept
		{
			auto& state = probeState();
			if (state.overflowLostCount == 0)
				state.overflowLostFirstSequence = sequence;
			state.overflowLostLastSequence = sequence;
			++state.overflowLostCount;
			state.totalLostCount.fetch_add(1, std::memory_order_relaxed);
		}

		void pushRecord(const ProbeRecord& record) noexcept
		{
			auto& state = probeState();
			if (!state.enabled.load(std::memory_order_relaxed) || state.hardLimit == 0)
				return;

			if (state.count < state.hardLimit)
			{
				const std::size_t index = (state.head + state.count) % state.hardLimit;
				state.recordBuffers[state.activeRecordBuffer][index] = record;
				++state.count;
				return;
			}

			const std::uint64_t lostSequence =
				state.recordBuffers[state.activeRecordBuffer][state.head].sequence;
			noteOverflowLostSequence(lostSequence);
			state.recordBuffers[state.activeRecordBuffer][state.head] = record;
			state.head = (state.head + 1) % state.hardLimit;
		}

		CorrelationStatus summarizeEchoChannel(const EchoCorrelationState& echo,
			EchoChannel channel, CorrelationStatus emptyStatus) noexcept
		{
			const bool mismatched = channel == EchoChannel::ImeChar
				? echo.imeMismatched : echo.charMismatched;
			const std::size_t matched = channel == EchoChannel::ImeChar
				? echo.imeMatched : echo.charMatched;
			const std::size_t expected = echo.windowUnicode ? echo.utf16Count : echo.ansiCount;
			if (echo.truncated && expected == 0)
				return CorrelationStatus::Unrepresentable;
			if (!echo.windowUnicode && !echo.ansiRepresentable)
				return CorrelationStatus::Unrepresentable;
			if (mismatched)
				return CorrelationStatus::Mismatched;
			if (expected > 0 && matched == expected)
				return CorrelationStatus::Matched;
			if (matched > 0)
				return CorrelationStatus::Partial;
			return emptyStatus;
		}

		void finishEchoCorrelationLocked(HWND hwnd, CorrelationStatus emptyStatus) noexcept
		{
			auto& state = probeState();
			if (!state.echo.active)
				return;

			const std::size_t expected = state.echo.windowUnicode
				? state.echo.utf16Count : state.echo.ansiCount;
			for (const EchoChannel channel : { EchoChannel::ImeChar, EchoChannel::WmChar })
			{
				ProbeRecord summary = makeRecord(RecordType::EchoSummary, hwnd);
				summary.compositionId = state.echo.compositionId;
				summary.echoChannel = channel;
				summary.correlationTargetSequence = state.echo.targetSequence;
				summary.matchedUnitCount = static_cast<std::uint32_t>(channel == EchoChannel::ImeChar
					? state.echo.imeMatched : state.echo.charMatched);
				summary.expectedUnitCount = static_cast<std::uint32_t>(expected);
				summary.correlationStatus = summarizeEchoChannel(state.echo, channel, emptyStatus);
				summary.correlationTruncated = state.echo.truncated;
				summary.ansiCodePage = state.echo.ansiCodePage;
				pushRecord(summary);
			}
			state.echo = {};
		}

		bool beginEchoCorrelationLocked(HWND hwnd, const wchar_t* text, std::size_t length,
			ProbeRecord& query) noexcept
		{
			auto& state = probeState();
			state.echo = {};
			state.echo.active = true;
			state.echo.windowUnicode = IsWindowUnicode(hwnd) != FALSE;
			state.echo.targetSequence = query.sequence;
			state.echo.compositionId = query.compositionId;
			state.echo.ansiCodePage = query.ansiCodePage != 0 ? query.ansiCodePage : GetACP();
			query.ansiCodePage = state.echo.ansiCodePage;

			if (length > state.echo.utf16.size())
			{
				state.echo.truncated = true;
				query.correlationTruncated = true;
				return false;
			}
			std::copy_n(text, length, state.echo.utf16.begin());
			state.echo.utf16Count = length;
			if (state.echo.windowUnicode)
				return true;

			BOOL usedDefault = FALSE;
			BOOL* usedDefaultPointer = state.echo.ansiCodePage == CP_UTF7 ||
				state.echo.ansiCodePage == CP_UTF8 ? nullptr : &usedDefault;
			const int required = WideCharToMultiByte(state.echo.ansiCodePage, 0, text,
				static_cast<int>(length), nullptr, 0, nullptr, usedDefaultPointer);
			if (required <= 0 || static_cast<std::size_t>(required) > state.echo.ansi.size())
			{
				state.echo.truncated = required > 0;
				query.correlationTruncated = state.echo.truncated;
				return false;
			}
			usedDefault = FALSE;
			const int converted = WideCharToMultiByte(state.echo.ansiCodePage, 0, text,
				static_cast<int>(length), reinterpret_cast<char*>(state.echo.ansi.data()),
				required, nullptr, usedDefaultPointer);
			query.ansiConversionUsedDefault = usedDefaultPointer && usedDefault != FALSE;
			state.echo.ansiRepresentable = converted == required &&
				(!usedDefaultPointer || usedDefault == FALSE);
			state.echo.ansiCount = converted > 0 ? static_cast<std::size_t>(converted) : 0;
			return state.echo.ansiRepresentable;
		}

		bool consumeExpectedUnits(const unsigned char* observed, std::size_t observedCount,
			const unsigned char* expected, std::size_t expectedCount,
			std::size_t& matched) noexcept
		{
			if (matched + observedCount > expectedCount)
				return false;
			for (std::size_t index = 0; index < observedCount; ++index)
			{
				if (observed[index] != expected[matched + index])
					return false;
			}
			matched += observedCount;
			return true;
		}

		bool consumeExpectedUtf16(wchar_t observed, const EchoCorrelationState& echo,
			std::size_t& matched) noexcept
		{
			if (matched >= echo.utf16Count || echo.utf16[matched] != observed)
				return false;
			++matched;
			return true;
		}

		void correlateTextMessageLocked(ProbeRecord& record, WPARAM wParam) noexcept
		{
			auto& state = probeState();
			if (record.messageId == WM_IME_CHAR)
			{
				record.textSource = TextSource::WmImeChar;
				record.echoChannel = EchoChannel::ImeChar;
			}
			else if (record.messageId == WM_CHAR)
			{
				record.textSource = TextSource::WmChar;
				record.echoChannel = EchoChannel::WmChar;
			}
			else
			{
				return;
			}

			if (!state.echo.active)
				return;
			record.compositionId = state.echo.compositionId;
			record.correlationTargetSequence = state.echo.targetSequence;
			record.ansiCodePage = state.echo.ansiCodePage;
			record.correlationTruncated = state.echo.truncated;
			std::size_t& matched = record.echoChannel == EchoChannel::ImeChar
				? state.echo.imeMatched : state.echo.charMatched;
			bool& mismatched = record.echoChannel == EchoChannel::ImeChar
				? state.echo.imeMismatched : state.echo.charMismatched;

			if (!state.echo.windowUnicode && !state.echo.ansiRepresentable)
			{
				record.correlationStatus = CorrelationStatus::Unrepresentable;
				return;
			}

			bool matches = false;
			const std::size_t expected = state.echo.windowUnicode
				? state.echo.utf16Count : state.echo.ansiCount;
			if (state.echo.truncated && expected == 0)
			{
				record.correlationStatus = CorrelationStatus::Unrepresentable;
				return;
			}
			if (state.echo.windowUnicode)
			{
				matches = consumeExpectedUtf16(static_cast<wchar_t>(wParam), state.echo, matched);
			}
			else if (record.messageId == WM_IME_CHAR)
			{
				unsigned char bytes[2]{};
				std::size_t byteCount = 1;
				if ((wParam & 0xFF00u) != 0)
				{
					bytes[0] = static_cast<unsigned char>((wParam >> 8) & 0xFFu);
					bytes[1] = static_cast<unsigned char>(wParam & 0xFFu);
					byteCount = 2;
				}
				else
				{
					bytes[0] = static_cast<unsigned char>(wParam & 0xFFu);
				}
				matches = consumeExpectedUnits(bytes, byteCount, state.echo.ansi.data(),
					state.echo.ansiCount, matched);
			}
			else
			{
				const unsigned char byte = static_cast<unsigned char>(wParam & 0xFFu);
				matches = consumeExpectedUnits(&byte, 1, state.echo.ansi.data(),
					state.echo.ansiCount, matched);
			}

			if (!matches)
				mismatched = true;
			record.matchedUnitCount = static_cast<std::uint32_t>(matched);
			record.expectedUnitCount = static_cast<std::uint32_t>(expected);
			record.correlationStatus = mismatched ? CorrelationStatus::Mismatched
				: (matched == expected ? CorrelationStatus::Matched : CorrelationStatus::Candidate);
		}

		void captureImeResultLocked(HWND hwnd, const ProbeRecord& parent) noexcept
		{
			finishEchoCorrelationLocked(hwnd, CorrelationStatus::Expired);
			ProbeRecord query = makeRecord(RecordType::ImmQuery, hwnd);
			query.parentSequence = parent.sequence;
			query.compositionId = parent.compositionId;
			query.textSource = TextSource::GcsResultStr;
			query.windowUnicode = parent.windowUnicode;
			query.ansiCodePage = probeState().ansiCodePage != 0 ? probeState().ansiCodePage : GetACP();
			HIMC context = ImmGetContext(hwnd);
			if (!context)
			{
				query.immQueryStatus = ImmQueryStatus::NoContext;
				pushRecord(query);
				return;
			}

			try
			{
				const LONG required = ImmGetCompositionStringW(context, GCS_RESULTSTR, nullptr, 0);
				if (required < 0)
				{
					query.immQueryStatus = ImmQueryStatus::SizeError;
				}
				else if (required == 0)
				{
					query.immQueryStatus = ImmQueryStatus::Empty;
				}
				else if ((required % sizeof(wchar_t)) != 0)
				{
					query.immQueryStatus = ImmQueryStatus::OddByteLength;
				}
				else if (static_cast<std::size_t>(required) > MaxImeResultBytes)
				{
					query.immQueryStatus = ImmQueryStatus::TooLarge;
				}
				else
				{
					std::vector<wchar_t> result(static_cast<std::size_t>(required) / sizeof(wchar_t));
					const LONG copied = ImmGetCompositionStringW(context, GCS_RESULTSTR,
						result.data(), static_cast<DWORD>(required));
					if (copied < 0 || copied > required || (copied % sizeof(wchar_t)) != 0)
					{
						query.immQueryStatus = ImmQueryStatus::ReadError;
					}
					else
					{
						const std::size_t length = static_cast<std::size_t>(copied) / sizeof(wchar_t);
						query.immQueryStatus = ImmQueryStatus::Success;
						query.immByteLength = static_cast<std::uint32_t>(copied);
						query.utf16Length = static_cast<std::uint32_t>(length);
						query.textMetadataPresent = true;
						query.validUtf16 = analyzeUtf16(result.data(), length, query.scalarCount);
						query.digestPresent = computeSessionDigest(result.data(), length, query.sessionDigest);
						if (query.validUtf16 && length > 0)
							beginEchoCorrelationLocked(hwnd, result.data(), length, query);
					}
				}
			}
			catch (...)
			{
				query.immQueryStatus = ImmQueryStatus::ReadError;
			}
			ImmReleaseContext(hwnd, context);
			pushRecord(query);
		}

		void appendHex(std::ostringstream& output, std::uint64_t value)
		{
			char buffer[32]{};
			std::snprintf(buffer, sizeof(buffer), "0x%016llX", static_cast<unsigned long long>(value));
			output << '"' << buffer << '"';
		}

		void appendHandle(std::ostringstream& output, HWND hwnd)
		{
			if (!hwnd)
			{
				output << "null";
				return;
			}
			appendHex(output, static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(hwnd)));
		}

		void appendBoolean(std::ostringstream& output, bool value)
		{
			output << (value ? "true" : "false");
		}

		void appendJsonString(std::ostringstream& output, const char* value)
		{
			output << '"';
			if (value)
			{
				for (const unsigned char* cursor =
					reinterpret_cast<const unsigned char*>(value); *cursor; ++cursor)
				{
					switch (*cursor)
					{
					case '"': output << "\\\""; break;
					case '\\': output << "\\\\"; break;
					case '\b': output << "\\b"; break;
					case '\f': output << "\\f"; break;
					case '\n': output << "\\n"; break;
					case '\r': output << "\\r"; break;
					case '\t': output << "\\t"; break;
					default:
						if (*cursor < 0x20)
						{
							char escaped[8]{};
							std::snprintf(escaped, sizeof(escaped), "\\u%04X",
								static_cast<unsigned int>(*cursor));
							output << escaped;
						}
						else
						{
							output << static_cast<char>(*cursor);
						}
						break;
					}
				}
			}
			output << '"';
		}

		void appendPoint(std::ostringstream& output, const POINT& point)
		{
			output << "{\"x\":" << point.x << ",\"y\":" << point.y << '}';
		}

		void appendRect(std::ostringstream& output, const RECT& rect)
		{
			output << "{\"left\":" << rect.left << ",\"top\":" << rect.top
				<< ",\"right\":" << rect.right << ",\"bottom\":" << rect.bottom << '}';
		}

		void appendAnchor(std::ostringstream& output, const ImeCaretAnchorSnapshot& anchor)
		{
			output << "{\"valid\":";
			appendBoolean(output, anchor.valid);
			output << ",\"targetOwner\":" << anchor.target.owner.value
				<< ",\"targetSubNode\":" << anchor.target.subNode
				<< ",\"scope\":" << anchor.scope.value
				<< ",\"clientPoint\":";
			appendPoint(output, anchor.clientPoint);
			output << ",\"clientLineRect\":";
			appendRect(output, anchor.clientLineRect);
			output << ",\"clientEditRect\":";
			appendRect(output, anchor.clientEditRect);
			output << ",\"layoutGeneration\":" << anchor.layoutGeneration
				<< ",\"publicationGeneration\":" << anchor.publicationGeneration << '}';
		}

		void appendGeometrySnapshot(std::ostringstream& output, const ProbeRecord& record)
		{
			output << ",\"geometrySnapshotPresent\":";
			appendBoolean(output, record.geometrySnapshotPresent);
			output << ",\"windowRect\":";
			if (record.windowRectAvailable) appendRect(output, record.windowRect);
			else output << "null";
			output << ",\"clientRect\":";
			if (record.clientRectAvailable) appendRect(output, record.clientRect);
			else output << "null";
			output << ",\"clientOriginScreen\":";
			if (record.clientOriginAvailable) appendPoint(output, record.clientOriginScreen);
			else output << "null";
			output << ",\"windowVisible\":";
			appendBoolean(output, record.windowVisible);
			output << ",\"windowMinimized\":";
			appendBoolean(output, record.windowMinimized);
			output << ",\"windowMaximized\":";
			appendBoolean(output, record.windowMaximized);
			output << ",\"dpiForWindow\":" << record.geometryDpiForWindow
				<< ",\"monitorHandle\":";
			if (record.monitorAvailable)
				appendHex(output, static_cast<std::uint64_t>(
					reinterpret_cast<std::uintptr_t>(record.monitor)));
			else output << "null";
			output << ",\"monitorDevice\":";
			if (record.monitorAvailable) appendJsonString(output, record.monitorDevice.data());
			else output << "null";
			output << ",\"monitorRect\":";
			if (record.monitorAvailable) appendRect(output, record.monitorRect);
			else output << "null";
			output << ",\"monitorWorkRect\":";
			if (record.monitorAvailable) appendRect(output, record.monitorWorkRect);
			else output << "null";
			output << ",\"primaryMonitor\":";
			if (record.monitorAvailable) appendBoolean(output, record.primaryMonitor);
			else output << "null";
		}

		void appendExperimentTag(std::ostringstream& output, const ProbeRecord& record)
		{
			if (!record.experimentTagged)
				return;
			const auto& rule = probeState().consumeRule;
			output << ",\"experimentId\":\"" << rule.experimentId
				<< "\",\"ruleId\":\"" << rule.ruleId << '"';
			if (record.type == RecordType::EasyXKey || record.type == RecordType::EasyXMouse ||
				record.type == RecordType::ImePosition)
				return;
			output << ",\"ruleMatched\":";
			appendBoolean(output, record.ruleMatched);
			output << ",\"matchOrdinal\":" << record.matchOrdinal
				<< ",\"requestedAction\":\"" << probeActionName(record.requestedAction)
				<< "\",\"actualAction\":\"" << probeActionName(record.actualAction)
				<< "\",\"failOpenApplied\":";
			appendBoolean(output, record.failOpenApplied);
			output << ",\"failOpenReason\":\"" << failOpenReasonName(record.failOpenReason) << '"';
		}

		void appendCommon(std::ostringstream& output, const ProbeRecord& record)
		{
			const auto& state = probeState();
			output << "{\"schemaVersion\":" << state.schemaVersion
				<< ",\"recordType\":\"" << recordTypeName(record.type)
				<< "\",\"sessionId\":\"" << state.sessionId.data()
				<< "\",\"sequence\":" << record.sequence
				<< ",\"sourceBackend\":\"" << sourceBackendName(record.type)
				<< "\",\"observationPoint\":\"" << observationPointName(record.type)
				<< "\",\"qpcTicks\":" << record.qpcTicks
				<< ",\"threadId\":" << record.threadId
				<< ",\"processId\":" << record.processId
				<< ",\"hwnd\":";
			appendHandle(output, record.hwnd);
			output << ",\"hwndInstanceId\":";
			if (record.hwndInstanceId == 0) output << "null"; else output << record.hwndInstanceId;
			output << ",\"dispatchDepth\":" << record.dispatchDepth
				<< ",\"modalDepth\":" << record.modalDepth
				<< ",\"imeState\":\"" << (record.imeStateAfter ? "composing" : "idle")
				<< "\",\"compositionId\":" << record.compositionId;
		}

		void appendImeFlags(std::ostringstream& output, std::uint32_t flags)
		{
			output << ",\"hasGcsCompStr\":"; appendBoolean(output, (flags & GCS_COMPSTR) != 0);
			output << ",\"hasGcsCompAttr\":"; appendBoolean(output, (flags & GCS_COMPATTR) != 0);
			output << ",\"hasGcsCompClause\":"; appendBoolean(output, (flags & GCS_COMPCLAUSE) != 0);
			output << ",\"hasGcsCursorPos\":"; appendBoolean(output, (flags & GCS_CURSORPOS) != 0);
			output << ",\"hasGcsResultStr\":"; appendBoolean(output, (flags & GCS_RESULTSTR) != 0);
			output << ",\"hasGcsResultClause\":"; appendBoolean(output, (flags & GCS_RESULTCLAUSE) != 0);
			output << ",\"hasCsInsertChar\":"; appendBoolean(output, (flags & CS_INSERTCHAR) != 0);
			output << ",\"hasCsNoMoveCaret\":"; appendBoolean(output, (flags & CS_NOMOVECARET) != 0);
			output << ",\"hasAnyGcsFlag\":"; appendBoolean(output, (flags & 0xFFFFu) != 0);
		}

		std::string serializeRecord(const ProbeRecord& record)
		{
			const auto& state = probeState();
			std::ostringstream output;
			appendCommon(output, record);

			switch (record.type)
			{
			case RecordType::SessionStart:
				output << ",\"startedAtUtc\":\"" << state.startedAtUtc.data()
					<< "\",\"qpcFrequency\":" << state.qpcFrequency
					<< ",\"uiThreadId\":" << state.uiThreadId
					<< ",\"hostThreadId\":" << state.hostThreadId
					<< ",\"softLimit\":" << state.softLimit
					<< ",\"hardLimit\":" << state.hardLimit;
				break;
			case RecordType::SessionEnd:
				output << ",\"lostCount\":" << record.lostCountValue;
				break;
			case RecordType::WindowAttach:
				output << ",\"isWindowUnicode\":"; appendBoolean(output, record.windowUnicode);
				output << ",\"windowClassDigest\":null,\"dpiForWindow\":" << record.dpiForWindow
					<< ",\"dpiAwareness\":" << record.dpiAwareness
					<< ",\"ansiCodePage\":" << record.ansiCodePage;
				break;
			case RecordType::ExperimentConfig:
			{
				const auto& rule = state.consumeRule;
				const char* targetName = messageName(rule.message);
				output << ",\"experimentId\":\"" << rule.experimentId
					<< "\",\"ruleId\":\"" << rule.ruleId
					<< "\",\"requestedAction\":\"" << probeActionName(rule.requestedAction)
					<< "\",\"targetMessageId\":" << rule.message
					<< ",\"targetMessage\":\"" << (targetName ? targetName : "unknown")
					<< "\",\"targetVirtualKey\":" << rule.virtualKey
					<< ",\"targetScanCode\":";
				if (rule.scanCodeSpecified) output << static_cast<unsigned int>(rule.scanCode);
				else output << "null";
				output
					<< ",\"imeCondition\":\"" << imeConditionName(rule.imeCondition)
					<< "\",\"maxMatches\":" << rule.maxMatches
					<< ",\"inputSource\":\"" << inputSourceName(rule.inputSource)
					<< "\",\"baselineId\":\"" << rule.baselineId
					<< "\",\"configurationValid\":";
				appendBoolean(output, rule.configurationValid);
				output << ",\"baselineMatched\":";
				appendBoolean(output, rule.baselineMatched);
				output << ",\"ruleEligible\":";
				appendBoolean(output, rule.eligible);
				output << ",\"failOpenReason\":\"" << failOpenReasonName(rule.failOpenReason)
					<< "\",\"expectedWindowUnicode\":";
				appendBoolean(output, rule.expectedWindowUnicode);
				output << ",\"actualWindowUnicode\":";
				appendBoolean(output, rule.actualWindowUnicode);
				output << ",\"expectedAnsiCodePage\":" << rule.expectedAnsiCodePage
					<< ",\"actualAnsiCodePage\":" << rule.actualAnsiCodePage
					<< ",\"expectedDpiForWindow\":" << rule.expectedDpiForWindow
					<< ",\"actualDpiForWindow\":" << rule.actualDpiForWindow
					<< ",\"expectedDpiAwareness\":" << rule.expectedDpiAwareness
					<< ",\"actualDpiAwareness\":" << rule.actualDpiAwareness
					<< ",\"imePositionConfigured\":";
				appendBoolean(output, rule.imePositionConfigured);
				output << ",\"imeCaretPoint\":{\"x\":" << rule.imeCaretPoint.x
					<< ",\"y\":" << rule.imeCaretPoint.y
					<< "},\"imeLineRect\":{\"left\":" << rule.imeLineRect.left
					<< ",\"top\":" << rule.imeLineRect.top
					<< ",\"right\":" << rule.imeLineRect.right
					<< ",\"bottom\":" << rule.imeLineRect.bottom << '}'
					<< ",\"imeName\":";
				if (rule.imeName.empty()) output << "null";
				else output << '"' << rule.imeName << '"';
				break;
			}
			case RecordType::WndProcEnter:
			{
				const char* knownName = messageName(record.messageId);
				char unknownName[32]{};
				if (!knownName)
				{
					std::snprintf(unknownName, sizeof(unknownName), "UNKNOWN_0x%04X", record.messageId);
					knownName = unknownName;
				}
				output << ",\"wndProcCallId\":" << record.wndProcCallId
					<< ",\"callDepth\":" << record.callDepth
					<< ",\"messageId\":" << record.messageId
					<< ",\"messageName\":\"" << knownName
					<< "\",\"messageCategory\":\"" << categoryName(record.messageCategory)
					<< "\",\"wParamPolicy\":\"" << valuePolicyName(record.wParamPolicy)
					<< "\",\"wParamHex\":";
				if (record.wParamPolicy == ValuePolicy::Raw || record.wParamPolicy == ValuePolicy::RawAndParsed)
					appendHex(output, record.wParamValue);
				else output << "null";
				output << ",\"lParamPolicy\":\"" << valuePolicyName(record.lParamPolicy)
					<< "\",\"lParamHex\":";
				if (record.lParamPolicy == ValuePolicy::Raw || record.lParamPolicy == ValuePolicy::RawAndParsed)
					appendHex(output, record.lParamValue);
				else output << "null";
				output << ",\"messageTime\":";
				if (record.messageTimeAvailable) output << record.messageTime;
				else output << "null";
				output << ",\"messageTimeSource\":\"thread_last_retrieved_message\"";

				if (state.geometryProbeEnabled && isGeometryMessage(record.messageId))
				{
					output << ",\"parentWndProcCallId\":";
					if (record.parentWndProcCallId == 0) output << "null";
					else output << record.parentWndProcCallId;
					output << ",\"rootWndProcCallId\":" << record.rootWndProcCallId;
					if (record.parsedPointPresent)
					{
						output << ",\"parsedPoint\":";
						appendPoint(output, record.parsedPoint);
					}
					if (record.parsedSizePresent)
					{
						output << ",\"parsedSize\":{\"width\":" << record.parsedSize.cx
							<< ",\"height\":" << record.parsedSize.cy << '}';
					}
					if (record.messageId == WM_SIZE)
						output << ",\"sizeType\":" << record.sizeType;
					if (record.windowPosPresent)
					{
						output << ",\"windowPos\":{\"x\":" << record.windowPos.x
							<< ",\"y\":" << record.windowPos.y
							<< ",\"cx\":" << record.windowPos.cx
							<< ",\"cy\":" << record.windowPos.cy
							<< ",\"flags\":" << record.windowPos.flags
							<< ",\"insertAfter\":";
						appendHandle(output, record.windowPos.hwndInsertAfter);
						output << '}';
					}
					if (record.messageId == WM_DPICHANGED)
					{
						output << ",\"dpiX\":" << record.dpiX << ",\"dpiY\":" << record.dpiY
							<< ",\"suggestedWindowRect\":";
						if (record.dpiChangePresent) appendRect(output, record.suggestedWindowRect);
						else output << "null";
					}
					if (record.messageId == WM_DISPLAYCHANGE)
						output << ",\"displayBitsPerPixel\":" << record.displayBitsPerPixel;
					appendGeometrySnapshot(output, record);
				}

				if (isKeyMessage(record.messageId))
				{
					output << ",\"virtualKey\":" << record.wParamValue
						<< ",\"action\":\"" <<
						((record.messageId == WM_KEYDOWN || record.messageId == WM_SYSKEYDOWN) ? "down" : "up")
						<< "\",\"repeatCount\":" << record.repeatCount
						<< ",\"scanCode\":" << static_cast<unsigned int>(record.scanCode)
						<< ",\"extended\":"; appendBoolean(output, record.extended);
					output << ",\"altContext\":"; appendBoolean(output, record.altContext);
					output << ",\"wasDown\":"; appendBoolean(output, record.wasDown);
					output << ",\"transition\":"; appendBoolean(output, record.transition);
					output << ",\"systemKey\":"; appendBoolean(output, record.systemKey);
					output << ",\"imeComposingAtObservation\":"; appendBoolean(output, record.imeStateBefore);
					output << ",\"modifierSource\":\"GetKeyState_observation_snapshot\""
						<< ",\"leftShift\":"; appendBoolean(output, record.leftShift);
					output << ",\"rightShift\":"; appendBoolean(output, record.rightShift);
					output << ",\"leftCtrl\":"; appendBoolean(output, record.leftCtrl);
					output << ",\"rightCtrl\":"; appendBoolean(output, record.rightCtrl);
					output << ",\"leftAlt\":"; appendBoolean(output, record.leftAlt);
					output << ",\"rightAlt\":"; appendBoolean(output, record.rightAlt);
					output << ",\"capsLock\":"; appendBoolean(output, record.capsLock);
					output << ",\"numLock\":"; appendBoolean(output, record.numLock);
					output << ",\"scrollLock\":"; appendBoolean(output, record.scrollLock);
				}

				if (record.messageId == WM_IME_COMPOSITION)
					appendImeFlags(output, record.imeFlags);
				if (record.messageId == WM_IME_STARTCOMPOSITION || record.messageId == WM_IME_COMPOSITION ||
					record.messageId == WM_IME_ENDCOMPOSITION)
				{
					const char* allocation = "none";
					if (record.compositionAllocation == CompositionAllocation::StartComposition)
						allocation = "start_composition";
					else if (record.compositionAllocation == CompositionAllocation::LazyMissingStart)
						allocation = "lazy_missing_start";
					output << ",\"imeStateBefore\":\"" << (record.imeStateBefore ? "composing" : "idle")
						<< "\",\"imeStateAfter\":\"" << (record.imeStateAfter ? "composing" : "idle")
						<< "\",\"compositionIdAllocation\":\"" << allocation << '"';
				}
				if (record.messageId == WM_UNICHAR)
				{
					output << ",\"unicodeNoCharQuery\":";
					appendBoolean(output, record.unicodeNoCharQuery);
				}
				if (record.messageId == WM_INPUTLANGCHANGEREQUEST)
				{
					output << ",\"requestFlags\":";
					appendHex(output, record.wParamValue);
					output << ",\"requestedInputLanguageHandle\":";
					appendHex(output, record.lParamValue);
					output << ",\"inputLanguageSysCharset\":";
					appendBoolean(output, record.inputLanguageSysCharset);
					output << ",\"inputLanguageForward\":";
					appendBoolean(output, record.inputLanguageForward);
					output << ",\"inputLanguageBackward\":";
					appendBoolean(output, record.inputLanguageBackward);
					output << ",\"requestedAnsiCodePage\":" << record.ansiCodePage;
				}
				if (record.messageId == WM_INPUTLANGCHANGE)
					output << ",\"ansiCodePage\":" << record.ansiCodePage;
				if (record.messageId == WM_GETDLGCODE)
				{
					output << ",\"queryVirtualKey\":" << record.wParamValue
						<< ",\"hasMessagePointer\":";
					appendBoolean(output, record.lParamValue != 0);
				}
				if (record.messageCategory == MessageCategory::Mouse)
				{
					output << ",\"captureHwndAtEnter\":";
					appendHandle(output, record.captureHwnd);
					output << ",\"captureChangedTo\":";
					appendHandle(output, record.captureChangedTo);
					if (record.messageId != WM_CAPTURECHANGED)
					{
						const int x = static_cast<short>(record.lParamValue & 0xFFFFu);
						const int y = static_cast<short>((record.lParamValue >> 16) & 0xFFFFu);
						output << ",\"mouseX\":" << x << ",\"mouseY\":" << y
							<< ",\"coordinateSpace\":\""
							<< ((record.messageId == WM_MOUSEWHEEL || record.messageId == WM_MOUSEHWHEEL)
								? "screen" : "client") << "\"";
					}
				}
				if (isTextMessage(record.messageId))
				{
					output << ",\"textPayload\":\"redacted\",\"isWindowUnicode\":";
					appendBoolean(output, record.windowUnicode);
					output << ",\"textSource\":\"" << textSourceName(record.textSource)
						<< "\",\"correlationStatus\":\"" << correlationStatusName(record.correlationStatus)
						<< "\",\"echoChannel\":\"" << echoChannelName(record.echoChannel)
						<< "\",\"correlationTargetSequence\":";
					if (record.correlationTargetSequence == 0) output << "null";
					else output << record.correlationTargetSequence;
					output << ",\"matchedUnitCount\":" << record.matchedUnitCount
						<< ",\"expectedUnitCount\":" << record.expectedUnitCount
						<< ",\"ansiCodePage\":" << record.ansiCodePage
						<< ",\"correlationTruncated\":";
					appendBoolean(output, record.correlationTruncated);
				}
				if (record.imePositionAttempted)
				{
					output << ",\"imePositionAttempted\":true,\"imePositionContextAvailable\":";
					appendBoolean(output, record.imePositionContextAvailable);
					output << ",\"compositionPositionSucceeded\":";
					appendBoolean(output, record.compositionPositionSucceeded);
					output << ",\"candidatePositionSucceeded\":";
					appendBoolean(output, record.candidatePositionSucceeded);
					output << ",\"imeCaretPoint\":{\"x\":" << record.imeCaretPoint.x
						<< ",\"y\":" << record.imeCaretPoint.y
						<< "},\"imeLineRect\":{\"left\":" << record.imeLineRect.left
						<< ",\"top\":" << record.imeLineRect.top
						<< ",\"right\":" << record.imeLineRect.right
						<< ",\"bottom\":" << record.imeLineRect.bottom << '}';
				}
				appendExperimentTag(output, record);
				break;
			}
			case RecordType::WndProcLeave:
				output << ",\"wndProcCallId\":" << record.wndProcCallId
					<< ",\"callDepth\":" << record.callDepth
					<< ",\"messageId\":" << record.messageId
					<< ",\"forwarded\":";
				appendBoolean(output, record.forwarded);
				output << ",\"forwardResultHex\":";
				appendHex(output, record.forwardResult);
				output << ",\"captureHwndAfter\":";
				appendHandle(output, record.captureHwnd);
				if (state.geometryProbeEnabled && isGeometryMessage(record.messageId))
				{
					output << ",\"parentWndProcCallId\":";
					if (record.parentWndProcCallId == 0) output << "null";
					else output << record.parentWndProcCallId;
					output << ",\"rootWndProcCallId\":" << record.rootWndProcCallId;
					appendGeometrySnapshot(output, record);
				}
				if (record.messageId == WM_GETDLGCODE)
				{
					const auto flags = static_cast<UINT>(record.forwardResult);
					output << ",\"wantTab\":"; appendBoolean(output, (flags & DLGC_WANTTAB) != 0);
					output << ",\"wantArrows\":"; appendBoolean(output, (flags & DLGC_WANTARROWS) != 0);
					output << ",\"wantChars\":"; appendBoolean(output, (flags & DLGC_WANTCHARS) != 0);
					output << ",\"wantAllKeys\":"; appendBoolean(output, (flags & DLGC_WANTALLKEYS) != 0);
					output << ",\"wantMessage\":"; appendBoolean(output, (flags & DLGC_WANTMESSAGE) != 0);
				}
				appendExperimentTag(output, record);
				break;
			case RecordType::ImePosition:
			{
				const char* triggerName = messageName(record.messageId);
				output << ",\"parentSequence\":" << record.parentSequence
					<< ",\"wndProcCallId\":" << record.wndProcCallId
					<< ",\"triggerMessageId\":" << record.messageId
					<< ",\"triggerMessage\":\"" << (triggerName ? triggerName : "unknown")
					<< "\",\"imePositionAttempted\":true,\"imePositionContextAvailable\":";
				appendBoolean(output, record.imePositionContextAvailable);
				output << ",\"compositionPositionSucceeded\":";
				appendBoolean(output, record.compositionPositionSucceeded);
				output << ",\"candidatePositionSucceeded\":";
				appendBoolean(output, record.candidatePositionSucceeded);
				output << ",\"imeCaretPoint\":{\"x\":" << record.imeCaretPoint.x
					<< ",\"y\":" << record.imeCaretPoint.y
					<< "},\"imeLineRect\":{\"left\":" << record.imeLineRect.left
					<< ",\"top\":" << record.imeLineRect.top
					<< ",\"right\":" << record.imeLineRect.right
					<< ",\"bottom\":" << record.imeLineRect.bottom << '}';
				appendExperimentTag(output, record);
				break;
			}
			case RecordType::EasyXKey:
				output << ",\"batchId\":" << record.batchId
					<< ",\"indexInBatch\":" << record.indexInBatch
					<< ",\"easyxMessageType\":\""
					<< (record.easyxDown ? "WM_KEYDOWN" : "WM_KEYUP")
					<< "\",\"easyxVkCode\":" << static_cast<unsigned int>(record.easyxVkCode)
					<< ",\"easyxScanCode\":" << static_cast<unsigned int>(record.easyxScanCode)
					<< ",\"easyxDown\":"; appendBoolean(output, record.easyxDown);
				output << ",\"easyxExtended\":"; appendBoolean(output, record.easyxExtended);
				output << ",\"easyxPreviousDown\":"; appendBoolean(output, record.easyxPreviousDown);
				output << ",\"easyxCtrl\":null,\"easyxShift\":null,\"easyxAlt\":null"
					<< ",\"modifierSource\":\"not_available_in_ExMessage_key_variant\""
					<< ",\"easyxMessageTime\":null,\"existingDispatchDecision\":\"dispatch_existing\"";
				appendExperimentTag(output, record);
				break;
			case RecordType::EasyXMouse:
			{
				const char* knownName = messageName(record.messageId);
				output << ",\"batchId\":" << record.batchId
					<< ",\"indexInBatch\":" << record.indexInBatch
					<< ",\"easyxMessageId\":" << record.messageId
					<< ",\"easyxMessageType\":\"" << (knownName ? knownName : "unknown")
					<< "\",\"x\":" << record.easyxMouseX
					<< ",\"y\":" << record.easyxMouseY
					<< ",\"wheel\":" << record.easyxMouseWheel
					<< ",\"leftButton\":"; appendBoolean(output, record.easyxLeftButton);
				output << ",\"middleButton\":"; appendBoolean(output, record.easyxMiddleButton);
				output << ",\"rightButton\":"; appendBoolean(output, record.easyxRightButton);
				output << ",\"ctrl\":"; appendBoolean(output, record.easyxCtrl);
				output << ",\"shift\":"; appendBoolean(output, record.easyxShift);
				appendExperimentTag(output, record);
				break;
			}
			case RecordType::EasyXWindow:
			{
				const char* knownName = messageName(record.messageId);
				output << ",\"batchId\":" << record.batchId
					<< ",\"indexInBatch\":" << record.indexInBatch
					<< ",\"easyxMessageId\":" << record.messageId
					<< ",\"easyxMessageType\":";
				appendJsonString(output, knownName ? knownName : "unknown");
				output << ",\"wParamHex\":";
				appendHex(output, record.wParamValue);
				output << ",\"lParamHex\":";
				appendHex(output, record.lParamValue);
				if (record.parsedPointPresent)
				{
					output << ",\"parsedPoint\":";
					appendPoint(output, record.parsedPoint);
				}
				if (record.parsedSizePresent)
				{
					output << ",\"parsedSize\":{\"width\":" << record.parsedSize.cx
						<< ",\"height\":" << record.parsedSize.cy << '}';
				}
				if (record.messageId == WM_SIZE)
					output << ",\"sizeType\":" << record.sizeType;
				break;
			}
			case RecordType::GeometrySafePoint:
				output << ",\"batchId\":" << record.batchId
					<< ",\"latestGeometryRootCallId\":";
				if (record.rootWndProcCallId == 0) output << "null";
				else output << record.rootWndProcCallId;
				appendGeometrySnapshot(output, record);
				output << ",\"publishedAnchor\":";
				appendAnchor(output, record.publishedAnchor);
				output << ",\"lastPositionedAnchor\":";
				appendAnchor(output, record.positionedAnchor);
				output << ",\"imeRuntime\":{\"windowActive\":";
				appendBoolean(output, record.imeWindowActive);
				output << ",\"refreshPending\":";
				appendBoolean(output, record.imeRefreshPending);
				output << ",\"textInputEligible\":";
				appendBoolean(output, record.imeTextInputEligible);
				output << ",\"inputContextDetached\":";
				appendBoolean(output, record.imeInputContextDetached);
				output << ",\"positionRequests\":" << record.imePositionRequests
					<< ",\"compositionSuccesses\":" << record.imeCompositionSuccesses
					<< ",\"candidateSuccesses\":" << record.imeCandidateSuccesses
					<< ",\"snapshotReadFailures\":" << record.imeSnapshotReadFailures
					<< ",\"missingContextSkips\":" << record.imeMissingContextSkips
					<< ",\"cancelRequests\":" << record.imeCancelRequests
					<< ",\"cancelSuccesses\":" << record.imeCancelSuccesses
					<< ",\"cancelFailures\":" << record.imeCancelFailures
					<< ",\"inputContextGateTransitions\":"
					<< record.imeInputContextGateTransitions
					<< ",\"inputContextDetachSuccesses\":"
					<< record.imeInputContextDetachSuccesses
					<< ",\"inputContextDetachFailures\":"
					<< record.imeInputContextDetachFailures
					<< ",\"inputContextRestoreSuccesses\":"
					<< record.imeInputContextRestoreSuccesses
					<< ",\"inputContextRestoreFailures\":"
					<< record.imeInputContextRestoreFailures << '}';
				break;
			case RecordType::ImmQuery:
				output << ",\"parentSequence\":" << record.parentSequence
					<< ",\"textSource\":\"" << textSourceName(record.textSource)
					<< "\",\"textPayload\":\"redacted\",\"immQueryStatus\":\""
					<< immQueryStatusName(record.immQueryStatus)
					<< "\",\"immByteLength\":" << record.immByteLength
					<< ",\"utf16Length\":";
				if (record.textMetadataPresent) output << record.utf16Length; else output << "null";
				output << ",\"scalarCount\":";
				if (record.textMetadataPresent) output << record.scalarCount; else output << "null";
				output << ",\"isValidUtf16\":";
				if (record.textMetadataPresent) appendBoolean(output, record.validUtf16); else output << "null";
				output << ",\"sessionDigest\":";
				if (record.digestPresent) output << '"' << record.sessionDigest.data() << '"';
				else output << "null";
				output << ",\"digestStatus\":\"" << (record.digestPresent ? "success" : "unavailable")
					<< "\",\"ansiCodePage\":" << record.ansiCodePage
					<< ",\"ansiConversionUsedDefault\":";
				appendBoolean(output, record.ansiConversionUsedDefault);
				output << ",\"correlationTruncated\":";
				appendBoolean(output, record.correlationTruncated);
				break;
			case RecordType::EchoSummary:
				output << ",\"echoChannel\":\"" << echoChannelName(record.echoChannel)
					<< "\",\"correlationStatus\":\"" << correlationStatusName(record.correlationStatus)
					<< "\",\"correlationTargetSequence\":" << record.correlationTargetSequence
					<< ",\"matchedUnitCount\":" << record.matchedUnitCount
					<< ",\"expectedUnitCount\":" << record.expectedUnitCount
					<< ",\"ansiCodePage\":" << record.ansiCodePage
					<< ",\"correlationTruncated\":";
				appendBoolean(output, record.correlationTruncated);
				break;
			case RecordType::HostBatchBegin:
				output << ",\"batchId\":" << record.batchId
					<< ",\"batchKind\":\"" << (record.modalBatch ? "modal" : "main") << '"';
				break;
			case RecordType::HostBatchEnd:
				output << ",\"batchId\":" << record.batchId
					<< ",\"batchKind\":\"" << (record.modalBatch ? "modal" : "main")
					<< "\",\"processedCount\":" << record.processedCount
					<< ",\"stopped\":";
				appendBoolean(output, record.batchStopped);
				break;
			case RecordType::FlushBoundary:
				output << ",\"flushReason\":\"input_batch_end\",\"flushLevel\":\""
					<< (record.durableFlush ? "durable" : "buffer")
					<< "\",\"bufferedRecordCount\":" << record.bufferedRecordCount
					<< ",\"lostCount\":" << record.lostCountValue;
				break;
			case RecordType::RecordsLost:
				output << ",\"lossReason\":\""
					<< (record.lossFromContention ? "wndproc_lock_contention" : "buffer_overwrite")
					<< "\",\"lostFirstSequence\":";
				if (record.lossFromContention) output << "null"; else output << record.lostFirstSequence;
				output << ",\"lostLastSequence\":";
				if (record.lossFromContention) output << "null"; else output << record.lostLastSequence;
				output << ",\"lostCount\":" << record.lostCountValue;
				break;
			default:
				break;
			}

			output << "}\n";
			return output.str();
		}

		bool writeAll(HANDLE file, const std::string& text) noexcept
		{
			std::size_t offset = 0;
			while (offset < text.size())
			{
				const std::size_t remaining = text.size() - offset;
				const DWORD chunk = static_cast<DWORD>((std::min)(remaining,
					static_cast<std::size_t>((std::numeric_limits<DWORD>::max)())));
				DWORD written = 0;
				if (!WriteFile(file, text.data() + offset, chunk, &written, nullptr) || written == 0)
					return false;
				offset += written;
			}
			return true;
		}

		void captureSnapshotLocked(FlushSnapshot& snapshot, bool durable)
		{
			auto& state = probeState();
			snapshot.hwnd = state.attachedHwnd;
			snapshot.hwndInstanceId = state.hwndInstanceId;
			snapshot.durable = durable || state.durableRequested;
			snapshot.bufferIndex = state.activeRecordBuffer;
			snapshot.head = state.head;
			snapshot.count = state.count;
			snapshot.hardLimit = state.hardLimit;
			state.activeRecordBuffer ^= 1u;
			state.head = 0;
			state.count = 0;

			if (state.overflowLostCount > 0)
			{
				ProbeRecord loss = makeRecord(RecordType::RecordsLost, state.attachedHwnd);
				loss.lostFirstSequence = state.overflowLostFirstSequence;
				loss.lostLastSequence = state.overflowLostLastSequence;
				loss.lostCountValue = state.overflowLostCount;
				snapshot.trailingRecords.push_back(loss);
			}
			state.overflowLostFirstSequence = 0;
			state.overflowLostLastSequence = 0;
			state.overflowLostCount = 0;

			const std::uint64_t contentionLoss = state.contentionLostCount.exchange(
				0, std::memory_order_relaxed);
			if (contentionLoss > 0)
			{
				ProbeRecord loss = makeRecord(RecordType::RecordsLost, state.attachedHwnd);
				loss.lossFromContention = true;
				loss.lostCountValue = contentionLoss;
				snapshot.trailingRecords.push_back(loss);
			}
			state.durableRequested = false;
		}

		bool writeSnapshot(const FlushSnapshot& snapshot) noexcept
		{
			auto& state = probeState();
			if (state.file == INVALID_HANDLE_VALUE)
				return false;
			try
			{
				for (std::size_t index = 0; index < snapshot.count; ++index)
				{
					const ProbeRecord& record = state.recordBuffers[snapshot.bufferIndex]
						[(snapshot.head + index) % snapshot.hardLimit];
					if (!writeAll(state.file, serializeRecord(record)))
						return false;
				}
				for (const ProbeRecord& record : snapshot.trailingRecords)
				{
					if (!writeAll(state.file, serializeRecord(record)))
						return false;
				}
				if (snapshot.durable && !FlushFileBuffers(state.file))
					return false;
			}
			catch (...)
			{
				return false;
			}
			return true;
		}

		void disableFileOutput() noexcept
		{
			auto& state = probeState();
			AcquireSRWLockExclusive(&state.lock);
			state.enabled.store(false, std::memory_order_release);
			const HANDLE file = state.file;
			state.file = INVALID_HANDLE_VALUE;
			state.activeRecordBuffer = 0;
			state.head = 0;
			state.count = 0;
			ReleaseSRWLockExclusive(&state.lock);
			if (file != INVALID_HANDLE_VALUE)
				CloseHandle(file);
		}

		bool isImePositionTrigger(UINT message) noexcept
		{
			return message == WM_SETFOCUS || message == WM_IME_SETCONTEXT ||
				message == WM_IME_STARTCOMPOSITION || message == WM_IME_COMPOSITION;
		}

		ImePositionRequest prepareImePositionRequestLocked(
			ProbeState& state, HWND hwnd, const ProbeRecord& record) noexcept
		{
			ImePositionRequest request{};
			auto& rule = state.consumeRule;
			if (!rule.requested || rule.inputSource != ProbeInputSource::Manual ||
				!rule.imePositionConfigured || !isImePositionTrigger(record.messageId) ||
				imePositionApiCallActive)
			{
				return request;
			}

			request.active = true;
			request.hwnd = hwnd;
			request.hwndInstanceId = state.hwndInstanceId;
			request.parentSequence = record.sequence;
			request.wndProcCallId = record.wndProcCallId;
			request.compositionId = record.compositionId;
			request.triggerMessage = record.messageId;
			request.caretPoint = rule.imeCaretPoint;
			request.lineRect = rule.imeLineRect;
			++rule.imePositionAttemptCount;
			return request;
		}

		ImePositionResult executeImePositionRequest(const ImePositionRequest& request) noexcept
		{
			ImePositionResult result{};
			if (!request.active)
				return result;

			// IMM32 setter 可能同步重入窗口过程，因此调用链中不能持有探针状态锁，
			// 也不能递归执行定位。
			imePositionApiCallActive = true;
			const HIMC context = ImmGetContext(request.hwnd);
			result.contextAvailable = context != nullptr;
			if (!context)
			{
				imePositionApiCallActive = false;
				return result;
			}

			COMPOSITIONFORM composition{};
			composition.dwStyle = CFS_POINT;
			composition.ptCurrentPos = request.caretPoint;
			result.compositionSucceeded =
				ImmSetCompositionWindow(context, &composition) != FALSE;

			CANDIDATEFORM candidate{};
			candidate.dwIndex = 0;
			candidate.dwStyle = CFS_EXCLUDE;
			candidate.ptCurrentPos = request.caretPoint;
			candidate.rcArea = request.lineRect;
			result.candidateSucceeded = ImmSetCandidateWindow(context, &candidate) != FALSE;
			ImmReleaseContext(request.hwnd, context);
			imePositionApiCallActive = false;
			return result;
		}

		void recordImePositionResult(
			ProbeState& state, const ImePositionRequest& request,
			const ImePositionResult& result) noexcept
		{
			if (!request.active)
				return;
			ExclusiveStateLock lock(state.lock);
			if (!state.enabled.load(std::memory_order_relaxed) ||
				state.attachedHwnd != request.hwnd ||
				state.hwndInstanceId != request.hwndInstanceId)
			{
				return;
			}

			auto& rule = state.consumeRule;
			ProbeRecord record = makeRecord(RecordType::ImePosition, request.hwnd);
			record.parentSequence = request.parentSequence;
			record.wndProcCallId = request.wndProcCallId;
			record.compositionId = request.compositionId;
			record.messageId = request.triggerMessage;
			record.imePositionAttempted = true;
			record.imePositionContextAvailable = result.contextAvailable;
			record.compositionPositionSucceeded = result.compositionSucceeded;
			record.candidatePositionSucceeded = result.candidateSucceeded;
			record.imeCaretPoint = request.caretPoint;
			record.imeLineRect = request.lineRect;
			record.experimentTagged = rule.requested;
			pushRecord(record);

			if (result.compositionSucceeded && result.candidateSucceeded)
			{
				rule.imePositionReady = true;
				++rule.imePositionSuccessCount;
			}
		}

		void evaluateConsumeRuleLocked(ProbeState& state, UINT message, WPARAM wParam,
			ProbeRecord& record, InputProbeWndProcToken& token) noexcept
		{
			auto& rule = state.consumeRule;
			if (!rule.requested || message != rule.message ||
				static_cast<UINT>(wParam) != rule.virtualKey ||
				(rule.scanCodeSpecified && record.scanCode != rule.scanCode))
			{
				return;
			}

			record.experimentTagged = true;
			record.configurationValid = rule.configurationValid;
			record.baselineMatched = rule.baselineMatched;
			record.ruleEligible = rule.eligible;
			record.requestedAction = rule.requestedAction;
			record.actualAction = ProbeAction::ObserveAndForward;
			record.failOpenReason = rule.failOpenReason;
			token.experimentTagged = true;
			auto copyFailOpenToToken = [&]() noexcept {
				token.failOpenApplied = record.failOpenApplied;
				token.failOpenReason = static_cast<std::uint8_t>(record.failOpenReason);
			};

			if (!rule.eligible)
			{
				record.failOpenApplied = rule.requestedAction == ProbeAction::ObserveAndConsume;
				copyFailOpenToToken();
				return;
			}

			const bool imeConditionMet = rule.imeCondition == ImeStateCondition::Any ||
				(rule.imeCondition == ImeStateCondition::IdleOnly && !state.imeComposing) ||
				(rule.imeCondition == ImeStateCondition::ComposingOnly && state.imeComposing);
			if (!imeConditionMet)
			{
				record.failOpenReason = FailOpenReason::ImeConditionNotMet;
				record.failOpenApplied = rule.requestedAction == ProbeAction::ObserveAndConsume;
				copyFailOpenToToken();
				return;
			}

			if (rule.requestedAction == ProbeAction::ObserveAndConsume &&
				rule.imeCondition == ImeStateCondition::ComposingOnly && !rule.imePositionReady)
			{
				record.failOpenReason = FailOpenReason::ImePositionUnavailable;
				record.failOpenApplied = true;
				copyFailOpenToToken();
				return;
			}

			if (state.totalLostCount.load(std::memory_order_relaxed) > 0 ||
				state.overflowLostCount > 0 ||
				state.contentionLostCount.load(std::memory_order_relaxed) > 0)
			{
				record.failOpenReason = FailOpenReason::CaptureLossObserved;
				record.failOpenApplied = rule.requestedAction == ProbeAction::ObserveAndConsume;
				copyFailOpenToToken();
				return;
			}

			if (rule.matchCount >= rule.maxMatches)
			{
				record.failOpenReason = FailOpenReason::MatchLimitReached;
				record.failOpenApplied = rule.requestedAction == ProbeAction::ObserveAndConsume;
				copyFailOpenToToken();
				return;
			}

			++rule.matchCount;
			record.ruleMatched = true;
			record.matchOrdinal = rule.matchCount;
			record.failOpenReason = FailOpenReason::None;
			record.actualAction = rule.requestedAction;
			token.ruleMatched = true;
			token.matchOrdinal = rule.matchCount;
			token.observeAndConsume = rule.requestedAction == ProbeAction::ObserveAndConsume;
			copyFailOpenToToken();
		}

		void materializeBatchBegin(HWND hwnd, InputProbeBatchToken& batch) noexcept
		{
			if (!batch.active || batch.materialized)
				return;
			ProbeRecord record = makeRecord(RecordType::HostBatchBegin, hwnd);
			record.qpcTicks = batch.startedQpcTicks;
			record.batchId = batch.batchId;
			record.dispatchDepth = batch.dispatchDepth;
			record.modalDepth = batch.modalDepth;
			record.modalBatch = batch.modal;
			pushRecord(record);
			batch.materialized = true;
		}
	}

	void inputProbeAttach(HWND hwnd) noexcept
	{
		auto& state = probeState();
		if (!hwnd || state.enabled.load(std::memory_order_acquire) ||
			!environmentFlagEnabled(L"STELLARX_INPUT_PROBE"))
			return;
		ExclusiveStateLock fileLock(probeFileWriteLock);
		FlushSnapshot snapshot;
		try
		{
			snapshot.trailingRecords.reserve(4);
			ExclusiveStateLock lock(state.lock);
			if (state.enabled.load(std::memory_order_relaxed))
				return;

			state.processId = GetCurrentProcessId();
			state.hostThreadId = GetCurrentThreadId();
			state.uiThreadId = GetWindowThreadProcessId(hwnd, nullptr);
			state.ansiCodePage = ansiCodePageForLayout(GetKeyboardLayout(state.uiThreadId));
			state.consumeRule = loadConsumeRule(hwnd, state.ansiCodePage);
			state.recordPointerMessages =
				environmentFlagEnabled(L"STELLARX_INPUT_PROBE_RECORD_POINTER");
			state.geometryProbeEnabled =
				environmentFlagEnabled(L"STELLARX_STAGE6A_GEOMETRY_PROBE");
			state.schemaVersion = state.geometryProbeEnabled ? 4u :
				state.recordPointerMessages ? 3u :
				(state.consumeRule.requested ? 2u : 1u);
			state.attachedHwnd = hwnd;
			state.hwndInstanceId = 1;
			state.nextSequence.store(0, std::memory_order_relaxed);
			state.nextWndProcCallId = 0;
			state.nextBatchId = 0;
			state.nextCompositionId = 0;
			state.latestGeometryRootCallId = 0;
			state.activeCompositionId = 0;
			state.dispatchDepth = 0;
			state.modalDepth = 0;
			state.imeComposing = false;
			state.echo = {};
			state.geometrySafePointRecorded = false;
			state.lastGeometrySafePoint = {};
			state.activeRecordBuffer = 0;
			state.head = 0;
			state.count = 0;
			state.overflowLostFirstSequence = 0;
			state.overflowLostLastSequence = 0;
			state.overflowLostCount = 0;
			state.contentionLostCount.store(0, std::memory_order_relaxed);
			state.totalLostCount.store(0, std::memory_order_relaxed);
			state.durableRequested = false;
			state.recordNonInputMessages = environmentFlagEnabled(L"STELLARX_INPUT_PROBE_RECORD_NON_INPUT");
			state.hardLimit = (std::max)(static_cast<std::size_t>(1),
				(std::min)(environmentSize(L"STELLARX_INPUT_PROBE_HARD_LIMIT", DefaultHardLimit),
					MaxBufferedRecords));
			state.softLimit = (std::max)(static_cast<std::size_t>(1),
				(std::min)(environmentSize(L"STELLARX_INPUT_PROBE_SOFT_LIMIT", DefaultSoftLimit),
					state.hardLimit));

			LARGE_INTEGER frequency{};
			QueryPerformanceFrequency(&frequency);
			state.qpcFrequency = static_cast<std::uint64_t>(frequency.QuadPart);
			state.sessionKeyReady = makeSessionId(state.sessionId, state.sessionKey);
			SYSTEMTIME utc{};
			makeUtcTimestamp(state.startedAtUtc, utc);

			const std::filesystem::path directory = outputDirectory();
			if (directory.empty())
				return;
			std::error_code error;
			std::filesystem::create_directories(directory, error);
			if (error)
				return;

			wchar_t fileName[160]{};
			std::swprintf(fileName, std::size(fileName),
				L"input-probe-%04u%02u%02u-%02u%02u%02u-p%lu-%hs.jsonl",
				utc.wYear, utc.wMonth, utc.wDay, utc.wHour, utc.wMinute, utc.wSecond,
				static_cast<unsigned long>(state.processId), state.sessionId.data());
			const std::filesystem::path path = directory / fileName;
			state.file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (state.file == INVALID_HANDLE_VALUE)
				return;

			state.enabled.store(true, std::memory_order_release);
			pushRecord(makeRecord(RecordType::SessionStart, nullptr));
			ProbeRecord attach = makeRecord(RecordType::WindowAttach, hwnd);
			attach.windowUnicode = IsWindowUnicode(hwnd) != FALSE;
			attach.dpiForWindow = dpiForWindow(hwnd);
			attach.dpiAwareness = awarenessForWindow(hwnd);
			attach.ansiCodePage = state.ansiCodePage;
			pushRecord(attach);
			if (state.consumeRule.requested)
				pushRecord(makeRecord(RecordType::ExperimentConfig, hwnd));
			captureSnapshotLocked(snapshot, false);
		}
		catch (...)
		{
			disableFileOutput();
			return;
		}
		if (!writeSnapshot(snapshot))
			disableFileOutput();
	}

	void inputProbeDetach(HWND hwnd) noexcept
	{
		auto& state = probeState();
		if (!state.enabled.load(std::memory_order_acquire))
			return;
		ExclusiveStateLock fileLock(probeFileWriteLock);
		FlushSnapshot snapshot;
		try
		{
			snapshot.trailingRecords.reserve(4);
			ExclusiveStateLock lock(state.lock);
			if (!state.enabled.load(std::memory_order_relaxed) || hwnd != state.attachedHwnd)
				return;
			finishEchoCorrelationLocked(hwnd, CorrelationStatus::Expired);
			captureSnapshotLocked(snapshot, true);
			snapshot.trailingRecords.push_back(makeRecord(RecordType::WindowDetach, hwnd));
			ProbeRecord sessionEnd = makeRecord(RecordType::SessionEnd, nullptr);
			sessionEnd.lostCountValue = state.totalLostCount.load(std::memory_order_relaxed);
			snapshot.trailingRecords.push_back(sessionEnd);
			state.enabled.store(false, std::memory_order_release);
			state.attachedHwnd = nullptr;
			state.echo = {};
		}
		catch (...)
		{
			disableFileOutput();
			return;
		}
		if (!writeSnapshot(snapshot))
			disableFileOutput();
		else
		{
			ExclusiveStateLock lock(state.lock);
			if (state.file != INVALID_HANDLE_VALUE)
			{
				CloseHandle(state.file);
				state.file = INVALID_HANDLE_VALUE;
			}
		}
	}

	InputProbeWndProcToken inputProbeWndProcEnter(
		HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
	{
		auto& state = probeState();
		InputProbeWndProcToken token{};
		if (!state.enabled.load(std::memory_order_acquire))
			return token;

		token.depthCounted = true;
		token.callDepth = ++wndProcDepth;
		token.message = message;
		if (token.callDepth <= MaxTrackedWndProcDepth)
		{
			const std::size_t stackIndex = token.callDepth - 1u;
			wndProcCallStack[stackIndex] = 0;
			wndProcRootCallStack[stackIndex] = 0;
			if (stackIndex > 0)
				token.parentCallId = wndProcCallStack[stackIndex - 1u];
		}
		if (!shouldObserveMessage(message, state.recordNonInputMessages,
			state.recordPointerMessages, state.geometryProbeEnabled))
			return token;
		ImePositionRequest positionRequest{};
		{
			ExclusiveStateLock lock(state.lock);
			if (!state.enabled.load(std::memory_order_relaxed) || hwnd != state.attachedHwnd)
				return token;
			token.callId = ++state.nextWndProcCallId;
			token.rootCallId = token.parentCallId;
			if (token.callDepth <= MaxTrackedWndProcDepth)
			{
				const std::size_t stackIndex = token.callDepth - 1u;
				if (stackIndex > 0 && wndProcRootCallStack[stackIndex - 1u] != 0)
					token.rootCallId = wndProcRootCallStack[stackIndex - 1u];
				if (token.rootCallId == 0)
					token.rootCallId = token.callId;
				wndProcCallStack[stackIndex] = token.callId;
				wndProcRootCallStack[stackIndex] = token.rootCallId;
			}
			else if (token.rootCallId == 0)
			{
				token.rootCallId = token.callId;
			}
			if (state.geometryProbeEnabled && isGeometryMessage(message))
				state.latestGeometryRootCallId = token.rootCallId;

			ProbeRecord record = makeRecord(RecordType::WndProcEnter, hwnd);
			record.wndProcCallId = token.callId;
			record.parentWndProcCallId = token.parentCallId;
			record.rootWndProcCallId = token.rootCallId;
			record.callDepth = token.callDepth;
			record.messageId = message;
			record.messageCategory = classifyMessage(message);
			record.windowUnicode = IsWindowUnicode(hwnd) != FALSE;
			record.messageTime = static_cast<std::uint32_t>(GetMessageTime());
			record.messageTimeAvailable = true;
			record.captureHwnd = GetCapture();
		record.imeStateBefore = state.imeComposing;
		record.imeStateAfter = state.imeComposing;
			if (state.geometryProbeEnabled && isGeometryMessage(message))
			{
				parseGeometryMessage(record, message, wParam, lParam);
				captureWindowGeometry(hwnd, record);
			}

		if (isKeyMessage(message))
		{
			record.wParamPolicy = ValuePolicy::Raw;
			record.lParamPolicy = ValuePolicy::RawAndParsed;
			record.wParamValue = static_cast<std::uint64_t>(wParam);
			record.lParamValue = static_cast<std::uint64_t>(static_cast<ULONG_PTR>(lParam));
			const std::uint64_t bits = record.lParamValue;
			record.repeatCount = static_cast<std::uint16_t>(bits & 0xFFFFu);
			record.scanCode = static_cast<std::uint8_t>((bits >> 16) & 0xFFu);
			record.extended = ((bits >> 24) & 1u) != 0;
			record.altContext = ((bits >> 29) & 1u) != 0;
			record.wasDown = ((bits >> 30) & 1u) != 0;
			record.transition = ((bits >> 31) & 1u) != 0;
			record.systemKey = message == WM_SYSKEYDOWN || message == WM_SYSKEYUP;
			record.leftShift = keyStateDown(VK_LSHIFT);
			record.rightShift = keyStateDown(VK_RSHIFT);
			record.leftCtrl = keyStateDown(VK_LCONTROL);
			record.rightCtrl = keyStateDown(VK_RCONTROL);
			record.leftAlt = keyStateDown(VK_LMENU);
			record.rightAlt = keyStateDown(VK_RMENU);
			record.capsLock = toggleStateOn(VK_CAPITAL);
			record.numLock = toggleStateOn(VK_NUMLOCK);
			record.scrollLock = toggleStateOn(VK_SCROLL);
		}
		else if (isTextMessage(message))
		{
			record.wParamPolicy = ValuePolicy::Redacted;
			record.lParamPolicy = ValuePolicy::ParsedOnly;
			record.unicodeNoCharQuery = message == WM_UNICHAR && wParam == UNICODE_NOCHAR;
		}
		else if (message == WM_IME_COMPOSITION)
		{
			record.wParamPolicy = ValuePolicy::Redacted;
			record.lParamPolicy = ValuePolicy::RawAndParsed;
			record.lParamValue = static_cast<std::uint64_t>(static_cast<ULONG_PTR>(lParam));
			record.imeFlags = static_cast<std::uint32_t>(lParam);
		}
		else if (message == WM_IME_SETCONTEXT || message == WM_INPUTLANGCHANGEREQUEST ||
			message == WM_INPUTLANGCHANGE)
		{
			record.wParamPolicy = ValuePolicy::Raw;
			record.lParamPolicy = ValuePolicy::RawAndParsed;
			record.wParamValue = static_cast<std::uint64_t>(wParam);
			record.lParamValue = static_cast<std::uint64_t>(static_cast<ULONG_PTR>(lParam));
			if (message == WM_INPUTLANGCHANGEREQUEST)
			{
				record.inputLanguageSysCharset = (wParam & INPUTLANGCHANGE_SYSCHARSET) != 0;
				record.inputLanguageForward = (wParam & INPUTLANGCHANGE_FORWARD) != 0;
				record.inputLanguageBackward = (wParam & INPUTLANGCHANGE_BACKWARD) != 0;
				record.ansiCodePage = ansiCodePageForLayout(reinterpret_cast<HKL>(lParam));
			}
			else if (message == WM_INPUTLANGCHANGE)
			{
				record.ansiCodePage = ansiCodePageForLayout(reinterpret_cast<HKL>(lParam));
			}
		}
		else if (message == WM_GETDLGCODE)
		{
			record.wParamPolicy = ValuePolicy::Raw;
			record.lParamPolicy = ValuePolicy::ParsedOnly;
			record.wParamValue = static_cast<std::uint64_t>(wParam);
			record.lParamValue = lParam != 0 ? 1u : 0u;
		}
		else if (isPointerMessage(message))
		{
			record.wParamPolicy = ValuePolicy::RawAndParsed;
			record.lParamPolicy = ValuePolicy::RawAndParsed;
			record.wParamValue = static_cast<std::uint64_t>(wParam);
			record.lParamValue = static_cast<std::uint64_t>(static_cast<ULONG_PTR>(lParam));
			if (message == WM_CAPTURECHANGED)
				record.captureChangedTo = reinterpret_cast<HWND>(lParam);
		}
		else
		{
			// 几何消息的原始值和解析策略已在上方确定，这里不再覆盖。
			if (!(state.geometryProbeEnabled && isGeometryMessage(message)))
			{
				record.wParamPolicy = ValuePolicy::ParsedOnly;
				record.lParamPolicy = ValuePolicy::ParsedOnly;
			}
		}

		if (message == WM_IME_STARTCOMPOSITION)
		{
			state.activeCompositionId = ++state.nextCompositionId;
			state.imeComposing = true;
			record.compositionId = state.activeCompositionId;
			record.compositionAllocation = CompositionAllocation::StartComposition;
			record.imeStateAfter = true;
		}
		else if (message == WM_IME_COMPOSITION && !state.imeComposing)
		{
			state.activeCompositionId = ++state.nextCompositionId;
			state.imeComposing = true;
			record.compositionId = state.activeCompositionId;
			record.compositionAllocation = CompositionAllocation::LazyMissingStart;
			record.imeStateAfter = true;
		}
		else
		{
			record.compositionId = state.activeCompositionId;
		}
		if (message == WM_IME_ENDCOMPOSITION)
			record.imeStateAfter = false;
		positionRequest = prepareImePositionRequestLocked(state, hwnd, record);
		if (isTextMessage(message))
			correlateTextMessageLocked(record, wParam);
		evaluateConsumeRuleLocked(state, message, wParam, record, token);

		token.compositionId = record.compositionId;
		token.enterSequence = record.sequence;
		token.recorded = true;
		pushRecord(record);
		if (message == WM_IME_COMPOSITION && (record.imeFlags & GCS_RESULTSTR) != 0)
			captureImeResultLocked(hwnd, record);
		if (message == WM_IME_STARTCOMPOSITION && state.echo.active)
			finishEchoCorrelationLocked(hwnd, CorrelationStatus::Expired);

		if (message == WM_IME_ENDCOMPOSITION)
		{
			state.imeComposing = false;
			state.activeCompositionId = 0;
			state.durableRequested = true;
		}
		else if (message == WM_KILLFOCUS || message == WM_CLOSE || message == WM_DESTROY ||
			(message == WM_ACTIVATEAPP && wParam == FALSE))
		{
			finishEchoCorrelationLocked(hwnd, CorrelationStatus::Expired);
			state.durableRequested = true;
		}
		else if (message == WM_INPUTLANGCHANGE)
		{
			finishEchoCorrelationLocked(hwnd, CorrelationStatus::Expired);
			state.ansiCodePage = record.ansiCodePage;
		}
		}

		// enter 记录会先于任何重入 IMM32 操作可见，因此嵌套消息取得更晚的序号；
		// result 记录随后追加，并用 parentSequence 关联本次入口。
		const ImePositionResult positionResult = executeImePositionRequest(positionRequest);
		recordImePositionResult(state, positionRequest, positionResult);

		return token;
	}

	LRESULT inputProbeWndProcLeave(
		const InputProbeWndProcToken& token, LRESULT result, bool forwardedToOldProc) noexcept
	{
		auto& state = probeState();
		if (!token.depthCounted)
			return result;
		if (token.recorded && state.enabled.load(std::memory_order_acquire))
		{
			ExclusiveStateLock lock(state.lock);
			if (state.enabled.load(std::memory_order_relaxed))
			{
				ProbeRecord record = makeRecord(RecordType::WndProcLeave, state.attachedHwnd);
				record.wndProcCallId = token.callId;
				record.parentWndProcCallId = token.parentCallId;
				record.rootWndProcCallId = token.rootCallId;
				record.callDepth = token.callDepth;
				record.messageId = token.message;
				record.compositionId = token.compositionId;
				record.captureHwnd = GetCapture();
				record.forwarded = forwardedToOldProc;
				record.forwardResult = static_cast<std::uint64_t>(static_cast<ULONG_PTR>(result));
				record.experimentTagged = token.experimentTagged;
				record.ruleMatched = token.ruleMatched;
				record.matchOrdinal = token.matchOrdinal;
				record.requestedAction = state.consumeRule.requestedAction;
				record.actualAction = token.observeAndConsume
					? ProbeAction::ObserveAndConsume : ProbeAction::ObserveAndForward;
				record.failOpenApplied = token.failOpenApplied;
				record.failOpenReason = static_cast<FailOpenReason>(token.failOpenReason);
				if (state.geometryProbeEnabled && isGeometryMessage(token.message))
					captureWindowGeometry(state.attachedHwnd, record);
				pushRecord(record);
				if (token.message == WM_IME_ENDCOMPOSITION)
					finishEchoCorrelationLocked(state.attachedHwnd, CorrelationStatus::Expired);
			}
		}
		if (token.callDepth > 0 && token.callDepth <= MaxTrackedWndProcDepth)
		{
			const std::size_t stackIndex = token.callDepth - 1u;
			wndProcCallStack[stackIndex] = 0;
			wndProcRootCallStack[stackIndex] = 0;
		}
		if (wndProcDepth > 0)
			--wndProcDepth;
		return result;
	}

	InputProbeBatchToken inputProbeHostBatchBegin(HWND hwnd, bool modal) noexcept
	{
		auto& state = probeState();
		InputProbeBatchToken token{};
		if (!state.enabled.load(std::memory_order_acquire))
			return token;
		ExclusiveStateLock lock(state.lock);
		if (!state.enabled.load(std::memory_order_relaxed) || hwnd != state.attachedHwnd)
			return token;
		token.active = true;
		token.modal = modal;
		token.batchId = ++state.nextBatchId;
		token.startedQpcTicks = qpcNow();
		token.dispatchDepth = ++state.dispatchDepth;
		if (modal)
			++state.modalDepth;
		token.modalDepth = state.modalDepth;
		return token;
	}

	void inputProbeObserveEasyXMessage(
		HWND hwnd, InputProbeBatchToken& batch, const ExMessage& message) noexcept
	{
		auto& state = probeState();
		if (!batch.active || !state.enabled.load(std::memory_order_acquire))
			return;
		ExclusiveStateLock lock(state.lock);
		if (!state.enabled.load(std::memory_order_relaxed) || hwnd != state.attachedHwnd)
			return;
		materializeBatchBegin(hwnd, batch);
		const std::uint32_t indexInBatch = batch.nextMessageIndex++;
		const bool keyMessage = message.message == WM_KEYDOWN || message.message == WM_KEYUP;
		const bool pointerMessage = isPointerMessage(message.message);
		const bool geometryMessage = state.geometryProbeEnabled &&
			isGeometryMessage(message.message);
		if (!keyMessage && !(state.recordPointerMessages && pointerMessage) && !geometryMessage)
			return;

		ProbeRecord record = makeRecord(
			keyMessage ? RecordType::EasyXKey :
				(geometryMessage ? RecordType::EasyXWindow : RecordType::EasyXMouse), hwnd);
		record.batchId = batch.batchId;
		record.indexInBatch = indexInBatch;
		record.dispatchDepth = batch.dispatchDepth;
		record.modalDepth = batch.modalDepth;
		record.messageId = message.message;
		if (keyMessage)
		{
			record.easyxVkCode = message.vkcode;
			record.easyxScanCode = message.scancode;
			record.easyxDown = message.message == WM_KEYDOWN;
			record.easyxExtended = message.extended;
			record.easyxPreviousDown = message.prevdown;
		}
		else if (pointerMessage)
		{
			record.easyxMouseX = message.x;
			record.easyxMouseY = message.y;
			record.easyxMouseWheel = message.wheel;
			record.easyxLeftButton = message.lbutton;
			record.easyxMiddleButton = message.mbutton;
			record.easyxRightButton = message.rbutton;
			record.easyxCtrl = message.ctrl;
			record.easyxShift = message.shift;
		}
		else
		{
			record.wParamValue = static_cast<std::uint64_t>(message.wParam);
			record.lParamValue = static_cast<std::uint64_t>(
				static_cast<ULONG_PTR>(message.lParam));
			if (message.message == WM_MOVE)
			{
				record.parsedPointPresent = true;
				record.parsedPoint.x = static_cast<short>(LOWORD(message.lParam));
				record.parsedPoint.y = static_cast<short>(HIWORD(message.lParam));
			}
			else if (message.message == WM_SIZE || message.message == WM_DISPLAYCHANGE)
			{
				record.parsedSizePresent = true;
				record.parsedSize.cx = static_cast<LONG>(LOWORD(message.lParam));
				record.parsedSize.cy = static_cast<LONG>(HIWORD(message.lParam));
				if (message.message == WM_SIZE)
					record.sizeType = static_cast<std::uint32_t>(message.wParam);
			}
		}
		if (state.consumeRule.requested)
		{
			record.experimentTagged = true;
		}
		pushRecord(record);
	}

	void inputProbeHostBatchEnd(
		HWND hwnd, InputProbeBatchToken& batch, std::size_t processed, bool stopped) noexcept
	{
		auto& state = probeState();
		if (!batch.active)
			return;
		ExclusiveStateLock lock(state.lock);
		if (state.enabled.load(std::memory_order_relaxed) && hwnd == state.attachedHwnd && processed > 0)
		{
			materializeBatchBegin(hwnd, batch);
			ProbeRecord record = makeRecord(RecordType::HostBatchEnd, hwnd);
			record.batchId = batch.batchId;
			record.dispatchDepth = batch.dispatchDepth;
			record.modalDepth = batch.modalDepth;
			record.modalBatch = batch.modal;
			record.processedCount = static_cast<std::uint32_t>((std::min)(processed,
				static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)())));
			record.batchStopped = stopped;
			pushRecord(record);
		}
		if (batch.modal && state.modalDepth > 0)
			--state.modalDepth;
		if (state.dispatchDepth > 0)
			--state.dispatchDepth;
		batch.active = false;
	}

	bool inputProbeGeometrySafePointEnabled(HWND hwnd) noexcept
	{
		auto& state = probeState();
		if (!state.enabled.load(std::memory_order_acquire))
			return false;
		ExclusiveStateLock lock(state.lock);
		return state.enabled.load(std::memory_order_relaxed) &&
			state.geometryProbeEnabled && hwnd == state.attachedHwnd;
	}

	void inputProbeObserveGeometrySafePoint(
		HWND hwnd, std::uint64_t batchId,
		const ImeCaretAnchorRuntimeSnapshot& imeRuntime) noexcept
	{
		auto& state = probeState();
		if (!state.enabled.load(std::memory_order_acquire))
			return;
		ExclusiveStateLock lock(state.lock);
		if (!state.enabled.load(std::memory_order_relaxed) ||
			!state.geometryProbeEnabled || hwnd != state.attachedHwnd)
		{
			return;
		}

		ProbeRecord candidate{};
		captureWindowGeometry(hwnd, candidate);
		copyImeRuntimeToRecord(imeRuntime, candidate);
		if (state.geometrySafePointRecorded &&
			sameGeometrySafePoint(candidate, state.lastGeometrySafePoint))
		{
			return;
		}
		ProbeRecord record = makeRecord(RecordType::GeometrySafePoint, hwnd);
		record.batchId = batchId;
		record.rootWndProcCallId = state.latestGeometryRootCallId;
		captureWindowGeometry(hwnd, record);
		copyImeRuntimeToRecord(imeRuntime, record);
		pushRecord(record);
		state.lastGeometrySafePoint = record;
		state.geometrySafePointRecorded = true;
	}

	void inputProbeFlushAtSafePoint(HWND hwnd) noexcept
	{
		auto& state = probeState();
		if (!state.enabled.load(std::memory_order_acquire))
			return;
		ExclusiveStateLock fileLock(probeFileWriteLock);
		FlushSnapshot snapshot;
		try
		{
			snapshot.trailingRecords.reserve(4);
			{
				ExclusiveStateLock lock(state.lock);
				if (!state.enabled.load(std::memory_order_relaxed) || hwnd != state.attachedHwnd ||
					(state.count == 0 && state.contentionLostCount.load(std::memory_order_relaxed) == 0 &&
						state.overflowLostCount == 0))
					return;
				const std::uint64_t pendingLoss = state.overflowLostCount +
					state.contentionLostCount.load(std::memory_order_relaxed);
				ProbeRecord boundary = makeRecord(RecordType::FlushBoundary, hwnd);
				boundary.bufferedRecordCount = static_cast<std::uint32_t>((std::min)(state.count,
					static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)())));
				boundary.durableFlush = state.durableRequested;
				boundary.lostCountValue = pendingLoss;
				pushRecord(boundary);
				captureSnapshotLocked(snapshot, state.durableRequested);
			}
		}
		catch (...)
		{
			disableFileOutput();
			return;
		}
		if (!writeSnapshot(snapshot))
			disableFileOutput();
	}
}

#endif
