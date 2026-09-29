#include "Table.h"
#include "SxLog.h"
#include "Window.h"
#include <algorithm>

namespace
{
	int TableMbcCharLen(const std::string& text, size_t index)
	{
		const unsigned char lead = static_cast<unsigned char>(text[index]);
		if (lead <= 0x7f) return 1;
		if (lead >= 0x81 && lead <= 0xfe && index + 1 < text.size())
		{
			const unsigned char trail = static_cast<unsigned char>(text[index + 1]);
			if (trail >= 0x40 && trail <= 0xfe && trail != 0x7f) return 2;
		}
		return 1;
	}

	std::string EllipsizeTableText(const std::string& text, int maxWidth)
	{
		// 逐个 MBCS 字符试探可用宽度，保证省略号不会把双字节字符拆成半个字节。
		if (maxWidth <= 0) return "";
		if (textwidth(text.c_str()) <= maxWidth) return text;
		const std::string ellipsis = "...";
		const int ellipsisWidth = textwidth(ellipsis.c_str());
		if (ellipsisWidth > maxWidth) return "";
		size_t index = 0;
		size_t lastFit = 0;
		while (index < text.size())
		{
			const size_t next = index + static_cast<size_t>(TableMbcCharLen(text, index));
			if (next > text.size()) break;
			if (textwidth(text.substr(0, next).c_str()) > maxWidth - ellipsisWidth) break;
			lastFit = next;
			index = next;
		}
		return text.substr(0, lastFit) + ellipsis;
	}

	std::vector<std::string> NormalizeTableRow(std::vector<std::string> row, size_t headerCount)
	{
		if (headerCount == 0)
			return row;

		if (row.size() > headerCount)
			row.resize(headerCount);
		else if (row.size() < headerCount)
			row.resize(headerCount, "");

		return row;
	}

	int CalculateTotalPages(size_t rowCount, int rowsPerPage)
	{
		if (rowsPerPage < 1)
			rowsPerPage = 1;

		const int total = static_cast<int>((rowCount + static_cast<size_t>(rowsPerPage) - 1) / static_cast<size_t>(rowsPerPage));
		return total > 0 ? total : 1;
	}

	std::string BuildPageNumberText(int currentPage, int totalPages)
	{
		return std::string(TABLE_STR_PAGE_PREFIX) + std::to_string(currentPage) +
			TABLE_STR_PAGE_MID + std::to_string(totalPages) + TABLE_STR_PAGE_SUFFIX;
	}
}

// 绘制表格的当前页
// 使用双循环绘制行和列，考虑分页偏移
void Table::drawTable()
{
	if (lineHeights.empty() || colWidths.empty())
		return;

	const int border = tableBorderWidth > 0 ? tableBorderWidth : 0;

	// 表体从“表头之下”开始
	dX = x + border;
	dY = y + border + lineHeights.at(0) + TABLE_HEADER_EXTRA;          // 表头高度
	uY = dY + lineHeights.at(0) + TABLE_ROW_EXTRA;

	size_t startRow = (currentPage - 1) * rowsPerPage;
	size_t endRow = startRow + (size_t)rowsPerPage < data.size() ? startRow + (size_t)rowsPerPage : data.size();

	for (size_t i = startRow; i < endRow; ++i)
	{
		for (size_t j = 0; j < data[i].size(); ++j)
		{
			uX = dX + colWidths.at(j) + TABLE_COL_GAP;
			fillrectangle(dX, dY, uX, uY);
			const std::string display = EllipsizeTableText(data[i][j],
				(std::max)(0, colWidths.at(j) - TABLE_PAD_X * 2));
			outtextxy(dX + TABLE_PAD_X, dY + TABLE_PAD_Y, LPCTSTR(display.c_str()));
			dX += colWidths.at(j) + TABLE_COL_GAP;
		}
		dX = x + border;
		dY = uY;
		uY = dY + lineHeights.at(0) + TABLE_ROW_EXTRA;
	}
}

