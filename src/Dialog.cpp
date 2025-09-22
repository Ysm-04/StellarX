#include "Dialog.h"

Dialog::Dialog(Window& h,std::string text,std::string message, StellarX::MessageBoxType type, bool modal)
	: Canvas(),message(message), type(type), modal(modal), hWnd(h), titleText(text)
{
	initializeDialog();
}



Dialog::~Dialog()
{
}

void Dialog::draw()
{
	if(!isVisible)
	{
		// 如果对话框不可见且需要清理，执行清理
		if (pendingCleanup && !isCleaning)
		{
			performDelayedCleanup();
		}
		return;
	}
    // 如果需要初始化，则执行初始化
    if (needsInitialization && isVisible)
    {
        initDialogSize();
        needsInitialization = false;
    }

    if (dirty && isVisible)
    {
        // 保存当前绘图状态
        saveStyle();


        // 保存背景（仅在第一次绘制时）
        if (saveBkImage == nullptr) {
            saveBkX = x - BorderWidth;
            saveBkY = y - BorderWidth;
            saveBkWidth = width + 2 * BorderWidth;
            saveBkHeight = height + 2 * BorderWidth;
            saveBkImage = new IMAGE(saveBkWidth, saveBkHeight);
            getimage(saveBkImage, saveBkX, saveBkY, saveBkWidth, saveBkHeight);
        }

		Canvas::setBorderColor(this->borderColor);
		Canvas::setLinewidth(this->BorderWidth);
		Canvas::setCanvasBkColor(this->backgroundColor);
		Canvas::setShape(StellarX::ControlShape::ROUND_RECTANGLE);

		//设置所有控件为脏状态
		for(auto& c :this->controls)
			c->setDirty(true);

		Canvas::draw();

		//绘制消息文本
		settextcolor(textStyle.color);

		//设置字体样式
		settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.lpszFace,
			textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
			textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);
		
		
		int ty = y + closeButtonHeight + titleToTextMargin; // 文本起始Y坐标
		for (auto line:lines)
		{
			int tx = this->x + ((this->width - textwidth(line.c_str())) / 2); // 文本起始X坐标
			outtextxy(tx, ty, LPCTSTR(line.c_str()));
			ty = ty + textheight(LPCTSTR(line.c_str())) + 5; // 每行文本高度加5像素间距
		}

        // 恢复绘图状态
        restoreStyle();

        dirty = false;
    }
}


bool Dialog::handleEvent(const ExMessage& msg)
{
	bool consume = false;
	if (!isVisible)
	{
		if (pendingCleanup && !isCleaning)
		{
			performDelayedCleanup();
		}
		return false;
	}

	// 如果正在清理或标记为待清理，则不处理事件
	if (pendingCleanup || isCleaning)
		return false;
	// 模态对话框：点击对话框外部区域时，发出提示音(\a)并吞噬该事件，不允许操作背景内容。
	 if (modal && msg.message == WM_LBUTTONUP &&
		(msg.x < x || msg.x > x + width || msg.y < y || msg.y > y + height))
	{
		std::cout << "\a" << std::endl;
		// 模态对话框不允许点击外部区域
		return true;
	}

	// 将事件传递给子控件处理
	 if (!consume)
		 consume = Canvas::handleEvent(msg);

	// 每次事件处理后检查是否需要执行延迟清理
	if (pendingCleanup && !isCleaning)
		performDelayedCleanup();
	return consume;
}

void Dialog::SetTitle(const std::string& title)
{
	this->titleText = title;
	if (this->title)
	{
		this->title->setText(title);
	}
	dirty = true;
}

void Dialog::SetMessage(const std::string& message)
{
	this->message = message;
	splitMessageLines();
	getTextSize();
	dirty = true;
}

void Dialog::SetType(StellarX::MessageBoxType type)
{
	this->type = type;
	// 重新初始化按钮
	initButtons();
	dirty = true;
}

void Dialog::SetModal(bool modal)
{
	this->modal = modal;
}


void Dialog::SetResult(StellarX::MessageBoxResult result)
{
	this->result = result;
}

