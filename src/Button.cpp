#include "Button.h"

Button::Button(int x, int y, int width, int height, const std::string text, StellarX::ButtonMode mode, StellarX::ControlShape shape)
	: Control(x, y, width, height)
{
	initButton(text, mode, shape, RGB(202, 255, 255), RGB(171, 196, 220), RGB(255, 255, 0));
}

Button::Button(int x, int y, int width, int height, const std::string text, COLORREF ct, COLORREF cf, StellarX::ButtonMode mode, StellarX::ControlShape shape)
	: Control(x, y, width, height)
{
	initButton(text, mode, shape, ct, cf, RGB(255, 255, 0));
}

Button::Button(int x, int y, int width, int height, const std::string text, COLORREF ct, COLORREF cf, COLORREF ch, StellarX::ButtonMode mode, StellarX::ControlShape shape)
	: Control(x, y, width, height)
{
	initButton(text, mode, shape, ct, cf, ch);
}
// ====== GBK/MBCS 安全：字符边界与省略号裁切 ======
static inline int gbk_char_len(const std::string& s, size_t i)
{
	unsigned char b = (unsigned char)s[i];
	if (b <= 0x7F) return 1; // ASCII
	if (b >= 0x81 && b <= 0xFE && i + 1 < s.size())
	{
		unsigned char b2 = (unsigned char)s[i + 1];
		if (b2 >= 0x40 && b2 <= 0xFE && b2 != 0x7F) return 2; // 合法双字节
	}
	return 1; // 容错
}

static inline void rtrim_spaces_gbk(std::string& s)
{
	while (!s.empty() && s.back() == ' ') s.pop_back();           // ASCII 空格
	while (s.size() >= 2)
	{                                       // 全角空格 A1 A1
		unsigned char a = (unsigned char)s[s.size() - 2];
		unsigned char b = (unsigned char)s[s.size() - 1];
		if (a == 0xA1 && b == 0xA1) s.resize(s.size() - 2);
		else break;
	}
}

static inline bool is_ascii_only(const std::string& s)
{
	for (unsigned char c : s) if (c > 0x7F) return false;
	return true;
}

static inline bool is_word_boundary_char(unsigned char c)
{
	return c == ' ' || c == '-' || c == '_' || c == '/' || c == '\\' || c == '.' || c == ':';
}

// 英文优先策略：优先在“词边界”回退，再退化到逐字符；省略号为 "..."
static std::string ellipsize_ascii_pref(const std::string& text, int maxW)
{
	if (maxW <= 0) return "";
	if (textwidth(LPCTSTR(text.c_str())) <= maxW) return text;

	const std::string ell = "...";
	int ellW = textwidth(LPCTSTR(ell.c_str()));
	if (ellW > maxW)
	{ // 连 ... 都放不下
		std::string e = ell;
		while (!e.empty() && textwidth(LPCTSTR(e.c_str())) > maxW) e.pop_back();
		return e; // 可能是 ".."、"." 或 ""
	}
	const int limit = maxW - ellW;

	// 先找到能放下的最长前缀
	size_t i = 0, lastFit = 0;
	while (i < text.size())
	{
		int clen = gbk_char_len(text, i);
		size_t j = text.size() < i + (size_t)clen ? text.size() : i + (size_t)clen;
		int w = textwidth(LPCTSTR(text.substr(0, j).c_str()));
		if (w <= limit) { lastFit = j; i = j; }
		else break;
	}
	if (lastFit == 0) return ell;

	// 在已适配前缀范围内，向左找最近的词边界
	size_t cutPos = lastFit;
	for (size_t k = lastFit; k > 0; --k)
	{
		unsigned char c = (unsigned char)text[k - 1];
		if (c <= 0x7F && is_word_boundary_char(c)) { cutPos = k - 1; break; }
	}

	std::string head = text.substr(0, cutPos);
	rtrim_spaces_gbk(head);
	head += ell;
	return head;
}

