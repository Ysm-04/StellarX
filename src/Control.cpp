#include "Control.h"
#include "SxLog.h"
#include<assert.h>
#include "Window.h"
#include <algorithm>

namespace
{
	// 单轴解算后的临时结果：只描述“在父局部坐标系中的位置和尺寸”。
	struct LayoutAxisResult
	{
		int pos = 0;
		int size = 0;
	};

	int ClampNonNegative(int value)
	{
		return value < 0 ? 0 : value;
	}

	int RoundDiv(long long numerator, long long denominator)
	{
		if (denominator == 0)
			return 0;
		if (numerator >= 0)
			return static_cast<int>((numerator + denominator / 2) / denominator);
		return -static_cast<int>(((-numerator) + denominator / 2) / denominator);
	}

	// 旧接口兼容层：
	// 将 setAnchor(a1, a2) 的双锚点输入映射为单轴 LayoutSpec。
	// 这里只负责“从旧输入翻译成新模型”，不直接参与后续求解。
	StellarX::AxisLayoutSpec BuildLegacyAxisSpec(StellarX::Anchor anchor1, StellarX::Anchor anchor2,
		StellarX::Anchor startAnchor, StellarX::Anchor endAnchor)
	{
		StellarX::AxisLayoutSpec spec{};
		spec.anchorStart = (anchor1 == startAnchor || anchor2 == startAnchor);
		spec.anchorEnd = (anchor1 == endAnchor || anchor2 == endAnchor);

		if (spec.anchorStart && spec.anchorEnd)
		{
			spec.sizePolicy = StellarX::AxisSizePolicy::Stretch;
			spec.alignPolicy = StellarX::AxisAlignPolicy::Start;
		}
		else if (spec.anchorEnd)
		{
			spec.sizePolicy = StellarX::AxisSizePolicy::FixedSize;
			spec.alignPolicy = StellarX::AxisAlignPolicy::End;
		}
		else
		{
			spec.sizePolicy = StellarX::AxisSizePolicy::FixedSize;
			spec.alignPolicy = StellarX::AxisAlignPolicy::Start;
		}

		return spec;
	}

	// 单轴统一解算器：
	// 输入设计态父尺寸、当前父尺寸、子控件设计态位置/尺寸，以及当前轴策略，
	// 输出该轴在“父局部坐标系”下的运行态位置与尺寸。
	LayoutAxisResult ResolveAxis(int parentDesignSize, int parentCurrentSize, int childDesignPos, int childDesignSize,
		const StellarX::AxisLayoutSpec& spec, bool allowStretch)
	{
		LayoutAxisResult result{};
		const int startMargin = childDesignPos;
		const int endMargin = parentDesignSize - (childDesignPos + childDesignSize);

		const bool wantsStretch = spec.sizePolicy == StellarX::AxisSizePolicy::Stretch;
		const bool canStretch = allowStretch && spec.anchorStart && spec.anchorEnd;

		if (wantsStretch && canStretch)
		{
			// 双边锚定 + Stretch：
			// 保持设计态中的起边距和终边距，让尺寸随父容器变化。
			result.pos = startMargin;
			result.size = ClampNonNegative(parentCurrentSize - startMargin - endMargin);
			return result;
		}

		// 其余情况一律按固定尺寸处理；即便调用方请求 Stretch，
		// 只要锚点条件或控件能力边界不满足，也会在这里自然降级为 FixedSize。
		result.size = ClampNonNegative(childDesignSize);

		if (spec.anchorStart && !spec.anchorEnd)
		{
			// 仅锚定起边：保持起边距，尺寸不变。
			result.pos = startMargin;
			return result;
		}

		if (!spec.anchorStart && spec.anchorEnd)
		{
			// 仅锚定终边：保持终边距，尺寸不变。
			result.pos = parentCurrentSize - endMargin - result.size;
			return result;
		}

		if (spec.anchorStart && spec.anchorEnd)
		{
			// 双边锚定但尺寸固定：
			// 位置由 alignPolicy 决定，用于表达“只位移、不拉伸”的场景。
			switch (spec.alignPolicy)
			{
			case StellarX::AxisAlignPolicy::End:
				result.pos = parentCurrentSize - endMargin - result.size;
				break;
			case StellarX::AxisAlignPolicy::Center:
				// 保持相对父容器中心的偏移关系。
				result.pos = childDesignPos + (parentCurrentSize - parentDesignSize) / 2;
				break;
			case StellarX::AxisAlignPolicy::Proportional:
			{
				// 保持设计态中的相对位置比例。
				// 注意：这里只调整位置，不改变尺寸。
				const int designTravel = parentDesignSize - result.size;
				const int currentTravel = parentCurrentSize - result.size;
				if (designTravel <= 0 || currentTravel <= 0)
					result.pos = startMargin;
				else
					result.pos = RoundDiv(static_cast<long long>(startMargin) * currentTravel, designTravel);
				break;
			}
			case StellarX::AxisAlignPolicy::Start:
			default:
				result.pos = startMargin;
				break;
			}
			return result;
		}

		// 无锚点：退回设计态位置，尺寸保持设计值。
		result.pos = childDesignPos;
		return result;
	}

