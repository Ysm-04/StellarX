#include "Canvas.h"
#include "SxLog.h"
#include "Window.h"

static bool SxIsNoisyMsg(UINT m)
{
	return m == WM_MOUSEMOVE;
}

static const char* SxCanvasMsgName(UINT m)
{
	switch (m)
	{
	case WM_MOUSEMOVE:   return "WM_MOUSEMOVE";
	case WM_LBUTTONDOWN: return "WM_LBUTTONDOWN";
	case WM_LBUTTONUP:   return "WM_LBUTTONUP";
	case WM_KEYDOWN:     return "WM_KEYDOWN";
	case WM_KEYUP:       return "WM_KEYUP";
	default:             return "WM_UNKNOWN";
	}
}

namespace
{
	enum class SxCanvasOverlayRedrawMode
	{
		None,
		RefreshSnapshot
	};

	bool SxCanvasRectValid(const RECT& rc)
	{
		return rc.right > rc.left && rc.bottom > rc.top;
	}

	bool SxCanvasRectsIntersect(const RECT& a, const RECT& b)
	{
		return a.left < b.right && a.right > b.left &&
			a.top < b.bottom && a.bottom > b.top;
	}

	RECT SxCanvasUnionRect(const RECT& a, const RECT& b)
	{
		RECT out{};
		out.left = (std::min)(a.left, b.left);
		out.top = (std::min)(a.top, b.top);
		out.right = (std::max)(a.right, b.right);
		out.bottom = (std::max)(a.bottom, b.bottom);
		return out;
	}
}

Canvas::Canvas()
	:Control(0, 0, 100, 100)
{
	this->id = "Canvas";
	// Canvas 是通用容器，当前阶段显式允许双轴 Stretch。
	this->layoutCapability.allowStretchX = true;
	this->layoutCapability.allowStretchY = true;
}

Canvas::Canvas(int x, int y, int width, int height)
	:Control(x, y, width, height)
{
	this->id = "Canvas";
	this->layoutCapability.allowStretchX = true;
	this->layoutCapability.allowStretchY = true;
}

void Canvas::relayoutManagedChildren()
{
	// Canvas 负责子控件从“父局部设计矩形”到“当前世界矩形”的转换。
	// 当 Canvas 自己的位置或尺寸变化后，所有受它管理的子控件都要重新走一次统一解算。
	for (auto& child : controls)
	{
		const StellarX::ResolvedLayoutRect rect =
			child->resolveLayoutRect(localWidth, localHeight, x, y, width, height);
		child->applyResolvedLayoutRect(rect);
	}
}

void Canvas::setX(int x)
{
	// 公开 setter 在 Canvas 上不能再视为“单纯改自己的 x”：
	// 一旦容器移动，子控件的世界坐标也必须整体重算。
	applyRuntimeRectDirect(x, y, width, height);
	relayoutManagedChildren();
	onWindowResize();
}

void Canvas::setY(int y)
{
	applyRuntimeRectDirect(this->x, y, width, height);
	relayoutManagedChildren();
	onWindowResize();
}

void Canvas::setWidth(int width)
{
	applyRuntimeRectDirect(x, y, width, height);
	relayoutManagedChildren();
	onWindowResize();
}

void Canvas::setHeight(int height)
{
	applyRuntimeRectDirect(x, y, width, height);
	relayoutManagedChildren();
	onWindowResize();
}

