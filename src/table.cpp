#include "StellarX/Table.h"
// 绘制表格的当前页
// 使用双循环绘制行和列，考虑分页偏移
void Table::drawTable()
{
	dX = x;
	dY = uY;
	uY = dY + lineHeights.at(0) + 10;
	
	for (int i = (currentPage * rowsPerPage - rowsPerPage); i < (currentPage*rowsPerPage) && i < data.size(); i++)
	{
		for (int j = 0; j < data[i].size(); j++)
		{
			uX = dX + colWidths.at(j) + 20;
			fillrectangle(dX, dY, uX, uY);
			outtextxy(dX + 10, dY + 5, LPCTSTR(data[i][j].c_str()));
			dX += this->colWidths.at(j) + 20;
		}
		dX = x;
		dY = uY;
		uY = dY + lineHeights.at(0) + 10;

	}
	uY = y + lineHeights.at(0) + 10;
}

void Table::drawHeader()
{
	
	uY = dY + lineHeights.at(0) + 10;
	for(int i = 0; i < headers.size(); i++)
	{
	    uX = dX + colWidths.at(i) + 20;
		fillrectangle(dX, dY, uX, uY);
		outtextxy(dX + 10, dY + 5, LPCTSTR(headers[i].c_str()));
		dX += this->colWidths.at(i) + 20;
	}

	
}
// 初始化文本宽度和高度计算
// 遍历所有数据和表头，计算每列的最大宽度和行高
// 此方法在数据变更时自动调用
void Table::initTextWaH()
{
	this->colWidths.resize(this->headers.size());
	this->lineHeights.resize(this->headers.size());
	int width = 0;
	int height = 0;
	//计算数据尺寸
	for (int i = 0; i < data.size(); i++)
	{
		for (int j = 0; j < data[i].size(); j++)
		{
			width = textwidth(LPCTSTR(data[i][j].c_str()));
			height = textheight(LPCTSTR(data[i][j].c_str()));
			if (width > this->colWidths.at(j))
				this->colWidths.at(j) = width;
			if (height > this->lineHeights[j])
				this->lineHeights.at(j) = height;
		}
	}

	for (int i = 0; i < this->headers.size(); i++)
	{
		width = textwidth(LPCTSTR(headers[i].c_str()));
		height = textheight(LPCTSTR(headers[i].c_str()));
		if (width > this->colWidths.at(i))
			this->colWidths.at(i) = width;
		if (height > this->lineHeights[i])
			this->lineHeights.at(i) = height;
	}

	// 计算表格总宽度和高度
	this->width = 0;
	for (int i = 0; i < colWidths.size(); i++)
		this->width += colWidths.at(i) + 20;
	LINESTYLE currentStyle;
	
	this->width += tableBorderWidth;

	this->height = lineHeights.at(0) * (rowsPerPage + 1) + rowsPerPage * 10+20 ; // 表头+数据行+页码区域

	// 如果背景图像不存在或尺寸不匹配，创建或重新创建
	if (saveBkImage == nullptr) {
		saveBkImage = new IMAGE(width, height);
	}
	else if (saveBkImage->getwidth() != width || saveBkImage->getheight() != height) {
		delete saveBkImage;
		saveBkImage = new IMAGE(width, height);
	}
}

void Table::initButton()
{
	int x1, x2;
	int y1, y2;
	x1 = pX - 70;
	x2 = pX + textwidth(LPCTSTR(pageNumtext.c_str())) + 10;
	y1 = y2 = pY;
	this->prevButton = new Button(x1, y1, 60, textheight(LPCTSTR(pageNumtext.c_str())), "上一页", RGB(0, 0, 0), RGB(255, 255, 255));
	this->nextButton = new Button(x2, y2, 60, textheight(LPCTSTR(pageNumtext.c_str())), "下一页", RGB(0, 0, 0), RGB(255, 255, 255));
	prevButton->setOnClickListener([this]() 
		{if (this->currentPage > 1)
	{
		this->currentPage--;
		this->dirty = true;
		this->draw();
	} });
	
	nextButton->setOnClickListener([this]()
		{if (this->currentPage < (this->totalPages))
	{
		this->currentPage++;
		this->dirty = true;
		this->draw();
	}});
}

