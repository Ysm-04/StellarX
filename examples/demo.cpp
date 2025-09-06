/**
 * @file demo.cpp
 * @brief 一个简单的演示程序，展示 StellarX GUI 框架的基本用法。
 * @description 创建一个带有按钮的窗口，点击按钮会改变其文本。
 */

#include <StellarX/StellarX.h>
#include <iostream>

int main()
{
    // 创建一个窗口 (Windows平台)
    Window mainWindow(800, 600, NULL, RGB(240, 240, 240), "StellarX 演示程序");

    // 创建一个按钮
    auto myButton = std::make_unique<Button>(300, 250, 200, 80, "点击我！", StellarX::ButtonMode::NORMAL, StellarX::ControlShape::ROUND_RECTANGLE);

    // 为按钮点击事件设置一个回调函数
    myButton->setOnClickListener([&myButton]() {
        std::cout << "按钮被点击了！" << std::endl;
        // 点击后改变按钮文本作为视觉反馈
        static bool 已切换 = false;
        if (已切换) {
            myButton->setButtonText("点击我！");
        }
        else {
            myButton->setButtonText("被点过了！");
        }
        已切换 = !已切换;
        });

    // 将按钮添加到窗口
    mainWindow.addControl(std::move(myButton));

    // 绘制窗口并运行事件循环 (Windows消息循环)
    mainWindow.draw();
    mainWindow.runEventLoop();

    return 0;
}