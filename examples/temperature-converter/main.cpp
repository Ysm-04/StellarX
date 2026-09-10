#include "StellarX.h"

#include <cmath>
#include <exception>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

using namespace std;
using namespace StellarX;

/*
 * Demo：温度转换器
 * 展示内容：双向单位换算、浮点输入校验、结果格式化和异常消息提示。
 * 来源：熊猫老师反馈并提供。
 */
namespace
{
// 要求整段文本都能解析为有限浮点数，拒绝 NaN、无穷大和尾随字符。
bool parseTemperature(const string& text, double& value)
{
    try
    {
        size_t parsedLength = 0;
        const double parsed = stod(text, &parsedLength);
        if (parsedLength != text.size() || !isfinite(parsed))
            return false;
        value = parsed;
        return true;
    }
    catch (const exception&)
    {
        return false;
    }
}

// 最多保留两位小数，并移除无意义的末尾 0；同时消除显示上的“-0”。
string formatTemperature(double value)
{
    if (abs(value) < 0.0005)
        value = 0.0;

    ostringstream output;
    output << fixed << setprecision(2) << value;
    string text = output.str();
    while (!text.empty() && text.back() == '0')
        text.pop_back();
    if (!text.empty() && text.back() == '.')
        text.pop_back();
    return text;
}
}

int main()
{
    const COLORREF backgroundColor = RGB(135, 206, 235);
    Window win(680, 420, 0, backgroundColor, "温度转换器");

    // 两行输入区分别负责摄氏转华氏、华氏转摄氏。
    auto title = make_unique<Label>(225, 45, "温度转换器", RGB(255, 255, 255), backgroundColor);
    title->textStyle.nHeight = 42;

    auto celsiusTitle = make_unique<Label>(55, 145, "摄氏度（℃）：", RGB(30, 30, 30), backgroundColor);
    auto fahrenheitTitle = make_unique<Label>(55, 225, "华氏度（℉）：", RGB(30, 30, 30), backgroundColor);
    celsiusTitle->textStyle.nHeight = 25;
    fahrenheitTitle->textStyle.nHeight = 25;

    auto celsiusInput = make_unique<TextBox>(200, 145, 90, 28, "", TextBoxmode::INPUT_MODE, ControlShape::ROUND_RECTANGLE);
    auto fahrenheitInput = make_unique<TextBox>(200, 225, 90, 28, "", TextBoxmode::INPUT_MODE, ControlShape::ROUND_RECTANGLE);
    auto toFahrenheit = make_unique<Button>(315, 145, 110, 30, "转换为 ℉", ButtonMode::NORMAL, ControlShape::ROUND_RECTANGLE);
    auto toCelsius = make_unique<Button>(315, 225, 110, 30, "转换为 ℃", ButtonMode::NORMAL, ControlShape::ROUND_RECTANGLE);

    auto fahrenheitResult = make_unique<Label>(455, 145, "等待转换", RGB(255, 255, 255), backgroundColor);
    auto celsiusResult = make_unique<Label>(455, 225, "等待转换", RGB(255, 255, 255), backgroundColor);
    fahrenheitResult->textStyle.nHeight = 25;
    celsiusResult->textStyle.nHeight = 25;
    auto hint = make_unique<Label>(145, 335, "支持双向转换，并检查绝对零度和输入格式", RGB(70, 70, 70), backgroundColor);
    hint->textStyle.nHeight = 20;

    TextBox* celsiusInputPtr = celsiusInput.get();
    TextBox* fahrenheitInputPtr = fahrenheitInput.get();
    Label* fahrenheitResultPtr = fahrenheitResult.get();
    Label* celsiusResultPtr = celsiusResult.get();

    // 摄氏转华氏：F = C × 1.8 + 32，并检查绝对零度与计算溢出。
    toFahrenheit->setOnClickListener([&win, celsiusInputPtr, fahrenheitResultPtr]()
        {
            double celsius = 0.0;
            if (!parseTemperature(celsiusInputPtr->getText(), celsius))
            {
                MessageBox::showModal(win, "请输入有效的摄氏温度。", "输入有误");
                return;
            }
            if (celsius < -273.15)
            {
                MessageBox::showModal(win, "摄氏温度不能低于绝对零度 -273.15 ℃。", "超出范围");
                return;
            }

            const double fahrenheit = celsius * 1.8 + 32.0;
            if (!isfinite(fahrenheit))
            {
                MessageBox::showModal(win, "转换结果超出可表示范围。", "数值过大");
                return;
            }
            fahrenheitResultPtr->setText(formatTemperature(fahrenheit) + " ℉");
            fahrenheitResultPtr->draw();
        });

    // 华氏转摄氏：C = (F - 32) ÷ 1.8。
    toCelsius->setOnClickListener([&win, fahrenheitInputPtr, celsiusResultPtr]()
        {
            double fahrenheit = 0.0;
            if (!parseTemperature(fahrenheitInputPtr->getText(), fahrenheit))
            {
                MessageBox::showModal(win, "请输入有效的华氏温度。", "输入有误");
                return;
            }
            if (fahrenheit < -459.67)
            {
                MessageBox::showModal(win, "华氏温度不能低于绝对零度 -459.67 ℉。", "超出范围");
                return;
            }

            const double celsius = (fahrenheit - 32.0) / 1.8;
            if (!isfinite(celsius))
            {
                MessageBox::showModal(win, "转换结果超出可表示范围。", "数值过大");
                return;
            }
            celsiusResultPtr->setText(formatTemperature(celsius) + " ℃");
            celsiusResultPtr->draw();
        });

    // Window 接管控件后完成首次绘制，并进入事件循环。
    win.addControl(move(title));
    win.addControl(move(celsiusTitle));
    win.addControl(move(fahrenheitTitle));
    win.addControl(move(celsiusInput));
    win.addControl(move(fahrenheitInput));
    win.addControl(move(toFahrenheit));
    win.addControl(move(toCelsius));
    win.addControl(move(fahrenheitResult));
    win.addControl(move(celsiusResult));
    win.addControl(move(hint));

    win.draw();
    return win.runEventLoop();
}
