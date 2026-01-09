#include "Control.h"
#include "SxLog.h"
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
	SX_LOGD("Control") << SX_T("重置可见状态: id=", "setIsVisible: id=")
		<< id
		<< " show=" << (show ? 1 : 0);

	if (this->show == show)
		return;

	this->show = show;
	this->dirty = true;

	if (!show)
	{
		// 隐藏：擦除自己在屏幕上的内容，并释放快照
		this->updateBackground();
		return;
	}

	// 显示：不在这里 requestRepaint（避免父容器快照未就绪时子控件抢跑 draw，污染快照）
	// 仅向上标脏，让事件收口阶段由容器统一重绘。
	if (parent)
		parent->setDirty(true);
}

void Control::onWindowResize()
{
	SX_LOGD("Layout") << SX_T("尺寸变化：id=", "onWindowResize: id=") << id
		<< SX_T(" -> 丢背景快照 + 标脏", " -> discardSnap + dirty");

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
	// 说明：
	// - 常规路径：子控件调用 requestRepaint(this->parent)，然后 parent 负责局部重绘（Canvas/TabControl override）
	// - 兜底路径：如果某个“容器控件”没 override requestRepaint，就会出现 parent==this 的递归风险
	//   此时我们改为向更上层冒泡，直到根重绘。
	if (parent == this)
	{
		SX_LOGW("Dirty")
			<< SX_T("requestRepaint（默认容器兜底）：id=", "requestRepaint(default-container-fallback): id=")
			<< id
			<< SX_T("，parent==this，向上层 parent 继续冒泡", " parent==this, bubble to upper parent");

		if (this->parent) this->parent->requestRepaint(this->parent);
		else onRequestRepaintAsRoot();
		return;
	}

	SX_LOGD("Dirty") << SX_T("请求重绘：id=","requestRepaint: id=") << id << " parent=" << (parent ? parent->getId() : "null");

	if (parent) parent->requestRepaint(parent);   // 交给容器处理（容器可局部重绘）
	else        onRequestRepaintAsRoot();         // 根兜底
}

void Control::onRequestRepaintAsRoot()
{
	SX_LOGI("Dirty")
		<< SX_T("触发根重绘：id=", "onRequestRepaintAsRoot: id=") << id
		<< SX_T("（从根节点开始重画）", " (root repaint)");


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
			SX_LOGD("Snap") <<SX_T("重新保存背景快照：id=", "saveBackground rebuild: id=") << id << " size=(" << w << "x" << h << ")";

			delete saveBkImage; saveBkImage = nullptr;
		}
	}
	else
		SX_LOGD("Snap") << SX_T("保存背景快照：id=", "saveBackground rebuild: id=") << id << " size=(" << w << "x" << h << ")";
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
		SX_LOGD("Snap") << SX_T("丢弃背景快照：id=","discardBackground: id=") << id << " hasSnap=" << (hasSnap ? 1 : 0);
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
