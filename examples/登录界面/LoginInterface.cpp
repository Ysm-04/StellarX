// 本Demo基于 StellarX 构建，轻量级的 Windows GUI 框架。
#include"StellarX.h"

int main()
{
	Window win(1300, 800, NULL, RGB(255, 255, 0), "记账管理系统");

	/*********登录界面***********/
	//标签
	std::unique_ptr<Label>  logIn_label[3];
	Label* p[3];
	logIn_label[0] = std::make_unique<Label>(90, 150, "欢迎使用餐馆记账管理系统");
	logIn_label[1] = std::make_unique<Label>(400, 300, "账号");
	logIn_label[2] = std::make_unique<Label>(400, 400, "密码");
	p[0] = logIn_label[0].get();
	for (auto& log : logIn_label)
	{
		log->setTextdisap(true);
		log->setLayoutMode(StellarX::LayoutMode::AnchorToEdges);
		log->textStyle.lpszFace = "华文行楷";
	}
	logIn_label[0]->textStyle.nHeight = 100;
	logIn_label[0]->setAnchor(StellarX::Anchor::Top, StellarX::Anchor::NoAnchor);
	logIn_label[1]->textStyle.nHeight = 50;
	logIn_label[1]->setAnchor(StellarX::Anchor::Bottom, StellarX::Anchor::NoAnchor);
	logIn_label[2]->textStyle.nHeight = 50;
	logIn_label[2]->setAnchor(StellarX::Anchor::Bottom, StellarX::Anchor::NoAnchor);

	//输入框
	std::unique_ptr<TextBox>  logIn_textBox[2];
	TextBox* logIn_textBox_ptr[2];
	logIn_textBox[0] = std::make_unique<TextBox>(500, 295, 450, 50);
	logIn_textBox[1] = std::make_unique<TextBox>(500, 395, 450, 50);
	logIn_textBox_ptr[0] = logIn_textBox[0].get();
	logIn_textBox_ptr[1] = logIn_textBox[1].get();
	for (auto& tb : logIn_textBox)
	{
		tb->setLayoutMode(StellarX::LayoutMode::AnchorToEdges);
		tb->setAnchor(StellarX::Anchor::Bottom, StellarX::Anchor::NoAnchor);
		tb->setMaxCharLen(15);
	}
	//按钮
	std::unique_ptr<Button>  logIn_Button[2];
	Button* logIn_Button_ptr[2];
	logIn_Button[0] = std::make_unique<Button>(350, 500, 300, 50, "管理员登录");
	logIn_Button[1] = std::make_unique<Button>(750, 500, 300, 50, "操作员登录");
	logIn_Button_ptr[0] = logIn_Button[0].get();
	logIn_Button_ptr[1] = logIn_Button[1].get();
	for (auto& b : logIn_Button)
	{
		b->setLayoutMode(StellarX::LayoutMode::AnchorToEdges);
		b->setAnchor(StellarX::Anchor::Bottom, StellarX::Anchor::NoAnchor);
	}
	//log画布
	auto log_Canvas = std::make_unique<Canvas>(0, 0, 1300, 800);
	Canvas* log_Canvas_ptr = log_Canvas.get();
	log_Canvas_ptr->setCanvasfillMode(StellarX::FillMode::Null);
	log_Canvas_ptr->setShape(StellarX::ControlShape::B_RECTANGLE);
	log_Canvas_ptr->setLayoutMode(StellarX::LayoutMode::AnchorToEdges);
	log_Canvas_ptr->setAnchor(StellarX::Anchor::Bottom, StellarX::Anchor::Top);

	//将log界面控价加入logCanvas统一管理
	for (auto& b : logIn_Button)
		log_Canvas_ptr->addControl(std::move(b));
	for (auto& tb : logIn_textBox)
		log_Canvas_ptr->addControl(std::move(tb));
	for (auto& la : logIn_label)
		log_Canvas_ptr->addControl(std::move(la));

	/**************业务UI****************/

	auto tabControl = std::make_unique<TabControl>(10, 10, 1280, 780);
	auto tabControl_ptr = tabControl.get();
	tabControl_ptr->setIsVisible(false);
	tabControl_ptr->setCanvasfillMode(StellarX::FillMode::Null);
	tabControl_ptr->setShape(StellarX::ControlShape::ROUND_RECTANGLE);
	tabControl_ptr->setTabPlacement(StellarX::TabPlacement::Left);
	tabControl_ptr->setTabBarHeight(100);
	//添加页签及页
	auto tabP = std::make_pair(std::make_unique<Button>(0, 0, 100, 100, "点餐"), std::make_unique<Canvas>(0, 0, 1290, 790));
	tabP.second->setCanvasfillMode(StellarX::FillMode::Null);
	tabP.second->setShape(StellarX::ControlShape::ROUND_RECTANGLE);

	tabControl_ptr->add(std::move(tabP));

	/*------------login事件-------------*/
	//管理员登录按钮事件
	logIn_Button_ptr[0]->setOnClickListener([&p, &tabControl_ptr, &log_Canvas_ptr, &logIn_textBox_ptr, &win]()
		{
			if ("\0" == logIn_textBox_ptr[0]->getText() || "\0" == logIn_textBox_ptr[1]->getText())
			{
				if ("\0" == logIn_textBox_ptr[0]->getText())logIn_textBox_ptr[0]->setTextBoxBk(RGB(255, 0, 0));
				if ("\0" == logIn_textBox_ptr[1]->getText())logIn_textBox_ptr[1]->setTextBoxBk(RGB(255, 0, 0));
				std::cout << "\a";
				StellarX::MessageBox::showModal(win, "账号或密码不能为空！", "提示");
			}
			else
			{
				log_Canvas_ptr->setIsVisible(false);
				tabControl_ptr->setIsVisible(true);
				win.draw("image\\bk1.jpg");
			}
		});
	//操作员登录按钮事件
	logIn_Button_ptr[1]->setOnClickListener([&tabControl_ptr, &log_Canvas_ptr, &logIn_textBox_ptr, &win]()
		{
			if ("\0" == logIn_textBox_ptr[0]->getText() || "\0" == logIn_textBox_ptr[1]->getText())
			{
				if ("\0" == logIn_textBox_ptr[0]->getText())logIn_textBox_ptr[0]->setTextBoxBk(RGB(255, 0, 0));
				if ("\0" == logIn_textBox_ptr[1]->getText())logIn_textBox_ptr[1]->setTextBoxBk(RGB(255, 0, 0));
				std::cout << "\a";
				StellarX::MessageBox::showModal(win, "账号或密码不能为空！", "提示");
			}
			else
			{
				log_Canvas_ptr->setIsVisible(false);
				tabControl_ptr->setIsVisible(true);
				win.draw("image\\bk1.jpg");
			}
		});

	win.addControl(std::move(log_Canvas));
	win.addControl(std::move(tabControl));
	win.draw("image\\bk1.jpg");

	return win.runEventLoop();
}