// 中文优先策略：严格逐“字符”(1/2字节)回退；省略号用全角 "…"
static std::string ellipsize_cjk_pref(const std::string& text, int maxW, const char* ellipsis = "…")
{
	if (maxW <= 0) return "";
	if (textwidth(LPCTSTR(text.c_str())) <= maxW) return text;

	std::string ell = ellipsis ? ellipsis : "…";
	int ellW = textwidth(LPCTSTR(ell.c_str()));
	if (ellW > maxW)
	{ // 连省略号都放不下
		std::string e = ell;
		while (!e.empty() && textwidth(LPCTSTR(e.c_str())) > maxW) e.pop_back();
		return e;
	}
	const int limit = maxW - ellW;

	size_t i = 0, lastFit = 0;
	while (i < text.size())
	{
		int clen = gbk_char_len(text, i);
		size_t j = text.size() < i + (size_t)clen ? text.size() : i + (size_t)clen;
		int w = textwidth(LPCTSTR(text.substr(0, j).c_str()));
		if (w <= limit) { lastFit = j; i = j; }
		else break;
	}
	if (lastFit == 0) return ell;

	std::string head = text.substr(0, lastFit);
	rtrim_spaces_gbk(head);
	head += ell;
	return head;
}

void Button::setTooltipStyle(COLORREF text, COLORREF bk, bool transparent)
{
	tipLabel.textStyle.color = text;
	tipLabel.setTextBkColor(bk);
	tipLabel.setTextdisap(transparent);
}

void Button::setTooltipTextsForToggle(const std::string& onText, const std::string& offText)
{
	tipTextOn = onText;
	tipTextOff = offText;
	tipUserOverride = true;
}

void Button::initButton(const std::string text, StellarX::ButtonMode mode, StellarX::ControlShape shape, COLORREF ct, COLORREF cf, COLORREF ch)
{
	this->id = "Button";
	this->text = text;
	this->mode = mode;
	this->shape = shape;
	this->buttonTrueColor = ct;
	this->buttonFalseColor = cf;
	this->buttonHoverColor = ch;
	this->click = false;
	this->hover = false;

	// === Tooltip 默认：文本=按钮文本；白底黑字；不透明；用当前按钮字体样式 ===
	tipTextClick = tipTextOn = tipTextOff = this->text;
	tipLabel.setText(tipTextClick);
	tipLabel.textStyle.color = (RGB(167, 170, 172));
	tipLabel.setTextBkColor(RGB(255, 255, 255));
	tipLabel.setTextdisap(false);
	tipLabel.textStyle = this->textStyle;  // 复用按钮字体样式
}


Button::~Button()
{
	if (buttonFileIMAGE)
		delete buttonFileIMAGE;
	buttonFileIMAGE = nullptr;
}