void Table::drawHeader()
{
	if (headers.empty() || lineHeights.empty() || colWidths.empty())
		return;

	const int border = tableBorderWidth > 0 ? tableBorderWidth : 0;
	// 内容区原点 = x+border, y+border
	dX = x + border;
	dY = y + border;
	uY = dY + lineHeights.at(0) + TABLE_HEADER_EXTRA;

	for (size_t i = 0; i < headers.size(); i++)
	{
		uX = dX + colWidths.at(i) + TABLE_COL_GAP;                // 注意这里是 +20，和表体一致
		fillrectangle(dX, dY, uX, uY);
		const std::string display = EllipsizeTableText(headers[i],
			(std::max)(0, colWidths.at(i) - TABLE_PAD_X * 2));
		outtextxy(dX + TABLE_PAD_X, dY + TABLE_PAD_Y, LPCTSTR(display.c_str()));
		dX += colWidths.at(i) + TABLE_COL_GAP;                    // 列间距 20
	}
}
// 遍历所有数据单元和表头，计算每列的最大宽度和每行的最大高度，
// 为后续绘制表格单元格提供尺寸依据。此计算在数据变更时自动触发。
void Table::initTextWaH()
{
	// 和绘制一致的单元内边距
	const int padX = TABLE_PAD_X;       // 左右 padding
	const int padY = TABLE_PAD_Y;        // 上下 padding
	const int colGap = TABLE_COL_GAP;     // 列间距
	const int border = tableBorderWidth > 0 ? tableBorderWidth : 0;
	size_t maxCols = headers.size();
	for (const auto& row : data)
		if (row.size() > maxCols)
			maxCols = row.size();

	// 统计每列最大文本宽 & 每列最大行高（包含数据 + 表头）
	colWidths.assign(maxCols, 0);
	lineHeights.assign(maxCols, 0);

	// 先看数据
	for (size_t i = 0; i < data.size(); ++i)
	{
		for (size_t j = 0; j < data[i].size(); ++j)
		{
			const int w = textwidth(LPCTSTR(data[i][j].c_str()));
			const int h = textheight(LPCTSTR(data[i][j].c_str()));
			if (w > colWidths[j])   colWidths[j] = w;
			if (h > lineHeights[j]) lineHeights[j] = h;
		}
	}
	// 再用表头更新（谁大取谁）
	for (size_t j = 0; j < headers.size(); ++j)
	{
		const int w = textwidth(LPCTSTR(headers[j].c_str()));
		const int h = textheight(LPCTSTR(headers[j].c_str()));
		if (w > colWidths[j])   colWidths[j] = w;
		if (h > lineHeights[j]) lineHeights[j] = h;
	}

	// 用“所有列的最大行高”作为一行的基准高度
	int maxLineH = 0;

	for (int h : lineHeights)
		if (h > maxLineH)
			maxLineH = h;
	if (maxLineH == 0)
		maxLineH = textheight(LPCTSTR("A"));
	std::fill(lineHeights.begin(), lineHeights.end(), maxLineH);
	if (rowsPerPage < 1)
		rowsPerPage = 1;

	// 列宽包含左右 padding：在计算完最大文本宽度后，加上 2*padX 作为单元格内边距
	for (size_t j = 0; j < colWidths.size(); ++j) {
		colWidths[j] += 2 * padX;
	}

	// 表内容总宽 = Σ(列宽 + 列间距)
	int contentW = 0;
	for (size_t j = 0; j < colWidths.size(); ++j)
		contentW += colWidths[j] + colGap;

	// 表头高 & 行高（与 drawHeader/drawTable 内部一致：+上下 padding）
	const int headerH = maxLineH + 2 * padY;
	const int rowH = maxLineH + 2 * padY;
	const int rowsH = rowH * rowsPerPage;

	// 页脚：
	const int pageTextH = textheight(LPCTSTR(pageNumtext.c_str()));
	const int btnTextH = textheight(LPCTSTR("上一页"));
	const int btnPadV = TABLE_BTN_TEXT_PAD_V;
	const int btnH = btnTextH + 2 * btnPadV;
	const int footerPad = TABLE_FOOTER_PAD;
	const int footerH = (pageTextH > btnH ? pageTextH : btnH) + footerPad;

	// 最终表宽/高：内容 + 对称边框
	this->width = contentW + (border << 1);
	this->height = headerH + rowsH + footerH + (border << 1);
	// 这是 Table 自身受控的“结构尺寸 -> 设计基线”刷新点：
	// 表格总宽高本来就由表头、表体、页脚和分页按钮共同决定，
	// 因此这里允许同步 localWidth/localHeight，作为后续锚点解算参考。
	const bool baselineChanged = (this->localWidth != this->width) || (this->localHeight != this->height);
	this->localWidth = this->width;
	this->localHeight = this->height;
	if (baselineChanged)
	{
		SX_LOGD("Layout")
			<< SX_T("Table 刷新结构设计基线：id=", "Table refresh structural design rect: id=") << id
			<< SX_T(" size=(", " size=(") << this->width << "," << this->height << ")";
	}
}

