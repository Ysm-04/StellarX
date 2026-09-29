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
		// 模态：直接创建并显示对话框，阻塞到用户关闭后返回结果。
		// 调用期间会运行 Dialog 自己的模态事件循环。
		static MessageBoxResult showModal(Window& wnd,
			const std::string& text,
			const std::string& caption = "提示",
			MessageBoxType type = MessageBoxType::OK);

		// 非模态：创建后立即返回，由 Window 托管生命周期和绘制层级。
		// 若同一窗口中已存在相同 caption/text 的可见非模态对话框，则不会重复创建。
		// 用户关闭时通过 onResult 异步返回结果；回调允许排队修改宿主控件树。
		static void showAsync(Window& wnd,
			const std::string& text,
			const std::string& caption = "提示",
			MessageBoxType type = MessageBoxType::OK,
			std::function<void(MessageBoxResult)> onResult = nullptr);
	};
} // namespace StellarX
