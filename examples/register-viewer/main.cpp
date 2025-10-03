// 本工具基于 StellarX 构建，轻量级的 Windows GUI 框架。
#include"StellarX.h"
#include <sstream>
#include<iomanip>
#include<array>

auto   blackColor = RGB(202, 255, 255);
char initData[33] = "00000000000000000000000000000000";//初始数据
bool gSigned = false;      //是否为有符号数
void main()
{ 
	Window mainWindow(700,500,NULL,RGB(255,255,255), "寄存器查看工具 V1.0——我在人间做废物                         (同类工具定制：3150131407(Q / V))");
	
	//选择区控件
	auto selectionAreaLabel = std::make_unique<Label>(18, 0,"32位选择区");
	selectionAreaLabel->setTextdisap(true);
	std::vector<std::unique_ptr<Label>>selectionAreaButtonLabel;
	std::vector<std::unique_ptr<Button>>selectionAreaButton;
	std::vector<Button*>selectionAreaButton_ptr;
	auto  selectionArea = std::make_unique <Canvas>(10, 10, 680, 150);

	selectionArea->setCanvasBkColor(blackColor);
	selectionArea->setShape(StellarX::ControlShape::B_ROUND_RECTANGLE);
	
	for (int y = 0; y < 2; y ++)
	{
		std::ostringstream os;
		for (int x = 0; x <16; x++)
		{
			if (0 == y)
			{
				selectionAreaButtonLabel.push_back(std::make_unique<Label>(x * 35 + 40 + 28 * (x / 4), 26, "", RGB(208, 208, 208)));
				os << std::setw(2) << std::setfill('0') << 31 - x;
				selectionAreaButtonLabel.back()->setText(os.str());
				selectionAreaButtonLabel.back()->setTextdisap(true);

				selectionAreaButton.push_back(
					std::make_unique<Button>(x * 35 + 42 + 28 * (x / 4), 58,20,32,"0",
						blackColor,RGB(171, 196, 220),StellarX::ButtonMode::TOGGLE));
				selectionAreaButton.back()->textStyle.color = RGB(226, 116, 152);
				selectionAreaButton.back()->setButtonShape(StellarX::ControlShape::B_RECTANGLE);
				selectionAreaButton_ptr.push_back(selectionAreaButton.back().get());
				int k = 32 - x - 1;
				//选择区按钮被点击后在二进制0和1之间切换，并更新initData
				selectionAreaButton_ptr.back()->setOnToggleOnListener([k, btn = selectionAreaButton_ptr.back()]()
					{
						btn->setButtonText("1");
						initData[k] = '1';
					});
				selectionAreaButton_ptr.back()->setOnToggleOffListener([k, btn = selectionAreaButton_ptr.back()]()
					{
						btn->setButtonText("0");
						initData[k] = '0';
					});
			}
			else
			{
				selectionAreaButtonLabel.push_back(std::make_unique<Label>(x * 35 + 40 + 28 * (x / 4), 90, "", RGB(208, 208, 208)));
				os << std::setw(2) << std::setfill('0') << 15-x;
				selectionAreaButtonLabel.back()->setText(os.str());
				selectionAreaButtonLabel.back()->setTextdisap(true);

				selectionAreaButton.push_back(
					std::make_unique<Button>(x * 35 + 42 + 28 * (x / 4), 120, 20, 32, "0",
						blackColor, RGB(171, 196, 220), StellarX::ButtonMode::TOGGLE));
				selectionAreaButton.back()->textStyle.color = RGB(226, 116, 152);
				selectionAreaButton.back()->setButtonShape(StellarX::ControlShape::B_RECTANGLE);
				selectionAreaButton_ptr.push_back(selectionAreaButton.back().get());
				int k =15 - x;
				selectionAreaButton.back()->setOnToggleOnListener([k,btn = selectionAreaButton_ptr.back()]()
					{
						btn->setButtonText("1");
						initData[k] = '1';
					});
				selectionAreaButton.back()->setOnToggleOffListener([k, btn = selectionAreaButton_ptr.back()]()
					{
						btn->setButtonText("0");
						initData[k] = '0';
					});
			}
			os.str("");     
			os.clear();     

		}
	}
	selectionArea->addControl(std::move(selectionAreaLabel));
	for (auto& s : selectionAreaButton)
		selectionArea->addControl(std::move(s));
	for (auto& s : selectionAreaButtonLabel)
		selectionArea->addControl(std::move(s));
	//功能区控件
	//功能区总容器
	auto function = std::make_unique<Canvas>(0, 0, 0, 0);
	function->setCanvasfillMode(StellarX::FillMode::Null);
	
	auto bitInvert = std::make_unique<Canvas>(10,170,220,70);
	auto leftShift = std::make_unique<Canvas>(240, 170, 220, 70);
	auto rightShift = std::make_unique<Canvas>(470, 170, 220, 70);
	bitInvert->setCanvasBkColor(blackColor);
	bitInvert->setShape(StellarX::ControlShape::B_ROUND_RECTANGLE);
	leftShift->setCanvasBkColor(blackColor);
	leftShift->setShape(StellarX::ControlShape::B_ROUND_RECTANGLE);
	rightShift->setCanvasBkColor(blackColor);
	rightShift->setShape(StellarX::ControlShape::B_ROUND_RECTANGLE);
	
	
	auto bitInvertLabel = std::make_unique<Label>(18,160,"位取反");
	bitInvertLabel->setTextdisap(true);
	auto leftShiftLabel = std::make_unique<Label>(248, 160, "左移位");
	leftShiftLabel->setTextdisap(true);
	auto rightShiftLabel = std::make_unique<Label>(478, 160, "右移位");
	rightShiftLabel->setTextdisap(true);

	// ====== 公用小工具======
	auto clamp = [](int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); };
	auto toInt = [](const std::string& s, int def = 0) {
		try { return std::stoi(s); }
		catch (...) { return def; }
		};
	// bit号(31..0) -> selectionAreaButton下标(0..31)
	auto vecIndexFromBit = [](int bit) { return 31 - bit; };

	// 读取当前32位点击态
	auto snapshotBits = [&]() {
		std::array<bool, 32> a{};
		for (int b = 0; b < 32; ++b)
			a[b] = selectionAreaButton_ptr[vecIndexFromBit(b)]->isClicked();
		return a;
		};

	// 应用目标态：仅当不同才 setButtonClick
	auto applyBits = [&](const std::array<bool, 32>& a) {
		for (int b = 0; b < 32; ++b) {
			auto btn = selectionAreaButton_ptr[vecIndexFromBit(b)];
			if (btn->isClicked() != a[b]) btn->setButtonClick(a[b]);
		}
		};


	//取反区控件
	std::array<std::unique_ptr<Label>, 4> bitInvertFunctionLabel;
	bitInvertFunctionLabel[0] = std::make_unique<Label>(35, 180, "低位");
	bitInvertFunctionLabel[1] = std::make_unique<Label>(90, 180, "高位");
	bitInvertFunctionLabel[2] = std::make_unique<Label>(15, 198, "从");
	bitInvertFunctionLabel[3] = std::make_unique<Label>(75, 198, "到");

	std::array<std::unique_ptr<TextBox>, 2> bitInvertFunctionTextBox;
	bitInvertFunctionTextBox[0] = std::make_unique<TextBox>(35, 203, 35, 30, "0");
	bitInvertFunctionTextBox[1] = std::make_unique<TextBox>(95, 203, 35, 30, "0");
	auto invL = bitInvertFunctionTextBox[0].get();
	auto invH = bitInvertFunctionTextBox[1].get();
	auto bitInvertFunctionButton = std::make_unique<Button>(150,195, 70, 35, "位取反",
		blackColor, RGB(171, 196, 220));
	bitInvertFunctionButton->textStyle.color = RGB(226, 116, 152);
	bitInvertFunctionButton->setButtonShape(StellarX::ControlShape::B_RECTANGLE);
	auto bitInvertFunctionButton_ptr = bitInvertFunctionButton.get();
	
	bitInvert->addControl(std::move(bitInvertFunctionButton));
	bitInvert->addControl(std::move(bitInvertLabel));
	for (auto& b : bitInvertFunctionTextBox)
	{
		b->setMaxCharLen(3);
		b->textStyle.color = RGB(226, 116, 152);
		b->setTextBoxBk(RGB(244, 234, 142));
		b->setTextBoxshape(StellarX::ControlShape::B_RECTANGLE);
		bitInvert->addControl(std::move(b));
	}
	for (auto& b : bitInvertFunctionLabel)
	{
		b->setTextdisap(true);
		bitInvert->addControl(std::move(b));
	}
	//左移控件
	auto leftShiftFunctionLabel = std::make_unique<Label>(435, 198, "位");
	leftShiftFunctionLabel->setTextdisap(true);

	auto leftShiftFunctionTextBox = std::make_unique<TextBox>(325, 195, 100, 30, "0");
	leftShiftFunctionTextBox->setMaxCharLen(3);
	leftShiftFunctionTextBox->textStyle.color = RGB(226, 116, 152);
	leftShiftFunctionTextBox->setTextBoxBk(RGB(244, 234, 142));
	leftShiftFunctionTextBox->setTextBoxshape(StellarX::ControlShape::B_RECTANGLE);
	auto shlBox = leftShiftFunctionTextBox.get();
	auto leftShiftFunctionButton = std::make_unique<Button>(250, 195, 60, 30, "左移",
		blackColor, RGB(171, 196, 220));
	leftShiftFunctionButton->textStyle.color = RGB(226, 116, 152);
	leftShiftFunctionButton->setButtonShape(StellarX::ControlShape::B_RECTANGLE);
	auto leftShiftFunctionButton_ptr = leftShiftFunctionButton.get();
	
	
	leftShift->addControl(std::move(leftShiftFunctionButton));
	leftShift->addControl(std::move(leftShiftFunctionTextBox));

	leftShift->addControl(std::move(leftShiftLabel));
	leftShift->addControl(std::move(leftShiftFunctionLabel));

	//右移控件
	auto rightShiftFunctionLabel = std::make_unique<Label>(665, 198, "位");
	rightShiftFunctionLabel->setTextdisap(true);
	auto rightShiftFunctionTextBox = std::make_unique<TextBox>(555, 195, 100, 30, "0");
	rightShiftFunctionTextBox->setMaxCharLen(3);
	rightShiftFunctionTextBox->textStyle.color = RGB(226, 116, 152);
	rightShiftFunctionTextBox->setTextBoxBk(RGB(244, 234, 142));
	rightShiftFunctionTextBox->setTextBoxshape(StellarX::ControlShape::B_RECTANGLE);
	auto shrBox = rightShiftFunctionTextBox.get();

	auto rightShiftFunctionButton = std::make_unique<Button>(480, 195, 60, 30, "右移",
		blackColor, RGB(171, 196, 220));
	rightShiftFunctionButton->textStyle.color = RGB(226, 116, 152);
	rightShiftFunctionButton->setButtonShape(StellarX::ControlShape::B_RECTANGLE);
	auto rightShiftFunctionButton_ptr = rightShiftFunctionButton.get();
	
	

	rightShift->addControl(std::move(rightShiftFunctionButton));

	rightShift->addControl(std::move(rightShiftFunctionTextBox));
	rightShift->addControl(std::move(rightShiftLabel));
	rightShift->addControl(std::move(rightShiftFunctionLabel));
	
	function->addControl(std::move(bitInvert));
	function->addControl(std::move(leftShift));
	function->addControl(std::move(rightShift));

	//显示区控件
	//数值显示
	auto NumericalDisplayArea = std::make_unique<Canvas>(10, 255, 680, 70);
	NumericalDisplayArea->setCanvasBkColor(blackColor);
	NumericalDisplayArea->setShape(StellarX::ControlShape::B_ROUND_RECTANGLE);
	
	std::array<std::unique_ptr<Label>, 3> NumericalDisplayAreaLabel;
	NumericalDisplayAreaLabel[0] = std::make_unique<Label>(18, 245, "数值显示区");
	NumericalDisplayAreaLabel[1] = std::make_unique<Label>(20, 278, "十六进制");
	NumericalDisplayAreaLabel[2] = std::make_unique<Label>(330, 278, "十进制");

	std::array<std::unique_ptr<TextBox>, 2> NumericalDisplayAreaTextBox;
	NumericalDisplayAreaTextBox[0] = std::make_unique<TextBox>(110, 275, 200, 30, "0");
	NumericalDisplayAreaTextBox[1] = std::make_unique<TextBox>(400, 275, 200, 30, "0");
	auto hex = NumericalDisplayAreaTextBox[0].get();
	auto dec = NumericalDisplayAreaTextBox[1].get();


	for (auto& b : NumericalDisplayAreaLabel)
	{
		b->setTextdisap(true);
		NumericalDisplayArea->addControl(std::move(b));
	}
	for (auto& b : NumericalDisplayAreaTextBox)
	{
		b->setMaxCharLen(11);
		b->textStyle.color = RGB(255, 69, 0);
		b->setTextBoxBk(RGB(141, 141, 141));
		b->setTextBoxshape(StellarX::ControlShape::B_RECTANGLE);
		b->setMode(StellarX::TextBoxmode::READONLY_MODE);
		NumericalDisplayArea->addControl(std::move(b));
	}
	//二进制显示
	auto BinaryDisplayArea = std::make_unique<Canvas>(10, 335, 680, 110);
	BinaryDisplayArea->setCanvasBkColor(blackColor);
	BinaryDisplayArea->setShape(StellarX::ControlShape::B_ROUND_RECTANGLE);
	
	std::array<std::unique_ptr<Label>, 3> BinaryDisplayAreaLabel;
	BinaryDisplayAreaLabel[0] = std::make_unique<Label>(18, 325, "二进制显示区");
	BinaryDisplayAreaLabel[1] = std::make_unique<Label>(35, 353, "上次值");
	BinaryDisplayAreaLabel[2] = std::make_unique<Label>(35, 400, "本次值");

	std::array<std::unique_ptr<TextBox>, 2> BinaryDisplayAreaTextBox;
	BinaryDisplayAreaTextBox[0] = std::make_unique<TextBox>(110, 350, 520, 30, "0000_0000_0000_0000_0000_0000_0000_0000");
	BinaryDisplayAreaTextBox[1] = std::make_unique<TextBox>(110, 400, 520, 30, "0000_0000_0000_0000_0000_0000_0000_0000");
	auto Last = BinaryDisplayAreaTextBox[0].get();
	auto This = BinaryDisplayAreaTextBox[1].get();


	for (auto& b : BinaryDisplayAreaLabel)
	{
		b->setTextdisap(true);
		BinaryDisplayArea->addControl(std::move(b));
	}
	for (auto& b : BinaryDisplayAreaTextBox)
	{
		b->setMaxCharLen(40);
		b->textStyle.color = RGB(255, 69, 0);
		b->setTextBoxBk(RGB(141, 141, 141));
		b->setTextBoxshape(StellarX::ControlShape::B_RECTANGLE);
		b->setMode(StellarX::TextBoxmode::READONLY_MODE);
		BinaryDisplayArea->addControl(std::move(b));
	}

	// 由位图 bits 生成数值
	auto valueFromBits = [](const std::array<bool, 32>& bits) -> uint32_t {
		uint32_t v = 0;
		for (int b = 0; b < 32; ++b) if (bits[b]) v |= (1u << b);
		return v;
		};

	// 由位图 bits 生成 "0000_0000_..._0000"（MSB→LSB：31..0）
	auto binaryGroupedFromBits = [](const std::array<bool, 32>& bits) -> std::string {
		std::string s; s.reserve(39);
		for (int b = 31; b >= 0; --b) {
			s.push_back(bits[b] ? '1' : '0');
			if (b % 4 == 0 && b != 0) s.push_back('_');
		}
		return s;
		};

	// 用“目标位图 bits”刷新显示区
	auto refreshDisplaysWithBits = [&](const std::string& prevThis,
		const std::array<bool, 32>& bits,
		TextBox* hex, TextBox* dec, TextBox* Last, TextBox* This)
		{
			const uint32_t val = valueFromBits(bits);
			const int32_t  s = static_cast<int32_t>(val);
			char hexbuf[16];
			std::snprintf(hexbuf, sizeof(hexbuf), "%08X", val);

			hex->setText(hexbuf);                   // HEX（大写8位）
			dec->setText(gSigned ? std::to_string(s) : std::to_string(val));      // DEC
			Last->setText(prevThis);                // 上次值 ← 刷新前的本次值
			This->setText(binaryGroupedFromBits(bits)); // 本次值 ← 由“目标位图”生成
		};


	bitInvertFunctionButton_ptr->setOnClickListener([=, &snapshotBits, &applyBits, &refreshDisplaysWithBits]() {
		const std::string prevThis = This->getText();

		int L = clamp(toInt(invL->getText(), 0), 0, 31);
		int H = clamp(toInt(invH->getText(), 0), 0, 31);
		if (L > H) std::swap(L, H);

		auto cur = snapshotBits();
		for (int b = L; b <= H; ++b) cur[b] = !cur[b];

		applyBits(cur);                                   // 只改按钮点击态（触发位按钮自回调）
		refreshDisplaysWithBits(prevThis, cur, hex, dec, Last, This);
		});


	leftShiftFunctionButton_ptr->setOnClickListener([=, &snapshotBits, &applyBits, &refreshDisplaysWithBits]() {
		const std::string prevThis = This->getText();

		int n = clamp(toInt(shlBox->getText(), 0), 0, 31);
		auto cur = snapshotBits();
		std::array<bool, 32> nxt{};                         // 默认全 0

		// 逻辑左移：高位丢弃、低位补 0
		for (int b = 31; b >= 0; --b) nxt[b] = (b >= n) ? cur[b - n] : false;

		applyBits(nxt);
		refreshDisplaysWithBits(prevThis, nxt, hex, dec, Last, This);
		});

	rightShiftFunctionButton_ptr->setOnClickListener([=, &snapshotBits, &applyBits, &refreshDisplaysWithBits]() {
		const std::string prevThis = This->getText();

		int n = clamp(toInt(shrBox->getText(), 0), 0, 31);
		auto cur = snapshotBits();
		std::array<bool, 32> nxt{};

		// 逻辑右移：低位丢弃、高位补 0
		for (int b = 0; b < 32; ++b) nxt[b] = (b + n <= 31) ? cur[b + n] : false;

		applyBits(nxt);
		refreshDisplaysWithBits(prevThis, nxt, hex, dec, Last, This);
		});

	//配置区控件clearrectangle(10, 440, 690, 490);
	auto configuration = std::make_unique<Canvas>(10, 455, 680, 40);
	configuration->setCanvasBkColor(blackColor);
	configuration->setShape(StellarX::ControlShape::B_ROUND_RECTANGLE);
	
    	auto configurationLabel = std::make_unique<Label>(20, 445, "配置区");
	configurationLabel->setTextdisap(true);

	std::array<std::unique_ptr<Button>,2> configurationButton;
	configurationButton[0] = std::make_unique<Button>(450, 465, 80, 20, "一键置0",
		blackColor, RGB(171, 196, 220));
	configurationButton[0]->textStyle.color = RGB(226, 116, 152);
	configurationButton[0]->setButtonShape(StellarX::ControlShape::B_RECTANGLE);
	
	configurationButton[1] = std::make_unique<Button>(550, 465, 80, 20, "一键置1",
		blackColor, RGB(171, 196, 220));
	configurationButton[1]->textStyle.color = RGB(226, 116, 152);
	configurationButton[1]->setButtonShape(StellarX::ControlShape::B_RECTANGLE);
	configurationButton[0]->setOnClickListener(
		[&]() {
			for (auto& s : selectionAreaButton_ptr)
				if (s->isClicked()) s->setButtonClick(false);

			// 刷新显示：prevThis 用当前 This 文本
			const std::string prevThis = This->getText();
			auto cur = snapshotBits();
			{
				char hexbuf[16];
				uint32_t u = 0; for (int b = 0; b < 32; ++b) if (cur[b]) u |= (1u << b);
				int32_t  s = static_cast<int32_t>(u);
				std::snprintf(hexbuf, sizeof(hexbuf), "%08X", u);
				hex->setText(hexbuf);
				dec->setText(gSigned ? std::to_string(s) : std::to_string(u));
				Last->setText(prevThis);
				This->setText(binaryGroupedFromBits(cur));
			}
		});

	configurationButton[1]->setOnClickListener(
		[&]() {
			for (auto& s : selectionAreaButton_ptr)
				if (!s->isClicked()) s->setButtonClick(true);

			const std::string prevThis = This->getText();
			auto cur = snapshotBits();
			char hexbuf[16];
			uint32_t u = 0; for (int b = 0; b < 32; ++b) if (cur[b]) u |= (1u << b);
			int32_t  s = static_cast<int32_t>(u);
			std::snprintf(hexbuf, sizeof(hexbuf), "%08X", u);
			hex->setText(hexbuf);
			dec->setText(gSigned ? std::to_string(s) : std::to_string(u));
			Last->setText(prevThis);
			This->setText(binaryGroupedFromBits(cur));
		});


	auto signedToggle = std::make_unique<Button>(
		350, 465, 80, 20, "无符号",
		blackColor, RGB(171, 196, 220), StellarX::ButtonMode::TOGGLE);
	signedToggle->textStyle.color = RGB(226, 116, 152);
	signedToggle->setButtonShape(StellarX::ControlShape::B_RECTANGLE);
	auto* signedTogglePtr = signedToggle.get();
	
	signedTogglePtr->setOnToggleOnListener([&]() {
		gSigned = true;
		signedTogglePtr->setButtonText("有符号");

		// 立即刷新十进制显示：用当前位图算出新值，仅改 dec
		auto cur = snapshotBits();
		const uint32_t u = [&] { uint32_t v = 0; for (int b = 0; b < 32; ++b) if (cur[b]) v |= (1u << b); return v; }();
		const int32_t  s = static_cast<int32_t>(u);
		dec->setText(std::to_string(s));
		});
	signedTogglePtr->setOnToggleOffListener([&]() {
		gSigned = false;
		signedTogglePtr->setButtonText("无符号");

		auto cur = snapshotBits();
		const uint32_t u = [&] { uint32_t v = 0; for (int b = 0; b < 32; ++b) if (cur[b]) v |= (1u << b); return v; }();
		dec->setText(std::to_string(u));
		});


	configuration->addControl(std::move(configurationButton[0]));
	configuration->addControl(std::move(configurationButton[1]));
	configuration->addControl(std::move(signedToggle));
	configuration->addControl(std::move(configurationLabel));


	mainWindow.addControl(std::move(selectionArea));
	mainWindow.addControl(std::move(function));
	mainWindow.addControl(std::move(NumericalDisplayArea));
	mainWindow.addControl(std::move(BinaryDisplayArea));
	mainWindow.addControl(std::move(configuration));

	mainWindow.draw();
	return mainWindow.runEventLoop();
}
