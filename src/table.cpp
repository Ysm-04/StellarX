#include "Table.h"
// 绘制表格的当前页
// 使用双循环绘制行和列，考虑分页偏移
void Table::drawTable()
{
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
			outtextxy(dX + TABLE_PAD_X, dY + TABLE_PAD_Y, LPCTSTR(data[i][j].c_str()));
			dX += colWidths.at(j) + TABLE_COL_GAP;
		}
		dX = x + border;
		dY = uY;
		uY = dY + lineHeights.at(0) + TABLE_ROW_EXTRA;
	}

}

void Table::drawHeader()
{
	
	const int border = tableBorderWidth > 0 ? tableBorderWidth : 0;
	// 内容区原点 = x+border, y+border
	dX = x + border;
	dY = y + border;
	uY = dY + lineHeights.at(0) + TABLE_HEADER_EXTRA;

	for (size_t i = 0; i < headers.size(); i++)
	{
		uX = dX + colWidths.at(i) + TABLE_COL_GAP;                // 注意这里是 +20，和表体一致
		fillrectangle(dX, dY, uX, uY);
		outtextxy(dX + TABLE_PAD_X, dY + TABLE_PAD_Y, LPCTSTR(headers[i].c_str()));
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

	// 统计每列最大文本宽 & 每列最大行高（包含数据 + 表头）
	colWidths.assign(headers.size(), 0);
	lineHeights.assign(headers.size(), 0);

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

	// 列的像素宽 = 内容宽 + 左右 padding
	// 表内容总宽 = Σ(列宽 + 列间距)
	int contentW = 0;
	for (size_t j = 0; j < colWidths.size(); ++j)
		contentW += (colWidths[j] + 2 * padX) + colGap;

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
		prevButton = new Button(prevX, btnY, prevW, btnH, TABLE_STR_PREV, RGB(0, 0, 0), RGB(255, 255, 255));
	else
	{
		prevButton->setX(prevX);
		prevButton->setY(btnY);
	}

	if (!nextButton)
		nextButton = new Button(nextX, btnY, nextW, btnH, TABLE_STR_NEXT, RGB(0, 0, 0), RGB(255, 255, 255));
	else
	{
		nextButton->setX(nextX);
		nextButton->setY(btnY);
	}

	prevButton->textStyle = this->textStyle;
	nextButton->textStyle = this->textStyle;
	prevButton->setFillMode(tableFillMode);
	nextButton->setFillMode(tableFillMode);

	prevButton->setOnClickListener([this]() 
		{
		if (currentPage > 1) 
		{
			--currentPage;
			dirty = true;
			if (pageNum) pageNum->setDirty(true);
		}
		});
	nextButton->setOnClickListener([this]() 
		{
		if (currentPage < totalPages) 
		{
			++currentPage;
			dirty = true;
			if (pageNum) pageNum->setDirty(true);
		}
		});
}

void Table::initPageNum()
{
	// 统一坐标系
	const int border = tableBorderWidth > 0 ? tableBorderWidth : 0;
	const int baseH = lineHeights.empty() ? 0 : lineHeights.at(0);
	const int headerH = baseH + TABLE_HEADER_EXTRA;
	const int rowsH = baseH * rowsPerPage + rowsPerPage * TABLE_ROW_EXTRA;

	// 内容宽度 = sum(colWidths + 20)；initTextWaH() 已把 this->width += 2*border
	// 因此 contentW = this->width - 2*border 更稳妥
	const int contentW = this->width - (border << 1);

	// 页脚顶部位置（表头 + 可视数据区 之后）
	pY = y + border + headerH + rowsH + TABLE_FOOTER_BLANK;         // +8 顶部留白

	// 按理来说 x + (this->width - textW) / 2;就可以
	// 但是在绘制时，发现控件偏右，因此减去40
	int textW = textwidth(LPCTSTR(pageNumtext.c_str()));
	pX = x + TABLE_PAGE_TEXT_OFFSET_X +(this->width - textW) / 2;

	if (!pageNum) 
		pageNum = new Label(pX, pY, pageNumtext);
	else          
	{
		pageNum->setX(pX);
		pageNum->setY(pY);
	}

	pageNum->textStyle = this->textStyle;
	if (StellarX::FillMode::Null == tableFillMode)
		pageNum->setTextdisap(true);                   // 透明文本
}

void Table::drawPageNum()
{
	
	pageNumtext = "第";
	pageNumtext+= std::to_string(currentPage);
	pageNumtext += "页/共";
	pageNumtext += std::to_string(totalPages);
	pageNumtext += "页";
	if (nullptr == pageNum)
		initPageNum();
	pageNum->setText(pageNumtext);
	pageNum->textStyle = this->textStyle;
	if (StellarX::FillMode::Null == tableFillMode)
		pageNum->setTextdisap(true);
	pageNum->draw();
	
}

void Table::drawButton()
{
	if (nullptr == prevButton || nullptr == nextButton)
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

Table::Table(int x, int y)
	:Control(x, y, 0, 0)
{
	this->id = "Table";
}

Table::~Table()
{
	if (this->prevButton)
		delete this->prevButton;
	if (this->nextButton)
		delete this->nextButton;
	if (this->pageNum)
		delete this->pageNum;
	if (this->saveBkImage)
		delete this->saveBkImage;
	this->prevButton = nullptr;
	this->nextButton = nullptr;
	this->pageNum = nullptr;
	this->saveBkImage = nullptr;
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
	settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.lpszFace,
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
		settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.lpszFace,
			textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
			textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);
		settextcolor(textStyle.color);
		setlinestyle((int)tableLineStyle, tableBorderWidth);
		setfillstyle((int)tableFillMode);
		setbkmode(TRANSPARENT);

		if (isNeedDrawHeaders)
		{
			// 重新设置表格样式
			setfillcolor(tableBkClor);
			setlinecolor(tableBorderClor);
			settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.lpszFace,
				textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
				textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);
			settextcolor(textStyle.color);
			setlinestyle((int)tableLineStyle, tableBorderWidth);
			setfillstyle((int)tableFillMode);
			setbkmode(TRANSPARENT);
		}
		//确保在绘制任何表格内容之前捕获背景
	   // 临时恢复样式，确保捕获正确的背景
		if ((!hasSnap) || (saveWidth != this->width) || (saveHeight != this->height)||!saveBkImage)
			saveBackground(this->x, this->y, this->width, this->height);
		// 恢复背景（清除旧内容）
		restBackground();
		// 绘制表头

		dX = x;
		dY = y;
		drawHeader();
		this->isNeedDrawHeaders = false;
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
	if(!show)return false;
	bool consume = false;
	if(!this->isShowPageButton)
		return consume;
	else
	{
		if(prevButton)consume = prevButton->handleEvent(msg);
		if (nextButton&&!consume)
			consume = nextButton->handleEvent(msg);
	}
	if (dirty)
		requestRepaint();
	return consume;
}

