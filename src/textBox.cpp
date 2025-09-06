// TxtBox.cpp
#include "textBox.h"


textBox::textBox(int x, int y, int width, int height, std::string text, StellarX::textBoxmode mode, StellarX::controlShape shape)
	:Control(x,y,width,height),text(text), mode(mode), shape(shape)
{
}

void textBox::draw()
{
    if(dirty)
    {
        saveStyle();
        setfillcolor(textBoxBkClor);
        setlinecolor(textBoxBorderClor);
        if (textStyle.nHeight > height - y)
            textStyle.nHeight = height;
        if (textStyle.nWidth > width - x)
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
        case StellarX::controlShape::RECTANGLE:
            fillrectangle(x, y, width, height);//有边框填充矩形
            outtextxy(x + 10, (y + (height - y - text_height) / 2), LPCTSTR(text.c_str()));
            break;
        case StellarX::controlShape::B_RECTANGLE:
            solidrectangle(x, y, width, height);//无边框填充矩形
            outtextxy(x + 10, (y + (height - y - text_height) / 2), LPCTSTR(text.c_str()));
            break;
        case StellarX::controlShape::ROUND_RECTANGLE:
            fillroundrect(x, y, width, height, 20, 20);//有边框填充圆角矩形
            outtextxy(x + 10, (y + (height - y - text_height) / 2), LPCTSTR(text.c_str()));
            break;
        case StellarX::controlShape::B_ROUND_RECTANGLE:
            solidroundrect(x, y, width, height, 20, 20);//无边框填充圆角矩形
            outtextxy(x + 10, (y + (height - y - text_height) / 2), LPCTSTR(text.c_str()));
            break;
        }
        
    }
    restoreStyle();
    dirty = false;     //标记不需要重绘
}

void textBox::handleEvent(const ExMessage& msg)
{
    bool hover = false;
    bool oldClick = click;

    switch (shape)
    {
    case StellarX::controlShape::RECTANGLE:
    case StellarX::controlShape::B_RECTANGLE:
    case StellarX::controlShape::ROUND_RECTANGLE:
    case StellarX::controlShape::B_ROUND_RECTANGLE:
        hover = (msg.x > x && msg.x < width && msg.y > y && msg.y < height);//判断鼠标是否在矩形按钮内
        break;
    }
    if (hover && msg.message == WM_LBUTTONUP)
    {
        click = true;
        if(StellarX::textBoxmode::INPUT_MODE == mode)
            dirty =  InputBox(LPTSTR(text.c_str()), maxCharLen,"输入框",NULL,text.c_str(), NULL ,NULL,false);
        else if (StellarX::textBoxmode::READONLY_MODE == mode)
        {
            dirty = false;
            InputBox(NULL, maxCharLen, "输出框（输入无效！）", NULL, text.c_str(), NULL, NULL, false);
        }
        flushmessage(EX_MOUSE | EX_KEY);
    }
    if (dirty)
        draw();

    if (click)
        click = false;
}

void textBox::setMode(StellarX::textBoxmode mode)
{
    this->mode = mode;
}

void textBox::setmaxCharLen(int len)
{
    if (len > 0)
        maxCharLen = len;
}

void textBox::settextBoxshape(StellarX::controlShape shape)
{
    switch (shape)
    {
    case StellarX::controlShape::RECTANGLE:
    case StellarX::controlShape::B_RECTANGLE:
    case StellarX::controlShape::ROUND_RECTANGLE:
    case StellarX::controlShape::B_ROUND_RECTANGLE:
        this->shape = shape;
        break;
	case StellarX::controlShape::CIRCLE:
    case StellarX::controlShape::B_CIRCLE:
    case StellarX::controlShape::ELLIPSE:
    case StellarX::controlShape::B_ELLIPSE:
        this->shape = StellarX::controlShape::RECTANGLE;
        break;
    }
}

void textBox::settextBoxBorder(COLORREF color)
{
    textBoxBorderClor = color;
}

void textBox::settextBoxBk(COLORREF color)
{
    textBoxBkClor = color;
}

std::string textBox::gettext()
{
    return this->text;
}