	const char* SxAxisName(bool horizontal)
	{
		return horizontal ? "X" : "Y";
	}

	void LogAxisDowngradeIfNeeded(const std::string& id, bool horizontal,
		const StellarX::AxisLayoutSpec& spec, bool allowStretch)
	{
		if (spec.sizePolicy != StellarX::AxisSizePolicy::Stretch)
			return;

		if (!spec.anchorStart || !spec.anchorEnd)
		{
			SX_LOGD("Layout")
				<< SX_T("布局降级：id=", "layout downgrade: id=") << id
				<< SX_T(" axis=", " axis=") << SxAxisName(horizontal)
				<< SX_T(" 原因=Stretch 但未同时锚定起止边", " reason=stretch without dual anchors");
			return;
		}

		if (!allowStretch)
		{
			SX_LOGD("Layout")
				<< SX_T("能力边界拦截：id=", "layout capability intercept: id=") << id
				<< SX_T(" axis=", " axis=") << SxAxisName(horizontal)
				<< SX_T(" 原因=当前控件禁止该轴 Stretch", " reason=stretch disabled by capability");
		}
	}

	void ActivateExplicitLayoutSpecMode(StellarX::LayoutMode& mode)
	{
		// 新公开布局 API 明确属于“按边约束解算”的语义层。
		// 一旦调用，说明外部希望控件进入统一锚定布局模型，
		// 因此直接切换到 AnchorToEdges，避免出现“策略已设但 layoutMode 仍是 Fixed”的半失效状态。
		mode = StellarX::LayoutMode::AnchorToEdges;
	}
}

bool StellarX::ControlText::operator==(const ControlText& text) const
{
	return nHeight == text.nHeight &&
		nWidth == text.nWidth &&
		faceName == text.faceName &&
		color == text.color &&
		nEscapement == text.nEscapement &&
		nOrientation == text.nOrientation &&
		nWeight == text.nWeight &&
		bItalic == text.bItalic &&
		bUnderline == text.bUnderline &&
		bStrikeOut == text.bStrikeOut;
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
		discardBackground();
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
	invalidateBackgroundSnapshot();
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
	// 旧 API 只作为兼容输入层存在：
	// 这里把历史上的 anchor_1 / anchor_2 映射为新的水平/垂直轴布局规格，
	// 后续统一解算全部以 layoutSpec 为准。
	this->layoutSpec.horizontal = BuildLegacyAxisSpec(anchor_1, anchor_2, StellarX::Anchor::Left, StellarX::Anchor::Right);
	this->layoutSpec.vertical = BuildLegacyAxisSpec(anchor_1, anchor_2, StellarX::Anchor::Top, StellarX::Anchor::Bottom);
}
void Control::setHorizontalLayoutSpec(const StellarX::AxisLayoutSpec& spec)
{
	ActivateExplicitLayoutSpecMode(this->layoutMode);
	this->layoutSpec.horizontal = spec;
}