void Table::setHeaders(std::initializer_list<std::string> headers)
{
	this->headers.clear();
	for (auto lis : headers)
		this->headers.push_back(lis);
	isNeedCellSize = true; // 标记需要重新计算单元格尺寸
	isNeedDrawHeaders = true; // 标记需要重新绘制表头
	dirty = true;
}

void Table::setData( std::vector<std::string> data)
{
	if (data.size() < headers.size())
		for (int i = 0; data.size() <= headers.size(); i++)
			data.push_back("");
	this->data.push_back(data);
	totalPages = ((int)this->data.size() + rowsPerPage - 1) / rowsPerPage;
	if (totalPages < 1)
		totalPages = 1;
	isNeedCellSize = true; // 标记需要重新计算单元格尺寸
	dirty = true;
}

void Table::setData( std::initializer_list<std::vector<std::string>> data)
{
	for (auto lis : data)
		if (lis.size() < headers.size())
		{
			for (size_t i = lis.size(); i< headers.size(); i++)
				lis.push_back("");
			this->data.push_back(lis);
		}
		else
			this->data.push_back(lis);
		
	totalPages = ((int)this->data.size() + rowsPerPage - 1) / rowsPerPage;
	if (totalPages < 1)
		totalPages = 1;
	isNeedCellSize = true; // 标记需要重新计算单元格尺寸
	dirty = true;
}

void Table::setRowsPerPage(int rows)
{
	this->rowsPerPage = rows;
	totalPages = ((int)data.size() + rowsPerPage - 1) / rowsPerPage;
	if (totalPages < 1)
		totalPages = 1;
	isNeedCellSize = true; // 标记需要重新计算单元格尺寸
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


