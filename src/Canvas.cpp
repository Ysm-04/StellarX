#include "Canvas.h"

Canvas::Canvas()
	:Control(0, 0, 100, 100)
{
	this->id = "Canvas";
}

Canvas::Canvas(int x, int y, int width, int height)
	:Control(x, y, width, height) 
{
	this->id = "Canvas";
}

void Canvas::clearAllControls()
{
	controls.clear();
}


void Canvas::draw()
{
	if (!dirty||!show)return;
	saveStyle();

	setlinecolor(canvasBorderClor);//设置线色
	setfillcolor(canvasBkClor);//设置填充色
	setfillstyle((int)canvasFillMode);//设置填充模式
	setlinestyle((int)canvasLineStyle, canvaslinewidth);
	
	if ((saveBkX != this->x) || (saveBkY != this->y) || (!hasSnap) || (saveWidth != this->width) || (saveHeight != this->height) || !saveBkImage)
		saveBackground(x, y, width, height);
	// 恢复背景（清除旧内容）
	restBackground();
	//根据画布形状绘制
	switch (shape)
	{
	case StellarX::ControlShape::RECTANGLE:
		fillrectangle(x,y,x+width,y+height);//有边框填充矩形
		break;
	case StellarX::ControlShape::B_RECTANGLE:
		solidrectangle(x, y, x + width, y + height);//无边框填充矩形
		break;
	case StellarX::ControlShape::ROUND_RECTANGLE:
		fillroundrect(x, y, x + width, y + height, rouRectangleSize.ROUND_RECTANGLEwidth, rouRectangleSize.ROUND_RECTANGLEheight);//有边框填充圆角矩形
		break;
	case StellarX::ControlShape::B_ROUND_RECTANGLE:
		solidroundrect(x, y, x + width, y + height, rouRectangleSize.ROUND_RECTANGLEwidth, rouRectangleSize.ROUND_RECTANGLEheight);//无边框填充圆角矩形
		break;
	}
	// 绘制所有子控件
	for (auto& control : controls)
	{
		control->setDirty(true);
		control->draw();
	}
	
	restoreStyle();
	dirty = false;	 //标记画布不需要重绘
}

bool Canvas::handleEvent(const ExMessage& msg)
{
	if (!show)return false;
	bool consumed = false;
	bool anyDirty = false;

	for (auto it = controls.rbegin(); it != controls.rend(); ++it)
	{
		consumed |= it->get()->handleEvent(msg);
		if (it->get()->isDirty()) anyDirty = true;
	}
	if (anyDirty) requestRepaint(parent);
	return consumed;
}

void Canvas::addControl(std::unique_ptr<Control> control)
{
	//坐标转化
	control->setX(control->getLocalX() + this->x);
	control->setY(control->getLocalY() + this->y);
	control->setParent(this);
	controls.push_back(std::move(control));
	dirty = true;
}

void Canvas::setShape(StellarX::ControlShape shape)
{
	switch (shape)
	{
	case StellarX::ControlShape::RECTANGLE:
	case StellarX::ControlShape::B_RECTANGLE:
	case StellarX::ControlShape::ROUND_RECTANGLE:
	case StellarX::ControlShape::B_ROUND_RECTANGLE:
		this->shape = shape;
		dirty = true;
		break;
	case StellarX::ControlShape::CIRCLE:
	case StellarX::ControlShape::B_CIRCLE:
	case StellarX::ControlShape::ELLIPSE:
	case StellarX::ControlShape::B_ELLIPSE:
		this->shape = StellarX::ControlShape::RECTANGLE;
		dirty = true;
		break;
	}
}

void Canvas::setCanvasfillMode(StellarX::FillMode mode)
{
	this->canvasFillMode = mode;
	dirty = true;
}

void Canvas::setBorderColor(COLORREF color)
{
	this->canvasBorderClor = color;
	dirty = true;
}

void Canvas::setCanvasBkColor(COLORREF color)
{
	this->canvasBkClor = color;	
	dirty = true;
}

void Canvas::setCanvasLineStyle(StellarX::LineStyle style)
{
	this->canvasLineStyle = style;
	dirty = true;
}


void Canvas::setLinewidth(int width)
{
	this->canvaslinewidth = width;
	dirty = true;
}

void Canvas::setIsVisible(bool visible)
{
	this->show = visible;
	dirty = true;
	for (auto& control : controls)
	{
		control->setIsVisible(visible);
		control->setDirty(true);
	}
	if (!visible)
		this->updateBackground();
}

void Canvas::setDirty(bool dirty)
{
	this->dirty = dirty;
	for(auto& control : controls)
		control->setDirty(dirty);
}

void Canvas::onWindowResize()
{
	Control::onWindowResize();          // 先处理自己
	for (auto& ch : controls)          // 再转发给所有子控件
		ch->onWindowResize();
}

void Canvas::requestRepaint(Control* parent)
{
	if (this == parent)
	{
		for (auto& control : controls)
			if (control->isDirty() && control->IsVisible())
				control->draw();
	}
	else
		onRequestRepaintAsRoot();
}



