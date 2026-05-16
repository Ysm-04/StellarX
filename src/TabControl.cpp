#include "TabControl.h"
#include "SxLog.h"
#include "Window.h"

namespace
{
	enum class SxTabOverlayRedrawMode
	{
		None,
		RefreshSnapshot
	};

	bool SxTabRectsIntersect(const RECT& a, const RECT& b)
	{
		return a.left < b.right && a.right > b.left &&
			a.top < b.bottom && a.bottom > b.top;
	}

	RECT SxTabUnionRect(const RECT& a, const RECT& b)
	{
		RECT out{};
		out.left = (std::min)(a.left, b.left);
		out.top = (std::min)(a.top, b.top);
		out.right = (std::max)(a.right, b.right);
		out.bottom = (std::max)(a.bottom, b.bottom);
		return out;
	}
}
inline void TabControl::initTabBar()
{
	if (controls.empty())return;
	int butW = max(this->width / (int)controls.size(), BUTMINWIDTH);
	int butH = max(this->height / (int)controls.size(), BUTMINHEIGHT);

	if (this->tabPlacement == StellarX::TabPlacement::Top || this->tabPlacement == StellarX::TabPlacement::Bottom)
		for (auto& c : controls)
		{
			c.first->setHeight(tabBarHeight);
			c.first->setWidth(butW);
		}
	else if (this->tabPlacement == StellarX::TabPlacement::Left || this->tabPlacement == StellarX::TabPlacement::Right)
		for (auto& c : controls)
		{
			c.first->setHeight(butH);
			c.first->setWidth(tabBarHeight);
		}
	int i = 0;
	switch (this->tabPlacement)
	{
	case StellarX::TabPlacement::Top:
		for (auto& c : controls)
		{
			c.first->setX(this->x + i * butW);
			c.first->setY(this->y);
			i++;
		}
		break;
	case StellarX::TabPlacement::Bottom:
		for (auto& c : controls)
		{
			c.first->setX(this->x + i * butW);
			c.first->setY(this->y + this->height - tabBarHeight);
			i++;
		}
		break;
	case StellarX::TabPlacement::Left:
		for (auto& c : controls)
		{
			c.first->setX(this->x);
			c.first->setY(this->y + i * butH);
			i++;
		}
		break;
	case StellarX::TabPlacement::Right:
		for (auto& c : controls)
		{
			c.first->setX(this->x + this->width - tabBarHeight);
			c.first->setY(this->y + i * butH);
			i++;
		}
		break;
	default:
		break;
	}
}

inline void TabControl::initTabPage()
{
	if (controls.empty())return;
	// TabControl 内部页签页仍然保留专用布局：
	// 这里负责把“当前选项卡容器矩形”拆分成页签栏和页面区，
	// 不把这部分细节下放到通用布局解算器里。
	// 但页面内容子树的世界坐标映射职责，继续归页面 Canvas 自己管理。
	// 因此这里不再手工遍历页内子控件做 setX/setY，
	// 只安置每个页面 Canvas 的矩形，让 Canvas 通过自身 relayoutManagedChildren() 收口。
	switch (this->tabPlacement)
	{
	case StellarX::TabPlacement::Top:
		for (auto& c : controls)
		{
			c.second->setX(this->x);
			c.second->setY(this->y + tabBarHeight);
			c.second->setWidth(this->width);
			c.second->setHeight(this->height - tabBarHeight);
		}
		break;
	case StellarX::TabPlacement::Bottom:
		for (auto& c : controls)
		{
			c.second->setX(this->x);
			c.second->setY(this->y);
			c.second->setWidth(this->width);
			c.second->setHeight(this->height - tabBarHeight);
		}
		break;
	case StellarX::TabPlacement::Left:
		for (auto& c : controls)
		{
			c.second->setX(this->x + tabBarHeight);
			c.second->setY(this->y);
			c.second->setWidth(this->width - tabBarHeight);
			c.second->setHeight(this->height);
		}
		break;
	case StellarX::TabPlacement::Right:
		for (auto& c : controls)
		{
			c.second->setX(this->x);
			c.second->setY(this->y);
			c.second->setWidth(this->width - tabBarHeight);
			c.second->setHeight(this->height);
		}
		break;
	default:
		break;
	}
}