void Canvas::applyResolvedLayoutRect(const StellarX::ResolvedLayoutRect& rect)
{
	// 统一解算器已经给出当前运行态世界矩形；
	// Canvas 在应用自身矩形后，还必须继续刷新全部子控件。
	applyRuntimeRectDirect(rect.worldX, rect.worldY, rect.width, rect.height);
	relayoutManagedChildren();
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
	if (StellarX::FillMode::Null != canvasFillMode)
		setfillcolor(canvasBkClor);//设置填充色
	setfillstyle((int)canvasFillMode);//设置填充模式
	setlinestyle((int)canvasLineStyle, canvaslinewidth);

	// 在绘制画布之前，先恢复并更新背景快照：
	// 1. 如果已有快照，则先回贴旧快照以清除之前的内容。
	// 2. 当坐标或尺寸变化，或缓存图像无效时，丢弃旧快照并重新抓取新的背景。
	int margin = canvaslinewidth > 1 ? canvaslinewidth : 1;
	if (hasSnap)
	{
		// 恢复旧快照，清除上一次绘制
		restBackground();
		// 如果位置或尺寸变了，或没有有效缓存，则重新抓取
		if (!saveBkImage || saveBkX != this->x - margin || saveBkY != this->y - margin || saveWidth != this->width + margin * 2 || saveHeight != this->height + margin * 2)
		{
			invalidateBackgroundSnapshot();
			saveBackground(this->x - margin, this->y - margin, this->width + margin * 2, this->height + margin * 2);
		}
	}
	else
	{
		// 首次绘制或没有快照时直接抓取背景
		saveBackground(this->x - margin, this->y - margin, this->width + margin * 2, this->height + margin * 2);
	}
	// 再次恢复最新快照，确保绘制区域干净
	restBackground();
	//根据画布形状绘制
	switch (shape)
	{
	case StellarX::ControlShape::RECTANGLE:
		fillrectangle(x, y, x + width, y + height);//有边框填充矩形
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
	if (!show) return false;
	resetEventVisualChanged();

	bool consumed = false;
	bool anyDirty = false;
	bool anyVisualChanged = false;
	Control* firstConsumer = nullptr;

	if (msg.message == WM_MOUSEMOVE)
	{
		// WM_MOUSEMOVE 需要特殊处理：
		// - 第一个命中的兄弟分支收到真实消息；
		// - 后续兄弟不再重新命中，只清理旧 hover / tooltip 等临时状态。
		for (auto it = controls.rbegin(); it != controls.rend(); ++it)
		{
			Control* c = it->get();
			if (!consumed)
			{
				bool cConsumed = c->handleEvent(msg);
				if (c->didEventAffectVisual()) anyVisualChanged = true;
				if (cConsumed)
				{
					firstConsumer = c;
					consumed = true;
				}
			}
			else
			{
				// 后续兄弟只走临时状态清理，不会再进入自己的 handleEvent()。
				// Tooltip 隐藏会先回贴旧快照，再改变 coverage；因此必须先保存旧覆盖范围，
				// 避免登记重绘时丢失旧 Tooltip 区域，导致上层 overlay 补画判断不完整。
				const RECT previousCoverage = c->getManagedRepaintCoverageRect();
				if (c->clearTransientMouseState())
				{
					if (Window* host = getHostWindow())
						host->requestManagedRepaint(c, previousCoverage);
					anyVisualChanged = true;
				}
			}
		}
	}
	else
	{
		for (auto it = controls.rbegin(); it != controls.rend(); ++it)
		{
			Control* c = it->get();
			bool cConsumed = c->handleEvent(msg);

			if (c->isDirty()) anyDirty = true;
			if (c->didEventAffectVisual()) anyVisualChanged = true;

			if (cConsumed)
			{
				firstConsumer = c;
				consumed = true;
				break;
			}
		}
	}

	if (firstConsumer && !SxIsNoisyMsg(msg.message))
	{
		SX_LOGD("Event") << SX_T("Canvas 消耗消息: ","Canvas consumed: ")
			<< SxCanvasMsgName(msg.message)
			<< SX_T(" 子控件 id=", " childId=") << firstConsumer->getId();
	}

	if (anyDirty)
	{
		// 只要任一子控件因本次事件进入 dirty，就把这笔重绘继续向上汇报。
		// 在托管模式下，这不会立即绘制，而是登记为 Canvas 对应的重绘 root。
		if (!SxIsNoisyMsg(msg.message))
			SX_LOGD("Dirty") << SX_T("Canvas检测有控件为脏状态 -> 请求重绘, ","Canvas anyDirty -> requestRepaint, ")<<"id = " << id;
		requestRepaint(parent);
	}
	markEventVisualChanged(anyVisualChanged);

	return consumed;
}

bool Canvas::clearTransientMouseState()
{
	bool changed = false;
	for (auto it = controls.rbegin(); it != controls.rend(); ++it)
	{
		Control* child = it->get();
		if (!child->IsVisible())
			continue;
		if (child->clearTransientMouseState())
			changed = true;
	}
	return changed;
}


void Canvas::addControl(std::unique_ptr<Control> control)
{
	control->setParent(this);
	// 新子控件加入容器时，立刻按“当前容器运行态矩形”解算一次，
	// 避免后续第一次 draw / resize 前 world 坐标仍停留在设计态。
	const StellarX::ResolvedLayoutRect rect =
		control->resolveLayoutRect(localWidth, localHeight, this->x, this->y, this->width, this->height);
	control->applyResolvedLayoutRect(rect);
	SX_LOGI("Canvas")
		<< SX_T("添加子控件：父=Canvas 子id=", "addControl: parent=Canvas childId=")
		<< control->getId()
		<< SX_T(" 相对坐标=(", " local=(")
		<< control->getLocalX() << "," << control->getLocalY()
		<< SX_T(") 绝对坐标=(", ") abs=(")
		<< control->getX() << "," << control->getY()
		<< ")";


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
	}
	if (!visible)
		discardBackground();
}

