﻿/*******************************************************************************
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

class Table :public Control
{
private:
	std::vector<std::vector<std::string>> data; // 表格数据
	std::vector<std::string> headers;           // 表格表头
	std::string pageNumtext = "页码标签";       // 页码标签文本

	int tableBorderWidth = 1;                   // 边框宽度

	std::vector<int> colWidths;                // 每列的宽度
	std::vector<int> lineHeights;			   // 每行的高度

	int rowsPerPage = 5;                        // 每页显示的行数
	int currentPage = 1;                        // 当前页码
	int totalPages  = 1;                         // 总页数

	bool isShowPageButton = true;			    // 是否显示翻页按钮
	bool isNeedDrawHeaders = true;              // 是否需要绘制表头
	bool isNeedCellSize = true;                 // 是否需要计算单元格尺寸

	Button* prevButton = nullptr;               // 上一页按钮
	Button* nextButton = nullptr;			    // 下一页按钮
	Label*  pageNum = nullptr;                   //页码文本
	
	int dX = x, dY = y;							// 单元格的开始坐标
	int uX = x, uY = y;							// 单元格的结束坐标

	int pX = 0;                                 //标签左上角坐标
	int pY = 0;                                 //标签左上角坐标

	StellarX::FillMode  tableFillMode = StellarX::FillMode::Solid;    //填充模式
	StellarX::LineStyle tableLineStyle = StellarX::LineStyle::Solid; // 线型
	COLORREF tableBorderClor = RGB(0, 0, 0);     // 表格边框颜色
	COLORREF tableBkClor = RGB(255, 255, 255);         // 表格背景颜色

	void initTextWaH();  //初始化文本像素宽度和高度
	void initButton();   //初始化翻页按钮
	void initPageNum();  //初始化页码标签

	void drawTable();    //绘制当前页
	void drawHeader();   //绘制表头
	void drawPageNum();  //绘制页码信息
	void drawButton();   //绘制翻页按钮
private:
	//用来检查对话框是否模态,此控件不做实现
	bool model() const override { return false; };
public:
	StellarX::ControlText textStyle; // 文本样式

public:
	Table(int x, int y);
	~Table();

	// 绘制表格
	void draw() override;
	//事件处理
	bool handleEvent(const ExMessage& msg) override;

	//设置表头
	void setHeaders(std::initializer_list<std::string> headers);
	//设置表格数据
	void setData(std::vector<std::string> data);
	void setData(std::initializer_list<std::vector<std::string>> data);
	//设置每页显示的行数
	void setRowsPerPage(int rows);
	//设置是否显示翻页按钮
	void showPageButton(bool isShow);
	//设置表格边框颜色
	void setTableBorder(COLORREF color);
	//设置表格背景颜色
	void setTableBk(COLORREF color);
	//设置填充模式
	void setTableFillMode(StellarX::FillMode mode);
	//设置线型
	void setTableLineStyle(StellarX::LineStyle style);
	//设置边框宽度
	void setTableBorderWidth(int width);

	//************************** 获取属性 *****************************/

	//获取当前页码
	int getCurrentPage() const;
	//获取总页数
	int getTotalPages() const;
	//获取每页显示的行数
	int getRowsPerPage() const;
	//获取是否显示翻页按钮
	bool getShowPageButton() const;
	//获取表格边框颜色
	COLORREF getTableBorder() const;
	//获取表格背景颜色
	COLORREF getTableBk() const;
	//获取填充模式
	StellarX::FillMode getTableFillMode() const;
	//获取线型
	StellarX::LineStyle getTableLineStyle() const;
	//获取表头
	std::vector<std::string> getHeaders () const;
	//获取表格数据
	std::vector<std::vector<std::string>> getData() const;
	//获取表格边框宽度
	int getTableBorderWidth() const;


};

