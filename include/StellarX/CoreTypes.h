/*******************************************************************************
 * @文件: CoreTypes.h
 * @摘要: 星垣(StellarX)框架核心类型定义文件
 * @描述:
 *     集中定义框架中使用的所有枚举类型和结构体，确保类型一致性。
 *     这是框架的类型系统基础，所有控件都依赖于此文件中定义的类型。
 *
 * @作者: 我在人间做废物
 * @日期: 2025 年 9 月
 ******************************************************************************/
#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#ifndef WINVER
#define WINVER _WIN32_WINNT
#endif
#include "easyx.h"
#include <cstdint>
#include <string>

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
	 * @结构体名称: ControlHandle
	 * @功能描述: 表示控件在当前进程内的稳定身份句柄
	 *
	 * @详细说明:
	 * 控件由 Window、Canvas、TabControl 等容器持有所有权，外部调用方不应长期保存
	 * 控件对象地址。ControlHandle 用一个不暴露对象地址的整数值表示控件身份，
	 * 用于后续 removeControl、findControl、getControlHandles 等安全查询和删除操作。
	 *
	 * 句柄值为 0 时表示无效句柄；非 0 值由控件构造时单调分配，只保证当前进程
	 * 本次运行期间的身份稳定，不保证跨进程、跨运行或序列化后仍然有效。
	 *
	 * @成员说明:
	 * std::uint64_t value = 0; - 句柄原始值，0 表示无效句柄。
	 *
	 * @使用示例:
	 *   StellarX::ControlHandle handle = wnd.addControl(std::move(control));
	 *   if (handle) wnd.removeControl(handle);
	 *
	 * @备注:
	 * 控件被移除后，旧句柄仍可安全用于比较或查询，但 findControl 可能返回 nullptr；
	 * 调用方不得把句柄视为对象生命周期仍然存在的证明。
	 */
	struct ControlHandle
	{
		/// 进程内单调分配的原始身份值；0 表示无效句柄。
		std::uint64_t value = 0;

		// 判断句柄是否有效；value 为 0 时返回 false。
		explicit operator bool() const noexcept { return value != 0; }
		// 比较两个句柄是否指向同一控件身份。
		bool operator==(const ControlHandle& other) const noexcept { return value == other.value; }
		// 比较两个句柄是否指向不同控件身份。
		bool operator!=(const ControlHandle& other) const noexcept { return value != other.value; }
	};

	/**
	 * @结构体名称: FocusTargetId
	 * @功能描述: 表示一个逻辑焦点参与节点的稳定身份
	 *
	 * @详细说明:
	 * owner 指向拥有该焦点节点的 Control。subNode 为 0 表示 Control 本身；框架
	 * 当前公开的焦点节点都使用该形式，调用方不应自行构造非 0 的内部节点编号。
	 * ControlHandle 在进程内单调分配且不复用，因此本结构不额外复制 generation。
	 */
	struct FocusTargetId
	{
		/// 拥有该焦点节点的控件句柄。
		ControlHandle owner{};
		/// 控件内部轻量节点编号；0 表示控件本身。
		std::uint32_t subNode = 0;

		/// 判断焦点身份是否有效。
		explicit operator bool() const noexcept { return static_cast<bool>(owner); }
		/// 比较两个焦点身份是否完全一致。
		bool operator==(const FocusTargetId& other) const noexcept
		{
			return owner == other.owner && subNode == other.subNode;
		}
		/// 比较两个焦点身份是否不同。
		bool operator!=(const FocusTargetId& other) const noexcept { return !(*this == other); }
	};

	// 标识一个 Window 内的逻辑焦点作用域。0 表示无效，根作用域由 FocusManager 创建。
	struct FocusScopeId
	{
		/// Window 内单调分配的作用域值；0 表示无效作用域。
		std::uint64_t value = 0;

		/// 判断作用域身份是否有效。
		explicit operator bool() const noexcept { return value != 0; }
		/// 比较两个作用域身份是否相同。
		bool operator==(const FocusScopeId& other) const noexcept { return value == other.value; }
		/// 比较两个作用域身份是否不同。
		bool operator!=(const FocusScopeId& other) const noexcept { return value != other.value; }
	};

	// 控件声明自己允许通过哪些入口获得逻辑焦点。
	enum class FocusPolicy : std::uint8_t
	{
		None = 0,              ///< 不允许获得逻辑焦点。
		Programmatic = 1u << 0, ///< 允许程序主动申请焦点。
		Pointer = 1u << 1,     ///< 允许通过 Pointer 点击获得焦点。
		Tab = 1u << 2          ///< 允许进入 Tab 遍历顺序。
	};

	/// 合并两组焦点入口策略。
	constexpr FocusPolicy operator|(FocusPolicy left, FocusPolicy right) noexcept
	{
		return static_cast<FocusPolicy>(static_cast<std::uint8_t>(left) |
			static_cast<std::uint8_t>(right));
	}

	/// 计算两组焦点入口策略的交集。
	constexpr FocusPolicy operator&(FocusPolicy left, FocusPolicy right) noexcept
	{
		return static_cast<FocusPolicy>(static_cast<std::uint8_t>(left) &
			static_cast<std::uint8_t>(right));
	}

	/// 判断 value 是否包含指定焦点入口策略。
	constexpr bool hasFocusPolicy(FocusPolicy value, FocusPolicy flag) noexcept
	{
		return (value & flag) != FocusPolicy::None;
	}

	/// 一次焦点变化请求的来源。
	enum class FocusReason : std::uint8_t
	{
		Programmatic,       ///< 程序主动申请。
		Pointer,            ///< Pointer 点击命中。
		TabTraversal,       ///< 普通 Tab 顺序遍历。
		TabRoving,          ///< 复合控件内部 roving-tab-stop 移动。
		ScopeActivation,    ///< 新焦点作用域激活。
		ScopeRestore,       ///< 作用域退出后的焦点恢复。
		InvalidationFallback ///< 当前焦点失效后的回退。
	};

	// FocusManager 的可断言结果；ReentrancyLimit 是独立错误，不依赖日志判断。
	enum class FocusError : std::uint8_t
	{
		None,              ///< 没有错误。
		InvalidTarget,     ///< 目标身份无效或已失效。
		NotFocusable,      ///< 目标策略不允许本次入口获得焦点。
		NotVisible,        ///< 目标或其祖先不可见。
		Disabled,          ///< 目标当前禁用。
		OutsideActiveScope, ///< 目标不在活动焦点作用域中。
		InvalidScope,      ///< 作用域身份或层级无效。
		ReentrancyLimit    ///< 焦点重入事务超过活性保护上限。
	};

	/// 焦点变化请求的可断言结果。
	struct FocusChangeResult
	{
		/// 本次请求是否实际改变了焦点身份。
		bool changed = false;
		/// 请求结果错误码。
		FocusError error = FocusError::None;
		/// 请求处理完成后的当前焦点身份。
		FocusTargetId focused{};
	};

	/**
	 * 当前焦点叶子发布给原生 IME 的最小 caret 锚点值。
	 *
	 * 坐标均为宿主 HWND 客户区坐标；clientPoint 是 caret 底部落点，
	 * clientLineRect 是当前单行文字区域，供 CFS_EXCLUDE 避让候选窗；
	 * clientEditRect 是完整文本编辑命中区，供属主线程判断 Pointer 是否
	 * 离开旧文本叶子。
	 * publicationGeneration 由 Window 的跨线程发布器填写，控件只提供
	 * target/scope、布局代次和几何。该结构不拥有控件、视图或 HDC。
	 */
	struct ImeCaretAnchorSnapshot
	{
		/// 发布该锚点的焦点目标。
		FocusTargetId target{};
		/// 发布时目标所属焦点作用域。
		FocusScopeId scope{};
		/// HWND 客户区中的候选窗定位点，通常为 caret 底端。
		POINT clientPoint{};
		/// HWND 客户区中的当前文字行矩形。
		RECT clientLineRect{};
		// 焦点文字叶节点拥有的完整客户区编辑表面。WndProc 只使用这份不可变
		// 几何区分“同一编辑器内点击”和“Pointer 离开”；HWND 属主线程绝不
		// 解析 target 背后的 Control。
		RECT clientEditRect{};
		/// 控件文字布局发生变化时递增的代次。
		std::uint64_t layoutGeneration = 0;
		/// Window 每次跨线程发布锚点时递增的代次。
		std::uint64_t publicationGeneration = 0;
		/// 当前快照是否可用于原生 IME 定位。
		bool valid = false;
	};

	/**
	 * 焦点参与协议的瞬时快照。
	 * 所有布尔值只对采集时刻有效；任何用户回调结束后都必须重新解析并拉取新快照。
	 */
	struct FocusTargetSnapshot
	{
		/// 当前焦点节点的稳定身份。
		FocusTargetId id{};
		/// 当前节点的逻辑父焦点身份。
		FocusTargetId logicalParent{};
		/// 当前节点所属焦点作用域。
		FocusScopeId scope{};
		/// 当前节点允许的焦点入口集合。
		FocusPolicy policy = FocusPolicy::None;
		/// 采集时节点及其祖先是否有效可见。
		bool visible = false;
		/// 采集时节点是否启用。
		bool enabled = false;
		/// 采集时节点是否允许通过任一入口获得焦点。
		bool focusable = false;
		/// 采集时节点是否进入普通 Tab 遍历顺序。
		bool tabStop = false;
		// 定向 TextInputEvent 路由使用的动态叶节点能力。只读或非文字控件
		// 返回 false，但不会因此丢失逻辑焦点。
		bool acceptsTextInput = false;
		// 只有能够自行绘制 IME 临时组合串的叶节点才返回 true。仅当焦点目标
		// 发布该能力时，WndProc 才能抑制系统默认组合窗。
		bool supportsInlineComposition = false;
		// 选区能力与文字修改能力独立：只读文本可支持 caret/选区，同时拒绝
		// TextInputEvent 投递。
		bool supportsTextSelection = false;
		/// 同一作用域内的显式 Tab 顺序。
		std::int32_t tabIndex = 0;
		// 焦点文字叶节点提供的可选客户区 IME 锚点。FocusManager 会在发布前
		// 把 target/scope 重新绑定到当前新鲜快照。
		ImeCaretAnchorSnapshot imeCaretAnchor{};
	};

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
	 *	std::string   faceName = "微软雅黑";    - 字体名称
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
		// 由对象拥有字体名，避免从临时字符串或外部缓冲区保存悬空指针。
		std::string faceName = "微软雅黑"; //- 字体名称
		COLORREF color = RGB(0, 0, 0);  //- 字体颜色
		int nEscapement = 0;       //- 字符串旋转角度
		int nOrientation = 0;      //- 字符旋转角度
		int nWeight = 0;           //- 字体粗细 范围0~1000 0表示默认
		bool bItalic = false;       //- 是否斜体
		bool bUnderline = false;    //- 是否下划线
		bool bStrikeOut = false;     //- 是否删除线

		// 比较两份字体样式是否完全一致。
		bool operator==(const ControlText& text) const;
		// 比较两份字体样式是否存在任意差异。
		bool operator!=(const ControlText& text) const { return !(*this == text); }
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

	/**
	 * @枚举名称: MessageBoxType
	 * @功能描述: 定义标准消息框按钮组合
	 *
	 * @详细说明:
	 * MessageBoxType 决定 Dialog / MessageBox 工厂创建哪些功能按钮，
	 * 例如只有确定按钮、确定取消按钮、是否按钮等。
	 *
	 * @取值说明:
	 * OK               - 只有确定按钮
	 * OKCancel         - 确定和取消按钮
	 * YesNo            - 是和否按钮
	 * YesNoCancel      - 是、否和取消按钮
	 * RetryCancel      - 重试和取消按钮
	 * AbortRetryIgnore - 中止、重试和忽略按钮
	 *
	 * @使用示例:
	 *   StellarX::MessageBoxType type = StellarX::MessageBoxType::YesNo;
	 */
	enum class MessageBoxType
	{
		OK,                // 只有确定按钮
		OKCancel,          // 确定和取消按钮
		YesNo,             // 是和否按钮
		YesNoCancel,       // 是、否和取消按钮
		RetryCancel,       // 重试和取消按钮
		AbortRetryIgnore,  // 中止、重试和忽略按钮
	};

	/**
	 * @枚举名称: MessageBoxResult
	 * @功能描述: 定义消息框关闭后的用户选择结果
	 *
	 * @详细说明:
	 * 模态消息框通过 showModal 返回该枚举；非模态消息框通过 showAsync 的回调返回。
	 * 当前取值与 Win32 常见 MessageBox 返回码保持一致，便于迁移和日志排查。
	 *
	 * @取值说明:
	 * OK     - 用户点击确定
	 * Cancel - 用户点击取消或关闭对话框
	 * Yes    - 用户点击是
	 * No     - 用户点击否
	 * Abort  - 用户点击中止
	 * Retry  - 用户点击重试
	 * Ignore - 用户点击忽略
	 *
	 * @使用示例:
	 *   StellarX::MessageBoxResult result =
	 *       StellarX::MessageBox::showModal(wnd, "继续执行？", "确认", StellarX::MessageBoxType::YesNo);
	 */
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
	 * @结构体名称: AxisLayoutSpec
	 * @摘要: 描述单个坐标轴上的锚点与解算策略
	 * @详细说明:
	 *   一个控件完整布局由“水平轴 + 垂直轴”两份 AxisLayoutSpec 组成。
	 *   anchorStart / anchorEnd 只表达当前轴是否锚定起止边；
	 *   sizePolicy 决定该轴是否允许拉伸；
	 *   alignPolicy 则用于固定尺寸时的位置求解。
	 *
	 * @成员说明:
	 * bool anchorStart               - 是否锚定当前轴起边（水平轴为左边，垂直轴为上边）。
	 * bool anchorEnd                 - 是否锚定当前轴终边（水平轴为右边，垂直轴为下边）。
	 * AxisSizePolicy sizePolicy      - 当前轴尺寸策略，决定是否随父容器变化而拉伸。
	 * AxisAlignPolicy alignPolicy    - 固定尺寸时的位置策略。
	 *
	 * @备注:
	 * anchorStart / anchorEnd 不直接决定尺寸，尺寸是否变化以 sizePolicy 为准。
	 ******************************************************************************/
	struct AxisLayoutSpec
	{
		bool anchorStart = false;
		bool anchorEnd = false;
		AxisSizePolicy sizePolicy = AxisSizePolicy::FixedSize;
		AxisAlignPolicy alignPolicy = AxisAlignPolicy::Start;
	};

	/*******************************************************************************
	 * @结构体名称: LayoutSpec
	 * @摘要: 控件完整布局规格，由水平轴和垂直轴两部分组成
	 * @详细说明:
	 *   LayoutSpec 是控件对统一布局器的完整声明。水平轴和垂直轴分别独立描述
	 *   锚点、尺寸策略和固定尺寸时的位置策略，Window / Canvas 在 resize 时据此
	 *   解算运行态矩形。
	 *
	 * @成员说明:
	 * AxisLayoutSpec horizontal - 水平轴布局规格。
	 * AxisLayoutSpec vertical   - 垂直轴布局规格。
	 ******************************************************************************/
	struct LayoutSpec
	{
		AxisLayoutSpec horizontal{};
		AxisLayoutSpec vertical{};
	};

	/*******************************************************************************
	 * @结构体名称: LayoutCapability
	 * @摘要: 控件能力边界，声明控件在哪些轴上允许 Stretch
	 * @详细说明:
	 *   规则表负责定义“某种组合如何解算”，LayoutCapability 负责声明
	 *   “该控件是否允许采用这种组合”。Table 只支持 X 轴拉伸，因此会显式
	 *   关闭 allowStretchY。
	 *
	 * @成员说明:
	 * bool allowStretchX - 是否允许该控件在水平轴使用 Stretch 尺寸策略。
	 * bool allowStretchY - 是否允许该控件在垂直轴使用 Stretch 尺寸策略。
	 *
	 * @备注:
	 * 能力边界用于保护控件自身实现约束；即使调用方设置了 Stretch，
	 * 不被允许的轴也应被布局层拒绝或降级处理。
	 ******************************************************************************/
	struct LayoutCapability
	{
		bool allowStretchX = true;
		bool allowStretchY = true;
	};

	/*******************************************************************************
	 * @结构体名称: ResolvedLayoutRect
	 * @摘要: 统一布局解算后的运行态矩形结果
	 * @详细说明:
	 *   localX/localY 表示解算完成后的“父局部坐标”；
	 *   worldX/worldY 表示映射到窗口世界坐标后的最终绘制位置；
	 *   width/height 为运行态尺寸。
	 *
	 * @成员说明:
	 * int localX  - 解算后的父局部 X 坐标。
	 * int localY  - 解算后的父局部 Y 坐标。
	 * int width   - 解算后的运行态宽度。
	 * int height  - 解算后的运行态高度。
	 * int worldX  - 映射到窗口坐标系后的 X 坐标。
	 * int worldY  - 映射到窗口坐标系后的 Y 坐标。
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