void Table::initButton()
{
	const int gap = TABLE_BTN_GAP;
	const int padH = TABLE_BTN_PAD_H;
	const int padV = TABLE_BTN_PAD_V;                                  // 按钮垂直内边距

	int pageW = textwidth(LPCTSTR(pageNumtext.c_str()));
	int lblH = textheight(LPCTSTR(pageNumtext.c_str()));

	// 统一按钮尺寸（用按钮文字自身宽高 + padding）
	int prevW = textwidth(LPCTSTR(TABLE_STR_PREV)) + padH * 2;
	int nextW = textwidth(LPCTSTR(TABLE_STR_NEXT)) + padH * 2;
	int btnH = lblH + padV * 2;

	// 基于“页码标签”的矩形来摆放：
	// prev 在页码左侧 gap 处；next 在右侧 gap 处；Y 对齐 pY
	int prevX = pX - gap - prevW;
	int nextX = pX + pageW + gap;
	int btnY = pY;                  // 和页码同一基线

	if (!prevButton)
		prevButton = std::make_unique<Button>(prevX, btnY, prevW, btnH, TABLE_STR_PREV, RGB(0, 0, 0), RGB(255, 255, 255));
	else
	{
		prevButton->setX(prevX);
		prevButton->setY(btnY);
	}

	if (!nextButton)
		nextButton = std::make_unique<Button>(nextX, btnY, nextW, btnH, TABLE_STR_NEXT, RGB(0, 0, 0), RGB(255, 255, 255));
	else
	{
		nextButton->setX(nextX);
		nextButton->setY(btnY);
	}

	prevButton->setParent(this);
	nextButton->setParent(this);

	prevButton->textStyle = this->textStyle;
	nextButton->textStyle = this->textStyle;
	prevButton->setFillMode(tableFillMode);
	nextButton->setFillMode(tableFillMode);

	prevButton->setOnClickListener([this]()
		{
			int oldPage = currentPage;
			if (currentPage > 1)
			{
				--currentPage;
				SX_LOGI("Table")
					<< SX_T("翻页：id=", "page change: id=") << id
					<< " " << oldPage << "->" << currentPage
					<< SX_T(" 总页数=", " total=") << totalPages
					<< SX_T(" 行数=", " rows=") << (int)data.size();


				dirty = true;
				if (pageNum) pageNum->setDirty(true);
			}
		});
	nextButton->setOnClickListener([this]()
		{
			int oldPage = currentPage;
			if (currentPage < totalPages)
			{
				++currentPage;
				SX_LOGI("Table")
					<< SX_T("翻页：id=", "page change: id=") << id
					<< " " << oldPage << "->" << currentPage
					<< SX_T(" 总页数=", " total=") << totalPages
					<< SX_T(" 行数=", " rows=") << (int)data.size();

				dirty = true;
				if (pageNum) pageNum->setDirty(true);
			}
		});
	isNeedButtonAndPageNum = false;
}

void Table::initPageNum()
{
	// 统一坐标系
	const int border = tableBorderWidth > 0 ? tableBorderWidth : 0;
	const int baseH = lineHeights.empty() ? 0 : lineHeights.at(0);
	const int headerH = baseH + TABLE_HEADER_EXTRA;
	const int rowsH = baseH * rowsPerPage + rowsPerPage * TABLE_ROW_EXTRA;

	// 页脚顶部位置（表头 + 可视数据区 之后）
	pY = y + border + headerH + rowsH + TABLE_FOOTER_BLANK;         // +8 顶部留白

	// 按理来说 x + (this->width - textW) / 2;就可以
	// 但是在绘制时，发现控件偏右，因此减去40
	int textW = textwidth(LPCTSTR(pageNumtext.c_str()));
	pX = x + TABLE_PAGE_TEXT_OFFSET_X + (this->width - textW) / 2;

	if (!pageNum)
		pageNum = std::make_unique<Label>(pX, pY, pageNumtext);
	else
	{
		pageNum->setX(pX);
		pageNum->setY(pY);
	}

	pageNum->setParent(this);

	pageNum->textStyle = this->textStyle;
	if (StellarX::FillMode::Null == tableFillMode)
		pageNum->setTextdisap(true);                   // 透明文本
}

