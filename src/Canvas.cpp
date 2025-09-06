#include "Canvas.h"

Canvas::Canvas(int x, int y, int width, int height)
	:Control(x, y, width, height) {}

void Canvas::draw()
{
	saveStyle();

	setlinecolor(canvasBorderClor);//设置线色
	setfillcolor(canvasBkClor);//设置填充色
	setfillstyle((int)canvasFillMode);//设置填充模式
	setlinestyle((int)canvasLineStyle, canvaslinewidth);
	
	//根据画布形状绘制
	switch (shape)
	{
	case StellarX::controlShape::RECTANGLE:
		fillrectangle(x, y, width, height);//有边框填充矩形
		break;
	case StellarX::controlShape::B_RECTANGLE:
		solidrectangle(x, y, width, height);//无边框填充矩形
		break;
	case StellarX::controlShape::ROUND_RECTANGLE:
		fillroundrect(x, y, width, height, 20, 20);//有边框填充圆角矩形
		break;
	case StellarX::controlShape::B_ROUND_RECTANGLE:
		solidroundrect(x, y, width, height, 20, 20);//无边框填充圆角矩形
		break;
	}
	// 绘制所有子控件
	for (auto& control : controls)
		control->draw();
	
	
	restoreStyle();
}

void Canvas::handleEvent(const ExMessage& msg)
{
	for (auto& control : controls) {
		control->handleEvent(msg);
	}
}

void Canvas::addControl(std::unique_ptr<Control> control)
{
	controls.push_back(std::move(control));
}

void Canvas::setShape(StellarX::controlShape shape)
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

void Canvas::setcanvasfillMode(StellarX::fillMode mode)
{
	this->canvasFillMode = mode;
}

void Canvas::setBorderColor(COLORREF color)
{
	this->canvasBorderClor = color;
}

void Canvas::setCanvasBkColor(COLORREF color)
{
	this->canvasBkClor = color;	
}

void Canvas::setcanvasLineStyle(StellarX::lineStyle style)
{
	this->canvasLineStyle = style;
}


void Canvas::setlinewidth(int width)
{
	this->canvaslinewidth = width;
}

