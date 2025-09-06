#include "button.h"

button::button(int x, int y, int width, int height, const std::string text, StellarX::buttonMode mode, StellarX::controlShape shape)
	: Control(x, y, width, height)
{
	initButton(text, mode, shape, RGB(202, 255, 255), RGB(171, 196, 220), RGB(255, 255, 0));
}

button::button(int x, int y, int width, int height, const std::string text, COLORREF ct, COLORREF cf, StellarX::buttonMode mode, StellarX::controlShape shape)
	: Control(x, y, width, height)
{
	initButton(text, mode, shape, ct, cf, RGB(255, 255, 0));
}

button::button(int x, int y, int width, int height, const std::string text, COLORREF ct, COLORREF cf, COLORREF ch, StellarX::buttonMode mode, StellarX::controlShape shape)
	: Control(x, y, width, height)
{
	initButton(text, mode, shape, ct, cf, ch);
}

void button::initButton(const std::string text, StellarX::buttonMode mode, StellarX::controlShape shape, COLORREF ct, COLORREF cf, COLORREF ch)
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


button::~button()
{
    if (buttonFileIMAGE) 
        delete buttonFileIMAGE;
	buttonFileIMAGE = nullptr;
}

void button::draw()
{
    if (dirty)
    {
        //保存当前样式和颜色
        saveStyle();

        if (StellarX::buttonMode::DISABLED == mode)   //设置禁用按钮色
        {
            setfillcolor(RGB(96, 96, 96));
            textStyle.bStrikeOut = 1;
        }
        else
        {
            if (hover)
                setfillcolor(buttonHoverColor);
            else
                setfillcolor(click ? buttonTrueColor : buttonFalseColor);
        }


        //设置字体背景色透明
        setbkmode(TRANSPARENT);
        //边框颜色
        setlinecolor(buttonBorderColor);
        if(this->textStyle !=oldStyle)
        {
            //设置字体颜色
            settextcolor(textStyle.color);

            //设置字体样式
            settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.lpszFace,
                textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
                textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);   //设置字体样式
        }
        //设置按钮填充模式
        setfillstyle(buttonFillMode, (int)buttonFillIma, buttonFileIMAGE);

        //获取字符串像素高度和宽度
        if((this->oldtext_width!= this->text_width|| this->oldtext_height!= this->text_height)
         ||(-1 == oldtext_width &&oldtext_height == -1))
        {
            this->oldtext_width = this->text_width = textwidth(LPCTSTR(this->text.c_str()));;
            this->oldtext_height = this->text_height = textheight(LPCTSTR(this->text.c_str()));;
		}

        
        //根据按钮形状绘制
        switch (shape)
        {
        case StellarX::controlShape::RECTANGLE:
            fillrectangle(x, y, width, height);//有边框填充矩形
            outtextxy((x + (width - x - text_width) / 2), (y + (height - y - text_height) / 2), LPCTSTR(text.c_str()));
            break;
        case StellarX::controlShape::B_RECTANGLE:
            solidrectangle(x, y, width, height);//无边框填充矩形
            outtextxy((x + (width - x - text_width) / 2), (y + (height - y - text_height) / 2), LPCTSTR(text.c_str()));
            break;
        case StellarX::controlShape::ROUND_RECTANGLE:
            fillroundrect(x, y, width, height, ROUND_RECTANGLEwidth, ROUND_RECTANGLEheight);//有边框填充圆角矩形
            outtextxy((x + (width - x - text_width) / 2), (y + (height - y - text_height) / 2), LPCTSTR(text.c_str()));
            break;
        case StellarX::controlShape::B_ROUND_RECTANGLE:
            solidroundrect(x, y, width, height, ROUND_RECTANGLEwidth, ROUND_RECTANGLEheight);//无边框填充圆角矩形
            outtextxy((x + (width - x - text_width) / 2), (y + (height - y - text_height) / 2), LPCTSTR(text.c_str()));
            break;
        case StellarX::controlShape::CIRCLE:
            fillcircle(x, y, (width - x) / 2);//有边框填充圆形
            outtextxy(x - text_width / 2, y - text_height / 2, LPCTSTR(text.c_str()));
            break;
        case StellarX::controlShape::B_CIRCLE:
            solidcircle(x, y, (width - x) / 2);//无边框填充圆形
            outtextxy(x - text_width / 2, y - text_height / 2, LPCTSTR(text.c_str()));
            break;
        case StellarX::controlShape::ELLIPSE:
            fillellipse(x, y, width, height);//有边框填充椭圆
            outtextxy((x + (width - x - text_width) / 2), (y + (height - y - text_height) / 2), LPCTSTR(text.c_str()));
            break;
        case StellarX::controlShape::B_ELLIPSE:
            solidellipse(x, y, width, height);//无边框填充椭圆
            outtextxy((x + (width - x - text_width) / 2), (y + (height - y - text_height) / 2), LPCTSTR(text.c_str()));
            break;
        }

        

        restoreStyle();//恢复默认字体样式和颜色

        dirty = false;     //标记按钮不需要重绘
    }
        
}
// 处理鼠标事件，检测点击和悬停状态
// 根据按钮模式和形状进行不同的处理
void button::handleEvent(const ExMessage& msg)
{
    bool oldHover = hover;
    bool oldClick = click;
    // 检测悬停状态（根据不同形状）
    switch (shape)
    {
    case StellarX::controlShape::RECTANGLE:
    case StellarX::controlShape::B_RECTANGLE:
    case StellarX::controlShape::ROUND_RECTANGLE:
    case StellarX::controlShape::B_ROUND_RECTANGLE:
        hover = (msg.x > x && msg.x < width && msg.y > y && msg.y < height);//判断鼠标是否在矩形按钮内
        break;
    case StellarX::controlShape::CIRCLE:
    case StellarX::controlShape::B_CIRCLE:
        hover = isMouseInCircle(msg.x, msg.y, x, y, (width - x) / 2);//判断鼠标是否在圆形按钮内
        break;
    case StellarX::controlShape::ELLIPSE:
    case StellarX::controlShape::B_ELLIPSE:
        hover = isMouseInEllipse(msg.x, msg.y, x, y, width, height);//判断鼠标是否在椭圆按钮内
        break;

    }

    if (hover && msg.message == WM_LBUTTONUP) 
    {
        if (mode == StellarX::buttonMode::NORMAL)    // 普通按钮 NORMAL
        {
            click = true;
            if (onClickCallback) onClickCallback();
            // 刷新消息队列，避免重复处理
            flushmessage(EX_MOUSE | EX_KEY);
        }
        else if (mode == StellarX::buttonMode::TOGGLE) // 切换状态  Toggle
        {
            click = !click;
            if (click && onToggleOnCallback) onToggleOnCallback();
            else if (!click && onToggleOffCallback) onToggleOffCallback();
            flushmessage(EX_MOUSE | EX_KEY);
        }
        else if (mode == StellarX::buttonMode::DISABLED) // 禁用状态 Disabled
            flushmessage(EX_MOUSE | EX_KEY);
    }
    if (hover != oldHover || click != oldClick)
        dirty = true; // 标记按钮需要重绘
   
    if (dirty) 
        draw();

    if (StellarX::buttonMode::NORMAL == mode)
        if (isClicked())
            click = false;
}

