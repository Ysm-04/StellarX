/*******************************************************************************
 * @文件: CoreTypes.h
 * @摘要: 星垣(StellarX)框架核心类型定义文件
 * @描述:
 *     集中定义框架中使用的所有枚举类型和结构体，确保类型一致性。
 *     这是框架的类型系统基础，所有控件都依赖于此文件中定义的类型。
 *
 * @作者: 我在人间做废物
 * @日期: September 2025
 ******************************************************************************/
#pragma once

#include "easyx.h"

 /**
  * @命名空间: StellarX
  *
  * @详细说明:
  * 集中定义框架中使用的所有枚举类型和结构体，确保类型一致性。
  *     这是框架的类型系统基础，所有控件都依赖于此文件中定义的类型
  *
  * @使用示例:
  *   StellarX::FillStyle::Horizontal - 填充样式
  *
  * @备注:
  *   不用单独包含此头文件，已在StellarX.h中包含，包含唯一对外头文件即可
  */
namespace StellarX
{
	/**
	 * @枚举类名称: FillStyle
	 * @功能描述: 用来定义控件填充图案的枚举类
	 *
	 * @详细说明:
	 *    根据此枚举类可以自定义控件填充图案
	 *    可以在控件初始化时设置填充图案
	 *    根据具体情况选择不同的填充图案
	 *    默认填充图案为水平线
	 *
	 * @取值说明:
	 *  Horizontal  -  水平线
	 *  Vertical    -  垂直线
	 *  FDiagonal   -  反斜线
	 *  BDiagonal   -  正斜线
	 *  Cross       -  十字
	 *  DiagCross   -  网格
	 *
	 * @使用示例:
	 *   FillStyle var = FillStyle::Horizontal;
	 *
	 * @备注:
	 *   此枚举类仅支持图案填充模式
	 */
	enum class FillStyle
	{
		Horizontal = HS_HORIZONTAL,	// 水平线
		Vertical = HS_VERTICAL,	    // 垂直线
		FDiagonal = HS_FDIAGONAL,	// 反斜线
		BDiagonal = HS_BDIAGONAL,	// 正斜线
		Cross = HS_CROSS,		    // 十字
		DiagCross = HS_DIAGCROSS	// 网格
	};

	/**
	 * @枚举类名称: FillMode
	 * @功能描述:   用来定义控件填充模式的枚举类
	 *
	 * @详细说明:
	 *    根据此枚举类可以自定义控件填充模式
	 *    可以在控件初始化时设置填充模式
	 *    根据具体情况选择不同的填充模式
	 *    默认填充模式为固实填充
	 *
	 * @取值说明:
	 *    Solid       -   固实填充
	 *	  Null        -   不填充
	 *	  Hatched     -   图案填充
	 *	  Pattern     -   自定义图案填充
	 *	  DibPattern  -   自定义图像填充
	 *
	 * @使用示例:
	 *   FillMode var = FillMode::Solid;
	 */
	enum class FillMode
	{
		Solid = BS_SOLID,       //固实填充
		Null = BS_NULL,         // 不填充
		Hatched = BS_HATCHED,   // 图案填充
		Pattern = BS_PATTERN,   // 自定义图案填充
		DibPattern = BS_DIBPATTERN  // 自定义图像填充
	};

	/**
	 * @枚举类名称: LineStyle
	 * @功能描述: 此枚举类用来定义控件边框线型
	 *
	 * @详细说明:
	 *       根据此枚举类可以自定义控件边框线型
	 *       可以在控件初始化时设置边框线型
	 *	     根据具体情况选择不同的线型
	 *       默认线型为实线
	 *
	 * @取值说明:
	 *     Solid          // 实线
	 *	   Dash           // 虚线
	 *	   Dot            // 点线
	 *	   DashDot        // 点划线
	 *	   DashDotDot     // 双点划线
	 *	   Null           // 无线
	 *
	 * @使用示例:
	 *   LineStyle var = LineStyle::Solid;
	 */
	enum class LineStyle
	{
		Solid = PS_SOLID,       // 实线
		Dash = PS_DASH,         // 虚线
		Dot = PS_DOT,           // 点线
		DashDot = PS_DASHDOT,   // 点划线
		DashDotDot = PS_DASHDOTDOT, // 双点划线
		Null = PS_NULL          // 无线
	};