void Button::draw()
{
	if (!dirty || !show)return;

	//保存当前样式和颜色
	saveStyle();

	if (StellarX::ButtonMode::DISABLED == mode)   //设置禁用按钮色
	{
		setfillcolor(DISABLEDCOLOUR);
		textStyle.bStrikeOut = true;
	}
	else
	{
		// 点击状态优先级最高，然后是悬停状态，最后是默认状态
		COLORREF col = click ? buttonTrueColor : (hover ? buttonHoverColor : buttonFalseColor);
		setfillcolor(col);
	}
	//
	//设置字体背景色透明
	setbkmode(TRANSPARENT);
	//边框颜色
	setlinecolor(buttonBorderColor);

	//设置字体颜色
	settextcolor(textStyle.color);
	//设置字体样式
	settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.lpszFace,
		textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
		textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);

	if (needCutText)
		cutButtonText();

	//获取字符串像素高度和宽度
	if ((this->oldtext_width != this->text_width || this->oldtext_height != this->text_height)
		|| (-1 == oldtext_width && oldtext_height == -1))
	{
		if (isUseCutText)
		{
			this->oldtext_width = this->text_width = textwidth(LPCTSTR(this->cutText.c_str()));
			this->oldtext_height = this->text_height = textheight(LPCTSTR(this->cutText.c_str()));
		}
		else
		{
			this->oldtext_width = this->text_width = textwidth(LPCTSTR(this->text.c_str()));
			this->oldtext_height = this->text_height = textheight(LPCTSTR(this->text.c_str()));
		}
	}

	//设置按钮填充模式
	setfillstyle((int)buttonFillMode, (int)buttonFillIma, buttonFileIMAGE);
	if ((saveBkX != this->x) || (saveBkY != this->y) || (!hasSnap) || (saveWidth != this->width) || (saveHeight != this->height) || !saveBkImage)
		saveBackground(this->x, this->y, (this->width + bordWith), (this->height + bordHeight));
	// 恢复背景（清除旧内容）
	restBackground();
	//根据按钮形状绘制
	switch (shape)
	{
	case StellarX::ControlShape::RECTANGLE://有边框填充矩形
		fillrectangle(x, y, x + width, y + height);
		isUseCutText ? outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(cutText.c_str()))
			: outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(text.c_str()));
		break;
	case StellarX::ControlShape::B_RECTANGLE://无边框填充矩形
		solidrectangle(x, y, x + width, y + height);
		isUseCutText ? outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(cutText.c_str()))
			: outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(text.c_str()));
		break;
	case StellarX::ControlShape::ROUND_RECTANGLE://有边框填充圆角矩形
		fillroundrect(x, y, x + width, y + height, rouRectangleSize.ROUND_RECTANGLEwidth, rouRectangleSize.ROUND_RECTANGLEheight);
		isUseCutText ? outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(cutText.c_str()))
			: outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(text.c_str()));
		break;
	case StellarX::ControlShape::B_ROUND_RECTANGLE://无边框填充圆角矩形
		solidroundrect(x, y, x + width, y + height, rouRectangleSize.ROUND_RECTANGLEwidth, rouRectangleSize.ROUND_RECTANGLEheight);
		isUseCutText ? outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(cutText.c_str()))
			: outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(text.c_str()));
		break;
	case StellarX::ControlShape::CIRCLE://有边框填充圆形
		fillcircle(x + width / 2, y + height / 2, min(width, height) / 2);
		isUseCutText ? outtextxy(x + width / 2 - text_width / 2, y + height / 2 - text_height / 2, LPCTSTR(cutText.c_str()))
			: outtextxy(x + width / 2 - text_width / 2, y + height / 2 - text_height / 2, LPCTSTR(text.c_str()));
		break;
	case StellarX::ControlShape::B_CIRCLE://无边框填充圆形
		solidcircle(x + width / 2, y + height / 2, min(width, height) / 2);
		isUseCutText ? outtextxy(x + width / 2 - text_width / 2, y + height / 2 - text_height / 2, LPCTSTR(cutText.c_str()))
			: outtextxy(x + width / 2 - text_width / 2, y + height / 2 - text_height / 2, LPCTSTR(text.c_str()));
		break;
	case StellarX::ControlShape::ELLIPSE://有边框填充椭圆
		fillellipse(x, y, x + width, y + height);
		isUseCutText ? outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(cutText.c_str()))
			: outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(text.c_str()));
		break;
	case StellarX::ControlShape::B_ELLIPSE://无边框填充椭圆
		solidellipse(x, y, x + width, y + height);
		isUseCutText ? outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(cutText.c_str()))
			: outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(text.c_str()));
		break;
	}

	restoreStyle();//恢复默认字体样式和颜色
	dirty = false;     //标记按钮不需要重绘

}
// 处理鼠标事件，检测点击和悬停状态
// 根据按钮模式和形状进行不同的处理
bool Button::handleEvent(const ExMessage& msg)
{
	if (!show)
		return false;

	bool oldHover = hover;
	bool oldClick = click;
	bool consume = false;//是否消耗事件
	// 记录鼠标位置（用于tip定位）
	if (msg.message == WM_MOUSEMOVE)
	{
		lastMouseX = msg.x;
		lastMouseY = msg.y;
	}
	// 检测悬停状态（根据不同形状）
	switch (shape)
	{
	case StellarX::ControlShape::RECTANGLE:
	case StellarX::ControlShape::B_RECTANGLE:
	case StellarX::ControlShape::ROUND_RECTANGLE:
	case StellarX::ControlShape::B_ROUND_RECTANGLE:
		hover = (msg.x > x && msg.x < (x + width) && msg.y > y && msg.y < (y + height));
		break;
	case StellarX::ControlShape::CIRCLE:
	case StellarX::ControlShape::B_CIRCLE:
		hover = isMouseInCircle(msg.x, msg.y, x + width / 2, y + height / 2, min(width, height) / 2);
		break;
	case StellarX::ControlShape::ELLIPSE:
	case StellarX::ControlShape::B_ELLIPSE:
		hover = isMouseInEllipse(msg.x, msg.y, x, y, x + width, y + height);
		break;
	}

	// 处理鼠标点击事件
	if (msg.message == WM_LBUTTONDOWN && hover && mode != StellarX::ButtonMode::DISABLED)
	{

		if (mode == StellarX::ButtonMode::NORMAL)
		{
			click = true;
			dirty = true;
			consume = true;
		}
		else if (mode == StellarX::ButtonMode::TOGGLE)
		{
			// TOGGLE模式在鼠标释放时处理
		}
	}
	// NORMAL 模式：鼠标在按钮上释放时才触发点击回调，如果移出区域则取消点击状态。
	// TOGGLE 模式：在释放时切换状态，并触发相应的开/关回调。
	else if (msg.message == WM_LBUTTONUP && hover && mode != StellarX::ButtonMode::DISABLED)
	{
		hideTooltip();  // 隐藏悬停提示
		if (mode == StellarX::ButtonMode::NORMAL && click)
		{
			if (onClickCallback) onClickCallback();
			click = false;
			dirty = true;
			consume = true;
			hideTooltip();
			// 清除消息队列中积压的鼠标和键盘消息，防止本次点击事件被重复处理
			flushmessage(EX_MOUSE | EX_KEY);
		}
		else if (mode == StellarX::ButtonMode::TOGGLE)
		{
			click = !click;
			if (click && onToggleOnCallback) onToggleOnCallback();
			else if (!click && onToggleOffCallback) onToggleOffCallback();
			dirty = true;
			consume = true;
			refreshTooltipTextForState();
			hideTooltip();
			// 清除消息队列中积压的鼠标和键盘消息，防止本次点击事件被重复处理
			flushmessage(EX_MOUSE | EX_KEY);
		}
	}
	// 处理鼠标移出区域的情况

	else if (msg.message == WM_MOUSEMOVE)
	{
		if (!hover && mode == StellarX::ButtonMode::NORMAL && click)
		{
			click = false;
			dirty = true;
		}
		else if (hover != oldHover)
			dirty = true;
	}

	if (tipEnabled)
	{
		if (hover && !oldHover)
		{
			// 刚刚进入悬停：开计时，暂不显示
			tipHoverTick = GetTickCount64();
			tipVisible = false;
		}
		if (!hover && oldHover)
		{
			// 刚移出：立即隐藏
			hideTooltip();
		}
		if (hover && !tipVisible)
		{
			// 到点就显示
			if (GetTickCount64() - tipHoverTick >= (ULONGLONG)tipDelayMs)
			{
				tipVisible = true;

				// 定位（跟随鼠标 or 相对按钮）
				int tipX = tipFollowCursor ? (lastMouseX + tipOffsetX) : lastMouseX;
				int tipY = tipFollowCursor ? (lastMouseY + tipOffsetY) : y + height;
				// 设置文本（用户可能动态改了提示文本
				if (tipUserOverride)
				{
					if (mode == StellarX::ButtonMode::NORMAL)
						tipLabel.setText(tipTextClick);
					else if (mode == StellarX::ButtonMode::TOGGLE)
						tipLabel.setText(click ? tipTextOn : tipTextOff);
				}
				else
					if (mode == StellarX::ButtonMode::TOGGLE)
						tipLabel.setText(click ? tipTextOn : tipTextOff);
				// 设置位置
				tipLabel.setX(tipX);
				tipLabel.setY(tipY);
				// 标记需要绘制
				tipLabel.setDirty(true);
			}
		}
	}

	// 如果状态发生变化，标记需要重绘
	if (hover != oldHover || click != oldClick)
		dirty = true;

	// 如果需要重绘，立即执行
	if (dirty)
		requestRepaint(parent);

	if (tipEnabled && tipVisible)
		tipLabel.draw();

	return consume;
}