StellarX::MessageBoxResult Dialog::GetResult() const
{
	return this->result;
}

bool Dialog::getModal() const
{
	return modal;
}

bool Dialog::IsVisible() const
{
	return isVisible;
}

bool Dialog::model() const
{
	return modal;
}

void Dialog::Show()
{
	if (pendingCleanup)
		performDelayedCleanup();

	isVisible = true;
	dirty = true;
	needsInitialization = true;
	close = false;
	shouldClose = false;  

	if (modal)
	{
		// 模态对话框需要阻塞当前线程直到对话框关闭
		while (isVisible && !close)
		{
			// 处理消息
			ExMessage msg;
			if (peekmessage(&msg, EX_MOUSE | EX_KEY))
			{
				handleEvent(msg);

				// 检查是否需要关闭
				if (shouldClose)
				{
					Close();
					break;
				}
			}

			// 重绘
			if (dirty)
			{
				draw();
				FlushBatchDraw();
			}

			// 避免CPU占用过高
			Sleep(10);
		}

		// 模态对话框关闭后执行清理
		if (pendingCleanup && !isCleaning)
		{
			performDelayedCleanup();

		}
	}
	else
	{
		// 非模态对话框只需标记为可见，由主循环处理
		dirty = true;
	}
}



void Dialog::Close()
{
	if (!isVisible) return;

	isVisible = false;
	close = true;
	dirty = true;
	pendingCleanup = true;  // 只标记需要清理，不立即执行
	auto& c  =  hWnd.getControls();
	for(auto& control:c)
		control->setDirty(true);

	// 工厂模式下非模态触发回调 返回结果
	if (resultCallback&& !modal) 
		resultCallback(this->result);
	
	
}