void Table::initPageNum()
{
	if (0 == pY)
		pY = uY + lineHeights.at(0) * rowsPerPage + rowsPerPage * 10+10;
	for (int i = 0; i < colWidths.size(); i++)
		this->pX += colWidths.at(i) + 20;
	this->pX -= textwidth(LPCTSTR(pageNumtext.c_str()));
	this->pX /= 2;
	this->pX += x;
	this->pageNum = new Label(this->pX, pY, pageNumtext);
	//pageNum->setTextdisap(true);
	pageNum->textStyle = this->textStyle;
}

void Table::drawPageNum()
{
	if (nullptr == pageNum)
		initPageNum();
	pageNumtext = std::to_string(currentPage);
	pageNumtext += "页/第";
	pageNumtext += std::to_string(totalPages);
	pageNumtext += "页";
	pageNum->setText(pageNumtext);
	pageNum->draw();
	
}

void Table::drawButton()
{
	if (nullptr == prevButton || nullptr == nextButton)
		initButton();
	prevButton->draw();
	nextButton->draw();

}

Table::Table(int x, int y)
	:Control(x, y, 0,0)
{	
	//this->saveBkImage = new IMAGE(this->width,this->height);
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
	if (this->dirty)
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

		// 是否需要计算单元格尺寸
		if (isNeedCellSize)
		{
			initTextWaH();
			isNeedCellSize = false;
		}

		// 在绘制表格之前捕获背景
		// 只有在第一次绘制或者尺寸变化时才需要重新捕获背景
		static bool firstDraw = true;
		if (firstDraw || isNeedDrawHeaders) {
			// 确保在绘制任何表格内容之前捕获背景
			if (saveBkImage) {
				// 临时恢复样式，确保捕获正确的背景
				restoreStyle();
				if(tableBorderWidth>1)
					getimage(saveBkImage, this->x- tableBorderWidth, this->y- tableBorderWidth, this->width+ tableBorderWidth, this->height+ tableBorderWidth);
				else
					getimage(saveBkImage, this->x, this->y, this->width, this->height);
				saveStyle(); // 重新保存样式

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
			firstDraw = false;
		}

		// 恢复背景（清除旧内容）
		if (saveBkImage) {
			if (tableBorderWidth > 1)
				putimage(this->x - tableBorderWidth, this->y - tableBorderWidth, saveBkImage);
			else
				putimage(this->x,this->y,this->saveBkImage);
		}

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

void Table::handleEvent(const ExMessage& msg)
{
	if(!this->isShowPageButton)
		return;
	else
	{
		prevButton->handleEvent(msg);
		nextButton->handleEvent(msg);
	}
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

void Table::setData(const std::vector<std::string>& data)
{
	this->data.push_back(data);
	totalPages = (this->data.size() + rowsPerPage - 1) / rowsPerPage;
	if (totalPages < 1)
		totalPages = 1;
	isNeedCellSize = true; // 标记需要重新计算单元格尺寸
	dirty = true;
}

void Table::setData(const std::initializer_list<std::vector<std::string>>& data)
{
	for (auto lis : data)
		this->data.push_back(lis);
	totalPages = (this->data.size() + rowsPerPage - 1) / rowsPerPage;
	if (totalPages < 1)
		totalPages = 1;
	isNeedCellSize = true; // 标记需要重新计算单元格尺寸
	dirty = true;
}

void Table::setRowsPerPage(int rows)
{
	this->rowsPerPage = rows;
	totalPages = (data.size() + rowsPerPage - 1) / rowsPerPage;
	if (totalPages < 1)
		totalPages = 1;
	isNeedCellSize = true; // 标记需要重新计算单元格尺寸
	dirty = true;
}

void Table::showPageButton(bool isShow)
{
	this->isShowPageButton = isShow;
}

void Table::setTableBorder(COLORREF color)
{
	this->tableBorderClor = color;
}

void Table::setTableBk(COLORREF color)
{
	this->tableBkClor = color;
}

void Table::setTableFillMode(StellarX::FillMode mode)
{
	if (StellarX::FillMode::Solid == mode || StellarX::FillMode::Null == mode)
		this->tableFillMode = mode;
	else
		this->tableFillMode = StellarX::FillMode::Solid;
}

void Table::setTableLineStyle(StellarX::LineStyle style)
{
	this->tableLineStyle = style;
}

void Table::setTableBorderWidth(int width)
{
	this->tableBorderWidth = width;
}

int Table::getCurrentPage() const
{
	return this->currentPage;
}

int Table::getTotalPages() const
{
	return this->totalPages;;
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


