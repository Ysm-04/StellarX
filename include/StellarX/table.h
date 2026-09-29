/*******************************************************************************
 * @类: Table
 * @摘要: 高级表格控件，支持分页显示和大数据量展示
 * @描述:
 *     提供完整的数据表格功能，包括表头、数据行、分页导航等。
 *     自动计算列宽行高，支持自定义样式和交互。
 *
 * @特性:
 *     - 自动分页和页码计算
 *     - 可配置的每页行数
 *     - 自定义边框样式和填充模式
 *     - 翻页按钮和页码显示
 *     - 背景缓存优化渲染性能
 *
 * @使用场景: 数据展示、报表生成、记录浏览等表格需求
 * @所属框架: 星垣(StellarX) GUI框架
 * @作者: 我在人间做废物
 ******************************************************************************/

#pragma once
#include "Control.h"
#include "Button.h"
#include "Label.h"

 // === Table metrics (layout) ===
#define TABLE_PAD_X                 10   // 单元格左右内边距
#define TABLE_PAD_Y                  5   // 单元格上下内边距
#define TABLE_COL_GAP               20   // 列间距（列与列之间）
#define TABLE_HEADER_EXTRA          10   // 表头额外高度（若不想复用 pad 计算）
#define TABLE_ROW_EXTRA             10   // 行额外高度（同上；或直接用 2*TABLE_PAD_Y）
#define TABLE_BTN_GAP               12   // 页码与按钮的水平间距
#define TABLE_BTN_PAD_H             12   // 按钮水平 padding
#define TABLE_BTN_PAD_V              0   // 按钮垂直 padding（initButton）
#define TABLE_BTN_TEXT_PAD_V         8   // 计算页脚高度时的按钮文字垂直 padding（initTextWaH）
#define TABLE_FOOTER_PAD            16   // 页脚额外高度（底部留白）
#define TABLE_FOOTER_BLANK           8   // 页脚顶部留白
#define TABLE_PAGE_TEXT_OFFSET_X   (-40) // 页码文本的临时水平修正

// === Table defaults (theme) ===
#define TABLE_DEFAULT_ROWS_PER_PAGE  5
#define TABLE_DEFAULT_BORDER_WIDTH   1
#define TABLE_DEFAULT_BORDER_COLOR   RGB(0,0,0)
#define TABLE_DEFAULT_BG_COLOR       RGB(255,255,255)

// === Strings (i18n ready) ===
#define TABLE_STR_PREV             "上一页"
#define TABLE_STR_NEXT             "下一页"
#define TABLE_STR_PAGE_PREFIX      "第"
#define TABLE_STR_PAGE_MID         "页/共"
#define TABLE_STR_PAGE_SUFFIX      "页"

class Table :public Control
{
private:
	std::vector<std::vector<std::string>> data; // 表格数据
	std::vector<std::string> headers;           // 表格表头
	std::string pageNumtext = "页码标签";       // 页码标签文本

	int tableBorderWidth = TABLE_DEFAULT_BORDER_WIDTH;                   // 边框宽度

	std::vector<int> colWidths;                // 每列的宽度
	std::vector<int> lineHeights;			   // 每行的高度

	int rowsPerPage = TABLE_DEFAULT_ROWS_PER_PAGE;  // 每页显示的行数
	int currentPage = 1;                        // 当前页码
	int totalPages = 1;                         // 总页数

	bool isShowPageButton = true;			    // 是否显示翻页按钮
	bool isNeedDrawHeaders = true;              // 是否需要绘制表头(暂时废弃，单做保留，后期优化可能用到)
	bool isNeedCellSize = true;                 // 是否需要计算单元格尺寸
	bool isNeedButtonAndPageNum = true;         // 是否需要计算翻页按钮和页码信息

	std::unique_ptr<Button> prevButton;        // 上一页按钮
	std::unique_ptr<Button> nextButton;	    // 下一页按钮
	std::unique_ptr<Label> pageNum;            //页码文本

	int dX = x, dY = y;							// 单元格的开始坐标
	int uX = x, uY = y;							// 单元格的结束坐标

	int pX = 0;                                 //标签左上角坐标
	int pY = 0;                                 //标签左上角坐标

	StellarX::FillMode  tableFillMode = StellarX::FillMode::Solid;    //填充模式
	StellarX::LineStyle tableLineStyle = StellarX::LineStyle::Solid; // 线型
	COLORREF tableBorderClor = TABLE_DEFAULT_BORDER_COLOR;     // 表格边框颜色
	COLORREF tableBkClor = TABLE_DEFAULT_BG_COLOR;         // 表格背景颜色

	void initTextWaH();  // 初始化文本像素宽度和高度，并据此更新列宽/行高缓存
	void initButton();   // 初始化或刷新上一页/下一页按钮
	void initPageNum();  // 初始化或刷新页码标签

