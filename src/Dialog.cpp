#include "Dialog.h"
#include "SxLog.h"

Dialog::Dialog(Window& h, std::string text, std::string message, StellarX::MessageBoxType type, bool modal)
	: Canvas(), message(message), type(type), modal(modal), hWnd(h), titleText(text)
{
	this->id = "Dialog";
	setHostWindow(&hWnd);
	show = false;
}

Dialog::~Dialog()
{
}

void Dialog::draw()
{
	if (!show)
	{
		// 如果对话框不可见且需要清理，执行清理
		if (pendingCleanup && !isCleaning)
		{
			performDelayedCleanup();
		}
		return;
	}
	// 如果需要初始化，则执行初始化
	if (needsInitialization && show)
	{
		initDialogSize();
		needsInitialization = false;
	}

	if (dirty && show)
	{
		// 保存当前绘图状态
		saveStyle();

		Canvas::setBorderColor(this->borderColor);
		Canvas::setLinewidth(BorderWidth);
		Canvas::setCanvasBkColor(this->backgroundColor);
		Canvas::setShape(StellarX::ControlShape::ROUND_RECTANGLE);

		Canvas::draw();

		//绘制消息文本
		settextcolor(textStyle.color);
		setbkmode(TRANSPARENT);

		//设置字体样式
		settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.lpszFace,
			textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
			textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);

		outtextxy(x + 5, y + 5, LPCTSTR(titleText.c_str()));

		int ty = y + closeButtonHeight + titleToTextMargin; // 文本起始Y坐标
		for (auto& line : lines)
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
	resetEventVisualChanged();
	if (!show)
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

	const bool isMouseMessage =
		msg.message == WM_MOUSEMOVE ||
		msg.message == WM_LBUTTONDOWN ||
		msg.message == WM_LBUTTONUP;
	const bool insideDialog =
		msg.x >= x && msg.x <= x + width &&
		msg.y >= y && msg.y <= y + height;

	// 模态对话框区域外鼠标事件不允许落到底层背景。
	if (modal && isMouseMessage && !insideDialog)
	{
		if (msg.message == WM_LBUTTONUP)
			std::cout << "\a" << std::endl;
		return true;
	}

	// 将事件传递给子控件处理
	if (!consume)
		consume = Canvas::handleEvent(msg);

	// 对话框矩形范围内的鼠标事件一律由对话框吞掉，避免穿透到底层控件。
	if (isMouseMessage && insideDialog)
		consume = true;

	// 每次事件处理后检查是否需要执行延迟清理
	if (pendingCleanup && !isCleaning)
		performDelayedCleanup();
	return consume;
}

void Dialog::SetTitle(const std::string& title)
{
	this->titleText = title;
	invalidateLayout(true);
}

void Dialog::SetMessage(const std::string& message)
{
	this->message = message;
	invalidateLayout(true);
}