void Canvas::setDirty(bool dirty)
{
	this->dirty = dirty;
	for (auto& control : controls)
		control->setDirty(dirty);
}

void Canvas::onWindowResize()
{
	// resize 语义已收口：
	// Canvas 不再在这里重新解算布局，只负责丢快照、标脏，并向子控件传播“环境已变化”。
	Control::onWindowResize();
	for (auto& child : controls)
		child->onWindowResize();
}

void Canvas::requestRepaint(Control* parent)
{
	if (shouldDeferManagedRepaint())
	{
		// 托管路径：由 Window 统一决定这次是否只重画本 Canvas，还是升级为补画 Dialog / 整体场景。
		if (auto* host = getHostWindow())
			host->requestManagedRepaint(this);
		return;
	}

	if (this == parent)
	{
		if (!show)
			return;

		// 关键护栏：
		// - Canvas 自己是脏的 / 没有快照 / 缓存图为空
		//   => 禁止局部重绘，直接升级为一次完整 draw（先把 dirty 置真，避免 draw() 早退）
		if (dirty || !hasSnap || !saveBkImage)
		{
			SX_LOG_TRACE("Dirty")
				<< SX_T("Canvas 局部重绘降级为全量重绘: id=", "Canvas partial->full draw: id=")
				<< id
				<< " dirty=" << (dirty ? 1 : 0)
				<< " hasSnap=" << (hasSnap ? 1 : 0);

			this->dirty = true;
			this->draw();
			return;
		}

		SX_LOG_TRACE("Dirty") << SX_T("Canvas 请求局部重绘：id=", "Canvas::requestRepaint(partial): id=") << id;

		RECT paintCoverage{};
		bool hasPaintCoverage = false;
		RECT persistentCoverage{};
		bool hasPersistentCoverage = false;
		auto commitManagedChild = [&](Control* child, SxCanvasOverlayRedrawMode overlayMode)
			{
				if (!child || !child->IsVisible())
					return;

				const bool directDirty = child->isDirty();
				const bool subtreeDirty = child->hasManagedDirtySubtree();

				if (overlayMode == SxCanvasOverlayRedrawMode::RefreshSnapshot)
				{
					// overlay 补画必须先作废旧快照：
					// 下层兄弟的持久内容刚刚已经写过像素，若继续沿用旧快照，会把旧背景再贴回来。
					child->invalidateBackgroundSnapshot();
					child->setDirty(true);
					child->draw();
				}
				else if (directDirty)
				{
					child->draw();
				}
				else if (subtreeDirty)
				{
					// 这次真正脏的是“child 下面的子树”，而不是 child 自身。
					// 例如嵌套 Canvas 中，第二层/第三层按钮脏了，但第一层 Canvas 自己并不 dirty。
					// 这里必须把这条直接子分支提交下去，否则深层按钮状态永远没有机会被真正画出来。
					child->commitManagedRepaint();
				}
				else
				{
					return;
				}

				const RECT childPaintRect = child->getManagedRepaintCoverageRect();
				if (!hasPaintCoverage)
				{
					paintCoverage = childPaintRect;
					hasPaintCoverage = true;
				}
				else
				{
					paintCoverage = SxCanvasUnionRect(paintCoverage, childPaintRect);
				}

				const RECT childPersistentRect = child->getManagedRepaintPersistentCoverageRect();
				if (!hasPersistentCoverage)
				{
					persistentCoverage = childPersistentRect;
					hasPersistentCoverage = true;
				}
				else
				{
					persistentCoverage = SxCanvasUnionRect(persistentCoverage, childPersistentRect);
				}
			};

		for (auto& control : controls)
		{
			Control* child = control.get();
			if (!child->IsVisible())
				continue;

			if (child->hasManagedDirtySubtree())
			{
				commitManagedChild(child, SxCanvasOverlayRedrawMode::None);
			}
			else if (hasPaintCoverage && SxCanvasRectsIntersect(child->getManagedRepaintCoverageRect(), paintCoverage))
			{
				// 位于本次累计 coverage 上方、且发生相交的兄弟控件，需要补画回最上层。
				// 但只有下层“持久内容”影响到它时，才允许作废并重新抓背景快照；
				// 如果只是被 Tooltip 等临时浮层覆盖，则跳过兄弟补画，避免透明控件回贴旧快照擦掉 Tooltip。
				const bool persistentHit = hasPersistentCoverage &&
					SxCanvasRectsIntersect(child->getManagedRepaintPersistentCoverageRect(), persistentCoverage);
				if (persistentHit)
					commitManagedChild(child, SxCanvasOverlayRedrawMode::RefreshSnapshot);
			}
		}

		return;
	}

	SX_LOG_TRACE("Dirty") << SX_T("Canvas 请求根级重绘：id=", "Canvas::requestRepaint(root): id=") << id;
	onRequestRepaintAsRoot();
}

