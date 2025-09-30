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

void Button::initButton(const std::string text, StellarX::ButtonMode mode, StellarX::ControlShape shape, COLORREF ct, COLORREF cf, COLORREF ch)
{
	this->text = text;
	this->mode = mode;
	this->shape = shape;
	this->buttonTrueColor = ct;
	this->buttonFalseColor = cf;
	this->buttonHoverColor = ch;
	this->click = false;
	this->hover = false;
}


Button::~Button()
{
    if (buttonFileIMAGE) 
        delete buttonFileIMAGE;
	buttonFileIMAGE = nullptr;
}

void Button::draw()
{
	if (dirty)
	{
		//保存当前样式和颜色
		saveStyle();

		if (StellarX::ButtonMode::DISABLED == mode)   //设置禁用按钮色
		{
			setfillcolor(RGB(96, 96, 96));
			textStyle.bStrikeOut = 1;
		}
		else
		{
			// 点击状态优先级最高，然后是悬停状态，最后是默认状态
			if (click)
				setfillcolor(buttonTrueColor);
			else if (hover)
				setfillcolor(buttonHoverColor);
			else
				setfillcolor(buttonFalseColor);
		}

		//设置字体背景色透明
		setbkmode(TRANSPARENT);
		//边框颜色
		setlinecolor(buttonBorderColor);
		if (this->textStyle != oldStyle)
		{
			//设置字体颜色
			settextcolor(textStyle.color);

			//设置字体样式
			settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.lpszFace,
				textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
				textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);   //设置字体样式
		}
		//设置按钮填充模式
		setfillstyle((int)buttonFillMode, (int)buttonFillIma, buttonFileIMAGE);

		//获取字符串像素高度和宽度
		if ((this->oldtext_width != this->text_width || this->oldtext_height != this->text_height)
			|| (-1 == oldtext_width && oldtext_height == -1))
		{
			this->oldtext_width = this->text_width = textwidth(LPCTSTR(this->text.c_str()));
			this->oldtext_height = this->text_height = textheight(LPCTSTR(this->text.c_str()));
		}


		//根据按钮形状绘制
		switch (shape)
		{
		case StellarX::ControlShape::RECTANGLE:
			fillrectangle(x, y, x + width, y + height);//有边框填充矩形
			outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(text.c_str()));
			break;
		case StellarX::ControlShape::B_RECTANGLE:
			solidrectangle(x, y, x + width, y + height);//无边框填充矩形
			outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(text.c_str()));
			break;
		case StellarX::ControlShape::ROUND_RECTANGLE:
			fillroundrect(x, y, x + width, y + height, rouRectangleSize.ROUND_RECTANGLEwidth, rouRectangleSize.ROUND_RECTANGLEheight);//有边框填充圆角矩形
			outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(text.c_str()));
			break;
		case StellarX::ControlShape::B_ROUND_RECTANGLE:
			solidroundrect(x, y, x + width, y + height, rouRectangleSize.ROUND_RECTANGLEwidth, rouRectangleSize.ROUND_RECTANGLEheight);//无边框填充圆角矩形
			outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(text.c_str()));
			break;
		case StellarX::ControlShape::CIRCLE:
			fillcircle(x + width / 2, y + height / 2, min(width, height) / 2);//有边框填充圆形
			outtextxy(x + width / 2 - text_width / 2, y + height / 2 - text_height / 2, LPCTSTR(text.c_str()));
			break;
		case StellarX::ControlShape::B_CIRCLE:
			solidcircle(x + width / 2, y + height / 2, min(width, height) / 2);//无边框填充圆形
			outtextxy(x + width / 2 - text_width / 2, y + height / 2 - text_height / 2, LPCTSTR(text.c_str()));
			break;
		case StellarX::ControlShape::ELLIPSE:
			fillellipse(x, y, x + width, y + height);//有边框填充椭圆
			outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(text.c_str()));
			break;
		case StellarX::ControlShape::B_ELLIPSE:
			solidellipse(x, y, x + width, y + height);//无边框填充椭圆
			outtextxy((x + (width - text_width) / 2), (y + (height - text_height) / 2), LPCTSTR(text.c_str()));
			break;
		}

		restoreStyle();//恢复默认字体样式和颜色

		dirty = false;     //标记按钮不需要重绘
	}
}
// 处理鼠标事件，检测点击和悬停状态
// 根据按钮模式和形状进行不同的处理
bool Button::handleEvent(const ExMessage& msg)
{
	bool oldHover = hover;
	bool oldClick = click;
	bool consume = false;//是否消耗事件

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
		if (mode == StellarX::ButtonMode::NORMAL && click)
		{
			if (onClickCallback) onClickCallback();
			click = false;
			dirty = true;
			consume = true;
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
		{
			dirty = true;
		}
	}

	// 如果状态发生变化，标记需要重绘
	if (hover != oldHover || click != oldClick)
	{
		dirty = true;
	}

	// 如果需要重绘，立即执行
	if (dirty)
	{
		draw();
	}
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
	//取值范围参考 buttMode的枚举注释
    this->mode = mode;
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
    loadimage(buttonFileIMAGE, imaNAme.c_str(),width,height);
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
}

void Button::setButtonText(std::string text)
{
	this->text = text;
	this->text_width = textwidth(LPCTSTR(this->text.c_str()));
	this->text_height = textheight(LPCTSTR(this->text.c_str()));
	this->dirty = true; // 标记需要重绘
}

void Button::setButtonShape(StellarX::ControlShape shape)
{
    this->shape = shape;
	this->dirty = true;
}

//允许通过外部函数修改按钮的点击状态，并执行相应的回调函数
void Button::setButtonClick(BOOL click)
{
	this->click = click;
	
	if (mode == StellarX::ButtonMode::NORMAL && click)
	{
		if (onClickCallback) onClickCallback();
		dirty = true;
		// 清除消息队列中积压的鼠标和键盘消息，防止本次点击事件被重复处理
		flushmessage(EX_MOUSE | EX_KEY);
	}
	else if (mode == StellarX::ButtonMode::TOGGLE)
	{
		if (click && onToggleOnCallback) onToggleOnCallback();
		else if (!click && onToggleOffCallback) onToggleOffCallback();
		dirty = true;
		// 清除消息队列中积压的鼠标和键盘消息，防止本次点击事件被重复处理
		flushmessage(EX_MOUSE | EX_KEY);
	}
	if (dirty)
		draw();
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

int Button::getButtonX() const
{
	return this->x;
}

int Button::getButtonY() const
{
	return this->y;
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