void Control::setVerticalLayoutSpec(const StellarX::AxisLayoutSpec& spec)
{
	ActivateExplicitLayoutSpecMode(this->layoutMode);
	this->layoutSpec.vertical = spec;
}

void Control::setHorizontalAnchors(bool left, bool right)
{
	ActivateExplicitLayoutSpecMode(this->layoutMode);
	this->layoutSpec.horizontal.anchorStart = left;
	this->layoutSpec.horizontal.anchorEnd = right;
}

void Control::setVerticalAnchors(bool top, bool bottom)
{
	ActivateExplicitLayoutSpecMode(this->layoutMode);
	this->layoutSpec.vertical.anchorStart = top;
	this->layoutSpec.vertical.anchorEnd = bottom;
}

void Control::setHorizontalSizePolicy(StellarX::AxisSizePolicy policy)
{
	ActivateExplicitLayoutSpecMode(this->layoutMode);
	this->layoutSpec.horizontal.sizePolicy = policy;
}

void Control::setVerticalSizePolicy(StellarX::AxisSizePolicy policy)
{
	ActivateExplicitLayoutSpecMode(this->layoutMode);
	this->layoutSpec.vertical.sizePolicy = policy;
}

void Control::setHorizontalAlignPolicy(StellarX::AxisAlignPolicy policy)
{
	ActivateExplicitLayoutSpecMode(this->layoutMode);
	this->layoutSpec.horizontal.alignPolicy = policy;
}