void Dialog::initButtons()
{
	controls.clear();

	switch (this->type)
	{
	case StellarX::MessageBoxType::OK:  // 只有确定按钮
	{
		auto okbutton = createDialogButton((this->x + (this->width - (functionButtonWidth * buttonNum + buttonMargin * (buttonNum - 1))) / 2),
			((this->y + (this->height - buttonAreaHeight)) + (buttonAreaHeight - functionButtonHeight) / 2),
			"确定"
			);
		okbutton->setOnClickListener([this]()
			{
				this->SetResult(StellarX::MessageBoxResult::OK);
				this->Close(); });

		okbutton->textStyle = this->textStyle;
		
		this->addControl(std::move(okbutton));
	}
		break;
	case StellarX::MessageBoxType::OKCancel: // 确定和取消按钮
	{
		auto okButton = createDialogButton(
			(this->x + (this->width - (functionButtonWidth * buttonNum + buttonMargin * (buttonNum - 1))) / 2),
				((this->y + (this->height - buttonAreaHeight)) + (buttonAreaHeight - functionButtonHeight) / 2),
				"确定"
			);
		okButton->setOnClickListener([this]()
			{
				this->SetResult(StellarX::MessageBoxResult::OK);
				this->hWnd.dialogClose = true;
				 this->Close(); });

		auto cancelButton = createDialogButton(
			(okButton.get()->getButtonX() + okButton.get()->getButtonWidth() + buttonMargin),
			 okButton.get()->getButtonY(),
			"取消"
		);
		cancelButton->setOnClickListener([this]()
			{
				this->SetResult(StellarX::MessageBoxResult::Cancel);
				this->hWnd.dialogClose = true;
				 this->Close(); });
		
		okButton->textStyle = this->textStyle;
		cancelButton->textStyle = this->textStyle;

		this->addControl(std::move(okButton));
		this->addControl(std::move(cancelButton));
	}
		break;
	case StellarX::MessageBoxType::YesNo:  // 是和否按钮
	{
		auto yesButton = createDialogButton(
			(this->x + (this->width - (functionButtonWidth * buttonNum + buttonMargin * (buttonNum - 1))) / 2),
			((this->y + (this->height - buttonAreaHeight)) + (buttonAreaHeight - functionButtonHeight) / 2),
			"是"
		);
		yesButton->setOnClickListener([this]()
			{
				this->SetResult(StellarX::MessageBoxResult::Yes);
				this->hWnd.dialogClose = true;
				 this->Close(); });

		auto noButton = createDialogButton(
			(yesButton.get()->getButtonX() + yesButton.get()->getButtonWidth() + buttonMargin),
			yesButton.get()->getButtonY(),
			"否"
		);
		noButton->setOnClickListener([this]()
			{
				this->SetResult(StellarX::MessageBoxResult::No);
				this->hWnd.dialogClose = true;
				 this->Close(); });

		yesButton->textStyle = this->textStyle;
		noButton->textStyle = this->textStyle;

		this->addControl(std::move(yesButton));
		this->addControl(std::move(noButton));
	}
		break;
	case StellarX::MessageBoxType::YesNoCancel: // 是、否和取消按钮
	{
		auto yesButton = createDialogButton(
			(this->x + (this->width - (functionButtonWidth * buttonNum + buttonMargin * (buttonNum - 1))) / 2),
			((this->y + (this->height - buttonAreaHeight)) + (buttonAreaHeight - functionButtonHeight) / 2),
			"是"
		);
		yesButton->setOnClickListener([this]()
			{
				this->SetResult(StellarX::MessageBoxResult::Yes);
				this->hWnd.dialogClose = true;
				 this->Close(); });

		auto noButton = createDialogButton(
			yesButton.get()->getButtonX() + yesButton.get()->getButtonWidth() + buttonMargin,
			yesButton.get()->getButtonY(), 
			"否"
		);
		noButton->setOnClickListener([this]()
			{
				this->SetResult(StellarX::MessageBoxResult::No);
				this->hWnd.dialogClose = true;
				 this->Close(); });

		auto cancelButton = createDialogButton(
			noButton.get()->getButtonX() + noButton.get()->getButtonWidth() + buttonMargin,
			noButton.get()->getButtonY(),
			"取消"
		);
		cancelButton->setOnClickListener([this]()
			{
				this->SetResult(StellarX::MessageBoxResult::Cancel);
				this->hWnd.dialogClose = true;
				 this->Close(); });

		yesButton->textStyle = this->textStyle;
		noButton->textStyle = this->textStyle;
		cancelButton->textStyle = this->textStyle;

		
		this->addControl(std::move(yesButton));
		this->addControl(std::move(noButton));
		this->addControl(std::move(cancelButton));
	}
	    break;
	case StellarX::MessageBoxType::RetryCancel: // 重试和取消按钮
	{
		auto retryButton = createDialogButton(
			(this->x + (this->width - (functionButtonWidth * buttonNum + buttonMargin * (buttonNum - 1))) / 2),
			((this->y + (this->height - buttonAreaHeight)) + (buttonAreaHeight - functionButtonHeight) / 2),
			"重试"
		);
		retryButton->setOnClickListener([this]()
			{
				this->SetResult(StellarX::MessageBoxResult::Retry);
				this->hWnd.dialogClose = true;
				 this->Close(); });
		
		auto cancelButton = createDialogButton(
			retryButton.get()->getButtonX() + retryButton.get()->getButtonWidth() + buttonMargin,
			retryButton.get()->getButtonY(), 
			"取消"
		);
		cancelButton->setOnClickListener([this]()
			{
				this->SetResult(StellarX::MessageBoxResult::Cancel);
				this->hWnd.dialogClose = true;
				 this->Close(); });

		retryButton->textStyle = this->textStyle;
		cancelButton->textStyle = this->textStyle;

		this->addControl(std::move(retryButton));
		this->addControl(std::move(cancelButton));
	}
	    break;
	case StellarX::MessageBoxType::AbortRetryIgnore: // 中止、重试和忽略按钮
	{
		auto abortButton = createDialogButton(
			(this->x + (this->width - (functionButtonWidth * buttonNum + buttonMargin* (buttonNum-1))) / 2),
			((this->y + (this->height - buttonAreaHeight)) + (buttonAreaHeight - functionButtonHeight) / 2),
			"中止"
		);
		abortButton->setOnClickListener([this]()
			{
				this->SetResult(StellarX::MessageBoxResult::Abort);
				this->hWnd.dialogClose = true;
				 this->Close();
			});
		auto retryButton = createDialogButton(
			abortButton.get()->getButtonX() + abortButton.get()->getButtonWidth() + buttonMargin,
			abortButton.get()->getButtonY(), 
			"重试"
		);
		retryButton->setOnClickListener([this]()
			{
				this->SetResult(StellarX::MessageBoxResult::Retry);
				this->hWnd.dialogClose = true;
				 this->Close();
			});
		auto ignoreButton = createDialogButton(
			retryButton.get()->getButtonX() + retryButton.get()->getButtonWidth() + buttonMargin,
			retryButton.get()->getButtonY(),
			"忽略"
		);
		ignoreButton.get()->setOnClickListener([this]()
			{
				this->SetResult(StellarX::MessageBoxResult::Ignore);
				this->hWnd.dialogClose = true;
				 this->Close();
			});

		abortButton->textStyle = this->textStyle;
		retryButton->textStyle = this->textStyle;
		ignoreButton->textStyle = this->textStyle;

		this->addControl(std::move(abortButton));
		this->addControl(std::move(retryButton));
		this->addControl(std::move(ignoreButton));
	}
		break;
	}
}

