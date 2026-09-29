#include "StellarX.h"

#include <array>
#include <cmath>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

using namespace std;
using namespace StellarX;

/*
 * Demo：BMI 计算器
 * 展示内容：多输入框校验、数值计算、条件分支、消息框和带图片的窗口背景。
 * 来源：熊猫老师反馈并提供。
 * 注意：BMI 结果只用于演示程序逻辑，不构成医疗建议。
 */
namespace
{
// 读取完整、有限且大于 0 的浮点数，拒绝空文本、无穷大和混合字符。
bool parsePositiveNumber(const string& text, double& value)
{
    try
    {
        size_t parsedLength = 0;
        const double parsed = stod(text, &parsedLength);
        if (parsedLength != text.size() || !isfinite(parsed) || parsed <= 0.0)
            return false;
        value = parsed;
        return true;
    }
    catch (const exception&)
    {
        return false;
    }
}

// BMI 保留两位小数，避免直接使用 to_string 产生过多小数位。
string formatBmi(double value)
{
    ostringstream output;
    output << fixed << setprecision(2) << value;
    return output.str();
}

// 获取当前 EXE 所在目录，使用户从任意工作目录启动程序时仍能找到 assets。
filesystem::path findExecutableDirectory()
{
    array<char, MAX_PATH> executablePath{};
    const DWORD length = GetModuleFileNameA(nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));
    if (length == 0 || length >= executablePath.size())
        return {};
    return filesystem::path(string(executablePath.data(), length)).parent_path();
}

// 依次兼容“EXE 同目录”“源码目录”“当前工作目录”和“仓库根目录”四种启动方式。
string findBackgroundImage()
{
    const filesystem::path executableDirectory = findExecutableDirectory();
    const filesystem::path sourceDirectory = filesystem::path(__FILE__).parent_path();
    const array<filesystem::path, 4> candidates = {
        executableDirectory / "assets" / "BMI.jpg",
        sourceDirectory / "assets" / "BMI.jpg",
        filesystem::path("assets") / "BMI.jpg",
        filesystem::path("examples") / "bmi-calculator" / "assets" / "BMI.jpg"
    };

    for (const auto& candidate : candidates)
    {
        error_code error;
        if (filesystem::exists(candidate, error) && !error)
            return candidate.string();
    }
    return "";
}
}

int main()
{
    const COLORREF backgroundColor = RGB(135, 195, 235);
    Window win(800, 620, 0, backgroundColor, "BMI 计算器");

    // 输入区：身高使用米，体重使用千克。
    auto heightTitle = make_unique<Label>(55, 430, "身高（m）：", RGB(255, 255, 255), backgroundColor);
    auto weightTitle = make_unique<Label>(285, 430, "体重（kg）：", RGB(255, 255, 255), backgroundColor);
    heightTitle->textStyle.nHeight = 25;
    weightTitle->textStyle.nHeight = 25;

    auto heightInput = make_unique<TextBox>(175, 430, 85, 28, "", TextBoxmode::INPUT_MODE, ControlShape::ROUND_RECTANGLE);
    auto weightInput = make_unique<TextBox>(410, 430, 85, 28, "", TextBoxmode::INPUT_MODE, ControlShape::ROUND_RECTANGLE);
    auto calculate = make_unique<Button>(525, 430, 90, 30, "计算", ButtonMode::NORMAL, ControlShape::ROUND_RECTANGLE);

    // 结果区包含 BMI 数值、分类提示和固定免责声明。
    auto separator = make_unique<Label>(35, 495, "----------------------------- 计算结果 -----------------------------", RGB(255, 255, 255), backgroundColor);
    separator->textStyle.nHeight = 24;
    auto bmiResult = make_unique<Label>(55, 530, "BMI：等待计算", RGB(30, 30, 30), backgroundColor);
    bmiResult->textStyle.nHeight = 25;
    auto advice = make_unique<Label>(55, 565, "请输入有效的身高和体重", RGB(30, 30, 30), backgroundColor);
    advice->textStyle.nHeight = 21;
    auto disclaimer = make_unique<Label>(500, 595, "仅供程序演示，不构成医疗建议", RGB(80, 80, 80), backgroundColor);
    disclaimer->textStyle.nHeight = 16;

    TextBox* heightPtr = heightInput.get();
    TextBox* weightPtr = weightInput.get();
    Label* bmiPtr = bmiResult.get();
    Label* advicePtr = advice.get();

    // 计算前分别验证身高和体重，错误输入通过模态消息框提示。
    calculate->setOnClickListener([&win, heightPtr, weightPtr, bmiPtr, advicePtr]()
        {
            double height = 0.0;
            double weight = 0.0;
            if (!parsePositiveNumber(heightPtr->getText(), height) || height < 0.5 || height > 2.5)
            {
                MessageBox::showModal(win, "请输入 0.5 到 2.5 之间的身高（单位：米）。", "身高输入有误");
                return;
            }
            if (!parsePositiveNumber(weightPtr->getText(), weight) || weight < 10.0 || weight > 500.0)
            {
                MessageBox::showModal(win, "请输入 10 到 500 之间的体重（单位：千克）。", "体重输入有误");
                return;
            }

            // BMI = 体重(kg) / 身高(m)²。
            const double bmi = weight / (height * height);
            bmiPtr->setText("BMI：" + formatBmi(bmi));
            if (bmi < 18.5)
                advicePtr->setText("偏瘦：建议结合个人情况关注营养和生活习惯。");
            else if (bmi < 24.0)
                advicePtr->setText("正常：请继续保持均衡饮食和规律运动。");
            else if (bmi < 28.0)
                advicePtr->setText("超重：建议关注饮食、运动和长期体重变化。");
            else
                advicePtr->setText("肥胖：建议咨询专业医务人员获取个性化建议。");

        });

    // 将控件统一交给 Window 管理。
    win.addControl(move(heightTitle));
    win.addControl(move(weightTitle));
    win.addControl(move(heightInput));
    win.addControl(move(weightInput));
    win.addControl(move(calculate));
    win.addControl(move(separator));
    win.addControl(move(bmiResult));
    win.addControl(move(advice));
    win.addControl(move(disclaimer));

    // 缺少图片时仍允许 Demo 使用纯色背景运行，并明确提示资源问题。
    const string backgroundImage = findBackgroundImage();
    if (backgroundImage.empty())
    {
        win.draw();
        MessageBox::showAsync(win, "未找到 assets/BMI.jpg，当前使用纯色背景。", "资源提示");
    }
    else
    {
        win.draw(backgroundImage);
    }
    return win.runEventLoop();
}