void Control::setVerticalAlignPolicy(StellarX::AxisAlignPolicy policy)
{
	ActivateExplicitLayoutSpecMode(this->layoutMode);
	this->layoutSpec.vertical.alignPolicy = policy;
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
StellarX::AxisLayoutSpec Control::getHorizontalLayoutSpec() const
{
	return layoutSpec.horizontal;
}

StellarX::AxisLayoutSpec Control::getVerticalLayoutSpec() const
{
	return layoutSpec.vertical;
}

const StellarX::LayoutSpec& Control::getLayoutSpec() const
{
	return layoutSpec;
}

const StellarX::LayoutCapability& Control::getLayoutCapability() const
{
	return layoutCapability;
}

void Control::setX(int x)
{
	// 先保存旧 coverage 并回贴旧快照，再写入新坐标；这样移动控件不会留下旧像素。
	const RECT previousCoverage = getManagedRepaintCoverageRect();
	const bool wasDrawn = hasValidBackgroundSnapshot();
	if (wasDrawn)
		discardBackground();
	this->x = x;
	dirty = true;
	if (wasDrawn)
	{
		if (auto* host = getHostWindow()) host->requestManagedRepaint(this, previousCoverage);
		else requestRepaint(parent);
	}
}

void Control::setY(int y)
{
	// Y 方向与 X 方向使用同一生命周期顺序，旧区域和新区域都由托管重绘统一收口。
	const RECT previousCoverage = getManagedRepaintCoverageRect();
	const bool wasDrawn = hasValidBackgroundSnapshot();
	if (wasDrawn)
		discardBackground();
	this->y = y;
	dirty = true;
	if (wasDrawn)
	{
		if (auto* host = getHostWindow()) host->requestManagedRepaint(this, previousCoverage);
		else requestRepaint(parent);
	}
}

void Control::setWidth(int width)
{
	// 尺寸变化会改变快照大小，必须先释放旧快照，避免新绘制复用错误尺寸的背景。
	const RECT previousCoverage = getManagedRepaintCoverageRect();
	const bool wasDrawn = hasValidBackgroundSnapshot();
	if (wasDrawn)
		discardBackground();
	this->width = (std::max)(0, width);
	dirty = true;
	if (wasDrawn)
	{
		if (auto* host = getHostWindow()) host->requestManagedRepaint(this, previousCoverage);
		else requestRepaint(parent);
	}
}

void Control::setHeight(int height)
{
	// 高度调整同样登记变化前的 coverage，供 Window 在提交阶段覆盖旧、新两个区域。
	const RECT previousCoverage = getManagedRepaintCoverageRect();
	const bool wasDrawn = hasValidBackgroundSnapshot();
	if (wasDrawn)
		discardBackground();
	this->height = (std::max)(0, height);
	dirty = true;
	if (wasDrawn)
	{
		if (auto* host = getHostWindow()) host->requestManagedRepaint(this, previousCoverage);
		else requestRepaint(parent);
	}
}

bool Control::clearTransientMouseState()
{
	// 基类默认没有 hover / tooltip / 临时按下态等鼠标瞬时状态。
	// 具体控件若需要在“未收到本次 WM_MOUSEMOVE”时做清理，重写此接口即可。
	return false;
}

void Control::commitCurrentGeometryAsDesignRect()
{
	// 该接口是“显式提交新的设计基线”的唯一入口之一。
	// 普通布局解算、父容器重排、窗口 resize 均不得自动回写 local*，
	// 否则会导致设计基线漂移，后续解算越来越不稳定。
	SX_LOGD("Layout")
		<< SX_T("提交设计基线：id=", "commit design rect: id=") << id
		<< SX_T(" world=(", " world=(") << x << "," << y << "," << width << "," << height << ")"
		<< SX_T(" parentLocalBefore=(", " parentLocalBefore=(")
		<< localx << "," << localy << "," << localWidth << "," << localHeight << ")";
	localx = parent ? (x - parent->getX()) : x;
	localy = parent ? (y - parent->getY()) : y;
	localWidth = width;
	localHeight = height;
	SX_LOGD("Layout")
		<< SX_T("提交设计基线完成：id=", "commit design rect done: id=") << id
		<< SX_T(" parentLocalAfter=(", " parentLocalAfter=(")
		<< localx << "," << localy << "," << localWidth << "," << localHeight << ")";
}

StellarX::ResolvedLayoutRect Control::resolveLayoutRect(int parentDesignW, int parentDesignH,
	int parentWorldX, int parentWorldY, int parentCurrentW, int parentCurrentH) const
{
	StellarX::ResolvedLayoutRect rect{};

	if (layoutMode != StellarX::LayoutMode::AnchorToEdges)
	{
		// 非锚点布局模式：直接使用设计态矩形，再映射到当前世界坐标。
		rect.localX = localx;
		rect.localY = localy;
		rect.width = localWidth;
		rect.height = localHeight;
		rect.worldX = parentWorldX + rect.localX;
		rect.worldY = parentWorldY + rect.localY;
		return rect;
	}

	// 第 1 层：先在父局部坐标系内分别解算水平轴和垂直轴。
	LogAxisDowngradeIfNeeded(id, true, layoutSpec.horizontal, layoutCapability.allowStretchX);
	LogAxisDowngradeIfNeeded(id, false, layoutSpec.vertical, layoutCapability.allowStretchY);
	const LayoutAxisResult horizontal = ResolveAxis(parentDesignW, parentCurrentW, localx, localWidth,
		layoutSpec.horizontal, layoutCapability.allowStretchX);
	const LayoutAxisResult vertical = ResolveAxis(parentDesignH, parentCurrentH, localy, localHeight,
		layoutSpec.vertical, layoutCapability.allowStretchY);

	// 第 2 层：把父局部矩形映射到世界坐标。
	rect.localX = horizontal.pos;
	rect.localY = vertical.pos;
	rect.width = horizontal.size;
	rect.height = vertical.size;
	rect.worldX = parentWorldX + rect.localX;
	rect.worldY = parentWorldY + rect.localY;
	return rect;
}

void Control::applyResolvedLayoutRect(const StellarX::ResolvedLayoutRect& rect)
{
	// 默认实现只应用运行态世界矩形。
	// 若某个控件还需要在应用后继续刷新内部布局，可重写此函数。
	applyRuntimeRectDirect(rect.worldX, rect.worldY, rect.width, rect.height);
}

void Control::applyRuntimeRectDirect(int worldX, int worldY, int width, int height)
{
	// 最底层运行态赋值入口：
	// 只修改当前世界坐标和运行态尺寸，不触碰设计基线 local*。
	this->x = worldX;
	this->y = worldY;
	this->width = width;
	this->height = height;
	dirty = true;
}
// 保存当前的绘图状态（字体、颜色、线型等）
// 在控件绘制前调用，确保不会影响全局绘图状态
void Control::saveStyle()
{

	gettextstyle(&currentFont); // 获取当前字体样式
	currentColor = gettextcolor(); // 获取当前字体颜色
	currentBorderColor = getlinecolor(); //保存当前边框颜色
	getlinestyle(&currentLineStyle); //保存当前线型
	currentBkColor = getfillcolor(); //保存当前填充色
}
// 恢复之前保存的绘图状态
// 在控件绘制完成后调用，恢复全局绘图状态
void Control::restoreStyle()
{
	settextstyle(&currentFont); // 恢复默认字体样式
	settextcolor(currentColor); // 恢复默认字体颜色
	setfillcolor(currentBkColor);
	setlinestyle(&currentLineStyle);
	setlinecolor(currentBorderColor);
	setfillstyle(BS_SOLID);//恢复填充
}

void Control::requestRepaint(Control* parent)
{
	if (shouldDeferManagedRepaint())
	{
		// 托管路径：当前正在 Window 的事件分发阶段，不能立即绘制；
		// 这里只登记 source，真正的 root 选择由 Window 在 requestManagedRepaint 中完成。
		if (auto* host = getHostWindow())
			host->requestManagedRepaint(this);
		return;
	}

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

	SX_LOG_TRACE("Dirty") << SX_T("请求重绘：id=","requestRepaint: id=") << id << " parent=" << (parent ? parent->getId() : "null");

	if (parent) parent->requestRepaint(parent);   // 交给容器处理（容器可局部重绘）
	else        onRequestRepaintAsRoot();         // 根兜底
}

void Control::onRequestRepaintAsRoot()
{
	if (shouldDeferManagedRepaint())
	{
		// 即使已经冒泡到 root，只要还在托管分发期，也不能直接绘制；
		// 仍然回到 Window 做统一提交。
		if (auto* host = getHostWindow())
			host->requestManagedRepaint(this);
		return;
	}

	SX_LOG_TRACE("Dirty")
		<< SX_T("触发根重绘：id=", "onRequestRepaintAsRoot: id=") << id
		<< SX_T("（从根节点开始重画）", " (root repaint)");


	discardBackground();
	setDirty(true);
	draw();    // 只有“无父”时才允许立即画，不会被谁覆盖
}

bool Control::shouldDeferManagedRepaint() const
{
	Window* host = getHostWindow();
	return host && (host->isManagedDispatchActive() || host->isMutationTransactionActive());
}

void Control::beginMutationScope()
{
	// 子控件不单独维护顶层事务，沿 parent 链把计数交给 Canvas 或 Window。
	if (parent)
		parent->beginMutationScope();
	else if (auto* host = getHostWindow())
		host->beginMutationScope();
}

void Control::endMutationScope()
{
	// 与 beginMutationScope 对称；最外层 root 结束后才有资格提交待处理队列。
	if (parent)
		parent->endMutationScope();
	else if (auto* host = getHostWindow())
		host->endMutationScope();
}

// 获取宿主 Window：
// - 顶层控件由 Window/addDialog 直接注入；
// - 子控件没有直接注入时，沿 parent 链向上回溯即可。
Window* Control::getHostWindow() const
{
	if (hostWindow)
		return hostWindow;
	return parent ? parent->getHostWindow() : nullptr;
}

// 托管重绘 root 选择规则：
// - 对于直接挂在 Window 下的控件，root 就是它自己；
// - 对于嵌套在 Canvas/TabControl/Dialog 内的子控件，沿 parent 向上找到与宿主 Window 同属一棵树的最上层控件。
Control* Control::getManagedRepaintRoot()
{
	Control* root = this;
	Window* host = getHostWindow();
	while (root->parent && root->parent->getHostWindow() == host)
		root = root->parent;
	return root;
}

Control* Control::getManagedRepaintDirectBranch(Control* root)
{
	if (!root)
		return nullptr;

	Control* branch = this;
	while (branch->parent && branch->parent != root)
		branch = branch->parent;
	return branch;
}

RECT Control::getBoundsRect() const
{
	RECT rc{};
	rc.left = x;
	rc.top = y;
	rc.right = x + width;
	rc.bottom = y + height;
	return rc;
}

RECT Control::getManagedRepaintCoverageRect() const
{
	// 基类默认认为“实际绘制 coverage == 控件本体矩形”。
	// 复合控件或带浮层的控件会 override，把附加绘制区域一并并入。
	return getBoundsRect();
}

RECT Control::getManagedRepaintPersistentCoverageRect() const
{
	// 基类默认认为持久绘制范围等于控件本体。
	// 只有 Tooltip 这类临时浮层需要从持久范围中剔除。
	return getBoundsRect();
}

bool Control::canCommitManagedPartialRepaint() const
{
	// 基类默认不承诺自己能安全做局部提交；
	// 只有 Canvas / TabControl / Dialog 这类“拥有完整背景语义”的 root 才会 override 为 true。
	return false;
}

bool Control::hasManagedDirtySubtree() const
{
	// 基类只感知自身是否为脏；
	// 容器类会 override 为“自身或其受管理子树是否有脏内容”。
	return dirty;
}

void Control::commitManagedRepaint()
{
	if (!show)
		return;
	// 基类兜底：如果没有更具体的容器实现，就按根级重绘处理。
	if (dirty)
		onRequestRepaintAsRoot();
}

void Control::saveBackground(int x, int y, int w, int h)
{

	if (w <= 0 || h <= 0) return;
	saveBkX = x; saveBkY = y; saveWidth = w; saveHeight = h;
	// 快照描述的是下一次局部恢复所需的屏幕背景，位置或尺寸变化时不能继续复用旧缓存。
	if (saveBkImage)
	{
		//尺寸变了才重建，避免反复 new/delete
		if (saveBkImage->getwidth() != w || saveBkImage->getheight() != h)
		{
			SX_LOGD("Snap") <<SX_T("重新保存背景快照：id=", "saveBackground rebuild: id=") << id << " size=(" << w << "x" << h << ")";

			saveBkImage.reset();
		}
	}
	else
		SX_LOGD("Snap") << SX_T("保存背景快照：id=", "saveBackground rebuild: id=") << id << " size=(" << w << "x" << h << ")";
	if (!saveBkImage) saveBkImage = std::make_unique<IMAGE>(w, h);

	SetWorkingImage(nullptr);                 // ★抓屏幕
	getimage(saveBkImage.get(), x, y, w, h);
	hasSnap = true;
}

void Control::restBackground()
{
	if (!hasSnap || !saveBkImage) return;
	// 直接回贴屏幕（与抓取一致）
	SetWorkingImage(nullptr);
	putimage(saveBkX, saveBkY, saveBkImage.get());
}

void Control::discardBackground()
{
	if (saveBkImage)
	{
		restBackground();
		SX_LOGD("Snap") << SX_T("丢弃背景快照：id=","discardBackground: id=") << id << " hasSnap=" << (hasSnap ? 1 : 0);
		saveBkImage.reset();
	}
	hasSnap = false; saveWidth = saveHeight = 0;
}

void Control::invalidateBackgroundSnapshot()
{
	if (saveBkImage)
	{
		SX_LOGD("Snap") << SX_T("作废背景快照：id=", "invalidateBackgroundSnapshot: id=") << id
			<< " hasSnap=" << (hasSnap ? 1 : 0);
		saveBkImage.reset();
	}
	hasSnap = false;
	saveBkX = saveBkY = 0;
	saveWidth = saveHeight = 0;
}

