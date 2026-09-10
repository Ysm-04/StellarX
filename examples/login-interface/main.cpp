/**
 * Demo：登录界面
 * 展示内容：表单输入、密码模式、输入校验、Canvas 显隐和 TabControl。
 * 来源：项目作者提供。
 *
 * 这是纯界面示例，只检查账号和密码是否为空，不连接真实认证服务。
 */
#include "StellarX.h"

#include <array>
#include <filesystem>
#include <memory>
#include <string>

using namespace StellarX;
namespace filesystem = std::filesystem;

namespace
{
filesystem::path executableDirectory()
{
    std::array<char, MAX_PATH> buffer{};
    const DWORD length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size())
        return filesystem::current_path();

    return filesystem::path(std::string(buffer.data(), length)).parent_path();
}

// 优先从 EXE 旁读取资源，同时保留源码目录和仓库根目录两种开发期启动方式。
std::string findBackgroundImage()
{
    const std::array<filesystem::path, 4> candidates = {
        executableDirectory() / "assets" / "bk1.jpg",
        filesystem::path(__FILE__).parent_path() / "assets" / "bk1.jpg",
        filesystem::path("assets") / "bk1.jpg",
        filesystem::path("examples") / "login-interface" / "assets" / "bk1.jpg"
    };

    for (const auto& candidate : candidates)
    {
        std::error_code error;
        if (filesystem::is_regular_file(candidate, error))
            return candidate.string();
    }
    return {};
}
}

int main()
{
    Window window(1300, 800, 0, RGB(255, 255, 255), "StellarX 登录界面示例");
    const std::string backgroundImage = findBackgroundImage();

    auto loginCanvas = std::make_unique<Canvas>(0, 0, 1300, 800);
    auto* loginCanvasPtr = loginCanvas.get();
    loginCanvas->setCanvasfillMode(FillMode::Null);
    loginCanvas->setShape(ControlShape::B_RECTANGLE);
    loginCanvas->setLayoutMode(LayoutMode::AnchorToEdges);
    loginCanvas->setAnchor(Anchor::Bottom, Anchor::Top);

    auto heading = std::make_unique<Label>(90, 150, "欢迎使用餐馆记账管理系统");
    heading->setTextdisap(true);
    heading->textStyle.nHeight = 72;
    heading->textStyle.lpszFace = "华文行楷";

    auto accountLabel = std::make_unique<Label>(400, 300, "账号");
    accountLabel->setTextdisap(true);
    accountLabel->textStyle.nHeight = 42;

    auto passwordLabel = std::make_unique<Label>(400, 400, "密码");
    passwordLabel->setTextdisap(true);
    passwordLabel->textStyle.nHeight = 42;

    auto accountBox = std::make_unique<TextBox>(500, 295, 450, 50);
    auto* accountBoxPtr = accountBox.get();
    accountBox->setMaxCharLen(32);

    auto passwordBox = std::make_unique<TextBox>(500, 395, 450, 50);
    auto* passwordBoxPtr = passwordBox.get();
    passwordBox->setMaxCharLen(32);
    passwordBox->setMode(TextBoxmode::PASSWORD_MODE);

    auto adminButton = std::make_unique<Button>(350, 500, 300, 50, "管理员登录");
    auto* adminButtonPtr = adminButton.get();
    auto operatorButton = std::make_unique<Button>(750, 500, 300, 50, "操作员登录");
    auto* operatorButtonPtr = operatorButton.get();

    loginCanvas->addControl(std::move(heading));
    loginCanvas->addControl(std::move(accountLabel));
    loginCanvas->addControl(std::move(passwordLabel));
    loginCanvas->addControl(std::move(accountBox));
    loginCanvas->addControl(std::move(passwordBox));
    loginCanvas->addControl(std::move(adminButton));
    loginCanvas->addControl(std::move(operatorButton));

    auto tabs = std::make_unique<TabControl>(10, 10, 1280, 780);
    auto* tabsPtr = tabs.get();
    tabs->setIsVisible(false);
    tabs->setCanvasfillMode(FillMode::Null);
    tabs->setShape(ControlShape::ROUND_RECTANGLE);
    tabs->setTabPlacement(TabPlacement::Left);
    tabs->setTabBarHeight(100);

    auto page = std::make_unique<Canvas>(0, 0, 1290, 790);
    page->setCanvasfillMode(FillMode::Null);
    page->setShape(ControlShape::ROUND_RECTANGLE);
    auto loginResult = std::make_unique<Label>(190, 90, "");
    auto* loginResultPtr = loginResult.get();
    loginResult->setTextdisap(true);
    loginResult->textStyle.nHeight = 36;
    page->addControl(std::move(loginResult));
    tabs->add(std::make_pair(
        std::make_unique<Button>(0, 0, 100, 100, "点餐"),
        std::move(page)));

    // 两种身份共用同一套校验逻辑，只把成功提示中的身份作为参数传入。
    const auto tryLogin = [=, &window](const std::string& role)
        {
            const bool accountMissing = accountBoxPtr->getText().empty();
            const bool passwordMissing = passwordBoxPtr->getText().empty();

            accountBoxPtr->setTextBoxBk(accountMissing ? RGB(255, 220, 220) : RGB(255, 255, 255));
            passwordBoxPtr->setTextBoxBk(passwordMissing ? RGB(255, 220, 220) : RGB(255, 255, 255));

            if (accountMissing || passwordMissing)
            {
                MessageBox::showModal(window, "账号或密码不能为空！", "输入提示");
                return;
            }

            loginResultPtr->setText(role + "登录成功（界面演示）");
            loginCanvasPtr->setIsVisible(false);
            tabsPtr->setIsVisible(true);
            if (backgroundImage.empty())
                window.draw();
            else
                window.draw(backgroundImage);
        };

    adminButtonPtr->setOnClickListener([tryLogin]() { tryLogin("管理员"); });
    operatorButtonPtr->setOnClickListener([tryLogin]() { tryLogin("操作员"); });

    window.addControl(std::move(loginCanvas));
    window.addControl(std::move(tabs));
    if (backgroundImage.empty())
        window.draw();
    else
        window.draw(backgroundImage);

    return window.runEventLoop();
}