void Table::drawPageNum()
{
	pageNumtext = BuildPageNumberText(currentPage, totalPages);
	if (nullptr == pageNum || isNeedButtonAndPageNum)
		initPageNum();
	pageNum->setText(pageNumtext);
	pageNum->textStyle = this->textStyle;
	if (StellarX::FillMode::Null == tableFillMode)
		pageNum->setTextdisap(true);
	// Table 每次整表重绘前都会先回贴自己的背景快照，
	// 页码区域会被一起清空。即便页码文本本身没变化，也必须强制把 Label 重新设脏，
	// 否则 Label::draw() 会因为 dirty=false 直接跳过，导致页码在表格重绘后消失。
	pageNum->setDirty(true);
	pageNum->draw();
}

void Table::drawButton()
{
	if ((nullptr == prevButton || nullptr == nextButton) || isNeedButtonAndPageNum)
		initButton();

	this->prevButton->textStyle = this->textStyle;
	this->nextButton->textStyle = this->textStyle;
	this->prevButton->setFillMode(tableFillMode);
	this->nextButton->setFillMode(tableFillMode);
	this->prevButton->setButtonShape(StellarX::ControlShape::B_RECTANGLE);
	this->nextButton->setButtonShape(StellarX::ControlShape::B_RECTANGLE);
	this->prevButton->setDirty(true);
	this->nextButton->setDirty(true);
	prevButton->draw();
	nextButton->draw();
}

void Table::setX(int x)
{
	const RECT previousCoverage = getManagedRepaintCoverageRect();
	const bool wasDrawn = hasValidBackgroundSnapshot();
	if (wasDrawn) discardBackground();
	this->x = x;
	isNeedButtonAndPageNum = true;
	dirty = true;
	if (wasDrawn)
	{
		if (auto* host = getHostWindow()) host->requestManagedRepaint(this, previousCoverage);
		else requestRepaint(parent);
	}
}

void Table::setY(int y)
{
	const RECT previousCoverage = getManagedRepaintCoverageRect();
	const bool wasDrawn = hasValidBackgroundSnapshot();
	if (wasDrawn) discardBackground();
	this->y = y;
	isNeedButtonAndPageNum = true;
	dirty = true;
	if (wasDrawn)
	{
		if (auto* host = getHostWindow()) host->requestManagedRepaint(this, previousCoverage);
		else requestRepaint(parent);
	}
}

void Table::setWidth(int width)
{
	const RECT previousCoverage = getManagedRepaintCoverageRect();
	const bool wasDrawn = hasValidBackgroundSnapshot();
	if (wasDrawn) discardBackground();
	// 运行态总宽必须和实际列宽严格一致，避免绘制超出快照与 coverage。
	// 同时考虑表体和分页栏的最小宽度，随后把余数分配到前几列，使列宽总和严格闭合。
	const int ncols = static_cast<int>(colWidths.size());
	if (ncols <= 0) {
		this->width = (std::max)(0, width);
		isNeedButtonAndPageNum = true;
		dirty = true;
		if (wasDrawn)
		{
			if (auto* host = getHostWindow()) host->requestManagedRepaint(this, previousCoverage);
			else requestRepaint(parent);
		}
		return;
	}
	const int border = (std::max)(0, tableBorderWidth);
	const int minColumnWidth = TABLE_PAD_X * 2 + 1;
	const int contentMinWidth = (border << 1) + ncols * (minColumnWidth + TABLE_COL_GAP);
	const int footerMinWidth = (border << 1) + textwidth(TABLE_STR_PREV) + textwidth(TABLE_STR_NEXT) +
		textwidth(pageNumtext.c_str()) + TABLE_BTN_PAD_H * 4 + TABLE_BTN_GAP * 2;
	const int minWidth = isShowPageButton ? (std::max)(contentMinWidth, footerMinWidth) : contentMinWidth;
	const int targetWidth = (std::max)(width, minWidth);
	const int targetColumnsWidth = targetWidth - (border << 1) - ncols * TABLE_COL_GAP;
	const int baseWidth = targetColumnsWidth / ncols;
	int remainder = targetColumnsWidth % ncols;
	for (int i = 0; i < ncols; ++i)
		colWidths[i] = baseWidth + (remainder-- > 0 ? 1 : 0);
	this->width = targetWidth;
	// 需要重新布局页脚元素
	isNeedButtonAndPageNum = true;
	dirty = true;
	if (wasDrawn)
	{
		if (auto* host = getHostWindow()) host->requestManagedRepaint(this, previousCoverage);
		else requestRepaint(parent);
	}
}