void Button::setOnClickListener(const std::function<void()>&& callback)
{
	this->onClickCallback = callback;
}

void Button::setOnToggleOnListener(const std::function<void()>&& callback)
{
	this->onToggleOnCallback = callback;
}
void Button::setOnToggleOffListener(const std::function<void()>&& callback)
{
	this->onToggleOffCallback = callback;
}

void Button::setbuttonMode(StellarX::ButtonMode mode)
{
	if (this->mode == StellarX::ButtonMode::DISABLED && mode != StellarX::ButtonMode::DISABLED)
		textStyle.bStrikeOut = false;
	//取值范围参考 buttMode的枚举注释
	this->mode = mode;
	dirty = true; // 标记需要重绘
}

void Button::setROUND_RECTANGLEwidth(int width)
{
	rouRectangleSize.ROUND_RECTANGLEwidth = width;
	this->dirty = true; // 标记需要重绘

}

void Button::setROUND_RECTANGLEheight(int height)
{
	rouRectangleSize.ROUND_RECTANGLEheight = height;
	this->dirty = true; // 标记需要重绘
}

bool Button::isClicked() const
{
	return this->click;
}

void Button::setFillMode(StellarX::FillMode mode)
{
	this->buttonFillMode = mode;
	this->dirty = true; // 标记需要重绘
}