	/**
	 * @结构体名称: ControlText
	 * @功能描述: 控件字体样式  可以自定义不同的样式
	 *
	 * @详细说明:
	 * 主要使用的场景为：需要修改或想自定义控件字体大小，字体样式，颜色等
	 *
	 * @成员说明:
	 *      int       nHeight = 0;          - 字体高度
	 *	    int       nWidth = 0;           - 字体宽度 如果为0则自适应
	 *	LPCTSTR       lpszFace = "宋体";    - 字体名称
	 *	COLORREF      color = RGB(0, 0, 0); - 字体颜色
	 *	     int      nEscapement = 0;      - 字符串旋转角度
	 *	     int      nOrientation = 0;     - 字符旋转角度
	 *	     int      nWeight = 0;          - 字体粗细 范围0~1000 0表示默认
	 *	     bool     bItalic = false;      - 是否斜体
	 *	     bool     bUnderline = false;   - 是否下划线
	 *	     bool     bStrikeOut = false;   - 是否删除线
	 */
	struct ControlText
	{
		int  nHeight = 0;           //- 字体高度
		int  nWidth = 0;            //- 字体宽度 如果为0则自适应
		LPCTSTR  lpszFace = "微软雅黑";     //- 字体名称
		COLORREF color = RGB(0, 0, 0);  //- 字体颜色
		int nEscapement = 0;       //- 字符串旋转角度
		int nOrientation = 0;      //- 字符旋转角度
		int nWeight = 0;           //- 字体粗细 范围0~1000 0表示默认
		bool bItalic = false;       //- 是否斜体
		bool bUnderline = false;    //- 是否下划线
		bool bStrikeOut = false;     //- 是否删除线

		bool operator!=(const ControlText& text);
		ControlText& operator=(const ControlText& text);
	};

	/**
	 * @枚举名称: ControlShape
	 * @功能描述: 枚举控件的不同几何样式
	 *
	 * @详细说明:
	 * 定义了四种（有无边框算一种）不同的几何样式，可以根据具体需求
	 * 自定义控件的形状。
	 *
	 * @取值说明:
	 *      RECTANGLE = 1,     //有边框矩形
	 *  	B_RECTANGLE,       //无边框矩形
	 *  	ROUND_RECTANGLE,   //有边框圆角矩形
	 *  	B_ROUND_RECTANGLE, //无边框圆角矩形
	 *  	CIRCLE,            //有边框圆形
	 *  	B_CIRCLE,          //无边框圆形
	 *  	ELLIPSE,           //有边框椭圆
	 *  	B_ELLIPSE          //无边框椭圆
	 *
	 * @使用示例:
	 *   ControlShape shape = ControlShape::ELLIPSE;
	 *
	 * @备注:
	 *   按钮类支持所有形状，部分控件只支持部分形状，具体请参考控件类。
	 */
	enum class ControlShape
	{
		RECTANGLE = 1,     //有边框矩形
		B_RECTANGLE,       //无边框矩形
		ROUND_RECTANGLE,   //有边框圆角矩形
		B_ROUND_RECTANGLE, //无边框圆角矩形
		CIRCLE,            //有边框圆形
		B_CIRCLE,          //无边框圆形
		ELLIPSE,           //有边框椭圆
		B_ELLIPSE          //无边框椭圆
	};

	/**
	 * @枚举类名称: TextBoxmode
	 * @功能描述: 定义了文本框的三种模式
	 *
	 * @详细说明:
	 * 需要限制文本框是否接受用户输入时使用
	 *
	 * @取值说明:
	 *   INPUT_MODE,   // 用户可输入模式
	 *	 READONLY_MODE // 只读模式
	 *   PASSWORD_MODE // 密码模式
	 */
	enum class TextBoxmode
	{
		INPUT_MODE,    // 用户可输入模式
		READONLY_MODE, // 只读模式
		PASSWORD_MODE  // 密码模式
	};

	/**
	 * @枚举名称: ButtonMode
	 * @功能描述: 定义按钮的工作模式
	 *
	 * @详细说明:
	 * 根据按钮的工作模式，按钮可以有不同的行为。
	 * 用户可以在具体情况下设置按钮的工作模式。
	 *
	 * @取值说明:
	 *   NORMAL = 1, -  普通模式，点击后触发回调，但不会保持状态。
	 *	 TOGGLE,     -  切换模式，点击后会在选中和未选中之间切换，触发不同的回调函数。
	 *	 DISABLED    -  禁用模式，按钮不可点击，显示为灰色，文本显示删除线。
	 *
	 * @使用示例:
	 *   ButtonMode mode = ButtonMode::NORMAL;
	 */
	enum class ButtonMode
	{
		NORMAL = 1, //普通模式
		TOGGLE,     //切换模式
		DISABLED    //禁用模式
	};