void Table::setHeight(int height)
{
	// 当前阶段 Table 明确不支持纵向 Stretch。
	// 高度链路依赖表头、表体、页脚、按钮和页码计算，
	// 因此这里保持空实现，避免被通用布局层错误拉高/压缩。
}

Table::Table(int x, int y)
	:Control(x, y, 0, 0)
{
	this->id = "Table";
	// Table 当前正式能力边界：
	// 仅允许 X 轴 Stretch，Y 轴固定尺寸。
	this->layoutCapability.allowStretchX = true;
	this->layoutCapability.allowStretchY = false;
}

Table::~Table() = default;

void Table::applyResolvedLayoutRect(const StellarX::ResolvedLayoutRect& rect)
{
	// Table 不能像普通控件那样直接写入 width/height：
	// 它的 setWidth() 内部会联动列宽与页脚布局，因此这里必须复用原有 setter 副作用。
	// 同时 Table 的 Y Fixed 是当前版本的正式行为边界：
	// 即便统一解算器给出了新的 rect.height，这里也不会接收该高度。
	if (this->x != rect.worldX)
		setX(rect.worldX);
	if (this->y != rect.worldY)
		setY(rect.worldY);
	if (this->width != rect.width)
		setWidth(rect.width);
	if (this->height != rect.height)
	{
		SX_LOGD("Layout")
			<< SX_T("Table 忽略 Y 轴运行态高度写入：id=", "Table ignore runtime Y resize: id=")
			<< id
			<< SX_T(" currentH=", " currentH=") << this->height
			<< SX_T(" requestedH=", " requestedH=") << rect.height;
	}
	dirty = true;
}

void Table::draw()
{
	//在这里先初始化保证翻页按钮不为空
	// 在一些容器中，Table不会被立即绘制可能导致事件事件传递时触发空指针警报
	// 由于单元格初始化依赖字体数据所以先设置一次字体样式
	// 先保存当前绘图状态
	saveStyle();

	// 设置表格样式
	setfillcolor(tableBkClor);
	setlinecolor(tableBorderClor);
	settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.faceName.c_str(),
		textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
		textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);
	settextcolor(textStyle.color);
	setlinestyle((int)tableLineStyle, tableBorderWidth);
	setfillstyle((int)tableFillMode);
	// 是否需要计算单元格尺寸
	if (isNeedCellSize)
	{
		initTextWaH();
		isNeedCellSize = false;
	}
	restoreStyle();
	if (this->dirty && this->show)
	{
		// 先保存当前绘图状态
		saveStyle();

		// 设置表格样式
		setfillcolor(tableBkClor);
		setlinecolor(tableBorderClor);
		settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.faceName.c_str(),
			textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
			textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);
		settextcolor(textStyle.color);
		setlinestyle((int)tableLineStyle, tableBorderWidth);
		setfillstyle((int)tableFillMode);
		setbkmode(TRANSPARENT);

		if (isNeedButtonAndPageNum)
		{
			// 数据变化可能同时改变表格宽度、页码位置和两个分页按钮的位置。
			// 必须在绘制新内容前完成三个内部控件的搬移，让 setX/setY 先回贴旧快照；
			// 否则后移动的分页按钮会用旧背景覆盖已经画好的新页码。
			pageNumtext = BuildPageNumberText(currentPage, totalPages);
			initPageNum();
			pageNum->textStyle = this->textStyle;
			pageNum->setText(pageNumtext);
			pageNum->setDirty(true);

			if (isShowPageButton)
				initButton();
			else
				isNeedButtonAndPageNum = false;
		}

		if (isNeedDrawHeaders)
		{
			// 重新设置表格样式
			setfillcolor(tableBkClor);
			setlinecolor(tableBorderClor);
			settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.faceName.c_str(),
				textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
				textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);
			settextcolor(textStyle.color);
			setlinestyle((int)tableLineStyle, tableBorderWidth);
			setfillstyle((int)tableFillMode);
			setbkmode(TRANSPARENT);
		}
		// 在绘制前先恢复并更新背景快照：
		// 如果已有快照且尺寸发生变化，先恢复旧快照以清除上一次绘制，然后丢弃旧快照再重新抓取新的区域。
		if (hasSnap)
		{
			// 始终先恢复旧背景，清除上一帧内容
			restBackground();
			// 当尺寸变化或缓存图像无效时，需要重新截图
			if (!saveBkImage || saveWidth != this->width || saveHeight != this->height)
			{
				invalidateBackgroundSnapshot();
				saveBackground(this->x, this->y, this->width, this->height);
			}
		}
		else
		{
			// 首次绘制时无背景缓存，直接抓取
			saveBackground(this->x, this->y, this->width, this->height);
		}
		// 恢复最新的背景，保证绘制区域干净
		restBackground();
		// 绘制表头

		//if (!headers.empty())
			drawHeader();
		// 绘制当前页
		drawTable();
		// 绘制页码标签
		drawPageNum();

		// 绘制翻页按钮
		if (this->isShowPageButton)
			drawButton();

		// 恢复绘图状态
		restoreStyle();
		dirty = false; // 标记不需要重绘
	}
}

