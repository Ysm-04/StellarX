// TextBox.cpp
#include "TextBox.h"


TextBox::TextBox(int x, int y, int width, int height, std::string text, StellarX::TextBoxmode mode, StellarX::ControlShape shape)
	:Control(x,y,width,height),text(text), mode(mode), shape(shape)
{
}

void TextBox::draw()
{
    if(dirty)
    {
        saveStyle();
        setfillcolor(textBoxBkClor);
        setlinecolor(textBoxBorderClor);
        if (textStyle.nHeight > height)
            textStyle.nHeight = height;
        if (textStyle.nWidth > width)
            textStyle.nWidth = width;
        settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.lpszFace,
            textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
            textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);
        
        settextcolor(textStyle.color);
        setbkmode(TRANSPARENT);
        int text_width = textwidth(LPCTSTR(text.c_str()));
        int text_height = textheight(LPCTSTR(text.c_str()));

     
        //根据形状绘制
        switch (shape)
        {
        case StellarX::ControlShape::RECTANGLE:
            fillrectangle(x,y,x+width,y+height);//有边框填充矩形
            outtextxy(x + 10, (y + (height - text_height) / 2), LPCTSTR(text.c_str()));
            break;
        case StellarX::ControlShape::B_RECTANGLE:
            solidrectangle(x, y, x + width, y + height);//无边框填充矩形
            outtextxy(x + 10, (y + (height - text_height) / 2), LPCTSTR(text.c_str()));
            break;
        case StellarX::ControlShape::ROUND_RECTANGLE:
            fillroundrect(x, y, x + width, y + height, rouRectangleSize.ROUND_RECTANGLEwidth, rouRectangleSize.ROUND_RECTANGLEheight);//有边框填充圆角矩形
            outtextxy(x + 10, (y + (height - text_height) / 2), LPCTSTR(text.c_str()));
            break;
        case StellarX::ControlShape::B_ROUND_RECTANGLE:
            solidroundrect(x, y, x + width, y + height, rouRectangleSize.ROUND_RECTANGLEwidth, rouRectangleSize.ROUND_RECTANGLEheight);//无边框填充圆角矩形
            outtextxy(x + 10, (y + (height - text_height) / 2), LPCTSTR(text.c_str()));
            break;
        }
		restoreStyle();
		dirty = false;     //标记不需要重绘
    }
    
}

bool TextBox::handleEvent(const ExMessage& msg)
{
    bool hover = false;
    bool oldClick = click;
    bool consume = false;

    switch (shape)
    {
    case StellarX::ControlShape::RECTANGLE:
    case StellarX::ControlShape::B_RECTANGLE:
    case StellarX::ControlShape::ROUND_RECTANGLE:
    case StellarX::ControlShape::B_ROUND_RECTANGLE:
        hover = (msg.x > x && msg.x < (x + width) && msg.y > y && msg.y < (y + height));//判断鼠标是否在矩形按钮内
        consume = false;
        break;
    }
    if (hover && msg.message == WM_LBUTTONUP)
    {
        click = true;
        if(StellarX::TextBoxmode::INPUT_MODE == mode)
		{
			dirty = InputBox(LPTSTR(text.c_str()), (int)maxCharLen, "输入框", NULL, text.c_str(), NULL, NULL, false);
			consume = true;
		}
        else if (StellarX::TextBoxmode::READONLY_MODE == mode)
        {
            dirty = false;
            InputBox(NULL, (int)maxCharLen, "输出框（输入无效！）", NULL, text.c_str(), NULL, NULL, false);
            consume = true;
        }
        flushmessage(EX_MOUSE | EX_KEY);
    }
    if (dirty)
        draw();

    if (click)
        click = false;
    return consume;
}

void TextBox::setMode(StellarX::TextBoxmode mode)
{
    this->mode = mode;
    this->dirty = true;
}

void TextBox::setMaxCharLen(size_t len)
{
    if (len > 0)
        maxCharLen = len;
    this->dirty = true;
}

void TextBox::setTextBoxshape(StellarX::ControlShape shape)
{
    switch (shape)
    {
    case StellarX::ControlShape::RECTANGLE:
    case StellarX::ControlShape::B_RECTANGLE:
    case StellarX::ControlShape::ROUND_RECTANGLE:
    case StellarX::ControlShape::B_ROUND_RECTANGLE:
        this->shape = shape;
        this->dirty = true;
        break;
	case StellarX::ControlShape::CIRCLE:
    case StellarX::ControlShape::B_CIRCLE:
    case StellarX::ControlShape::ELLIPSE:
    case StellarX::ControlShape::B_ELLIPSE:
        this->shape = StellarX::ControlShape::RECTANGLE;
        this->dirty = true;
        break;
    }
}

void TextBox::setTextBoxBorder(COLORREF color)
{
    textBoxBorderClor = color;
    this->dirty = true;
}

void TextBox::setTextBoxBk(COLORREF color)
{
    textBoxBkClor = color;
    this->dirty = true;
}

std::string TextBox::getText() const
{
    return this->text;
}


