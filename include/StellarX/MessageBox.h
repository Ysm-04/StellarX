/*******************************************************************************
 * @类: MessageBox
 * @摘要: 消息框工厂类，提供简化的对话框创建接口
 * @描述:
 *     封装对话框的创建和显示逻辑，提供静态方法供快速调用。
 *     支持模态阻塞和非模态回调两种使用方式。
 *
 * @特性:
 *     - 静态方法调用，无需实例化
 *     - 自动处理模态和非模态的逻辑差异
 *     - 集成到窗口的对话框管理系统中
 *     - 提供去重机制防止重复对话框
 *
 * @使用场景: 快速创建标准消息框，减少样板代码
 * @所属框架: 星垣(StellarX) GUI框架
 * @作者: 我在人间做废物
 ******************************************************************************/
#pragma once
#include <string>
#include <functional>
#include "CoreTypes.h"
#include "Dialog.h"
#include "Window.h"

namespace StellarX
{
	class MessageBox
	{
	public:
		// 模态：阻塞直到关闭，返回结果
		static MessageBoxResult showModal(Window& wnd,
			const std::string& text,
			const std::string& caption = "提示",
			MessageBoxType type = MessageBoxType::OK);

		// 非模态：立即返回，通过回调异步获取结果
		static void showAsync(Window& wnd,
			const std::string& text,
			const std::string& caption = "提示",
			MessageBoxType type = MessageBoxType::OK,
			std::function<void(MessageBoxResult)> onResult = nullptr);
	};
} // namespace StellarX