	void drawTable();    // 绘制当前页的数据行
	void drawHeader();   // 绘制表头区域
	void drawPageNum();  // 绘制页码信息
	void drawButton();   // 绘制翻页按钮
	// 统一解算后的内部应用入口：
	// Table 需要复用自己已有的 setWidth/setX 等副作用逻辑，因此单独接管应用过程。
	void applyResolvedLayoutRect(const StellarX::ResolvedLayoutRect& rect) override;
private:
	// Table 不是模态对话框，固定返回 false。
	bool model() const override { return false; };
public:
	StellarX::ControlText textStyle; // 文本样式
	// 修改表格运行态 X，并同步内部页脚元素位置
	void setX(int x) override;
	// 修改表格运行态 Y，并同步内部页脚元素位置
	void setY(int y) override;
	// 修改表格运行态宽度；当前支持横向拉伸。
	// 会重新计算列宽、页脚控件位置和托管重绘 coverage。
	void setWidth(int width) override;
	// 修改表格运行态高度；当前版本作为行为边界明确忽略纵向 Stretch。
	// 表格高度由表头、行高、页脚和按钮布局共同计算，外部写入会被有意丢弃。
	void setHeight(int height) override;
public:
	// 构造一个基础表格，后续通过表头和数据初始化内容
	Table(int x, int y);
	// 析构表格并释放内部按钮、页码标签等资源。
	~Table();

	// 绘制表格背景、表头、当前页数据、页码和翻页按钮。
	void draw() override;
	// 处理翻页按钮事件，并在页码变化时请求重绘。
	bool handleEvent(const ExMessage& msg) override;
	// 只清理分页按钮的 hover / tooltip / 临时按下态，不触发表格整体重绘。
	bool clearTransientMouseState() override;
	// Table 重绘时会一并绘制页码 Label 和分页按钮，coverage 需要把这些内部绘制单元并入。
	RECT getManagedRepaintCoverageRect() const override;
	// 持久 coverage 排除分页按钮 Tooltip 等临时浮层。
	RECT getManagedRepaintPersistentCoverageRect() const override;

	// 设置表头；会按新的表头列数规范化已有数据行，并重新计算分页和列宽。
	void setHeaders(std::initializer_list<std::string> headers);
	// 设置表格数据（单行追加）。
	// 该接口是 append 语义，不会替换已有数据；如需覆盖请先 clearData() 或 resetTable()。
	void setData(std::vector<std::string> data);
	// 设置表格数据（多行追加）；如需覆盖请先 clearData() 或 resetTable()
	void setData(std::initializer_list<std::vector<std::string>> data);
	// 设置每页显示的行数；小于 1 会被夹紧为 1，并同步修正当前页和总页数。
	void setRowsPerPage(int rows);
	// 设置是否显示翻页按钮。
	void showPageButton(bool isShow);
	// 设置表格边框颜色。
	void setTableBorder(COLORREF color);
	// 设置表格背景颜色。
	void setTableBk(COLORREF color);
	// 设置表格填充模式。
	void setTableFillMode(StellarX::FillMode mode);
	// 设置表格边框线型。
	void setTableLineStyle(StellarX::LineStyle style);
	// 设置表格边框宽度。
	void setTableBorderWidth(int width);
	// 清空表头并标记尺寸缓存失效。
	void clearHeaders();
	// 清空表格数据并重置分页状态。
	void clearData();
	// 清空表头和数据，恢复为空表状态。
	void resetTable();
	// 窗口变化时丢弃背景快照并标记表格需要重绘。
	void onWindowResize() override;

	//************************** 获取属性 *****************************/

	// 获取当前页码。
	int getCurrentPage() const;
	// 获取总页数。
	int getTotalPages() const;
	// 获取每页显示的行数。
	int getRowsPerPage() const;
	// 获取是否显示翻页按钮。
	bool getShowPageButton() const;
	// 获取表格边框颜色。
	COLORREF getTableBorder() const;
	// 获取表格背景颜色。
	COLORREF getTableBk() const;
	// 获取表格填充模式。
	StellarX::FillMode getTableFillMode() const;
	// 获取表格边框线型。
	StellarX::LineStyle getTableLineStyle() const;
	// 获取表头副本。
	std::vector<std::string> getHeaders() const;
	// 获取表格数据副本。
	std::vector<std::vector<std::string>> getData() const;
	// 获取表格边框宽度。
	int getTableBorderWidth() const;
	// 获取表格当前运行态尺寸；高度为内部内容计算结果，不受 setHeight() 外部写入驱动。
	int getTableWidth() const;
	// 获取表格当前运行态高度。
	int getTableHeight() const;
};