bool Table::handleEvent(const ExMessage& msg)
{
	if (!show)return false;
	resetEventVisualChanged();
	bool consume = false;
	bool anyVisualChanged = false;
	if (!this->isShowPageButton)
		return consume;

	if (msg.message == WM_MOUSEMOVE)
	{
		// Table 内部分页按钮也按“后绘制者优先”的倒序语义处理。
		// drawButton() 里是 prev 再 next，因此这里先 next 再 prev。
		if (nextButton)
		{
			if (!consume)
			{
				consume = nextButton->handleEvent(msg);
				if (nextButton->didEventAffectVisual())
					anyVisualChanged = true;
			}
			else if (nextButton->clearTransientMouseState())
			{
				anyVisualChanged = true;
			}
		}
		if (prevButton)
		{
			if (!consume)
			{
				consume = prevButton->handleEvent(msg);
				if (prevButton->didEventAffectVisual())
					anyVisualChanged = true;
			}
			else if (prevButton->clearTransientMouseState())
			{
				anyVisualChanged = true;
			}
		}
	}
	else
	{
		// 点击类消息仍保持“后绘制者优先”的倒序命中语义。
		// 但和 WM_MOUSEMOVE 一样，分页按钮的视觉状态变化必须向上提升为 Table 重绘，
		// 否则按钮内部已经变成 hover/click，父级却不知道需要重画整张 Table，
		// 就会出现“状态实际变了，但屏幕上直到下一次翻页才看见”的现象。
		if (nextButton)
		{
			consume = nextButton->handleEvent(msg);
			if (nextButton->didEventAffectVisual())
				anyVisualChanged = true;
		}
		if (prevButton && !consume)
		{
			consume = prevButton->handleEvent(msg);
			if (prevButton->didEventAffectVisual())
				anyVisualChanged = true;
		}
	}

	// Table 当前还没有像 TabControl 那样把分页按钮做成可独立提交的内部绘制单元。
	// 因此只要内部按钮的 hover / click / leave 造成视觉变化，就必须把整张 Table 设脏，
	// 让父容器通过现有的 Table::draw() 链把页脚区域正确重画出来。
	if (anyVisualChanged)
		dirty = true;

	if (dirty)
		requestRepaint(parent);
	markEventVisualChanged(anyVisualChanged || dirty);
	return consume;
}

bool Table::clearTransientMouseState()
{
	bool changed = false;
	if (nextButton && nextButton->clearTransientMouseState())
		changed = true;
	if (prevButton && prevButton->clearTransientMouseState())
		changed = true;
	return changed;
}

RECT Table::getManagedRepaintCoverageRect() const
{
	// coverage 包含表体、页码和分页按钮的实际绘制范围，而不只包含表格本体。
	RECT coverage = getBoundsRect();

	if (pageNum && pageNum->IsVisible())
	{
		const RECT pageRect = pageNum->getManagedRepaintCoverageRect();
		coverage.left = (std::min)(coverage.left, pageRect.left);
		coverage.top = (std::min)(coverage.top, pageRect.top);
		coverage.right = (std::max)(coverage.right, pageRect.right);
		coverage.bottom = (std::max)(coverage.bottom, pageRect.bottom);
	}
	if (prevButton && prevButton->IsVisible())
	{
		const RECT prevRect = prevButton->getManagedRepaintCoverageRect();
		coverage.left = (std::min)(coverage.left, prevRect.left);
		coverage.top = (std::min)(coverage.top, prevRect.top);
		coverage.right = (std::max)(coverage.right, prevRect.right);
		coverage.bottom = (std::max)(coverage.bottom, prevRect.bottom);
	}
	if (nextButton && nextButton->IsVisible())
	{
		const RECT nextRect = nextButton->getManagedRepaintCoverageRect();
		coverage.left = (std::min)(coverage.left, nextRect.left);
		coverage.top = (std::min)(coverage.top, nextRect.top);
		coverage.right = (std::max)(coverage.right, nextRect.right);
		coverage.bottom = (std::max)(coverage.bottom, nextRect.bottom);
	}

	return coverage;
}

