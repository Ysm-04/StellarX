#pragma once

#include "CoreTypes.h"
#include "InputEvents.h"
#include "SxTextEditModelInternal.h"

#include <string>
#include <string_view>

namespace StellarX::Internal
{
	// 控制器处理一条编辑命令后的语义结果。
	enum class TextEditCommandStatus
	{
		Unhandled,        ///< 该按键不属于文本编辑命令，可继续交给其它路由阶段。
		HandledUnchanged, ///< 命令已消费，但模型内容和选区没有变化。
		HandledChanged,   ///< 命令已消费，并修改了文本或选区。
		Rejected          ///< 命令属于编辑操作，但被模式、输入或容量规则拒绝。
	};

	// 本次命令涉及的剪贴板方向，用于测试失败路径而不记录实际文字。
	enum class TextClipboardOperation
	{
		None, ///< 未访问剪贴板。
		Read, ///< 从剪贴板读取并尝试粘贴。
		Write ///< 向剪贴板写入复制或剪切内容。
	};

	// 编辑命令的可断言结果，不包含文本内容。
	struct TextEditCommandResult
	{
		TextEditCommandStatus status = TextEditCommandStatus::Unhandled;
		TextClipboardOperation clipboardOperation = TextClipboardOperation::None;
		bool clipboardSucceeded = false;

		// 是否已由文本编辑层认领该命令。
		bool handled() const noexcept
		{
			return status != TextEditCommandStatus::Unhandled;
		}

		// 是否确实改变了文本或选区状态。
		bool changed() const noexcept
		{
			return status == TextEditCommandStatus::HandledChanged;
		}
	};

	// 剪贴板访问通过可注入接口隔离，使命令和模式测试不会读取、覆盖或记录
	// 用户的真实剪贴板内容。
	class TextClipboard
	{
	public:
		// 允许通过接口安全释放具体剪贴板适配器。
		virtual ~TextClipboard() = default;
		// 读取 Unicode 文本；失败时不得向 text 留下半成品。
		virtual bool readUnicode(std::u16string& text) = 0;
		// 写入 Unicode 文本，并以返回值显式报告系统剪贴板是否接受。
		virtual bool writeUnicode(std::u16string_view text) = 0;
	};

	// 使用 Win32 剪贴板 API 的默认适配器；owner 只用于打开剪贴板，不拥有窗口。
	class WindowsTextClipboard final : public TextClipboard
	{
	public:
		// 绑定一个借用的原生窗口句柄，调用方负责保证使用期间有效。
		explicit WindowsTextClipboard(HWND owner) noexcept : owner_(owner) {}
		// 从 CF_UNICODETEXT 读取并转换为 UTF-16。
		bool readUnicode(std::u16string& text) override;
		// 把 UTF-16 写入 CF_UNICODETEXT。
		bool writeUnicode(std::u16string_view text) override;

	private:
		HWND owner_ = nullptr;
	};

	// 两个规范化函数遇到非法 UTF-16 时都保持 destination 不变。普通文字事件
	// 丢弃 C0/DEL；单行粘贴还会把 CRLF、CR、LF 和 Tab 统一折叠为空格。
	bool normalizeTextInput(std::u16string_view source, std::u16string& destination);
	bool normalizeSingleLinePaste(std::u16string_view source,
		std::u16string& destination);

	// 纯编辑控制器，不拥有 HWND、焦点、绘制或路由状态，也不记录文本内容；
	// 单元测试通过它独立验证编辑命令与模式规则。
	class TextEditController final
	{
	public:
		// 返回当前编辑模型的只读视图。
		const TextEditModel& model() const noexcept { return model_; }
		// 仅供测试直接构造模型边界状态。
		TextEditModel& modelForTesting() noexcept { return model_; }
		// 查询当前 TextBox 工作模式。
		TextBoxmode mode() const noexcept { return mode_; }
		// 切换输入、只读或密码模式；不主动改写既有模型。
		void setMode(TextBoxmode mode) noexcept { mode_ = mode; }

		// 规范化并应用一次正式文字提交。
		TextEditCommandResult handleTextInput(std::u16string_view text);
		// 把单个 KeyEvent 解释为编辑或剪贴板命令。
		TextEditCommandResult handleKey(const KeyEvent& key, TextClipboard& clipboard);

	private:
		bool allowsInput() const noexcept;
		bool allowsCopy() const noexcept;
		bool allowsCut() const noexcept;
		std::u16string selectedText() const;

		TextEditModel model_;
		TextBoxmode mode_ = TextBoxmode::INPUT_MODE;
	};
}