	/**
	 * @结构体名称: RouRectangle
	 * @功能描述: 定义了控件圆角矩形样式时圆角的椭圆尺寸
	 *
	 * @详细说明:
	 * 需要修改控件圆角矩形样式时的圆角椭圆。
	 *
	 * @成员说明:
	 *  int ROUND_RECTANGLEwidth = 20;  //构成圆角矩形的圆角的椭圆的宽度。
	 *  int ROUND_RECTANGLEheight = 20;  //构成圆角矩形的圆角的椭圆的高度。
	 */
	struct RouRectangle
	{
		int ROUND_RECTANGLEwidth = 20;  //构成圆角矩形的圆角的椭圆的宽度。
		int ROUND_RECTANGLEheight = 20;  //构成圆角矩形的圆角的椭圆的高度。
	};

	// 消息框类型
	enum class MessageBoxType
	{
		OK,                // 只有确定按钮
		OKCancel,          // 确定和取消按钮
		YesNo,             // 是和否按钮
		YesNoCancel,       // 是、否和取消按钮
		RetryCancel,       // 重试和取消按钮
		AbortRetryIgnore,  // 中止、重试和忽略按钮
	};

	// 消息框返回值
	enum class MessageBoxResult
	{
		OK = 1,         // 确定按钮
		Cancel = 2,     // 取消按钮
		Yes = 6,        // 是按钮
		No = 7,         // 否按钮
		Abort = 3,      // 中止按钮
		Retry = 4,      // 重试按钮
		Ignore = 5      // 忽略按钮
	};
#if 0   //布局管理器相关  —待实现—
	/*
	 *
	 *@枚举名称: LayoutKind
	 * @功能描述 : 定义布局管理类型
	 *
	 *@详细说明 :
	 * 根据布局管理类型，控件可以有不同的布局方式。
	 * 用户可以在具体情况下设置布局管理类型。
	 *
	 * @取值说明 :
	 * Absolute：不管，保持子控件自己的坐标（向后兼容）。
	 * HBox：    水平方向排队；支持固定宽、权重分配、对齐、拉伸。
	 * VBox：    竖直方向排队；同上。
	 * Grid（网格）：按行列摆；支持固定/自适应/权重行列；支持跨行/跨列；单元内对齐/拉伸。
	 *
	 */
	 // 布局类型
	enum class LayoutKind
	{
		Absolute = 1,
		HBox,
		VBox,
		Grid,
		Flow,
		Stack
	};

	// 布局参数
	struct LayoutParams
	{
		// 边距左、右、上、下
		int marginL = 0, marginR = 0, marginT = 0, marginB = 0;
		// 固定尺寸（>=0 强制；-1 用控件当前尺寸）
		int fixedW = -1, fixedH = -1;
		// 主轴权重（HBox=宽度、VBox=高度、Grid见下）
		float weight = 0.f;
		// 对齐（非拉伸时生效）
		enum Align { Start = 0, Center = 1, End = 2, Stretch = 3 };
		int alignX = Start; // HBox: 次轴=Y；VBox: 次轴=X；Grid: 单元内
		int alignY = Start; // Grid ：控制单元内垂直（HBox / VBox通常只用 alignX）
		// Grid 专用（可先不做）
		int gridRow = 0, gridCol = 0, rowSpan = 1, colSpan = 1;
		// Flow 专用（可先不做）
		int flowBreak = 0; // 1=强制换行
	};
#endif

	/*
	 * @枚举名称: TabPlacement
	 * @功能描述: 定义了选项卡页签的不同位置
	 *
	 * @详细说明:
	 * 根据选项卡页签的位置，选项卡页签可以有不同的布局方式。
	 *
	 * @成员说明:
	 *  Top,   - 选项卡页签位于顶部
	 *  Bottom, - 选项卡页签位于底部
	 *  Left,  - 选项卡页签位于左侧
	 *  Right  - 选项卡页签位于右侧
	 *
	 * @使用示例:
	 *   TabPlacement placement = TabPlacement::Top;
	 */
	enum class TabPlacement
	{
		Top,
		Bottom,
		Left,
		Right
	};
	/*
	* @枚举名称: LayoutMode
	* @功能描述: 定义了两种布局模式
	*
	* @详细说明:
	* 根据不同模式，在窗口拉伸时采用不同的布局策略
	*
	* @成员说明:
	* Fixed,          - 固定布局
	* AnchorToEdges   - 锚定布局
	*
	* @使用示例:
	*   LayoutMode mode = LayoutMode::Fixed;
	*/
	enum class LayoutMode
	{
		Fixed,
		AnchorToEdges
	};
	/*
	* @枚举名称: Anchor
	* @功能描述: 定义了控件相对于窗口锚定的位置
	*
	* @详细说明:
	* 根据不同的锚定位置，有不同的拉伸策略
	*
	* @成员说明:
	*  Top,    - 锚定上边，控件上边与窗口上侧距离保持不变
	*  Bottom, - 锚定底边，控件底边与窗口底边距离保持不变
	*  Left,   - 锚定左边，控件左边与窗口左侧距离保持不变
	*  Right   - 锚定右边，控件上边与窗口右侧距离保持不变
	*
	* @使用示例:
	*   Anchor a = Anchor::Top;
	*/
	enum class Anchor
	{
		NoAnchor = 0,
		Left = 1,
		Right,
		Top,
		Bottom
	};