void TabControl::refreshRuntimeLayout()
{
	// 这是 TabControl 的内部专用布局入口：
	// 外层先通过统一解算得到 TabControl 自身矩形，
	// 再由这里继续安置页签按钮和页面区。
	initTabBar();
	initTabPage();
}

TabControl::TabControl() :Canvas()
{
	this->id = "TabControl";
	// TabControl 作为外层容器，当前阶段显式允许双轴 Stretch；
	// 内部页签栏和页面区仍由自己的专用布局逻辑管理。
	this->layoutCapability.allowStretchX = true;
	this->layoutCapability.allowStretchY = true;
}

TabControl::TabControl(int x, int y, int width, int height)
	: Canvas(x, y, width, height)
{
	this->id = "TabControl";
	this->layoutCapability.allowStretchX = true;
	this->layoutCapability.allowStretchY = true;
}

TabControl::~TabControl()
{
}

void TabControl::setX(int x)
{
	applyRuntimeRectDirect(x, y, width, height);
	refreshRuntimeLayout();
	onWindowResize();
}

void TabControl::setY(int y)
{
	applyRuntimeRectDirect(x, y, width, height);
	refreshRuntimeLayout();
	onWindowResize();
}

void TabControl::setWidth(int width)
{
	applyRuntimeRectDirect(x, y, width, height);
	refreshRuntimeLayout();
	onWindowResize();
}

void TabControl::setHeight(int height)
{
	applyRuntimeRectDirect(x, y, width, height);
	refreshRuntimeLayout();
	onWindowResize();
}

void TabControl::applyResolvedLayoutRect(const StellarX::ResolvedLayoutRect& rect)
{
	// TabControl 作为外层控件接入统一解算；
	// 但页签栏和页面区仍由自身专用逻辑继续排布。
	applyRuntimeRectDirect(rect.worldX, rect.worldY, rect.width, rect.height);
	refreshRuntimeLayout();
}

void TabControl::draw()
{
	if (!dirty || !show)return;
	   // 绘制画布背景和基本形状及其子画布控件
	Canvas::draw();
	for (auto& c : controls)
	{
		c.second->setDirty(true);
		c.second->draw();
	}
	for (auto& c : controls)
	{
		c.first->setDirty(true);
		c.first->draw();
	}

	// 首次绘制时处理默认激活页签
	if (IsFirstDraw)
	{
		if (defaultActivation >= 0 && defaultActivation < (int)controls.size())
			controls[defaultActivation].first->setButtonClick(true);
		else if (defaultActivation >= (int)controls.size())//索引越界则激活最后一个
			controls[controls.size() - 1].first->setButtonClick(true);
		IsFirstDraw = false;//避免重复处理
	}
	dirty = false;
}

bool TabControl::handleEvent(const ExMessage& msg)
{
	if (!show)return false;
	resetEventVisualChanged();
	bool consume = false;
	bool anyVisualChanged = false;

	// TabControl 的同层页签按钮/页面区事件分发顺序，
	// 与 Window / Canvas 保持一致：按倒序处理，后加入者视为更靠上层。
	if (msg.message == WM_MOUSEMOVE)
	{
		for (auto it = controls.rbegin(); it != controls.rend(); ++it)
		{
			if (!consume)
			{
				if (it->first->handleEvent(msg))
				{
					consume = true;
				}
				if (it->first->didEventAffectVisual())
					anyVisualChanged = true;
			}
			else if (it->first->clearTransientMouseState())
			{
				anyVisualChanged = true;
			}
		}

		for (auto it = controls.rbegin(); it != controls.rend(); ++it)
		{
			if (!it->second->IsVisible())
				continue;

			if (!consume)
			{
				if (it->second->handleEvent(msg))
				{
					consume = true;
				}
				if (it->second->didEventAffectVisual())
					anyVisualChanged = true;
			}
			else if (it->second->clearTransientMouseState())
			{
				anyVisualChanged = true;
			}
		}
	}
	else
	{
		for (auto it = controls.rbegin(); it != controls.rend(); ++it)
			if (it->first->handleEvent(msg))
			{
				consume = true;
				break;
			}
		if (!consume)
		{
			for (auto it = controls.rbegin(); it != controls.rend(); ++it)
				if (it->second->IsVisible())
					if (it->second->handleEvent(msg))
					{
						consume = true;
						break;
					}
		}
	}
	if (dirty)
		requestRepaint(parent);
	markEventVisualChanged(anyVisualChanged || dirty);
	return consume;
}