void Dialog::SetType(StellarX::MessageBoxType type)
{
	this->type = type;
	invalidateLayout(true);
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

bool Dialog::model() const
{
	return modal;
}

void Dialog::Show()
{
	if (pendingCleanup)
		performDelayedCleanup();
	SX_LOGI("Dialog") << SX_T("对话框弹出：是否模态=","Dialog::Show: modal=") << (modal ? 1 : 0);

	show = true;
	dirty = true;
	needsInitialization = true;

	hWnd.dialogOpen = true;// 通知窗口有对话框打开

	if (modal)
	{
		// 模态对话框需要阻塞当前线程直到对话框关闭
		// 记录当前窗口客户区尺寸，供轮询对比
		RECT rc0;
		GetClientRect(hWnd.getHwnd(), &rc0);
		int lastW = rc0.right - rc0.left;
		int lastH = rc0.bottom - rc0.top;

		while (show)
		{
			// ① 轮询窗口尺寸（不依赖 WM_SIZE）
			RECT rc;
			GetClientRect(hWnd.getHwnd(), &rc);
			const int cw = rc.right - rc.left;
			const int ch = rc.bottom - rc.top;

			if (cw != lastW || ch != lastH)
			{
				lastW = cw;
				lastH = ch;
				SX_LOGD("Resize") <<SX_T("模态对话框检测到窗口大小变化：（", "Modal dialog detected window size change: (") << cw << "x" << ch << ")";

				// 通知父窗口：有新尺寸 → 标记 needResizeDirty
				hWnd.scheduleResizeFromModal(cw, ch);

				// 立即统一收口：父窗重绘 背景+普通控件（不会画到这只模态）
				hWnd.pumpResizeIfNeeded();

				// 这只模态只重新居中，不参与拉伸；背景快照需要在新位置重抓。
				recenterInHostWindow();
			}

			// ② 处理这只对话框的鼠标/键盘（沿用原来 EX_MOUSE | EX_KEY）
			ExMessage msg;
			if (peekmessage(&msg, EX_MOUSE | EX_KEY))
			{
				handleEvent(msg);
				if (!show)
					break;
			}

			// ③ 最后一笔：只画这只模态，保证永远在最上层
			if (dirty)
			{
				BeginBatchDraw();
				this->draw();   // 注意：不要 requestRepaint(parent)，只画自己
				EndBatchDraw();
				dirty = false;
			}

			Sleep(10);
		}

		if (pendingCleanup && !isCleaning)
			performDelayedCleanup();
	}
	else
		// 非模态对话框只需标记为可见，由主循环处理
		dirty = true;
}

void Dialog::Close()
{
	if (!show) return;

	show = false;
	dirty = true;
	pendingCleanup = true;  // 只标记需要清理，不立即执行

	// 工厂模式下非模态触发回调 返回结果
	if (resultCallback && !modal)
		resultCallback(this->result);
}

void Dialog::recenterInHostWindow()
{
	if (!show)
		return;

	// 尚未完成首次初始化时，保持“延迟初始化”语义，由首次 draw 统一创建子控件。
	if (needsInitialization || width <= 0 || height <= 0)
	{
		dirty = true;
		return;
	}

	const int newX = (hWnd.getWidth() - width) / 2;
	const int newY = (hWnd.getHeight() - height) / 2;
	invalidateBackgroundSnapshot();

	x = newX;
	y = newY;
	rebuildChrome();
	dirty = true;
}

void Dialog::initButtons()
{
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
				this->hWnd.dialogClose = true;
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
			(okButton.get()->getX() + okButton.get()->getWidth() + buttonMargin),
			okButton.get()->getY(),
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
			(yesButton.get()->getX() + yesButton.get()->getWidth() + buttonMargin),
			yesButton.get()->getY(),
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
			yesButton.get()->getX() + yesButton.get()->getWidth() + buttonMargin,
			yesButton.get()->getY(),
			"否"
		);
		noButton->setOnClickListener([this]()
			{
				this->SetResult(StellarX::MessageBoxResult::No);
				this->hWnd.dialogClose = true;
				this->Close(); });

		auto cancelButton = createDialogButton(
			noButton.get()->getX() + noButton.get()->getWidth() + buttonMargin,
			noButton.get()->getY(),
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
			retryButton.get()->getX() + retryButton.get()->getWidth() + buttonMargin,
			retryButton.get()->getY(),
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
			(this->x + (this->width - (functionButtonWidth * buttonNum + buttonMargin * (buttonNum - 1))) / 2),
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
			abortButton.get()->getX() + abortButton.get()->getWidth() + buttonMargin,
			abortButton.get()->getY(),
			"重试"
		);
		retryButton->setOnClickListener([this]()
			{
				this->SetResult(StellarX::MessageBoxResult::Retry);
				this->hWnd.dialogClose = true;
				this->Close();
			});
		auto ignoreButton = createDialogButton(
			retryButton.get()->getX() + retryButton.get()->getWidth() + buttonMargin,
			retryButton.get()->getY(),
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
			(this->x + this->width - closeButtonWidth) - 3, (this->y + 3), closeButtonWidth - 1, closeButtonHeight,
			"X",           // 按钮文本
			RGB(255, 0, 0),      // 按钮被点击颜色
			this->canvasBkClor, // 按钮背景颜色
			RGB(255, 0, 0),	  // 按钮被悬停颜色
			StellarX::ButtonMode::NORMAL,
			StellarX::ControlShape::B_RECTANGLE
		);
	but.get()->setButtonFalseColor(this->backgroundColor);
	but.get()->enableTooltip(false);
	but->setOnClickListener([this]() {
		this->SetResult(StellarX::MessageBoxResult::Cancel);
		this->hWnd.dialogClose = true;
		this->Close(); });

	this->closeButton = but.get();
	this->addControl(std::move(but));
}

void Dialog::splitMessageLines()
{
	lines.clear(); // 先清空现有的行

	std::string currentLine;
	for (size_t i = 0; i < message.length(); i++) {
		// 处理 换行符 \r\n  \n  \r
		const bool hasNext = (i + 1 < message.length());
		const bool isLineBreak = (message[i] == '\r' || message[i] == '\n');
		const bool isCrLf = hasNext && message[i] == '\r' && message[i + 1] == '\n';
		if (isLineBreak || isCrLf)
		{
			if (!currentLine.empty()) {
				lines.push_back(currentLine);
				currentLine.clear();
			}

			if (isCrLf)
				i++;
			continue;
		}

		currentLine += message[i];
	}

	// 添加最后一行（如果有内容）
	if (!currentLine.empty())
	{
		lines.push_back(currentLine);
	}

	// 如果消息为空，至少添加一个空行
	if (lines.empty())
	{
		lines.push_back("");
	}
}

void Dialog::getTextSize()
{
	saveStyle();
	settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.lpszFace,
		textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
		textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);
	this->textHeight = 0;
	this->textWidth = 0;
	for (auto& text : lines)
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

void Dialog::invalidateLayout(bool clearChildren)
{
	if (clearChildren)
		clearControls();

	this->textWidth = 0;
	this->textHeight = 0;
	this->buttonNum = 0;
	this->needsInitialization = true;
	this->dirty = true;
}

void Dialog::rebuildChrome()
{
	clearControls();
	initButtons();
	initCloseButton();
}

