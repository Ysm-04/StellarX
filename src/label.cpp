#include "Label.h"

Label::Label()
	:Control(0, 0, 0, 0)
{
	this->id = "Label";
	this->text = "默认标签";
	textStyle.color = RGB(0,0,0);
	textBkColor = RGB(255, 255, 255);; //默认白色背景
}

Label::Label(int x, int y, std::string text, COLORREF textcolor, COLORREF bkColor)
	:Control(x, y, 0, 0)
{
	this->id = "Label";
	this->text = text;
	textStyle.color = textcolor;
	textBkColor = bkColor; //默认白色背景
}

void Label::draw()
{
	if (dirty && show)
	{
		saveStyle();
		if (textBkDisap)
			setbkmode(TRANSPARENT); //设置背景透明
		else
		{
			setbkmode(OPAQUE); //设置背景不透明
			setbkcolor(textBkColor); //设置背景颜色
		}
		settextcolor(textStyle.color);
		settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.lpszFace,
			textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
			textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);   //设置字体样式
		if (0 == this->width || 0 == this->height)
		{
			this->width = textwidth(text.c_str());
			this->height = textheight(text.c_str());
		}
		if ((saveBkX != this->x) || (saveBkY != this->y) || (!hasSnap) || (saveWidth != this->width) || (saveHeight != this->height) || !saveBkImage)
			saveBackground(this->x, this->y,this->width,this->height);
		// 恢复背景（清除旧内容）
		restBackground();
		outtextxy(x, y, LPCTSTR(text.c_str()));
		restoreStyle();
		dirty = false;
	}
}
//用于“隐藏提示框”时调用（还原并释放快照）
void Label::hide()
{
	restBackground();    // 还原屏幕像素
	discardBackground(); // 作废快照，防止错贴旧图
	dirty = false;
}
void Label::setTextdisap(bool key)
{
	textBkDisap = key;
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