bool TabControl::clearTransientMouseState()
{
	bool changed = false;
	for (auto it = controls.rbegin(); it != controls.rend(); ++it)
	{
		if (it->first->IsVisible() && it->first->clearTransientMouseState())
			changed = true;

		if (it->second->IsVisible() && it->second->clearTransientMouseState())
			changed = true;
	}
	return changed;
}

void TabControl::add(std::pair<std::unique_ptr<Button>, std::unique_ptr<Canvas>>&& control)
{
	controls.push_back(std::move(control));
	initTabBar();
	initTabPage();
	size_t idx = controls.size() - 1;
	controls[idx].first->setParent(this);
	controls[idx].first->enableTooltip(true);
	controls[idx].first->setbuttonMode(StellarX::ButtonMode::TOGGLE);

	controls[idx].first->setOnToggleOnListener([this, idx]()
		{
			int prevIdx = -1;
			for (size_t i = 0; i < controls.size(); ++i)
			{
				if (controls[i].second->IsVisible())
				{
					prevIdx = (int)i;
					break;
				}
			}
			for (auto& tab : controls)
			{
				if (tab.first->getButtonText() != controls[idx].first->getButtonText() && tab.first->isClicked())
					tab.first->setButtonClick(false);
			}
			
		
			SX_LOGI("Tab") << SX_T("激活选项卡：","activate tab: ") << prevIdx << "->" << (int)idx
				<< " text=" << controls[idx].first->getButtonText();
			controls[idx].second->onWindowResize();
			controls[idx].second->setIsVisible(true);
			dirty = true;
			
			
		});
	controls[idx].first->setOnToggleOffListener([this, idx]()
		{
			SX_LOGI("Tab") << SX_T("关闭选项卡：id=","deactivate tab: idx=") << (int)idx
				<< " text=" << controls[idx].first->getButtonText();

			controls[idx].second->setIsVisible(false);
			dirty = true;
		});
	controls[idx].second->setParent(this);
	controls[idx].second->setLinewidth(canvaslinewidth);
	controls[idx].second->setIsVisible(false);
}

void TabControl::add(std::string tabText, std::unique_ptr<Control> control)
{
	control->setDirty(true);
	for (auto& tab : controls)
	{
		if (tab.first->getButtonText() == tabText)
		{
			control->setParent(tab.second.get());
			control->setIsVisible(tab.second->IsVisible());
			tab.second->addControl(std::move(control));
			break;
		}
	}
}

void TabControl::setTabPlacement(StellarX::TabPlacement placement)
{
	this->tabPlacement = placement;
	setDirty(true);
	initTabBar();
	initTabPage();
}

void TabControl::setTabBarHeight(int height)
{
	tabBarHeight = height;
	setDirty(true);
	initTabBar();
	initTabPage();
}

void TabControl::setIsVisible(bool visible)
{
	// 先让基类 Canvas 处理自己的回贴/丢快照逻辑
	Canvas::setIsVisible(visible);
	for (auto& tab : controls)
	{
		if(true == visible)
		{
			tab.first->setIsVisible(visible);
			//页也要跟着关/开，否则它们会保留旧的 saveBkImage
			if (tab.first->isClicked())
				tab.second->setIsVisible(true);
			else
				tab.second->setIsVisible(false);
			tab.second->setDirty(true);
		}
		else
		{
			tab.first->setIsVisible(visible);
			tab.second->setIsVisible(visible);
		}
	}
}