void Button::setFillIma(StellarX::FillStyle ima)
{
	buttonFillIma = ima;
	this->dirty = true;
}

void Button::setFillIma(std::string imaNAme)
{
	if (buttonFileIMAGE)
	{
		delete buttonFileIMAGE;
		buttonFileIMAGE = nullptr;
	}
	buttonFileIMAGE = new IMAGE;
	loadimage(buttonFileIMAGE, imaNAme.c_str(), width, height);
	this->dirty = true;
}


void Button::setButtonBorder(COLORREF Border)
{
	buttonBorderColor = Border;
	this->dirty = true;
}

void Button::setButtonFalseColor(COLORREF color)
{
	this->buttonFalseColor = color;
	this->dirty = true;
}

void Button::setButtonText(const char* text)
{
	this->text = std::string(text);
	this->text_width = textwidth(LPCTSTR(this->text.c_str()));
	this->text_height = textheight(LPCTSTR(this->text.c_str()));
	this->dirty = true;
	this->needCutText = true;
	if (!tipUserOverride)
		tipTextClick = tipTextOn = tipTextOff = text;
}

void Button::setButtonText(std::string text)
{
	this->text = text;
	this->text_width = textwidth(LPCTSTR(this->text.c_str()));
	this->text_height = textheight(LPCTSTR(this->text.c_str()));
	this->dirty = true; // 标记需要重绘
	this->needCutText = true;
	if (!tipUserOverride)
		tipTextClick = tipTextOn = tipTextOff = text;
}

void Button::setButtonShape(StellarX::ControlShape shape)
{
	this->shape = shape;
	this->dirty = true;
	this->needCutText = true;
}