void Dialog::initCloseButton()
{
	//初始化关闭按钮
	auto but = std::make_unique<Button>
		(
			(this->x + this->width - closeButtonWidth) - 3, (this->y+3), closeButtonWidth-1, closeButtonHeight,
			"×",           // 按钮文本
			RGB(255, 0, 0),      // 按钮被点击颜色
			this->canvasBkClor, // 按钮背景颜色
			RGB(255, 0, 0),	  // 按钮被悬停颜色
			StellarX::ButtonMode::NORMAL,
			StellarX::ControlShape::B_RECTANGLE
		);
	but.get()->setButtonFalseColor(this->backgroundColor);
	but->setOnClickListener([this]() {
		this->SetResult(StellarX::MessageBoxResult::Cancel);
		this->hWnd.dialogClose = true;
		this->Close(); });
	
	this->closeButton = but.get();
	this->addControl(std::move(but));
}

void Dialog::initTitle()
{
	this->title = std::make_unique<Label>(this->x+5,this->y+5,titleText,textStyle.color);
	title->setTextdisap(true);
	title->textStyle = this->textStyle;

	this->addControl(std::move(title));
}

void Dialog::splitMessageLines()
{
	lines.clear(); // 先清空现有的行

	std::string currentLine;
	for (size_t i = 0; i < message.length(); i++) {
		// 处理 换行符 \r\n  \n  \r
		if (i + 1 < message.length() && (message[i] == '\r' || message[i] == '\n')||(message[i] == '\r' && message[i+1] == '\n'))
		{
			if (!currentLine.empty()) {
				lines.push_back(currentLine);
				currentLine.clear();
			}
			
			if (message[i] == '\r' && message[i + 1] == '\n')
				i++;
			continue;
		}

		currentLine += message[i];
	}

	// 添加最后一行（如果有内容）
	if (!currentLine.empty()) {
		lines.push_back(currentLine);
	}

	// 如果消息为空，至少添加一个空行
	if (lines.empty()) {
		lines.push_back("");
	}
}