void TabControl::onWindowResize()
{
	// 本轮不再在 onWindowResize 中重做页签布局，
	// 这里只负责失效快照、标脏，并把 resize 语义向页签按钮和页面传递。
	Control::onWindowResize();
	for (auto& c : controls)
	{
		c.first->onWindowResize();
		c.second->onWindowResize();
	}
	dirty = true;
}

int TabControl::getActiveIndex() const
{
	int idx = -1;
	for (auto& c : controls)
	{
		idx++;
		if (c.first->isClicked())
			return idx;
	}
	return -1;
}

void TabControl::setActiveIndex(int idx)
{
	if (IsFirstDraw)
		defaultActivation = idx;
	else
	{
		// 外部重复激活“已经处于激活状态”的页签时，不应再把整条 onToggleOn 链重跑一遍。
		// 否则当前可见页面会重复执行 onWindowResize()/setIsVisible(true)，
		// 对页内像 Table 这种会越出页面边界绘制的控件，容易把快照链再次扰乱，留下残影。
		if (idx >= 0 && idx < controls.size())
		{
			if (getActiveIndex() == idx)
				return;
			controls[idx].first->setButtonClick(true);
		}
	}
}

int TabControl::count() const
{
	return (int)controls.size();
}

int TabControl::indexOf(const std::string& tabText) const
{
	int idx = -1;
	for (auto& c : controls)
	{
		idx++;
		if (c.first->getButtonText() == tabText)
			return idx;
	}

	return -1;
}

void TabControl::setDirty(bool dirty)
{
	this->dirty = dirty;
	for (auto& c : controls)
	{
		c.first->setDirty(dirty);
		c.second->setDirty(dirty);
	}
}

void TabControl::requestRepaint(Control* parent)
{
	if (shouldDeferManagedRepaint())
	{
		// 托管路径：TabControl 作为“页签栏 + 当前页面”的统一重绘 root 登记到 Window。
		if (auto* host = getHostWindow())
			host->requestManagedRepaint(this);
		return;
	}

	if (this == parent)
	{
		RECT paintCoverage{};
		bool hasPaintCoverage = false;
		RECT persistentCoverage{};
		bool hasPersistentCoverage = false;
		auto commitTabUnit = [&](Control* unit, SxTabOverlayRedrawMode overlayMode)
			{
				if (!unit || !unit->IsVisible())
					return;

				const bool directDirty = unit->isDirty();
				const bool subtreeDirty = unit->hasManagedDirtySubtree();

				if (overlayMode == SxTabOverlayRedrawMode::RefreshSnapshot)
				{
					// 下层单元的持久内容已经写过像素，上层页签/页面作为 overlay 补画时，
					// 必须先丢掉旧快照，重新抓取当前背景后再画，否则会把旧背景再贴回来。
					unit->invalidateBackgroundSnapshot();
					unit->setDirty(true);
					unit->draw();
				}
				else if (directDirty)
				{
					unit->draw();
				}
				else if (subtreeDirty)
				{
					unit->commitManagedRepaint();
				}
				else
				{
					return;
				}

				const RECT paintRect = unit->getManagedRepaintCoverageRect();
				if (!hasPaintCoverage)
				{
					paintCoverage = paintRect;
					hasPaintCoverage = true;
				}
				else
				{
					paintCoverage = SxTabUnionRect(paintCoverage, paintRect);
				}

				const RECT persistentRect = unit->getManagedRepaintPersistentCoverageRect();
				if (!hasPersistentCoverage)
				{
					persistentCoverage = persistentRect;
					hasPersistentCoverage = true;
				}
				else
				{
					persistentCoverage = SxTabUnionRect(persistentCoverage, persistentRect);
				}
			};

		// 局部重绘必须和 draw() 维持同一套顺序：
		// 先页面，再页签按钮。
		// 否则页签按钮上的 Tooltip 会被后画的页面盖掉，
		// 表现为“有页面打开时 Tooltip 看不到，所有页关闭时才正常”。
		for (auto& control : controls)
		{
			Control* page = control.second.get();
			if (!page->IsVisible())
				continue;

			if (page->hasManagedDirtySubtree())
			{
				commitTabUnit(page, SxTabOverlayRedrawMode::None);
			}
			else if (hasPaintCoverage && SxTabRectsIntersect(page->getManagedRepaintCoverageRect(), paintCoverage))
			{
				const bool persistentHit = hasPersistentCoverage &&
					SxTabRectsIntersect(page->getManagedRepaintPersistentCoverageRect(), persistentCoverage);
				if (persistentHit)
					commitTabUnit(page, SxTabOverlayRedrawMode::RefreshSnapshot);
			}
		}

		for (auto& control : controls)
		{
			Control* button = control.first.get();
			if (!button->IsVisible())
				continue;

			if (button->hasManagedDirtySubtree())
			{
				commitTabUnit(button, SxTabOverlayRedrawMode::None);
			}
			else if (hasPaintCoverage && SxTabRectsIntersect(button->getManagedRepaintCoverageRect(), paintCoverage))
			{
				const bool persistentHit = hasPersistentCoverage &&
					SxTabRectsIntersect(button->getManagedRepaintPersistentCoverageRect(), persistentCoverage);
				if (persistentHit)
					commitTabUnit(button, SxTabOverlayRedrawMode::RefreshSnapshot);
			}
		}
		return;
	}

	onRequestRepaintAsRoot();
}

