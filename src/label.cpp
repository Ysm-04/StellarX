#include "Label.h"

Label::Label()
	:Control(0, 0, 0, 0)
{
	this->text = "默认标签";
	textColor = RGB(0,0,0);
	textBkColor = RGB(255, 255, 255);; //默认白色背景
}

Label::Label(int x, int y, std::string text, COLORREF textcolor, COLORREF bkColor)
	:Control(x, y, 0, 0)
{
	this->text = text;
	textColor = textcolor;
	textBkColor = bkColor; //默认白色背景
}

void Label::draw()
{
	if(dirty)
	{
		saveStyle();
		if (textBkDisap)
			setbkmode(TRANSPARENT); //设置背景透明
		else
		{
			setbkmode(OPAQUE); //设置背景不透明
			setbkcolor(textBkColor); //设置背景颜色
		}
		settextcolor(textColor);
		settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.lpszFace,
			textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
			textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);   //设置字体样式
		outtextxy(x, y, LPCTSTR(text.c_str()));
		restoreStyle();
		dirty = false;
	}
}

void Label::setTextdisap(bool key)
{
	textBkDisap = key;
	this->dirty = true;
}

void Label::setTextColor(COLORREF color)
{
	textColor = color;
	this->dirty = true;
}

void Label::setTextBkColor(COLORREF color)
{
	textBkColor = color;
	this->dirty = true;
}

void Label::setText(std::string text)
{
	this->text = text;
	this->dirty = true;
}
