#pragma once
/*******************************************************************************
 * @文件: CoreTypes.h
 * @摘要: 星垣框架核心类型定义文件
 * @描述:
 *     集中定义框架中使用的所有枚举类型和结构体，确保类型一致性。
 *     这是框架的类型系统基础，所有控件都依赖于此文件中定义的类型。
 *
 * @作者: 我在人间做废物
 * @日期: September 2025
 ******************************************************************************/
#include"easyX.h"
namespace StellarX
{
    /**
 * @枚举类名称: hatchStyle
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
 * 	Vertical    -  垂直线
 * 	FDiagonal   -  反斜线
 * 	BDiagonal   -  正斜线
 * 	Cross       -  十字
 * 	DiagCross   -  网格
 *
 * @使用示例:
 *   // 示例代码展示如何使用此枚举类
 *   hatchStyle var = hatchStyle::Horizontal;
 *
 * @备注:
 *   此枚举类仅支持图案填充模式
 *   枚举类在使用时，需要使用::进行调用，还要注意大小写
 */
    enum class FillStyle {
        Horizontal = HS_HORIZONTAL,	// 水平线
        Vertical = HS_VERTICAL,	// 垂直线
        FDiagonal = HS_FDIAGONAL,	// 反斜线
        BDiagonal = HS_BDIAGONAL,	// 正斜线
        Cross = HS_CROSS,		// 十字
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
     *   // 示例代码展示如何使用此枚举类
     *   FillMode var = FillMode::Solid;
     *
     * @备注:
     *  枚举类在使用时，需要使用::进行调用，还要注意大小写
     */
    enum class FillMode
    {
        Solid = BS_SOLID,       //固实填充
        Null = BS_NULL,       // 不填充
        Hatched = BS_HATCHED,    // 图案填充
        Pattern = BS_PATTERN,    // 自定义图案填充
        DibPattern = BS_DIBPATTERN  // 自定义图像填充
    };
    /**
     * @枚举类名称: linStyle
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
     *   // 示例代码展示如何使用此枚举类
     *   LineStyle var = LineStyle::Solid;
     *
     * @备注:
     *   枚举类在使用时，需要使用::进行调用，还要注意大小写
     */
    enum class LineStyle {
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

     *	       bool  operator!=(const ControlText& text);
     *	ControlText& operator=(const ControlText& text
     */
    struct ControlText
    {
        int  nHeight = 0;           //- 字体高度
        int  nWidth = 0;            //- 字体宽度 如果为0则自适应
        LPCTSTR  lpszFace = "宋体";     //- 字体名称
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
     *   ControlShape shape = ELLIPSE;
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
     * @功能描述: 定义了文本框的两种模式 
     * 
     * @详细说明:
     * 需要限制文本框是否接受用户输入时使用
     * 
     * @取值说明:
	 *   INPUT_MODE,   // 用户可输入模式
	 *	 READONLY_MODE // 只读模式
     * 
     * @使用示例:
     *   // 示例代码展示如何使用此枚举类
     *   StellarX::TextBoxmode var = EnumClassName::VALUE1;
     * 
     * @备注:
     *   枚举类的特性、与普通枚举的区别
     */
	enum class TextBoxmode
	{
		INPUT_MODE,  // 用户可输入模式
		READONLY_MODE // 只读模式
	};

    /**
 * @枚举名称: ButtonMode
 * @功能描述: brief
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
 * Button b1(100, 100, 120, 120, "测试按钮", RGB(128, 0, 0), RGB(255, 9, 9));
 *
 */
    enum class ButtonMode
    {
        NORMAL = 1, //普通模式，点击后触发回调，但不会保持状态。
        TOGGLE,     //切换模式，点击后会在选中和未选中之间切换，触发不同的回调函数。
        DISABLED    //禁用模式，按钮不可点击，显示为灰色，文本显示删除线。
    };
	struct RouRectangle
    {
        int ROUND_RECTANGLEwidth = 20;  //构成圆角矩形的圆角的椭圆的宽度。     
        int ROUND_RECTANGLEheight = 20;  //构成圆角矩形的圆角的椭圆的高度。
    };
};
