/**
 * @file demo.cpp
 * @brief 一个简单的演示程序，展示 StellarX GUI 框架的基本用法。
 * @description 创建一个带有按钮的窗口，点击按钮会弹出对话框
 */

 // 只需包含这一个头文件即可使用所有功能
#include "StellarX.h"

// 程序入口点（请使用WinMain以获得更好的兼容性）
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// 1. 创建一个640x480的窗口，背景为白色，标题为"我的应用"
	Window mainWindow(640, 480, 0, RGB(255, 255, 255), "我的第一个星垣应用");

	// 2. 创建一个按钮 (使用智能指针管理)
	auto myButton = std::make_unique<Button>(
		250, 200, 140, 40, // x, y, 宽度, 高度
		"点击我",           // 按钮文本
		StellarX::ButtonMode::NORMAL,
		StellarX::ControlShape::ROUND_RECTANGLE
	);

	// 3. 为按钮设置点击事件（使用Lambda表达式）
	myButton->setOnClickListener([&mainWindow]() {
		// 使用消息框工厂创建模态对话框
		auto result = StellarX::MessageBox::showModal(
			mainWindow,
			"欢迎使用星垣GUI\r\n作者：我在人间做废物",
			"问候",
			StellarX::MessageBoxType::OKCancel
		);
		// 处理对话框结果
		if (result == StellarX::MessageBoxResult::OK) {
			// 用户点击了确定按钮
		}
		});



	// 4. (可选)设置按钮样式
	myButton->textStyle.nHeight = 20;
	myButton->textStyle.color = RGB(0, 0, 128); // 深蓝色文字
	myButton->setButtonBorder(RGB(0, 128, 255)); // 蓝色边框

	// 5. 将按钮添加到窗口
	mainWindow.addControl(std::move(myButton));

	// 6. 绘制窗口
	mainWindow.draw();

	// 7. 进入消息循环，等待用户交互
	mainWindow.runEventLoop();

	return 0;
}