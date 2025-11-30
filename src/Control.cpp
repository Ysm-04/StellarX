#include "Control.h"
#include<assert.h>

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
void Control::setIsVisible(bool show)
{
	if (!show)
		this->updateBackground();
	this->show = show;
}
void Control::onWindowResize()
{
	// 自己：丢快照 + 标脏
	discardBackground();
	setDirty(true);
}
void Control::setLayoutMode(StellarX::LayoutMode layoutMode_)
{
	this->layoutMode = layoutMode_;
}
void Control::setAnchor(StellarX::Anchor anchor_1, StellarX::Anchor anchor_2)
{
	this->anchor_1 = anchor_1;
	this->anchor_2 = anchor_2;
}
StellarX::Anchor Control::getAnchor_1() const
{
	return this->anchor_1;
}
StellarX::Anchor Control::getAnchor_2() const
{
	return this->anchor_2;
}
StellarX::LayoutMode Control::getLayoutMode() const
{
	return this->layoutMode;
}
// 保存当前的绘图状态（字体、颜色、线型等）
// 在控件绘制前调用，确保不会影响全局绘图状态
void Control::saveStyle()
{

	gettextstyle(currentFont); // 获取当前字体样式
	*currentColor = gettextcolor(); // 获取当前字体颜色
	*currentBorderColor = getlinecolor(); //保存当前边框颜色
	getlinestyle(currentLineStyle); //保存当前线型
	*currentBkColor = getfillcolor(); //保存当前填充色
}
// 恢复之前保存的绘图状态
// 在控件绘制完成后调用，恢复全局绘图状态
void Control::restoreStyle()
{
	settextstyle(currentFont); // 恢复默认字体样式
	settextcolor(*currentColor); // 恢复默认字体颜色
	setfillcolor(*currentBkColor);
	setlinestyle(currentLineStyle);
	setlinecolor(*currentBorderColor);
	setfillstyle(BS_SOLID);//恢复填充
}

void Control::requestRepaint(Control* parent)
{
	if (parent) parent->requestRepaint(parent);   // 向上冒泡
	else        onRequestRepaintAsRoot();   // 到根控件/窗口兜底
}

void Control::onRequestRepaintAsRoot()
{

	discardBackground();
	setDirty(true);
	draw();    // 只有“无父”时才允许立即画，不会被谁覆盖
}

void Control::saveBackground(int x, int y, int w, int h)
{
	
	if (w <= 0 || h <= 0) return;
	saveBkX = x; saveBkY = y; saveWidth = w; saveHeight = h;
	if (saveBkImage)
	{
		//尺寸变了才重建，避免反复 new/delete
		if (saveBkImage->getwidth() != w || saveBkImage->getheight() != h)
		{
			delete saveBkImage; saveBkImage = nullptr;
		}
	}
	if (!saveBkImage) saveBkImage = new IMAGE(w, h);

	SetWorkingImage(nullptr);                 // ★抓屏幕
	getimage(saveBkImage, x, y, w, h);
	hasSnap = true;
}

void Control::restBackground()
{
	if (!hasSnap || !saveBkImage) return;
	// 直接回贴屏幕（与抓取一致）
	SetWorkingImage(nullptr);
	putimage(saveBkX, saveBkY, saveBkImage);
}

void Control::discardBackground()
{
	if (saveBkImage)
	{
		restBackground();
		delete saveBkImage;
		saveBkImage = nullptr;
	}
	hasSnap = false; saveWidth = saveHeight = 0;
}

void Control::updateBackground()
{
	restBackground();
	discardBackground();
}