// 计算逻辑：对话框宽度取【文本区域最大宽度】和【按钮区域总宽度】中的较大值。
// 对话框高度 = 标题栏 + 文本区 + 按钮区 + 各种间距。
void Dialog::initDialogSize()
{
	this->textStyle.nHeight = 20;
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
		(buttonNum > 0 ? (buttonNum + 1) * buttonMargin : 0);

	// 计算文本区域宽度（包括边距）
	int textAreaWidth = textWidth + textToBorderMargin * 2;
	saveStyle();
	settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.lpszFace,
		textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
		textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);
	int titleAreaWidth = textwidth(LPCTSTR(titleText.c_str())) + textToBorderMargin * 2 + closeButtonWidth + buttonMargin;
	restoreStyle();

	// 对话框宽度取文本、标题和按钮区域中的较大值，并确保最小宽度
	this->width = buttonAreaWidth > textAreaWidth ? buttonAreaWidth : textAreaWidth;
	if (titleAreaWidth > this->width)
		this->width = titleAreaWidth;
	this->width = this->width > 200 ? this->width : 200;

	// 计算对话框高度
	// 高度 = 标题栏高度 + 文本区域高度 + 按钮区域高度 + 间距
	int textAreaHeight = textHeight * (int)lines.size() + 5 * ((int)lines.size() - 1); // 文本行高+行间距
	this->height = closeButtonHeight +        // 标题栏高度
		titleToTextMargin +        // 标题到文本的间距
		textAreaHeight +           // 文本区域高度
		buttonAreaHeight;          // 按钮区域高度

	// 居中定位对话框
	this->x = (hWnd.getWidth() - this->width) / 2;
	this->y = (hWnd.getHeight() - this->height) / 2;

	rebuildChrome();
}

void Dialog::addControl(std::unique_ptr<Control> control)
{
	control->setParent(this);
	controls.push_back(std::move(control));
	dirty = true;
}

// 延迟清理策略：由于对话框绘制时保存了背景快照，必须在对话框隐藏后、
// 所有控件析构前恢复背景，否则会导致背景图像被错误覆盖。
// 此方法在对话框不可见且被标记为待清理时由 draw() 或 handleEvent() 调用。
void Dialog::performDelayedCleanup()
{
	if (isCleaning) return;

	isCleaning = true;

	auto& c = hWnd.getControls();
	for (auto& control : c)
		control->setDirty(true);

	controls.clear();

	// 重置指针
	closeButton = nullptr;
	// 释放背景图像资源
	if (saveBkImage && hasSnap)
	{
		restBackground();
		FlushBatchDraw();
		invalidateBackgroundSnapshot();
	}
	if (!(saveBkImage && hasSnap))
	{
		// 没有背景快照：强制一次完整重绘，立即擦掉残影
		hWnd.pumpResizeIfNeeded(); // 如果正好有尺寸标志，顺便统一收口
		// 即使没有尺寸变化，也重绘一帧
		BeginBatchDraw();
		// 背景
		if (hWnd.getBkImage() && !hWnd.getBkImageFile().empty())
			putimage(0, 0, hWnd.getBkImage());
		else { setbkcolor(hWnd.getBkcolor()); cleardevice(); }
		// 所有普通控件
		for (auto& c : hWnd.getControls()) c->draw();
		// 其他对话框（this 已经 show=false，会早退不绘）
		EndBatchDraw();
		FlushBatchDraw();
	}
	// 重置状态
	needsInitialization = true;
	pendingCleanup = false;
	isCleaning = false;
}

void Dialog::SetResultCallback(std::function<void(StellarX::MessageBoxResult)> cb)
{
	resultCallback = std::move(cb);
}

std::string Dialog::GetCaption() const
{
	return titleText;
}

std::string Dialog::GetText() const
{
	return message;
}

void Dialog::clearControls()
{
	for (auto& control : controls)
		control->invalidateBackgroundSnapshot();

	controls.clear();
	// 重置按钮指针
	closeButton = nullptr;
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

void Dialog::requestRepaint(Control* parent)
{
	if (shouldDeferManagedRepaint())
	{
		// 非模态 Dialog 在 Window 主循环中也走托管提交；
		// 这样底层控件和对话框的绘制顺序由 Window 统一收口控制。
		if (auto* host = getHostWindow())
			host->requestManagedRepaint(this);
		return;
	}

	if (this == parent)
	{
		for (auto& control : controls)
			if (control->isDirty() && control->IsVisible())
				control->draw();
	}
	else
		onRequestRepaintAsRoot();
}

bool Dialog::canCommitManagedPartialRepaint() const
{
	// Dialog 只有在“自身底板不脏 + 仍持有有效背景快照”时，
	// 才能安全地只更新内部按钮，而不重画整个对话框底板。
	return show && !dirty && hasValidBackgroundSnapshot();
}

void Dialog::commitManagedRepaint()
{
	if (!show)
		return;

	if (canCommitManagedPartialRepaint())
	{
		// 背景快照完好：沿用 Dialog 自己已有的局部重绘路径。
		requestRepaint(this);
		return;
	}

	// 对话框底板本身已脏，或快照失效：必须整 Dialog 重画。
	this->dirty = true;
	onRequestRepaintAsRoot();
}
