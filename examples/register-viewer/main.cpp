/**
 * Demo：32 位寄存器查看器
 * 展示内容：Toggle Button、位运算、输入校验和多种数值格式联动。
 * 来源：项目作者提供。
 */
#include "StellarX.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

using namespace StellarX;

namespace
{
bool parseBitIndex(const std::string& text, int& value)
{
    try
    {
        size_t parsedLength = 0;
        const int parsed = std::stoi(text, &parsedLength);
        if (parsedLength != text.size() || parsed < 0 || parsed > 31)
            return false;
        value = parsed;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

uint32_t bitsToValue(const std::array<bool, 32>& bits)
{
    uint32_t value = 0;
    for (int bit = 0; bit < 32; ++bit)
    {
        if (bits[bit])
            value |= (1u << bit);
    }
    return value;
}

std::array<bool, 32> valueToBits(uint32_t value)
{
    std::array<bool, 32> bits{};
    for (int bit = 0; bit < 32; ++bit)
        bits[bit] = (value & (1u << bit)) != 0;
    return bits;
}

std::string formatBinary(const std::array<bool, 32>& bits)
{
    std::string result;
    result.reserve(39);
    for (int bit = 31; bit >= 0; --bit)
    {
        result.push_back(bits[bit] ? '1' : '0');
        if (bit != 0 && bit % 4 == 0)
            result.push_back('_');
    }
    return result;
}

std::string formatHex(uint32_t value)
{
    char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "%08X", value);
    return buffer;
}
}

int main()
{
    const COLORREF panelColor = RGB(202, 255, 255);
    const COLORREF buttonColor = RGB(171, 196, 220);
    const COLORREF accentColor = RGB(226, 116, 152);

    Window window(720, 550, 0, RGB(255, 255, 255), "StellarX 32 位寄存器查看器");
    bool signedMode = false;
    bool batchUpdatingBits = false;
    std::function<void()> refreshAfterBitToggle;

    auto selectionArea = std::make_unique<Canvas>(10, 10, 700, 170);
    selectionArea->setCanvasBkColor(panelColor);
    selectionArea->setShape(ControlShape::B_ROUND_RECTANGLE);

    auto selectionTitle = std::make_unique<Label>(18, -5, "32 位选择区");
    selectionTitle->setTextdisap(true);
    selectionArea->addControl(std::move(selectionTitle));

    // 直接按 bit 编号保存按钮指针，避免视觉顺序与数值位序相互转换。
    std::array<Button*, 32> bitButtons{};
    for (int bit = 31; bit >= 0; --bit)
    {
        const bool upperRow = bit >= 16;
        const int column = upperRow ? 31 - bit : 15 - bit;
        const int x = column * 35 + 27 + 28 * (column / 4);
        const int labelY = upperRow ? 24 : 92;
        const int buttonY = upperRow ? 54 : 122;

        auto bitLabel = std::make_unique<Label>(x - 2, labelY, "", RGB(208, 208, 208));
        std::ostringstream labelText;
        labelText << std::setw(2) << std::setfill('0') << bit;
        bitLabel->setText(labelText.str());
        bitLabel->setTextdisap(true);

        auto bitButton = std::make_unique<Button>(
            x, buttonY, 25, 30, "0", panelColor, buttonColor,
            ButtonMode::TOGGLE);
        auto* bitButtonPtr = bitButton.get();
        bitButtons[bit] = bitButtonPtr;
        bitButton->textStyle.color = accentColor;
        bitButton->setButtonShape(ControlShape::B_RECTANGLE);

        bitButton->setOnToggleOnListener(
            [bitButtonPtr, &batchUpdatingBits, &refreshAfterBitToggle]()
            {
                bitButtonPtr->setButtonText("1");
                if (!batchUpdatingBits && refreshAfterBitToggle)
                    refreshAfterBitToggle();
            });
        bitButton->setOnToggleOffListener(
            [bitButtonPtr, &batchUpdatingBits, &refreshAfterBitToggle]()
            {
                bitButtonPtr->setButtonText("0");
                if (!batchUpdatingBits && refreshAfterBitToggle)
                    refreshAfterBitToggle();
            });

        selectionArea->addControl(std::move(bitLabel));
        selectionArea->addControl(std::move(bitButton));
    }

    auto operationArea = std::make_unique<Canvas>(10, 190, 700, 90);
    operationArea->setCanvasBkColor(panelColor);
    operationArea->setShape(ControlShape::B_ROUND_RECTANGLE);
    auto operationTitle = std::make_unique<Label>(18, -5, "位运算区");
    operationTitle->setTextdisap(true);
    operationArea->addControl(std::move(operationTitle));

    auto invertLowLabel = std::make_unique<Label>(18, 27, "低位");
    invertLowLabel->setTextdisap(true);
    auto invertLowBox = std::make_unique<TextBox>(58, 25, 42, 30, "0");
    auto* invertLowBoxPtr = invertLowBox.get();
    invertLowBox->setMaxCharLen(2);
    auto invertHighLabel = std::make_unique<Label>(108, 27, "高位");
    invertHighLabel->setTextdisap(true);
    auto invertHighBox = std::make_unique<TextBox>(148, 25, 42, 30, "0");
    auto* invertHighBoxPtr = invertHighBox.get();
    invertHighBox->setMaxCharLen(2);
    auto invertButton = std::make_unique<Button>(198, 25, 70, 30, "取反", panelColor, buttonColor);
    auto* invertButtonPtr = invertButton.get();

    auto leftShiftButton = std::make_unique<Button>(285, 25, 70, 30, "左移", panelColor, buttonColor);
    auto* leftShiftButtonPtr = leftShiftButton.get();
    auto leftShiftBox = std::make_unique<TextBox>(362, 25, 42, 30, "0");
    auto* leftShiftBoxPtr = leftShiftBox.get();
    leftShiftBox->setMaxCharLen(2);
    auto leftShiftUnit = std::make_unique<Label>(410, 27, "位");
    leftShiftUnit->setTextdisap(true);

    auto rightShiftButton = std::make_unique<Button>(465, 25, 70, 30, "右移", panelColor, buttonColor);
    auto* rightShiftButtonPtr = rightShiftButton.get();
    auto rightShiftBox = std::make_unique<TextBox>(542, 25, 42, 30, "0");
    auto* rightShiftBoxPtr = rightShiftBox.get();
    rightShiftBox->setMaxCharLen(2);
    auto rightShiftUnit = std::make_unique<Label>(590, 27, "位");
    rightShiftUnit->setTextdisap(true);

    operationArea->addControl(std::move(invertLowLabel));
    operationArea->addControl(std::move(invertLowBox));
    operationArea->addControl(std::move(invertHighLabel));
    operationArea->addControl(std::move(invertHighBox));
    operationArea->addControl(std::move(invertButton));
    operationArea->addControl(std::move(leftShiftButton));
    operationArea->addControl(std::move(leftShiftBox));
    operationArea->addControl(std::move(leftShiftUnit));
    operationArea->addControl(std::move(rightShiftButton));
    operationArea->addControl(std::move(rightShiftBox));
    operationArea->addControl(std::move(rightShiftUnit));

    auto displayArea = std::make_unique<Canvas>(10, 290, 700, 160);
    displayArea->setCanvasBkColor(panelColor);
    displayArea->setShape(ControlShape::B_ROUND_RECTANGLE);
    auto displayTitle = std::make_unique<Label>(18, -5, "数值显示区");
    displayTitle->setTextdisap(true);

    auto hexLabel = std::make_unique<Label>(20, 30, "十六进制");
    hexLabel->setTextdisap(true);
    auto hexBox = std::make_unique<TextBox>(110, 28, 190, 30, "00000000", TextBoxmode::READONLY_MODE);
    auto* hexBoxPtr = hexBox.get();
    hexBox->setMaxCharLen(8);

    auto decimalLabel = std::make_unique<Label>(350, 30, "十进制");
    decimalLabel->setTextdisap(true);
    auto decimalBox = std::make_unique<TextBox>(430, 28, 220, 30, "0", TextBoxmode::READONLY_MODE);
    auto* decimalBoxPtr = decimalBox.get();
    decimalBox->setMaxCharLen(11);

    auto previousLabel = std::make_unique<Label>(20, 75, "上次值");
    previousLabel->setTextdisap(true);
    auto previousBox = std::make_unique<TextBox>(110, 73, 540, 30,
        "0000_0000_0000_0000_0000_0000_0000_0000", TextBoxmode::READONLY_MODE);
    auto* previousBoxPtr = previousBox.get();
    previousBox->setMaxCharLen(39);

    auto currentLabel = std::make_unique<Label>(20, 118, "本次值");
    currentLabel->setTextdisap(true);
    auto currentBox = std::make_unique<TextBox>(110, 116, 540, 30,
        "0000_0000_0000_0000_0000_0000_0000_0000", TextBoxmode::READONLY_MODE);
    auto* currentBoxPtr = currentBox.get();
    currentBox->setMaxCharLen(39);

    displayArea->addControl(std::move(displayTitle));
    displayArea->addControl(std::move(hexLabel));
    displayArea->addControl(std::move(hexBox));
    displayArea->addControl(std::move(decimalLabel));
    displayArea->addControl(std::move(decimalBox));
    displayArea->addControl(std::move(previousLabel));
    displayArea->addControl(std::move(previousBox));
    displayArea->addControl(std::move(currentLabel));
    displayArea->addControl(std::move(currentBox));

    auto configurationArea = std::make_unique<Canvas>(10, 460, 700, 60);
    configurationArea->setCanvasBkColor(panelColor);
    configurationArea->setShape(ControlShape::B_ROUND_RECTANGLE);
    auto configurationTitle = std::make_unique<Label>(18, -5, "配置区");
    configurationTitle->setTextdisap(true);

    auto signedToggle = std::make_unique<Button>(335, 20, 90, 28, "无符号", panelColor, buttonColor, ButtonMode::TOGGLE);
    auto* signedTogglePtr = signedToggle.get();
    auto clearButton = std::make_unique<Button>(440, 20, 100, 28, "一键置 0", panelColor, buttonColor);
    auto* clearButtonPtr = clearButton.get();
    auto fillButton = std::make_unique<Button>(555, 20, 100, 28, "一键置 1", panelColor, buttonColor);
    auto* fillButtonPtr = fillButton.get();

    configurationArea->addControl(std::move(configurationTitle));
    configurationArea->addControl(std::move(signedToggle));
    configurationArea->addControl(std::move(clearButton));
    configurationArea->addControl(std::move(fillButton));

    auto snapshotBits = [&bitButtons]()
        {
            std::array<bool, 32> bits{};
            for (int bit = 0; bit < 32; ++bit)
                bits[bit] = bitButtons[bit]->isClicked();
            return bits;
        };

    std::string displayedBinary = formatBinary(snapshotBits());
    auto refreshDisplays = [&](bool recordPrevious)
        {
            const auto bits = snapshotBits();
            const uint32_t value = bitsToValue(bits);
            const std::string newBinary = formatBinary(bits);

            if (recordPrevious && newBinary != displayedBinary)
                previousBoxPtr->setText(displayedBinary);
            displayedBinary = newBinary;
            currentBoxPtr->setText(newBinary);
            hexBoxPtr->setText(formatHex(value));
            decimalBoxPtr->setText(signedMode
                ? std::to_string(static_cast<int32_t>(value))
                : std::to_string(value));
        };

    // 程序批量修改按钮时暂停逐位刷新，完成后只更新一次显示区。
    auto applyBits = [&](const std::array<bool, 32>& targetBits)
        {
            batchUpdatingBits = true;
            for (int bit = 0; bit < 32; ++bit)
            {
                if (bitButtons[bit]->isClicked() != targetBits[bit])
                    bitButtons[bit]->setButtonClick(targetBits[bit]);
            }
            batchUpdatingBits = false;
            refreshDisplays(true);
        };

    refreshAfterBitToggle = [&refreshDisplays]() { refreshDisplays(true); };

    invertButtonPtr->setOnClickListener([&]()
        {
            int low = 0;
            int high = 0;
            if (!parseBitIndex(invertLowBoxPtr->getText(), low)
                || !parseBitIndex(invertHighBoxPtr->getText(), high)
                || low > high)
            {
                MessageBox::showModal(window, "请输入 0~31，且低位不得大于高位。", "取反范围错误");
                return;
            }

            auto bits = snapshotBits();
            for (int bit = low; bit <= high; ++bit)
                bits[bit] = !bits[bit];
            applyBits(bits);
        });

    leftShiftButtonPtr->setOnClickListener([&]()
        {
            int count = 0;
            if (!parseBitIndex(leftShiftBoxPtr->getText(), count))
            {
                MessageBox::showModal(window, "移位数必须是 0~31 的整数。", "左移参数错误");
                return;
            }
            applyBits(valueToBits(bitsToValue(snapshotBits()) << count));
        });

    rightShiftButtonPtr->setOnClickListener([&]()
        {
            int count = 0;
            if (!parseBitIndex(rightShiftBoxPtr->getText(), count))
            {
                MessageBox::showModal(window, "移位数必须是 0~31 的整数。", "右移参数错误");
                return;
            }
            applyBits(valueToBits(bitsToValue(snapshotBits()) >> count));
        });

    clearButtonPtr->setOnClickListener([&]() { applyBits(valueToBits(0)); });
    fillButtonPtr->setOnClickListener([&]() { applyBits(valueToBits(UINT32_MAX)); });

    signedTogglePtr->setOnToggleOnListener([&]()
        {
            signedMode = true;
            signedTogglePtr->setButtonText("有符号");
            refreshDisplays(false);
        });
    signedTogglePtr->setOnToggleOffListener([&]()
        {
            signedMode = false;
            signedTogglePtr->setButtonText("无符号");
            refreshDisplays(false);
        });
    signedTogglePtr->enableTooltip(true);
    signedTogglePtr->setTooltipTextsForToggle("当前为有符号模式", "当前为无符号模式");

    window.addControl(std::move(selectionArea));
    window.addControl(std::move(operationArea));
    window.addControl(std::move(displayArea));
    window.addControl(std::move(configurationArea));
    window.draw();
    return window.runEventLoop();
}
