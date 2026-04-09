#include "TabControl.h"
#include "SxLog.h"
#include "Window.h"
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

TabControl::TabControl() :Canvas()
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
	bool consume = false;
	for (auto& c : controls)
		if (c.first->handleEvent(msg))
		{
			consume = true;
			break;
		}
	if (!consume)
	{
		for (auto& c : controls)
			if (c.second->IsVisible())
				if (c.second->handleEvent(msg))
				{
					consume = true;
					break;
				}
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
	return -1;
}

void TabControl::setActiveIndex(int idx)
{
	if (IsFirstDraw)
		defaultActivation = idx;
	else
	{
		if (idx >= 0 && idx < controls.size())
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
	for (auto& c : controls)
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
	if (shouldDeferManagedRepaint())
	{
		// 托管路径：TabControl 作为“页签栏 + 当前页面”的统一重绘 root 登记到 Window。
		if (auto* host = getHostWindow())
			host->requestManagedRepaint(this);
		return;
	}

	if (this == parent)
	{
		for (auto& control : controls)
		{
			if (control.first->isDirty() && control.first->IsVisible())
				control.first->draw();
			 if (control.second->isDirty() && control.second->IsVisible())
				control.second->draw();
		}
	}

	else
		onRequestRepaintAsRoot();
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