bool TabControl::hasManagedDirtySubtree() const
{
	if (dirty)
		return true;

	for (const auto& control : controls)
	{
		if (control.first->IsVisible() && control.first->hasManagedDirtySubtree())
			return true;
		if (control.second->IsVisible() && control.second->hasManagedDirtySubtree())
			return true;
	}
	return false;
}

RECT TabControl::getManagedRepaintCoverageRect() const
{
	// TabControl 的 draw() 会写自身背景、全部页签按钮以及全部页面。
	// 因此 coverage 也必须按“页签按钮 + 页面”递归并集，避免内部 Tooltip 等附加绘制区域被漏算。
	RECT coverage = getBoundsRect();
	for (const auto& control : controls)
	{
		if (control.first->IsVisible())
			coverage = SxTabUnionRect(coverage, control.first->getManagedRepaintCoverageRect());
		if (control.second->IsVisible())
			coverage = SxTabUnionRect(coverage, control.second->getManagedRepaintCoverageRect());
	}
	return coverage;
}

RECT TabControl::getManagedRepaintPersistentCoverageRect() const
{
	// 持久 coverage 排除页签按钮 Tooltip 等临时浮层，
	// 用于判断上层页签/页面补画时是否允许刷新背景快照。
	RECT coverage = getBoundsRect();
	for (const auto& control : controls)
	{
		if (control.first->IsVisible())
			coverage = SxTabUnionRect(coverage, control.first->getManagedRepaintPersistentCoverageRect());
		if (control.second->IsVisible())
			coverage = SxTabUnionRect(coverage, control.second->getManagedRepaintPersistentCoverageRect());
	}
	return coverage;
}

bool TabControl::canCommitManagedPartialRepaint() const
{
	// TabControl 只有在自己本体不脏且背景快照有效时，才允许只更新脏页签/脏页面。
	return show && !dirty && hasValidBackgroundSnapshot();
}

void TabControl::commitManagedRepaint()
{
	if (!show)
		return;

	if (canCommitManagedPartialRepaint())
	{
		// 页签栏和页面基线都还有效：沿用原有局部重绘逻辑。
		requestRepaint(this);
		return;
	}

	// 自身布局或背景已经变化：升级为整 TabControl 重画。
	this->dirty = true;
	onRequestRepaintAsRoot();
}
