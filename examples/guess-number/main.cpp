#include "StellarX.h"

#include <exception>
#include <memory>
#include <random>
#include <string>

using namespace std;
using namespace StellarX;

/*
 * Demo：猜数字游戏
 * 展示内容：TextBox 输入、Button 回调、Label 状态更新、随机数生成和输入校验。
 * 来源：熊猫老师反馈并提供。
 */
namespace
{
// 将文本完整解析为 1~100 的整数。
// parsedLength 用于拒绝“12abc”这类只解析了前半段的输入。
bool parseGuess(const string& text, int& value)
{
    try
    {
        size_t parsedLength = 0;
        const int parsed = stoi(text, &parsedLength);
        if (parsedLength != text.size() || parsed < 1 || parsed > 100)
            return false;
        value = parsed;
        return true;
    }
    catch (const exception&)
    {
        return false;
    }
}
}

int main()
{
    Window win(620, 420, 0, RGB(135, 206, 235), "猜数字游戏");

    // 构建标题、输入区、结果区、统计区和重置按钮。
    auto title = make_unique<Label>(205, 30, "猜数字游戏", RGB(255, 255, 255), RGB(135, 206, 235));
    title->textStyle.nHeight = 40;

    auto instruction = make_unique<Label>(75, 100, "请输入 1 到 100 之间的整数", RGB(30, 30, 30), RGB(135, 206, 235));
    instruction->textStyle.nHeight = 28;
    auto input = make_unique<TextBox>(185, 155, 150, 32, "", TextBoxmode::INPUT_MODE, ControlShape::ROUND_RECTANGLE);
    auto submit = make_unique<Button>(355, 155, 80, 32, "提交", ButtonMode::NORMAL, ControlShape::ROUND_RECTANGLE);

    auto result = make_unique<Label>(180, 225, "等待你的第一次猜测", RGB(255, 255, 255), RGB(135, 206, 235));
    result->textStyle.nHeight = 30;
    auto countTitle = make_unique<Label>(205, 280, "已猜次数：", RGB(30, 30, 30), RGB(135, 206, 235));
    countTitle->textStyle.nHeight = 24;
    auto countView = make_unique<Label>(325, 280, "0", RGB(255, 255, 255), RGB(135, 206, 235));
    countView->textStyle.nHeight = 24;
    auto reset = make_unique<Button>(250, 345, 120, 34, "再来一次", ButtonMode::NORMAL, ControlShape::B_ROUND_RECTANGLE);

    // 保存回调所需的控件地址，控件所有权稍后交给 Window。
    TextBox* inputPtr = input.get();
    Label* resultPtr = result.get();
    Label* countPtr = countView.get();

    // mt19937 比传统 rand() 更适合生成分布均匀、状态明确的随机数。
    random_device seed;
    mt19937 generator(seed());
    uniform_int_distribution<int> distribution(1, 100);
    int answer = distribution(generator);
    int guessCount = 0;

    // 提交按钮：先验证输入，再统计次数并更新“太大/太小/正确”的提示。
    submit->setOnClickListener([&win, &answer, &guessCount, inputPtr, resultPtr, countPtr]()
        {
            int guess = 0;
            if (!parseGuess(inputPtr->getText(), guess))
            {
                MessageBox::showModal(win, "请输入 1 到 100 之间的整数。", "输入有误");
                return;
            }

            ++guessCount;
            countPtr->setText(to_string(guessCount));
            if (guess > answer)
                resultPtr->setText("太大了，再试一次");
            else if (guess < answer)
                resultPtr->setText("太小了，再试一次");
            else
                resultPtr->setText("猜对了！点击“再来一次”继续");

            countPtr->draw();
            resultPtr->draw();
        });

    // “再来一次”必须修改外层 answer 和 guessCount，不能重新声明同名局部变量。
    // 同时清空输入框和上一轮结果，保证界面状态与业务状态一致。
    reset->setOnClickListener([&answer, &guessCount, &generator, &distribution, inputPtr, resultPtr, countPtr]()
        {
            answer = distribution(generator);
            guessCount = 0;
            inputPtr->setText("");
            resultPtr->setText("已生成新的数字");
            countPtr->setText("0");
            resultPtr->draw();
            countPtr->draw();
        });

    // Window 接管所有控件的生命周期。
    win.addControl(move(title));
    win.addControl(move(instruction));
    win.addControl(move(input));
    win.addControl(move(submit));
    win.addControl(move(result));
    win.addControl(move(countTitle));
    win.addControl(move(countView));
    win.addControl(move(reset));

    win.draw();
    return win.runEventLoop();
}
