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
	if (!dirty || !show)
	{
		for (auto& control : controls)
			if (auto c = dynamic_cast<Table*>(control.get()))
				c->draw();
		return;
	}
	saveStyle();
	setlinecolor(canvasBorderClor);//设置线色
    if(StellarX::FillMode::Null != canvasFillMode)
        setfillcolor(canvasBkClor);//设置填充色
	setfillstyle((int)canvasFillMode);//设置填充模式
	setlinestyle((int)canvasLineStyle, canvaslinewidth);
	
    // 在绘制画布之前，先恢复并更新背景快照：
    // 1. 如果已有快照，则先回贴旧快照以清除之前的内容。
    // 2. 当坐标或尺寸变化，或缓存图像无效时，丢弃旧快照并重新抓取新的背景。
    if (hasSnap)
    {
        // 恢复旧快照，清除上一次绘制
        restBackground();
        // 如果位置或尺寸变了，或没有有效缓存，则重新抓取
        if (!saveBkImage || saveBkX != this->x || saveBkY != this->y || saveWidth != this->width || saveHeight != this->height)
        {
            discardBackground();
            saveBackground(this->x, this->y, this->width, this->height);
        }
    }
    else
    {
        // 首次绘制或没有快照时直接抓取背景
        saveBackground(this->x, this->y, this->width, this->height);
    }
    // 再次恢复最新快照，确保绘制区域干净
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
    // 首先处理自身的快照等逻辑
    Control::onWindowResize();

    // 记录父容器原始尺寸（用于计算子控件的右/下边距）
    int origParentW = this->localWidth;
    int origParentH = this->localHeight;

    // 当前容器的新尺寸
    int finalW = this->width;
    int finalH = this->height;

    // 当前容器的新坐标（全局坐标）
    int parentX = this->x;
    int parentY = this->y;

    // 调整每个子控件在 AnchorToEdges 模式下的位置与尺寸
    for (auto& ch : controls)
    {
        // Only adjust when using anchor-to-edges layout
        if (ch->getLayoutMode() == StellarX::LayoutMode::AnchorToEdges)
        {
            // Determine whether this child is a Table; tables keep their height constant
            bool isTable = (dynamic_cast<Table*>(ch.get()) != nullptr);

            // Unpack anchors
            auto a1 = ch->getAnchor_1();
            auto a2 = ch->getAnchor_2();

            bool anchorLeft   = (a1 == StellarX::Anchor::Left   || a2 == StellarX::Anchor::Left);
            bool anchorRight  = (a1 == StellarX::Anchor::Right  || a2 == StellarX::Anchor::Right);
            bool anchorTop    = (a1 == StellarX::Anchor::Top    || a2 == StellarX::Anchor::Top);
            bool anchorBottom = (a1 == StellarX::Anchor::Bottom || a2 == StellarX::Anchor::Bottom);

            // If it's a table, treat as anchored left and right horizontally and anchored top vertically by default.
            if (isTable)
            {
                anchorLeft = true;
                anchorRight = true;
                // If no explicit vertical anchor was provided, default to top.
                if (!(anchorTop || anchorBottom))
                {
                    anchorTop = true;
                }
            }

            // Compute new X and width
            int newX = ch->getX();
            int newWidth = ch->getWidth();
            if (anchorLeft && anchorRight)
            {
                // Scale horizontally relative to parent's size.
                if (origParentW > 0)
                {
                    // Maintain proportional position and size based on original local values.
                    double scaleW = static_cast<double>(finalW) / static_cast<double>(origParentW);
                    newX     = parentX + static_cast<int>(ch->getLocalX() * scaleW + 0.5);
                    newWidth = static_cast<int>(ch->getLocalWidth() * scaleW + 0.5);
                }
                else
                {
                    // Fallback: keep original
                    newX     = parentX + ch->getLocalX();
                    newWidth = ch->getLocalWidth();
                }
            }
            else if (anchorLeft && !anchorRight)
            {
                // Only left anchored: keep original width and left margin.
                newWidth = ch->getLocalWidth();
                newX     = parentX + ch->getLocalX();
            }
            else if (!anchorLeft && anchorRight)
            {
                // Only right anchored: keep original width and right margin.
                newWidth = ch->getLocalWidth();
                int origRightDist = origParentW - (ch->getLocalX() + ch->getLocalWidth());
                newX = parentX + finalW - origRightDist - newWidth;
            }
            else
            {
                // No horizontal anchor: position relative to parent's left and width unchanged.
                newWidth = ch->getLocalWidth();
                newX     = parentX + ch->getLocalX();
            }
            ch->setX(newX);
            ch->setWidth(newWidth);

            // Compute new Y and height
            int newY = ch->getY();
            int newHeight = ch->getHeight();
            if (isTable)
            {
                // Table: Height remains constant; adjust Y based on anchors.
                newHeight = ch->getLocalHeight();
                if (anchorTop && anchorBottom)
                {
                    // If both top and bottom anchored, scale Y but keep height.
                    if (origParentH > 0)
                    {
                        double scaleH = static_cast<double>(finalH) / static_cast<double>(origParentH);
                        newY = parentY + static_cast<int>(ch->getLocalY() * scaleH + 0.5);
                    }
                    else
                    {
                        newY = parentY + ch->getLocalY();
                    }
                }
                else if (anchorTop && !anchorBottom)
                {
                    // Top anchored only
                    newY = parentY + ch->getLocalY();
                }
                else if (!anchorTop && anchorBottom)
                {
                    // Bottom anchored only
                    int origBottomDist = origParentH - (ch->getLocalY() + ch->getLocalHeight());
                    newY = parentY + finalH - origBottomDist - newHeight;
                }
                else
                {
                    // No vertical anchor: default to top
                    newY = parentY + ch->getLocalY();
                }
            }
            else
            {
                if (anchorTop && anchorBottom)
                {
                    // Scale vertically relative to parent's size.
                    if (origParentH > 0)
                    {
                        double scaleH = static_cast<double>(finalH) / static_cast<double>(origParentH);
                        newY      = parentY + static_cast<int>(ch->getLocalY() * scaleH + 0.5);
                        newHeight = static_cast<int>(ch->getLocalHeight() * scaleH + 0.5);
                    }
                    else
                    {
                        newY      = parentY + ch->getLocalY();
                        newHeight = ch->getLocalHeight();
                    }
                }
                else if (anchorTop && !anchorBottom)
                {
                    // Top anchored only: keep height constant
                    newHeight = ch->getLocalHeight();
                    newY      = parentY + ch->getLocalY();
                }
                else if (!anchorTop && anchorBottom)
                {
                    // Bottom anchored only: keep height and adjust Y relative to bottom
                    newHeight = ch->getLocalHeight();
                    int origBottomDist = origParentH - (ch->getLocalY() + ch->getLocalHeight());
                    newY = parentY + finalH - origBottomDist - newHeight;
                }
                else
                {
                    // No vertical anchor: position relative to parent's top, height constant.
                    newHeight = ch->getLocalHeight();
                    newY      = parentY + ch->getLocalY();
                }
            }
            ch->setY(newY);
            ch->setHeight(newHeight);
        }
        // Always forward the window resize event to the child (recursively).
        ch->onWindowResize();
    }
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