RECT Table::getManagedRepaintPersistentCoverageRect() const
{
	// Table 当前仍按整表绘制保证正确性；分页按钮 Tooltip 属于临时浮层，
	// 不能进入持久 coverage，否则可能污染外层兄弟控件快照。
	RECT coverage = getBoundsRect();

	if (pageNum && pageNum->IsVisible())
	{
		const RECT pageRect = pageNum->getManagedRepaintPersistentCoverageRect();
		coverage.left = (std::min)(coverage.left, pageRect.left);
		coverage.top = (std::min)(coverage.top, pageRect.top);
		coverage.right = (std::max)(coverage.right, pageRect.right);
		coverage.bottom = (std::max)(coverage.bottom, pageRect.bottom);
	}
	if (prevButton && prevButton->IsVisible())
	{
		const RECT prevRect = prevButton->getManagedRepaintPersistentCoverageRect();
		coverage.left = (std::min)(coverage.left, prevRect.left);
		coverage.top = (std::min)(coverage.top, prevRect.top);
		coverage.right = (std::max)(coverage.right, prevRect.right);
		coverage.bottom = (std::max)(coverage.bottom, prevRect.bottom);
	}
	if (nextButton && nextButton->IsVisible())
	{
		const RECT nextRect = nextButton->getManagedRepaintPersistentCoverageRect();
		coverage.left = (std::min)(coverage.left, nextRect.left);
		coverage.top = (std::min)(coverage.top, nextRect.top);
		coverage.right = (std::max)(coverage.right, nextRect.right);
		coverage.bottom = (std::max)(coverage.bottom, nextRect.bottom);
	}

	return coverage;
}

void Table::setHeaders(std::initializer_list<std::string> headers)
{
	this->headers.clear();
	for (auto& lis : headers)
		this->headers.push_back(lis);

	if (!this->headers.empty())
	{
		for (auto& row : this->data)
			row = NormalizeTableRow(std::move(row), this->headers.size());
	}

	totalPages = CalculateTotalPages(this->data.size(), rowsPerPage);
	if (currentPage > totalPages)
		currentPage = totalPages;

	SX_LOGI("Table") << SX_T("设置表头：id=","setHeaders: id=") << id << SX_T("总数="," count=") << (int)this->headers.size();
	isNeedCellSize = true; // 标记需要重新计算单元格尺寸
	isNeedDrawHeaders = true; // 标记需要重新绘制表头
	isNeedButtonAndPageNum = true;
	dirty = true;

}

void Table::setData(std::vector<std::string> data)
{
	data = NormalizeTableRow(std::move(data), headers.size());

	this->data.push_back(data);

	totalPages = CalculateTotalPages(this->data.size(), rowsPerPage);
	if (currentPage > totalPages)
		currentPage = totalPages;

	isNeedCellSize = true;
	isNeedButtonAndPageNum = true;
	dirty = true;

	SX_LOGI("Table")
		<< SX_T("新增Data：id=", "appendRow: id=") << id
		<< SX_T(" 本行列数=", " cols=") << (int)data.size()
		<< SX_T(" 数据总行数=", " totalRows=") << (int)this->data.size()
		<< SX_T(" 总页数=", " totalPages=") << totalPages;
}


void Table::setData(std::initializer_list<std::vector<std::string>> data)
{
	int addedRows = 0;
	for (auto row : data)
	{
		this->data.push_back(NormalizeTableRow(std::move(row), headers.size()));
		++addedRows;
	}

	totalPages = CalculateTotalPages(this->data.size(), rowsPerPage);
	if (currentPage > totalPages)
		currentPage = totalPages;
	isNeedCellSize = true; // 标记需要重新计算单元格尺寸
	isNeedButtonAndPageNum = true;
	dirty = true;
	SX_LOGI("Table")
		<< SX_T("新增多行Data：id=", "appendRows: id=") << id
		<< SX_T(" 新增行数=", " addedRows=") << addedRows
		<< SX_T(" 数据总行数=", " totalRows=") << (int)this->data.size()
		<< SX_T(" 总页数=", " totalPages=") << totalPages;


}

