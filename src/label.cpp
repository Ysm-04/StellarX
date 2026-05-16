#include "Label.h"

namespace
{
	// Label 的内容尺寸刷新不依赖 EasyX 当前绘图上下文，
	// 直接使用 GDI 依据“文本 + 字体样式”测量宽高。
	// 这样无论是文本变化、字体样式变化，还是控件在窗口正式绘制前进入 dirty，
	// 都可以先把运行态尺寸收口好，而不必等到 draw() 再决定几何。
	void MeasureLabelText(const std::string& text, const StellarX::ControlText& style, int& width, int& height)
	{
		width = 0;
		height = 0;

		HDC hdc = CreateCompatibleDC(nullptr);
		if (!hdc)
			return;

		LOGFONTA fontDesc{};
		fontDesc.lfHeight = style.nHeight;
		fontDesc.lfWidth = style.nWidth;
		fontDesc.lfEscapement = style.nEscapement;
		fontDesc.lfOrientation = style.nOrientation;
		fontDesc.lfWeight = style.nWeight;
		fontDesc.lfItalic = style.bItalic ? TRUE : FALSE;
		fontDesc.lfUnderline = style.bUnderline ? TRUE : FALSE;
		fontDesc.lfStrikeOut = style.bStrikeOut ? TRUE : FALSE;
		fontDesc.lfCharSet = DEFAULT_CHARSET;
		fontDesc.lfOutPrecision = OUT_DEFAULT_PRECIS;
		fontDesc.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		fontDesc.lfQuality = DEFAULT_QUALITY;
		fontDesc.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		lstrcpynA(fontDesc.lfFaceName, style.lpszFace ? style.lpszFace : "微软雅黑", LF_FACESIZE);

		HFONT font = CreateFontIndirectA(&fontDesc);
		HFONT oldFont = nullptr;
		if (font)
			oldFont = (HFONT)SelectObject(hdc, font);

		if (text.empty())
		{
			TEXTMETRICA tm{};
			if (GetTextMetricsA(hdc, &tm))
				height = tm.tmHeight;
		}
		else
		{
			SIZE size{};
			if (GetTextExtentPoint32A(hdc, text.c_str(), (int)text.size(), &size))
			{
				width = size.cx;
				height = size.cy;
			}
			if (height == 0)
			{
				TEXTMETRICA tm{};
				if (GetTextMetricsA(hdc, &tm))
					height = tm.tmHeight;
			}
		}

		if (font)
		{
			if (oldFont)
				SelectObject(hdc, oldFont);
			DeleteObject(font);
		}
		DeleteDC(hdc);
	}
}

Label::Label()
	:Control(0, 0, 0, 0)
{
	this->id = "Label";
	// Label 当前阶段明确走“内容驱动尺寸”语义：
	// 外部布局器只安置位置，不通过 Stretch 改写它的宽高。
	this->layoutCapability.allowStretchX = false;
	this->layoutCapability.allowStretchY = false;
	this->text = "默认标签";
	textStyle.color = RGB(0, 0, 0);
	textBkColor = RGB(255, 255, 255);; //默认白色背景
}

Label::Label(int x, int y, std::string text, COLORREF textcolor, COLORREF bkColor)
	:Control(x, y, 0, 0)
{
	this->id = "Label";
	this->layoutCapability.allowStretchX = false;
	this->layoutCapability.allowStretchY = false;
	this->text = text;
	textStyle.color = textcolor;
	textBkColor = bkColor; //默认白色背景
}

void Label::refreshContentDrivenRuntimeGeometry()
{
	// Label 的尺寸来源于“当前文本 + 当前字体样式”。
	// 这里只更新运行态 width/height，不自动回写 localWidth/localHeight，
	// 避免内容变化把设计基线偷偷带偏。
	if (contentMeasureValid &&
		lastMeasuredText == text &&
		!(lastMeasuredStyle != textStyle))
	{
		return;
	}

	int newWidth = 0;
	int newHeight = 0;
	MeasureLabelText(text, textStyle, newWidth, newHeight);

	const bool sizeChanged = (newWidth != this->width) || (newHeight != this->height);
	if (sizeChanged && hasSnap)
	{
		// 运行态尺寸变化时，先恢复旧背景，避免文本缩短后旧像素残留在屏幕上。
		discardBackground();
	}

	this->width = newWidth;
	this->height = newHeight;
	lastMeasuredText = text;
	lastMeasuredStyle = textStyle;
	contentMeasureValid = true;
	dirty = true;
}

void Label::draw()
{
	if (dirty && show)
	{
		saveStyle();
		if (textBkDisap)
			setbkmode(TRANSPARENT); //设置背景透明
		else
		{
			setbkmode(OPAQUE); //设置背景不透明
			setbkcolor(textBkColor); //设置背景颜色
		}
		settextcolor(textStyle.color);
		settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.lpszFace,
			textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
			textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);   //设置字体样式
		if ((saveBkX != this->x) || (saveBkY != this->y) || (!hasSnap) || (saveWidth != this->width) || (saveHeight != this->height) || !saveBkImage)
			saveBackground(this->x, this->y, this->width, this->height);
		// 恢复背景（清除旧内容）
		restBackground();
		outtextxy(x, y, LPCTSTR(text.c_str()));
		restoreStyle();
		dirty = false;
	}
}

void Label::hide()
{
	discardBackground(); // 还原并释放快照
	dirty = false;
}

void Label::setDirty(bool dirty)
{
	if (dirty)
	{
		// 只要 Label 进入脏状态，就在绘制前把内容驱动尺寸同步到运行态。
		// 这样文本变化和字体样式变化都会在 draw() 之前完成几何刷新。
		refreshContentDrivenRuntimeGeometry();
	}
	this->dirty = dirty;
}

void Label::setTextdisap(bool key)
{
	textBkDisap = key;
	this->dirty = true;
}

void Label::setTextBkColor(COLORREF color)
{
	textBkColor = color;
	this->dirty = true;
}

void Label::setText(std::string text)
{
	this->text = text;
	refreshContentDrivenRuntimeGeometry();
}

void Label::applyResolvedLayoutRect(const StellarX::ResolvedLayoutRect& rect)
{
	// 通用布局器给出的 rect.width/rect.height 只代表“外部几何解算结果”。
	// Label 当前阶段的语义是内容驱动尺寸，因此这里只接位置，不接外部宽高，
	// 宽高继续由 refreshContentDrivenRuntimeGeometry() 按文本和字体样式刷新。
	refreshContentDrivenRuntimeGeometry();
	applyRuntimeRectDirect(rect.worldX, rect.worldY, this->width, this->height);
}