//允许通过外部函数修改按钮的点击状态，并执行相应的回调函数
void Button::setButtonClick(BOOL click)
{
	this->click = click;

	if (mode == StellarX::ButtonMode::NORMAL && click)
	{
		if (onClickCallback) onClickCallback();
		dirty = true;
		hideTooltip();
		// 清除消息队列中积压的鼠标和键盘消息，防止本次点击事件被重复处理
		flushmessage(EX_MOUSE | EX_KEY);
	}
	else if (mode == StellarX::ButtonMode::TOGGLE)
	{
		if (click && onToggleOnCallback) onToggleOnCallback();
		else if (!click && onToggleOffCallback) onToggleOffCallback();
		dirty = true;
		refreshTooltipTextForState();
		hideTooltip();
		// 清除消息队列中积压的鼠标和键盘消息，防止本次点击事件被重复处理
		flushmessage(EX_MOUSE | EX_KEY);
	}
	if (dirty)
		requestRepaint(parent);
}


std::string Button::getButtonText() const
{
	return this->text;
}

const char* Button::getButtonText_c() const
{
	return this->text.c_str();
}

StellarX::ButtonMode Button::getButtonMode() const
{
	return this->mode;
}

StellarX::ControlShape Button::getButtonShape() const
{
	return this->shape;
}

StellarX::FillMode Button::getFillMode() const
{
	return this->buttonFillMode;
}

StellarX::FillStyle Button::getFillIma() const
{
	return this->buttonFillIma;
}

IMAGE* Button::getFillImaImage() const
{
	return this->buttonFileIMAGE;
}

COLORREF Button::getButtonBorder() const
{
	return this->buttonBorderColor;
}

COLORREF Button::getButtonTextColor() const
{
	return this->textStyle.color;
}

StellarX::ControlText Button::getButtonTextStyle() const
{
	return this->textStyle;
}

int Button::getButtonWidth() const
{
	return this->width;
}

int Button::getButtonHeight() const
{
	return this->height;
}



bool Button::isMouseInCircle(int mouseX, int mouseY, int x, int y, int radius)
{
	double dis = sqrt(pow(mouseX - x, 2) + pow(mouseY - y, 2));
	if (dis <= radius)
		return true;
	else
		return false;
}

bool Button::isMouseInEllipse(int mouseX, int mouseY, int x, int y, int width, int height)
{
	int centerX = (x + width) / 2;
	int centerY = (y + height) / 2;
	int majorAxis = (width - x) / 2;
	int minorAxis = (height - y) / 2;
	double dx = mouseX - centerX;
	double dy = mouseY - centerY;
	double normalizedDistance = (dx * dx) / (majorAxis * majorAxis) + (dy * dy) / (minorAxis * minorAxis);

	// 判断鼠标是否在椭圆内
	if (normalizedDistance <= 1.0)
		return true;
	else
		return false;
}

void Button::cutButtonText()
{
	const int contentW = 1 > this->width - 2 * padX ? 1 : this->width - 2 * padX;
	// 放得下：不截断，直接用原文
	if (textwidth(LPCTSTR(this->text.c_str())) <= contentW) {
		isUseCutText = false;
		needCutText = false;
		cutText.clear();
		return;
	}

	// 放不下：按语言偏好裁切（ASCII→词边界；CJK→逐字符，不撕裂双字节）
	if (is_ascii_only(this->text))
	{
		cutText = ellipsize_ascii_pref(this->text, contentW);   // "..."
	}
	else
	{
		cutText = ellipsize_cjk_pref(this->text, contentW, "…"); // 全角省略号

	}
	isUseCutText = true;
	needCutText = false;

}

void Button::hideTooltip()
{
	if (tipVisible)
	{
		tipVisible = false;
		tipLabel.hide(); // 还原快照+作废，防止残影
		tipHoverTick = GetTickCount64(); // 重置计时基线
	}
}

void Button::refreshTooltipTextForState()
{
	if (tipUserOverride)   return; // 用户显式设置过 tipText，保持不变
	if (mode == StellarX::ButtonMode::NORMAL)
		tipLabel.setText(tipTextClick);
	else if (mode == StellarX::ButtonMode::TOGGLE)
		tipLabel.setText(click ? tipTextOn : tipTextOff);
}