	/*******************************************************************************
	 * @枚举: AxisSizePolicy
	 * @摘要: 单轴尺寸策略，决定该轴是否允许因父容器变化而拉伸
	 * @详细说明:
	 *   该枚举只决定“尺寸是否变化”，不决定“位置如何求解”。
	 *   若需要在固定尺寸下决定控件如何位移，应结合 AxisAlignPolicy 一起使用。
	 *
	 * @成员说明:
	 *   Stretch   - 允许尺寸随父容器变化而变化
	 *   FixedSize - 尺寸保持设计态大小，仅位置按锚点/位移策略变化
	 ******************************************************************************/
	enum class AxisSizePolicy
	{
		Stretch,
		FixedSize
	};

	/*******************************************************************************
	 * @枚举: AxisAlignPolicy
	 * @摘要: 单轴固定尺寸时的位置策略
	 * @详细说明:
	 *   该策略只在“该轴不拉伸”时生效，用于决定控件在父容器变化后如何重新定位。
	 *   其中 Proportional 只用于位置求解，不参与尺寸缩放。
	 *
	 * @成员说明:
	 *   Start        - 保持起边距离
	 *   End          - 保持终边距离
	 *   Center       - 保持相对父容器中心的偏移关系
	 *   Proportional - 保持设计态中的相对位置比例
	 ******************************************************************************/
	enum class AxisAlignPolicy
	{
		Start,
		End,
		Center,
		Proportional
	};

	/*******************************************************************************
	 * @结构体: AxisLayoutSpec
	 * @摘要: 描述单个坐标轴上的锚点与解算策略
	 * @详细说明:
	 *   一个控件完整布局由“水平轴 + 垂直轴”两份 AxisLayoutSpec 组成。
	 *   anchorStart / anchorEnd 只表达当前轴是否锚定起止边；
	 *   sizePolicy 决定该轴是否允许拉伸；
	 *   alignPolicy 则用于固定尺寸时的位置求解。
	 ******************************************************************************/
	struct AxisLayoutSpec
	{
		bool anchorStart = false;
		bool anchorEnd = false;
		AxisSizePolicy sizePolicy = AxisSizePolicy::FixedSize;
		AxisAlignPolicy alignPolicy = AxisAlignPolicy::Start;
	};

	/*******************************************************************************
	 * @结构体: LayoutSpec
	 * @摘要: 控件完整布局规格，由水平轴和垂直轴两部分组成
	 ******************************************************************************/
	struct LayoutSpec
	{
		AxisLayoutSpec horizontal{};
		AxisLayoutSpec vertical{};
	};

	/*******************************************************************************
	 * @结构体: LayoutCapability
	 * @摘要: 控件能力边界，声明控件在哪些轴上允许 Stretch
	 * @详细说明:
	 *   规则表负责定义“某种组合如何解算”，LayoutCapability 负责声明
	 *   “该控件是否允许采用这种组合”。例如 Table 当前阶段仅允许 X 轴拉伸，
	 *   因此 allowStretchY 会被显式关闭。
	 ******************************************************************************/
	struct LayoutCapability
	{
		bool allowStretchX = true;
		bool allowStretchY = true;
	};

	/*******************************************************************************
	 * @结构体: ResolvedLayoutRect
	 * @摘要: 统一布局解算后的运行态矩形结果
	 * @详细说明:
	 *   localX/localY 表示解算完成后的“父局部坐标”；
	 *   worldX/worldY 表示映射到窗口世界坐标后的最终绘制位置；
	 *   width/height 为运行态尺寸。
	 ******************************************************************************/
	struct ResolvedLayoutRect
	{
		int localX = 0;
		int localY = 0;
		int width = 0;
		int height = 0;
		int worldX = 0;
		int worldY = 0;
	};
}
