#include "Dialog.h"
#include "SxLog.h"
#include "SxEventLoopInternal.h"
#include "SxInputProbeInternal.h"
#include "SxKeyboardRouterInternal.h"

namespace
{
	class ScopedFocusScope
	{
		StellarX::FocusManager* manager = nullptr;
		StellarX::FocusScopeId scope{};
	public:
		ScopedFocusScope(StellarX::FocusManager& focusManager,
			StellarX::FocusScopeId focusScope) noexcept
			: manager(&focusManager), scope(focusScope)
		{
		}
		ScopedFocusScope(const ScopedFocusScope&) = delete;
		ScopedFocusScope& operator=(const ScopedFocusScope&) = delete;
		~ScopedFocusScope()
		{
			if (manager && scope && !manager->popScope(scope))
				SX_LOGE("Dialog") << "failed to pop modal focus scope";
		}
		explicit operator bool() const noexcept { return static_cast<bool>(scope); }
	};
}

Dialog::Dialog(Window& h, std::string text, std::string message, StellarX::MessageBoxType type, bool modal)
	: Canvas(), message(message), type(type), modal(modal), hWnd(h), titleText(text)
{
	this->id = "Dialog";
	// Dialog 采用“居中 + 自管内部 chrome”的布局方式，
	// 不参与外部 Stretch；宿主窗口变化时只重新居中。
	this->layoutCapability.allowStretchX = false;
	this->layoutCapability.allowStretchY = false;
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
	if (!modal)
		hWnd.commitPendingNonModalDialogActivation(*this);

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
		settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.faceName.c_str(),
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
	invalidateLayout();
}

void Dialog::SetMessage(const std::string& message)
{
	this->message = message;
	invalidateLayout();
}

