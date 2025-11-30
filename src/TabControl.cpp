#include "TabControl.h"

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
			c.first->setY(this->y+this->height - tabBarHeight);
			i++;
		}
		break;
	case StellarX::TabPlacement::Left:
		for (auto& c : controls)
		{
			c.first->setX(this->x);
			c.first->setY(this->y+i* butH);
			i++;
		}
		break;
	case StellarX::TabPlacement::Right:
		for (auto& c : controls)
		{
			c.first->setX(this->x+this->width - tabBarHeight);
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
	//子控件坐标原点
	int nX = 0;
	int nY = 0;
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
		nX = this->x;
		nY = this->y + tabBarHeight;
		for (auto& c : controls)
		{
			for (auto& v : c.second->getControls())
			{
				v->setX(v->getLocalX() + nX);
				v->setY(v->getLocalY() + nY);
			}
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
		nX = this->x;
		nY = this->y;
		for (auto& c : controls)
		{
			for (auto& v : c.second->getControls())
			{
				v->setX(v->getLocalX() + nX);
				v->setY(v->getLocalY() + nY);
			}
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
		nX = this->x + tabBarHeight;
		nY = this->y;
		for (auto& c : controls)
		{
			for (auto& v : c.second->getControls())
			{
				v->setX(v->getLocalX() + nX);
				v->setY(v->getLocalY() + nY);
			}
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
		nX = this->x;
		nY = this->y;
		for (auto& c : controls)
		{
			for (auto& v : c.second->getControls())
			{
				v->setX(v->getLocalX() + nX);
				v->setY(v->getLocalY() + nY);
			}
		}
		break;
	default:
		break;
	}
}

TabControl::TabControl():Canvas()
{
	this->id = "TabControl";
}

TabControl::TabControl(int x, int y, int width, int height)
	: Canvas(x, y, width, height)
{
	this->id = "TabControl";
}

TabControl::~TabControl()
{
}

void TabControl::setX(int x)
{
	this->x = x;
	initTabBar();
	initTabPage();
	dirty = true;
	for (auto& c : controls)
	{
		c.first->onWindowResize();
		c.second->onWindowResize();
	}

}

void TabControl::setY(int y)
{
	this->y = y;
	initTabBar();
	initTabPage();
	dirty = true;
	for (auto& c : controls)
	{
		c.first->onWindowResize();
		c.second->onWindowResize();
	}
}

void TabControl::draw()
{
	if (!dirty || !show)return;
 //   // 在绘制 TabControl 之前，先恢复并更新背景快照：
	//int margin = canvaslinewidth > 1 ? canvaslinewidth : 1;
	//if (hasSnap)
	//{
	//	// 恢复旧快照，清除上一次绘制
	//	restBackground();
	//	// 如果位置或尺寸变了，或没有有效缓存，则重新抓取
	//	if (!saveBkImage || saveBkX != this->x - margin || saveBkY != this->y - margin || saveWidth != this->width + margin * 2 || saveHeight != this->height + margin * 2)
	//	{
	//		discardBackground();
	//		saveBackground(this->x - margin, this->y - margin, this->width + margin * 2, this->height + margin * 2);
	//	}
	//}
	//else
	//{
	//	// 首次绘制或没有快照时直接抓取背景
	//	saveBackground(this->x - margin, this->y - margin, this->width + margin * 2, this->height + margin * 2);
	//}
 //   // 再次恢复最新背景，保证绘制区域干净
 //   restBackground();
    // 绘制画布背景和基本形状及其子画布控件
    Canvas::draw();
	for (auto& c : controls)
	{
		c.first->setDirty(true);
		c.first->draw();
	}
	for (auto& c : controls)
	{
		c.second->setDirty(true);
		c.second->draw();
	}
	dirty = false;

}

bool TabControl::handleEvent(const ExMessage& msg)
{
	if (!show)return false;
	bool consume = false;
	for (auto& c : controls)
		if (c.first->handleEvent(msg))
		{
			consume = true;
			break;
		}
	for (auto& c : controls)
		if(c.second->IsVisible())
			if (c.second->handleEvent(msg))
			{
				consume = true;
				break;
			}
	if (dirty)
		requestRepaint(parent);
	return consume;
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

	controls[idx].first->setOnToggleOnListener([this,idx]()
		{
			controls[idx].second->setIsVisible(true);
			controls[idx].second->onWindowResize();
			for (auto& tab : controls)
			{
				if (tab.first->getButtonText() != controls[idx].first->getButtonText())
				{
					tab.first->setButtonClick(false);
					tab.second->setIsVisible(false);
				}
			}
			dirty = true;
		});
	controls[idx].first->setOnToggleOffListener([this,idx]()
		{
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
			control->setIsVisible( tab.second->IsVisible());
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
	Canvas::setIsVisible(visible);   // <--- 新增

	this->show = visible;
	for (auto& tab : controls)
	{
		tab.first->setIsVisible(visible);
		//页也要跟着关/开，否则它们会保留旧的 saveBkImage
		tab.second->setIsVisible(visible);  
		tab.second->setDirty(true);
	}
}

void TabControl::onWindowResize()
{
    // 调用基类的窗口变化处理，丢弃快照并标记脏
    Control::onWindowResize();
    // 根据当前 TabControl 的新尺寸重新计算页签栏和页面区域
    initTabBar();
    initTabPage();
    // 转发窗口尺寸变化给所有页签按钮和页面
    for (auto& c : controls)
    {
        c.first->onWindowResize();
        c.second->onWindowResize();
    }
    // 尺寸变化后需要重绘自身
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
	return idx;
}

void TabControl::setActiveIndex(int idx)
{
	if (idx < 0 || idx > controls.size() - 1) return;
	if (controls[idx].first->getButtonMode() == StellarX::ButtonMode::DISABLED)return;
	if (controls[idx].first->isClicked())
	{
		if (controls[idx].second->IsVisible())
			return;
		else
			controls[idx].second->setIsVisible(true);
	}
	else
	{
		controls[idx].first->setButtonClick(true);
	}

}

int TabControl::count() const
{
	return (int)controls.size();
}

int TabControl::indexOf(const std::string& tabText) const
{
	int idx = -1;
	for(auto& c : controls)
	{
		idx++;
		if (c.first->getButtonText() == tabText)
			return idx;
	}
            			
	return idx;
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
	if (this == parent)
	{
		for (auto& control : controls)
		{
			if (control.first->isDirty() && control.first->IsVisible())
				control.first->draw();
			else if (control.second->isDirty()&&control.second->IsVisible())
				control.second->draw();
		}
	}

	else
		onRequestRepaintAsRoot();
}
