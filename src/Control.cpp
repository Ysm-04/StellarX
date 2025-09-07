#include "StellarX/Control.h"

StellarX::ControlText& StellarX::ControlText::operator=(const ControlText& text)
{
	{
		nHeight = text.nHeight;
		nWidth = text.nWidth;
		lpszFace = text.lpszFace;
		color = text.color;
		nEscapement = text.nEscapement;
		nOrientation = text.nOrientation;
		nWeight = text.nWeight;
		bItalic = text.bItalic;
		bUnderline = text.bUnderline;
		bStrikeOut = text.bStrikeOut;
		return *this;
	}
}

bool StellarX::ControlText::operator!=(const ControlText& text)
{
	if(nHeight != text.nHeight)
		return true;
	else if (nWidth != text.nWidth)
		return true;
	else if (lpszFace != text.lpszFace)
		return true;
	else if (color != text.color)
		return true;
	else if (nEscapement != text.nEscapement)
		return true;
	else if (nOrientation != text.nOrientation)
		return true;
	else if (nWeight != text.nWeight)
		return true;
	else if (bItalic != text.bItalic)
		return true;
	else if (bUnderline != text.bUnderline)
		return true;
	else if (bStrikeOut != text.bStrikeOut)
		return true;
	return false;
}
// 保存当前的绘图状态（字体、颜色、线型等）
// 在控件绘制前调用，确保不会影响全局绘图状态
void Control::saveStyle()
{
	gettextstyle(&currentFont); // 获取当前字体样式
	currentColor = gettextcolor(); // 获取当前字体颜色
	currentBorderColor = getlinecolor(); //保存当前边框颜色
	getlinestyle(currentLineStyle); //保存当前线型
	currentBkColor = getfillcolor(); //保存当前填充色
}
// 恢复之前保存的绘图状态
// 在控件绘制完成后调用，恢复全局绘图状态
void Control::restoreStyle()
{
	settextstyle(&currentFont); // 恢复默认字体样式
	settextcolor(currentColor); // 恢复默认字体颜色
	setfillcolor(currentBkColor);
	setlinestyle(currentLineStyle);
	setlinecolor(currentBorderColor);
	setfillstyle(BS_SOLID);//恢复填充
}