void Dialog::SetType(StellarX::MessageBoxType type)
{
	this->type = type;
	invalidateLayout();
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

bool Dialog::handleFrameworkDefault(const StellarX::KeyEvent& key)
{
	if (!show || pendingCleanup || isCleaning ||
		key.action != StellarX::KeyAction::Down || key.isAutoRepeat())
		return false;

	if (key.virtualKey == VK_ESCAPE)
	{
		SetResult(StellarX::MessageBoxResult::Cancel);
		Close();
		return true;
	}

	if (key.virtualKey != VK_RETURN || key.modifiers.control() ||
		!defaultFocusTarget)
		return false;

	auto* button = dynamic_cast<Button*>(findControl(defaultFocusTarget.owner));
	if (!button || defaultFocusTarget.subNode != 0)
		return false;
	// 默认按钮无需当前逻辑焦点。回调可能关闭/删除 Dialog；调用后不再访问
	// button 或 this 的成员。
	button->activateFromKeyboard(false);
	return true;
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
		// 模态循环也是事件循环入口；开始排空 EasyX 消息前先锁定共享键盘管线。
		StellarX::Internal::lockKeyboardPipeline(hWnd);
		StellarX::Internal::lockPointerPipeline(hWnd);
		if (StellarX::Internal::keyboardPipelineMode(hWnd) ==
			StellarX::Internal::KeyboardPipelineMode::Routed)
		{
			// 在 scope 切换前先取消底层路径的键盘 armed 状态；随后真实焦点会
			// 进入模态子树，底层逻辑焦点由 FocusScope 负责保存和恢复。
			StellarX::Internal::KeyboardRouter::suspendFocusedPath(hWnd);
		}
		// 模态对话框需要阻塞当前线程直到对话框关闭
		if (!hWnd.hasLiveNativeWindow())
		{
			show = false;
			hWnd.dialogOpen = false;
			return;
		}

		// 记录当前窗口客户区尺寸，供轮询对比
		RECT rc0{};
		if (!GetClientRect(hWnd.getHwnd(), &rc0))
		{
			show = false;
			hWnd.dialogOpen = false;
			StellarX::Internal::inputProbeFlushAtSafePoint(hWnd.getHwnd());
			return;
		}
		int lastW = rc0.right - rc0.left;
		int lastH = rc0.bottom - rc0.top;
		// 模态 Dialog 不在 Window::dialogs owning 树中。必须先建立完整 chrome，
		// 再用受 Show() 生命周期约束的临时注册让 FocusManager 可以解析其身份。
		if (needsInitialization)
		{
			initDialogSize();
			needsInitialization = false;
		}

		{
			auto modalRegistration = hWnd.registerModalDialog(*this);
			const StellarX::FocusScopeId scopeId = modalRegistration
				? hWnd.getFocusManager().pushScope(
					{ getHandle(), 0 }, defaultFocusTarget)
				: StellarX::FocusScopeId{};
			ScopedFocusScope focusScope(hWnd.getFocusManager(), scopeId);
			if (!modalRegistration || !focusScope)
			{
				SX_LOGE("Dialog") << "failed to activate modal focus scope";
				show = false;
				hWnd.dialogOpen = false;
			}

		while (show)
		{
			if (!hWnd.drainNativeInputAtSafePoint(false, &show))
			{
				show = false;
				hWnd.dialogOpen = false;
				StellarX::Internal::inputProbeFlushAtSafePoint(hWnd.getHwnd());
				break;
			}
			if (!show)
				break;
			if (!hWnd.hasLiveNativeWindow())
			{
				show = false;
				hWnd.dialogOpen = false;
				break;
			}

			// 模态对话框保留自己的轮询循环，但消息同样批量处理，并在每轮末尾
			// 推进内部时钟、提交托管重绘，行为边界与 Window 主循环保持一致。
			// ① 轮询窗口尺寸（不依赖 WM_SIZE）
			RECT rc{};
			if (!GetClientRect(hWnd.getHwnd(), &rc))
			{
				show = false;
				hWnd.dialogOpen = false;
				StellarX::Internal::inputProbeFlushAtSafePoint(hWnd.getHwnd());
				break;
			}
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
			auto probeBatch = StellarX::Internal::inputProbeHostBatchBegin(hWnd.getHwnd(), true);
			const auto batch = StellarX::Internal::drainMessageBatch(msg,
				[](ExMessage& next) { return peekmessage(&next, EX_MOUSE | EX_KEY); },
				[&](const ExMessage& next) {
					StellarX::Internal::inputProbeObserveEasyXMessage(hWnd.getHwnd(), probeBatch, next);
					const bool easyXKeyboardMessage =
						StellarX::Internal::isEasyXKeyboardMessage(next.message);
					const bool easyXPointerMessage =
						StellarX::Internal::isEasyXPointerMessage(next.message);
					if (easyXKeyboardMessage)
						++hWnd.easyXKeyboardMessagesDrained;
					if (easyXPointerMessage)
						++hWnd.easyXPointerMessagesDrained;
					if (StellarX::Internal::shouldDeliverLegacyEasyXMessage(hWnd, next.message))
					{
						if (easyXKeyboardMessage)
							++hWnd.legacyEasyXKeyboardBusinessDeliveries;
						if (easyXPointerMessage)
							++hWnd.legacyEasyXPointerBusinessDeliveries;
						hWnd.dispatchManagedBusinessMessage(next);
					}
					return show;
				});
			StellarX::Internal::inputProbeHostBatchEnd(
				hWnd.getHwnd(), probeBatch, batch.processed, batch.stopped);
			if (!show)
				break;
			if (!hWnd.drainNativeInputAtSafePoint(false, &show))
			{
				show = false;
				hWnd.dialogOpen = false;
				StellarX::Internal::inputProbeFlushAtSafePoint(hWnd.getHwnd());
				break;
			}
			if (!show)
				break;
			if (!hWnd.hasLiveNativeWindow())
			{
				show = false;
				hWnd.dialogOpen = false;
				StellarX::Internal::inputProbeFlushAtSafePoint(hWnd.getHwnd());
				break;
			}

			// ③ 推进当前最内层模态的内部时钟。外层模态循环在嵌套 Show()
			// 期间暂停，不应继续产生 Hover/Tooltip 等交互状态。
			this->tick(GetTickCount64());

			// ④ 提交本轮全部托管视觉变化。Window 使用完整 z-order，先提交
			// 普通控件/非模态/外层模态，再把当前最内层模态补回最上层。
			hWnd.collectDirtyRootsForManagedRepaint();
			hWnd.flushManagedRepaint();

			// ⑤ 防御性兜底：正常的子控件、焦点和 tick 变化已经由上面的托管
			// flush 收口；这里只处理尚未登记为 managed root 的整 Dialog 脏状态。
			if (dirty)
			{
				BeginBatchDraw();
				this->draw();   // 注意：不要 requestRepaint(parent)，只画自己
				EndBatchDraw();
				dirty = false;
			}
			if (!hWnd.isMutationTransactionActive())
				StellarX::Internal::inputProbeFlushAtSafePoint(hWnd.getHwnd());

			if (batch.shouldSleep())
				Sleep(1);
		}
		} // focus scope 先 pop，随后 modal registration 注销

		hWnd.reconcileNonModalDialogScope();
		hWnd.dialogOpen = false;
		if (!controls.empty() && !isCleaning)
			performDelayedCleanup();
	}
	else
	{
		// chrome 会在首次 draw 中物化；届时再 push scope，避免把尚不存在的
		// 默认按钮身份写入焦点状态。
		hWnd.requestNonModalDialogActivation(*this);
		// 非模态对话框只需标记为可见，由主循环处理
		dirty = true;
	}
}