void Table::setRowsPerPage(int rows)
{
	this->rowsPerPage = rows;
	if (this->rowsPerPage < 1)
		this->rowsPerPage = 1;
	totalPages = CalculateTotalPages(data.size(), rowsPerPage);
	if (currentPage > totalPages)
		currentPage = totalPages;
	isNeedCellSize = true; // 标记需要重新计算单元格尺寸
	isNeedButtonAndPageNum = true;
	dirty = true;
}

void Table::showPageButton(bool isShow)
{
	this->isShowPageButton = isShow;
	this->dirty = true;
}

void Table::setTableBorder(COLORREF color)
{
	this->tableBorderClor = color;
	this->dirty = true;
}

void Table::setTableBk(COLORREF color)
{
	this->tableBkClor = color;
	this->dirty = true;
}

void Table::setTableFillMode(StellarX::FillMode mode)
{
	if (StellarX::FillMode::Solid == mode || StellarX::FillMode::Null == mode)
		this->tableFillMode = mode;
	else
		this->tableFillMode = StellarX::FillMode::Solid;
	if (this->prevButton && this->nextButton && this->pageNum)
	{
		this->prevButton->textStyle = this->textStyle;
		this->nextButton->textStyle = this->textStyle;
		this->prevButton->setFillMode(tableFillMode);
		this->nextButton->setFillMode(tableFillMode);
		if (StellarX::FillMode::Null == tableFillMode)
			pageNum->setTextdisap(true);
		this->prevButton->setDirty(true);
		this->nextButton->setDirty(true);
	}
	this->dirty = true;
}

void Table::setTableLineStyle(StellarX::LineStyle style)
{
	this->tableLineStyle = style;
	this->dirty = true;
}

void Table::setTableBorderWidth(int width)
{
	this->tableBorderWidth = width;
	this->dirty = true;
}

void Table::clearHeaders()
{
	this->headers.clear();
	isNeedCellSize = true; // 标记需要重新计算单元格尺寸
	isNeedDrawHeaders = true; // 标记需要重新绘制表头
	isNeedButtonAndPageNum = true;// 标记需要重新计算翻页按钮和页码信息
	dirty = true;
	SX_LOGI("Table") << SX_T("清除表头：id=","clearHeaders: id=" )<< id;

}

void Table::clearData()
{
	this->data.clear();
	this->currentPage = 1;
	this->totalPages = CalculateTotalPages(this->data.size(), rowsPerPage);
	isNeedCellSize = true; // 标记需要重新计算单元格尺寸
	isNeedButtonAndPageNum = true;// 标记需要重新计算翻页按钮和页码信息
	dirty = true;
	SX_LOGI("Table") << SX_T("清除表格数据：id=","clearData: id=") << id;
}

void Table::resetTable()
{
	clearHeaders();
	clearData();
}

void Table::onWindowResize()
{
	Control::onWindowResize();          // 先处理自己
	if (this->prevButton && this->nextButton && this->pageNum)
	{
		prevButton->onWindowResize();
		nextButton->onWindowResize();
		pageNum->onWindowResize();
	}
}

int Table::getCurrentPage() const
{
	return this->currentPage;
}

int Table::getTotalPages() const
{
	return this->totalPages;
}

int Table::getRowsPerPage() const
{
	return this->rowsPerPage;
}

bool Table::getShowPageButton() const
{
	return this->isShowPageButton;
}

COLORREF Table::getTableBorder() const
{
	return this->tableBorderClor;
}

COLORREF Table::getTableBk() const
{
	return this->tableBkClor;
}

StellarX::FillMode Table::getTableFillMode() const
{
	return this->tableFillMode;
}

StellarX::LineStyle Table::getTableLineStyle() const
{
	return this->tableLineStyle;
}

std::vector<std::string> Table::getHeaders() const
{
	return this->headers;
}

std::vector<std::vector<std::string>> Table::getData() const
{
	return this->data;
}

int Table::getTableBorderWidth() const
{
	return this->tableBorderWidth;
}

int Table::getTableWidth() const
{
	int temp = 0;
	for (auto& w : colWidths)
		temp += w;
	return temp;
}

int Table::getTableHeight() const
{
	return this->height;
}