bool Canvas::hasManagedDirtySubtree() const
{
	if (dirty)
		return true;

	for (const auto& child : controls)
	{
		if (!child->IsVisible())
			continue;
		if (child->hasManagedDirtySubtree())
			return true;
	}
	return false;
}

RECT Canvas::getManagedRepaintCoverageRect() const
{
	// Canvas::draw() 会先写自身背景，再强制绘制全部可见子控件。
	// 因此它的实际 coverage 不能只看本体矩形，还要把可见子控件 coverage 递归并入。
	RECT coverage = getBoundsRect();
	for (const auto& child : controls)
	{
		if (!child->IsVisible())
			continue;
		coverage = SxCanvasUnionRect(coverage, child->getManagedRepaintCoverageRect());
	}
	return coverage;
}

RECT Canvas::getManagedRepaintPersistentCoverageRect() const
{
	// 持久 coverage 只描述会进入背景快照语义的范围。
	// 子控件 Tooltip 等临时浮层不会被并入，避免兄弟控件补画时抓到临时像素。
	RECT coverage = getBoundsRect();
	for (const auto& child : controls)
	{
		if (!child->IsVisible())
			continue;
		coverage = SxCanvasUnionRect(coverage, child->getManagedRepaintPersistentCoverageRect());
	}
	return coverage;
}

bool Canvas::canCommitManagedPartialRepaint() const
{
	// Canvas 只有在“自己本体不脏 + 仍持有有效背景快照”时，
	// 才能安全地做局部提交（即只更新内部脏子控件）。
	return show && !dirty && hasValidBackgroundSnapshot();
}

void Canvas::commitManagedRepaint()
{
	if (!show)
		return;

	if (canCommitManagedPartialRepaint())
	{
		// 快照完好：沿用 Canvas 自己已有的局部重绘逻辑。
		requestRepaint(this);
		return;
	}

	// 自身已经脏了，或快照失效：必须升级为整 Canvas 重画。
	this->dirty = true;
	onRequestRepaintAsRoot();
}