void Dialog::Close()
{
	if (!show) return;

	// 关闭只改变可见状态并登记一次全场景收口；对象和快照清理延后到安全阶段执行。
	show = false;
	dirty = true;
	pendingCleanup = true;  // 只标记需要清理，不立即执行
	if (!modal)
		hWnd.onNonModalDialogClosed(getHandle());
	notifyFocusSubtreeStateChanged();
	hWnd.dialogClose = true;
	hWnd.requestFullSceneRedraw();

	// 工厂模式下非模态触发回调 返回结果
	if (resultCallback && !modal)
	{
		// 回调可能继续修改宿主控件树，因此也纳入 MutationScope，禁止遍历期间立即销毁对象。
		auto mutationScope = enterMutationScope();
		resultCallback(this->result);
	}
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
	const int deltaX = newX - x;
	const int deltaY = newY - y;
	if (deltaX == 0 && deltaY == 0)
		return;

	invalidateBackgroundSnapshot();
	x = newX;
	y = newY;

	// 重居中不改变标题、正文、按钮组合或 Dialog 尺寸，因此只平移已有
	// chrome。先丢弃子控件快照，避免 setX/setY 在新位置尚未完整合成时
	// 触发单个按钮的立即提交；随后由 Dialog 整体重绘统一收口。
	for (auto& control : controls)
	{
		if (!control)
			continue;
		control->invalidateBackgroundSnapshot();
		control->setX(control->getX() + deltaX);
		control->setY(control->getY() + deltaY);
	}
	dirty = true;
}

void Dialog::initButtons()
{
	defaultFocusTarget = {};
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
		defaultFocusTarget = { okbutton->getHandle(), 0 };

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
		defaultFocusTarget = { okButton->getHandle(), 0 };

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
		defaultFocusTarget = { yesButton->getHandle(), 0 };

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
		defaultFocusTarget = { yesButton->getHandle(), 0 };

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
		defaultFocusTarget = { retryButton->getHandle(), 0 };

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
		defaultFocusTarget = { abortButton->getHandle(), 0 };

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
	// 右上角 X 仍可鼠标命中和程序化聚焦，但不进入 Tab 顺序。
	but->setFocusPolicy(StellarX::FocusPolicy::Pointer |
		StellarX::FocusPolicy::Programmatic);
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
	settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.faceName.c_str(),
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

void Dialog::invalidateLayout()
{
	// 只登记重建请求。SetTitle/SetMessage/SetType 可能从任意控件回调中
	// 调用；在 setter 内直接 clear 会让当前回调栈持有的 this/兄弟地址
	// 立即失效。旧 chrome 在下一次 draw 的安全收口阶段才物理退休；但从
	// 本调用返回起就必须停止命中和键盘路由，不能在同一消息批次继续工作。
	std::vector<StellarX::ControlHandle> staleHandles;
	staleHandles.reserve(controls.size());
	for (const auto& control : controls)
		if (control) staleHandles.push_back(control->getHandle());
	if (!staleHandles.empty())
		hWnd.getFocusManager().notifySubtreesPendingRemoval(staleHandles);
	for (auto& control : controls)
		if (control) control->setIsVisible(false);

	this->textWidth = 0;
	this->textHeight = 0;
	this->buttonNum = 0;
	this->needsInitialization = true;
	this->dirty = true;
}

void Dialog::rebuildChrome()
{
	// Dialog::draw() 在首次布局和内容变更时可能位于普通/模态两种循环。
	// 显式事务保证“批量失效旧句柄 -> 释放旧对象 -> 建立新 chrome”只在
	// 完整重建后提交，retireChromeControls() 自身不越权 flush 整棵 Window。
	auto mutationScope = enterMutationScope();
	retireChromeControls();
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
	settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.faceName.c_str(),
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
	{
		auto mutationScope = enterMutationScope();
		retireChromeControls();
		// 清理阶段不直接写屏；Window 会在安全点统一重绘剩余场景。
		invalidateBackgroundSnapshot();
		hWnd.requestFullSceneRedraw();
		// 重置状态
		needsInitialization = true;
		pendingCleanup = false;
	}
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

void Dialog::retireChromeControls()
{
	std::vector<StellarX::ControlHandle> retiredHandles;
	retiredHandles.reserve(controls.size());
	for (const auto& control : controls)
		if (control) retiredHandles.push_back(control->getHandle());

	if (!retiredHandles.empty())
		hWnd.getFocusManager().notifySubtreesPendingRemoval(retiredHandles);

	for (auto& control : controls)
		if (control) control->invalidateBackgroundSnapshot();

	controls.clear();
	closeButton = nullptr;
	defaultFocusTarget = {};
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