void Dialog::getTextSize()
{
	saveStyle();
	settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.lpszFace,
		textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
		textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);
	for (auto text : lines)
	{
		int w = textwidth(LPCTSTR(text.c_str()));
		int h = textheight(LPCTSTR(text.c_str()));
		if (this->textHeight < h)
			this->textHeight = h;
		if (this->textWidth < w)
			this->textWidth = w;
	}
	restoreStyle();
}
// 计算逻辑：对话框宽度取【文本区域最大宽度】和【按钮区域总宽度】中的较大值。
// 对话框高度 = 标题栏 + 文本区 + 按钮区 + 各种间距。
void Dialog::initDialogSize()
{
	splitMessageLines();  // 分割消息行
	getTextSize();       // 获取文本最大尺寸

	// 获取功能按钮数量
	switch (this->type)
	{
	case StellarX::MessageBoxType::OK:  // 只有确定按钮
		buttonNum = 1;
		break;
	case StellarX::MessageBoxType::OKCancel:    // 确定和取消按钮
	case StellarX::MessageBoxType::YesNo:       // 是和否按钮
	case StellarX::MessageBoxType::RetryCancel: // 重试和取消按钮
		buttonNum = 2;
		break;
	case StellarX::MessageBoxType::YesNoCancel:       // 是、否和取消按钮
	case StellarX::MessageBoxType::AbortRetryIgnore: // 中止、重试和忽略按钮
		buttonNum = 3;
		break;
	}

	// 计算按钮区域宽度
	int buttonAreaWidth = buttonNum * functionButtonWidth +
		(buttonNum > 0 ? (buttonNum +1) * buttonMargin : 0);

	// 计算文本区域宽度（包括边距）
	int textAreaWidth = textWidth + textToBorderMargin * 2 ;

	// 对话框宽度取两者中的较大值，并确保最小宽度
	this->width = buttonAreaWidth > textAreaWidth ? buttonAreaWidth : textAreaWidth;
	this->width = this->width > 200 ? this->width : 200;

	// 计算对话框高度
	// 高度 = 标题栏高度 + 文本区域高度 + 按钮区域高度 + 间距
	int textAreaHeight = textHeight * (int)lines.size() + 5*((int)lines.size()-1); // 文本行高+行间距
	this->height = closeButtonHeight +        // 标题栏高度
		titleToTextMargin +        // 标题到文本的间距
		textAreaHeight +           // 文本区域高度
		buttonAreaHeight;          // 按钮区域高度

	// 居中定位对话框
	this->x = (hWnd.getWidth() - this->width) / 2;
	this->y = (hWnd.getHeight() - this->height) / 2;

	//this->textStyle.nWidth = 10;
	this->textStyle.nHeight = 20;

	initButtons();     // 初始化按钮
	initTitle();       // 初始化标题标签
	initCloseButton(); // 初始化关闭按钮
}

void Dialog::initializeDialog()
{
	needsInitialization = true;
	pendingCleanup = false;
	isCleaning = false;
	close = false;
	isVisible = false;

}

// 延迟清理策略：由于对话框绘制时保存了背景快照，必须在对话框隐藏后、
// 所有控件析构前恢复背景，否则会导致背景图像被错误覆盖。
// 此方法在对话框不可见且被标记为待清理时由 draw() 或 handleEvent() 调用。
void Dialog::performDelayedCleanup()
{
	if (isCleaning) return;

	isCleaning = true;

	// 清除所有控件
	controls.clear();

	// 重置指针
	closeButton = nullptr;
	title.reset();

	// 释放背景图像资源
	if (saveBkImage)
	{
		// 恢复背景
		putimage(saveBkX, saveBkY, saveBkImage);
		delete saveBkImage;
		saveBkImage = nullptr;
	}

	// 重置状态
	needsInitialization = true;
	pendingCleanup = false;
	isCleaning = false;
	shouldClose = false;
}

void Dialog::SetResultCallback(std::function<void(StellarX::MessageBoxResult)> cb)
{
	resultCallback = std::move(cb);
}

std::string Dialog::GetCaption() const
{
	return titleText;
}

void Dialog::clearControls()
{
	controls.clear();
	// 重置按钮指针
	closeButton = nullptr;
	title.reset(); // 释放标题资源
}

std::unique_ptr<Button> Dialog::createDialogButton(int x, int y, const std::string& text)
{
	auto btn = std::make_unique<Button>(
		x, y, functionButtonWidth, functionButtonHeight,
		text,
		buttonTrueColor,  // 点击色
		buttonFalseColor,  // 背景色
		buttonHoverColor,  // 悬停色
		StellarX::ButtonMode::NORMAL,
		StellarX::ControlShape::RECTANGLE
	);
	
	return btn;
}