void button::setOnClickListener(const std::function<void()>&& callback)
{
    this->onClickCallback = callback;
}

void button::setOnToggleOnListener(const std::function<void()>&& callback)
{
    this->onToggleOnCallback = callback;
}
void button::setOnToggleOffListener(const std::function<void()>&& callback)
{
	this->onToggleOffCallback = callback;
}

void button::setbuttonMode(StellarX::buttonMode mode)
{
	//取值范围参考 buttMode的枚举注释
    this->mode = mode;
}

int button::setROUND_RECTANGLEwidth(int width)
{
    return ROUND_RECTANGLEwidth = width;
}

int button::setROUND_RECTANGLEheight(int height)
{
    return ROUND_RECTANGLEheight = height;
}

bool button::isClicked() const
{
    return this->click;
}

void button::setFillMode(int mode)
{
    buttonFillMode = mode;
}

void button::setFillIma(StellarX::fillStyle ima)
{
    buttonFillIma = ima;
}

void button::setFillIma(std::string imaNAme)
{
    buttonFileIMAGE = new IMAGE;
    loadimage(buttonFileIMAGE, imaNAme.c_str(),width-x,height-y);
}


void button::setbuttonBorder(COLORREF Border)
{
    buttonBorderColor = Border;
}

void button::setbuttonText(const char* text)
{
    this->text = std::string(text);
	this->text_width = textwidth(LPCTSTR(this->text.c_str()));
	this->text_height = textheight(LPCTSTR(this->text.c_str()));
}

void button::setbuttonText(std::string text)
{
	this->text = text;
	this->text_width = textwidth(LPCTSTR(this->text.c_str()));
	this->text_height = textheight(LPCTSTR(this->text.c_str()));
	this->dirty = true; // 标记需要重绘
}

void button::setbuttonShape(StellarX::controlShape shape)
{
    this->shape = shape;
}


std::string button::getbuttonText() const
{
    return this->text;
}

const char* button::getbuttonText_c() const
{
    return this->text.c_str();
}

StellarX::buttonMode button::getbuttonMode() const
{
    return this->mode;
}

StellarX::controlShape button::getbuttonShape() const
{
    return this->shape;
}

int button::getFillMode() const
{
    return this->buttonFillMode;
}

StellarX::fillStyle button::getFillIma() const
{
    return this->buttonFillIma;
}

IMAGE* button::getFillImaImage() const
{
    return this->buttonFileIMAGE;
}

COLORREF button::getbuttonBorder() const
{
    return this->buttonBorderColor;
}

COLORREF button::getbuttonTxtColor() const
{
    return this->textStyle.color;
}

StellarX::controlText button::getbuttonTextStyle() const
{
    return this->textStyle;
}


bool button::isMouseInCircle(int mouseX, int mouseY, int x, int y, int radius)
{
    double dis = sqrt(pow(mouseX - x, 2) + pow(mouseY - y, 2));
    if (dis <= radius)
        return true;
    else 
        return false;
}

bool button::isMouseInEllipse(int mouseX, int mouseY, int x, int y, int width, int height)
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


