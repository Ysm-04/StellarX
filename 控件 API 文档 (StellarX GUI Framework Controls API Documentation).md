# StellarX GUI Framework API Documentation （中英双语）

## CoreTypes（基础类型）

**CoreTypes** 模块定义了 StellarX 框架中使用的所有基础枚举和结构体类型，以确保类型一致性（Core types module defines all fundamental enums and structs used in the StellarX framework to ensure type consistency）。主要包括以下内容：

- **FillStyle（填充图案样式）**：定义控件填充图案的枚举类型。例如 Horizontal 表示水平线填充，Vertical 表示垂直线，FDiagonal 表示反斜线，BDiagonal 表示正斜线，Cross 表示十字，DiagCross 表示网格。（Defines patterns for filling control backgrounds. For example, Horizontal for horizontal lines, Vertical for vertical lines, FDiagonal for forward diagonal lines, BDiagonal for backward diagonal lines, Cross for crosshatch, DiagCross for grid pattern.）默认填充图案为水平线 (The default pattern is horizontal line)。

- **FillMode（填充模式）**：定义控件背景的填充模式，包括纯色、无填充、图案填充、自定义图案、自定义图片填充等。（Defines how control backgrounds are filled: solid color, no fill, hatched pattern, custom pattern, or custom image fill, etc.）例如 Solid（固实填充），Null（不填充），Hatched（图案填充），Pattern（自定义图案），DibPattern（自定义图像）。默认填充模式为 Solid 固实填充 (default is solid fill)。

- **LineStyle（线型样式）**：定义控件边框的线型风格枚举。例如 Solid 表示实线，Dash 表示虚线，Dot 表示点线，DashDot 表示点划线，DashDotDot 表示双点划线，Null 表示无边框线。（Defines line style for control borders. For example, Solid for solid line, Dash for dashed line, Dot for dotted line, DashDot for dash-dot line, DashDotDot for double dash-dot, Null for no border line.）默认边框线型为实线 (default line style is solid)。

- **ControlShape（控件形状）**：定义控件的几何形状类型，共提供矩形、圆角矩形、圆形、椭圆，每种形状分别有“有边框”和“无边框”两种版本。（Enumerates geometric shape types for controls: rectangle, rounded rectangle, circle, and ellipse – each in bordered and borderless variants.）例如 RECTANGLE（有边框矩形）、B_RECTANGLE（无边框矩形）、ROUND_RECTANGLE（有边框圆角矩形）、B_ROUND_RECTANGLE（无边框圆角矩形）、CIRCLE（有边框圆形）、B_CIRCLE（无边框圆形）、ELLIPSE（有边框椭圆）、B_ELLIPSE（无边框椭圆）。按钮类支持所有形状，而某些控件可能只支持部分形状（Button supports all shapes, while some controls only support a subset of shapes）。

- **TextBoxmode（文本框模式）**：定义文本框控件的工作模式枚举，包括 `INPUT_MODE`（用户可输入模式）、`READONLY_MODE`（只读模式）和 `PASSWORD_MODE`（密码模式）。这允许设置文本框是否可输入、仅显示或以密码形式显示。（Defines the operating mode of a TextBox control: `INPUT_MODE` for editable text input, `READONLY_MODE` for read-only display, and `PASSWORD_MODE` for password input mode, allowing configuration of whether the text box accepts input or is display-only or masked as password input.）

- **ButtonMode（按钮模式）**：定义按钮控件的工作模式枚举，包括 `NORMAL`（普通模式）、`TOGGLE`（切换模式）和 `DISABLED`（禁用模式）。普通模式下按钮每次点击触发回调但不保持状态；切换模式下按钮在选中/未选中之间切换，并触发不同回调；禁用模式下按钮不可点击，显示灰化且文本带删除线。（Defines the working mode of a Button: `NORMAL` mode triggers a callback on each click without maintaining state; `TOGGLE` mode toggles between pressed and unpressed states and triggers different callbacks for each; `DISABLED` mode makes the button non-clickable, typically grayed out with strikethrough text to indicate disabled state.）

- **MessageBoxType（消息框类型）**：枚举标准消息框的按钮组合类型，包括 OK（只有“确定”按钮）、OKCancel（“确定”和“取消”）、YesNo（“是”和“否”）、YesNoCancel（“是”“否”“取消”）、RetryCancel（“重试”和“取消”）、AbortRetryIgnore（“中止”“重试”“忽略”）等类型。（Enumerates the standard combinations of buttons for a message box: e.g., OK (OK button only), OKCancel (OK and Cancel), YesNo, YesNoCancel, RetryCancel, AbortRetryIgnore, etc.）开发者可根据需要选择消息框包含的按钮组合 (Developers can select the appropriate set of buttons for the message box as needed)。

- **MessageBoxResult（消息框结果）**：枚举消息框的返回结果类型，与 MessageBoxType 相对应，包括 OK（确定）、Cancel（取消）、Yes（是）、No（否）、Abort（中止）、Retry（重试）、Ignore（忽略）等。（Enumerates the possible results of a message box, corresponding to which button was pressed: OK, Cancel, Yes, No, Abort, Retry, Ignore, etc.）模态消息框会返回这些枚举值表示用户选择 (Modal message boxes return one of these values to indicate the user's choice)。

- **LayoutMode（布局模式）**：定义窗口拉伸时控件的布局策略，包括 `Fixed`（固定布局）和 `AnchorToEdges`（锚定布局）。（Defines how controls behave when the window is resized: `Fixed` for no resizing (controls maintain their size/position), and `AnchorToEdges` for anchoring controls to container edges so they stretch or move accordingly.）通过 **Control** 的接口可以设置控件的布局模式 (The layout mode for a control can be set via the **Control** class’s interface)。

- **Anchor（锚点位置）**：定义控件相对于父窗口的锚定边缘位置，用于 AnchorToEdges 布局模式。包括 Left（左锚定）、Right（右锚定）、Top（上锚定）、Bottom（下锚定）以及 NoAnchor（不锚定）。可以组合两个锚点来固定控件的两个方向。（Defines which edges of the parent container a control is anchored to (used when layout mode is AnchorToEdges). Options include Left, Right, Top, Bottom, plus NoAnchor for no anchoring. Two anchors (horizontal and vertical) are typically used together to lock the control’s position in both directions.）

- **TabPlacement（选项卡位置）**：定义 TabControl 选项卡标签栏的位置，包括 Top（页签在顶部）、Bottom（在底部）、Left（在左侧）、Right（在右侧）。选择不同值会将选项卡的标签（按钮）放置在容器的不同边缘。（Defines the placement of the tab headers in a TabControl: Top, Bottom, Left, or Right. The chosen value determines on which edge of the TabControl the tab buttons are displayed.）

- **RouRectangle（圆角矩形参数）**：用于定义控件圆角矩形形状时圆角的椭圆尺寸的结构体。包含两个整数成员 ROUND_RECTANGLEwidth 和 ROUND_RECTANGLEheight，默认值均为 20，表示圆角矩形拐角处椭圆的宽度和高度。（A struct defining the ellipse size of the corners for rounded rectangle shapes. It has two int members, ROUND_RECTANGLEwidth and ROUND_RECTANGLEheight (both default to 20), which represent the width and height of the ellipse used for the rounded corners of a rectangle.）通过修改这个结构体，可以调整控件圆角的弧度 (By modifying these values, one can adjust the curvature of a control’s rounded corners).

- **ControlText（控件文本样式）**：定义控件字体和文本颜色样式的结构体。它包含多个字段用于描述文本外观：

  - `nHeight`：字体高度（像素值）(font height in pixels)
  - `nWidth`：字体宽度（像素值，如果为0则自动适配高度）(font width in pixels; 0 means auto-adjust to height)
  - `lpszFace`：字体名称，默认“微软雅黑” (font face name, default is "微软雅黑" font)
  - `color`：文字颜色，默认黑色 (text color, default RGB(0,0,0) black)
  - `nEscapement`：字符串整体旋转角度（单位0.1度，如900表示90度）(text escapement angle for the entire string, in tenths of degrees, e.g., 900 means 90°)
  - `nOrientation`：单个字符旋转角度（单位0.1度）(orientation angle of individual characters, also in tenths of degrees)
  - `nWeight`：字体粗细(字重)，0表示默认，范围0~1000 (font weight/thickness, 0 means default; range 0~1000)
  - `bItalic`：是否斜体 (italic flag)
  - `bUnderline`：是否下划线 (underline flag)
  - `bStrikeOut`：是否删除线 (strikethrough flag)

  此结构可用于控件的文本样式自定义，如 **Button**、**Label** 等控件都有 `textStyle` 成员使用该结构。（This struct is used to customize text appearance for controls. For example, **Button**, **Label**, etc., include a `textStyle` member of this type to specify font and text style.）

## Control（控件基类）

**类名**：`Control`
 **中文简要说明**：所有控件的抽象基类，定义通用接口和基础功能，不直接实例化。（*Abstract base class for all UI controls, defining common interfaces and fundamental functionality. It is not meant to be instantiated directly*。）

**继承关系**：无（Control 为顶级基类）(None. Control is the top-level base class for all controls).

**功能摘要**: Control 提供控件的基本属性（如位置、尺寸）和通用方法，并实现绘图状态保存/恢复机制，确保控件绘制不影响全局状态。它还声明了一些纯虚方法供子类实现，例如 `draw()` 和 `handleEvent()`。（*The Control class provides basic properties (position, size, visibility) and common methods for controls. It implements mechanisms for saving and restoring drawing state to ensure control rendering does not disturb global graphics state. It also declares abstract methods like `draw()` and `handleEvent()` that subclasses must implement.*）

**公共方法 (Public Methods)**：

- `draw()` – **纯虚函数**，由子类实现，用于绘制控件内容 (pure virtual; draws the control’s appearance, implemented by each concrete control subclass).
- `bool handleEvent(const ExMessage& msg)` – **纯虚函数**，由子类实现，处理输入事件消息，返回事件是否被控件消费 (pure virtual; handles input events for the control and returns true if the event is consumed)。
- `void setIsVisible(bool show)` – 设置控件的可见性。传入 `false` 将隐藏控件，`true` 将显示控件。默认实现改变内部 `show` 状态并将控件标记为需要重绘 (Sets the control’s visibility. False to hide the control, true to show it. The base implementation updates the internal `show` flag and marks the control as needing redraw)。**（说明**：某些复合控件会重载此方法以同步子控件，例如 **TabControl** 对该方法进行了重载，实现显示/隐藏所有选项卡页) (Note: Some composite controls override this to also affect their children – e.g. **TabControl** overrides setIsVisible to show/hide all tab pages).
- `void setDirty(bool dirty)` – 将控件标记为“脏”，表示需要重绘 (Marks the control as dirty, i.e., requiring a redraw). 默认实现设置内部标志，并**对容器控件**会将所有子控件一起标记 (Base implementation sets the internal flag; container controls like Canvas also mark child controls dirty).
- `void setParent(Control* parent)` – 设置控件的父容器指针 (Assigns a parent control container to this control). 用于建立控件层次关系 (Used to establish parent-child relationship for nested controls).
- `bool IsVisible() const` – 返回控件当前是否处于可见状态 (Checks if the control is currently set to be visible).
- `std::string getId() const` – 获取控件的标识字符串ID (Returns the string identifier of the control).
- `bool isDirty() const` – 判断控件是否被标记为需要重绘 (Returns whether the control is marked as dirty (needing redraw)).
- `void setX(int x), setY(int y)` – 设置控件左上角的位置坐标 (Sets the control’s top-left position). 基类实现更新位置并将控件标记为脏，需要重绘 (Base implementation updates the coordinate and marks the control dirty)。某些容器控件（如 **Canvas**）会重载这些方法，以便在移动自身时同步更新子控件位置 (Some container controls like **Canvas** override these to also adjust children’s positions when the container moves).
- `void setWidth(int w), setHeight(int h)` – 设置控件尺寸宽度和高度 (Sets the control’s width and height). 基类实现更新尺寸并将控件标记为需要重绘 (Base implementation updates the size and marks the control dirty)。
- `int getX() const, getY() const, getWidth() const, getHeight() const` – 获取控件当前的全局坐标位置及宽高 (Returns the control’s current absolute X, Y coordinates and its width and height)。
- `int getRight() const, getBottom() const` – 获取控件右边缘和下边缘的全局坐标 (Returns the coordinate of the control’s right edge (x+width) and bottom edge (y+height)).
- `int getLocalX() const, getLocalY() const, getLocalWidth() const, getLocalHeight() const` – 获取控件相对于父容器的坐标和尺寸 (Returns the control’s position and size in its parent’s local coordinate space).
- `int getLocalRight() const, getLocalBottom() const` – 获取控件右边缘和下边缘相对于父容器坐标系的位置 (Returns the local-space coordinates of the control’s right and bottom edges).
- **布局相关方法 (Layout-related)**:
  - `void setLayoutMode(StellarX::LayoutMode mode)` – 设置控件的布局模式，如固定或锚定布局 (Sets the control’s layout mode (e.g., Fixed or AnchorToEdges)). 基类存储所选模式，窗口大小变化时会参考此设置调整控件 (The base class stores the mode; on window resize, this informs how the control should adjust, if at all).
  - `void setAnchor(StellarX::Anchor anchor1, StellarX::Anchor anchor2)` – 设置控件在锚定布局模式下的水平和垂直锚点 (Sets the horizontal and vertical anchor positions for AnchorToEdges layout mode). 例如可以将控件锚定在左上（Top+Left）以保持与窗口左上角的距离不变 (For example, anchoring Top+Left keeps the control’s top-left distance to the container constant).
  - `StellarX::Anchor getAnchor_1() const, getAnchor_2() const` – 获取当前设置的水平和垂直锚点值 (Returns the currently configured primary and secondary anchor).
  - `StellarX::LayoutMode getLayoutMode() const` – 获取当前布局模式 (Gets the current layout mode).
  - `void onWindowResize()` – 虚函数，窗口尺寸变化时由系统或父容器调用，用于让控件丢弃过期的背景缓存并适应新尺寸。基类实现默认丢弃背景快照并标记控件为脏，需要重绘 (Virtual function called when the window or parent container is resized. The base implementation discards any saved background snapshot and marks the control dirty for redraw)。大多数控件使用基类行为，但容器控件会在重载中调整子控件布局 (Most controls use the base behavior, but container controls override this to adjust child layouts).
- **其他方法 (Other methods)**:
  - `void updateBackground()` – 主动释放旧背景快照并重新保存当前背景，用于在控件尺寸变化后更新缓存以防止显示错位。调用该方法会抓取控件当前位置的新背景 (This forces the control to discard any old background image and capture a new background snapshot, typically used after resizing to avoid misalignment of cached content).
  - `virtual bool model() const = 0` – **纯虚函数**，用于对话框控件检查自身是否为模态。非对话框类可实现为简单返回 false。（*Pure virtual function to indicate if the control is a modal dialog. Non-dialog controls implement this to always return false; only dialog-related classes override it to reflect modal state*）.

**保护方法 (Protected Methods)**：*(通常供控件内部或子类使用)*

- `void requestRepaint(Control* parent)` – 向上请求父容器重绘。本控件或其子控件需要重绘时调用，遍历到最顶层容器触发实际重绘。基础实现为：如果存在父容器则递归通知父容器重绘，否则（无父，已到窗口根）调用 `onRequestRepaintAsRoot()` (Requests the parent to schedule a repaint of this control. Called when this control (or one of its children) becomes dirty and needs redraw. The base implementation propagates the request up the chain: if a parent exists, call parent’s requestRepaint, otherwise if at the root, call `onRequestRepaintAsRoot()` to trigger a top-level redraw).
- `void onRequestRepaintAsRoot()` – 当控件是最顶层且需要重绘时，由框架调用以执行实际的刷新动作。典型实现是在窗口事件循环中检测到需要重绘时调用 (Called when the control is the root (no parent) and a repaint has been requested, to perform the actual redraw. In practice, the framework calls this during the window’s event loop when it detects a repaint is needed).
- `void saveBackground(int x, int y, int w, int h)` – 保存控件区域在屏幕上的背景图像快照，用于实现擦除重绘时不留下残影。在控件首次绘制或移动/隐藏时抓取其背景 (Saves a snapshot of the background under the control’s area (at position x,y with size w×h) to facilitate restoring the background when the control is moved or removed, preventing ghosting artifacts).
- `void restBackground()` – 恢复先前保存的背景快照，将其绘制回控件区域，从而擦除控件上一次绘制的内容。(Restores the previously saved background image to the control’s area, effectively erasing the control’s last drawn appearance. Typically used at the start of a redraw to clear old content.)
- `void discardBackground()` – 丢弃并释放当前保存的背景快照。当控件或窗口尺寸变化、控件销毁时需要调用以防止使用失效的背景缓存。(Discards the stored background snapshot (if any). Called when the control or window size changes, or when the control is destroyed, to avoid using an outdated background cache.)
- `void saveStyle()` / `void restoreStyle()` – 保存当前全局绘图状态（颜色、线型、填充等）并在绘制完成后恢复，以防止控件绘制改变全局状态。Control 基类实现了对 EasyX 图形状态的保存和恢复，每个控件在 `draw()` 开始时应调用 `saveStyle()`，结束时调用 `restoreStyle()` (Saves the current global drawing state (colors, line style, fill style, etc.) and restores it after drawing, to ensure the control’s drawing doesn’t alter the global state. The Control base provides these to wrap drawing code: call `saveStyle()` at the start of `draw()`, and `restoreStyle()` at the end).

**成员变量 (Member Variables)** *(除特殊说明外，均为 `protected` 访问权限 — Protected unless otherwise noted)*：

- `std::string id` – 控件的字符串ID标识符 (String identifier for the control). 可能用于调试或控件查找 (Often used for debugging or identifying controls).

- `int x, y` – 控件左上角的**全局**坐标 (X and Y coordinates of the control’s top-left corner in the **global (window) coordinate space**).

- `int width, height` – 控件当前的宽度和高度 (Current width and height of the control).

- `int localx, localy` – 控件左上角相对于父控件的局部坐标 (Local X, Y position relative to its parent control’s origin). 对于顶级控件（如窗口直接子控件），local 坐标通常与全局相同 (For top-level controls (children of the window), local coordinates are typically the same as global).

- `int localWidth, localHeight` – 控件在父容器坐标系下的尺寸大小 (Width and height of the control in the parent’s coordinate system). 初始化时等于控件自身宽高，但在父容器缩放或布局时可能用于计算 (Initialized to the control’s own width/height; used in layout calculations if needed).

- `Control* parent` – 父控件指针 (Pointer to the parent control). 为 `nullptr` 则表示本控件无父（可能是顶层元素）(If `nullptr`, this control has no parent, i.e., it’s a root element).

- `bool dirty` – 控件重绘标记。如果为 true 表示控件内容已改变或无效，需要在下一个周期重新绘制 (Dirty flag indicating the control needs to be redrawn. True means the control’s content has changed or invalidated and should be redrawn on the next cycle). 绘制完成后通常将 dirty 重置为 false (After drawing, this is reset to false until something changes again).

- `bool show` – 控件可见性标志。true 表示控件应显示，false 则控件被隐藏，绘制时应跳过 (Visibility flag: true if the control should be visible, false if hidden. Hidden controls are skipped during rendering).

- **布局属性 (Layout-related)**:

  - `StellarX::LayoutMode layoutMode` – 控件的布局模式（Fixed 或 AnchorToEdges），决定窗口大小变化时的位置调整策略 (Specifies the control’s layout mode (Fixed or AnchorToEdges), determining how it behaves when the window is resized). 默认为 Fixed 固定布局。
  - `StellarX::Anchor anchor_1, anchor_2` – 控件相对父容器的锚点设置（如 Top/Bottom/Left/Right）。当 layoutMode 为 AnchorToEdges 时使用，表示控件的哪个边固定在父容器的哪个边上 (Anchor positions (primary and secondary) relative to parent container, used when layoutMode is AnchorToEdges. Determines which side(s) of the control remain fixed to which side of the parent). 例如 anchor_1=Top, anchor_2=Right 表示控件上边和右边相对父窗口距离保持不变 (e.g., Top + Right means the control’s top and right edges maintain constant distance from the parent’s top and right edges).

- **背景快照属性 (Background snapshot)**:

  - `IMAGE* saveBkImage` – 保存的背景图像指针，用于在控件重绘前恢复覆盖区域背景 (Pointer to an IMAGE storing the snapshot of the background behind the control). 如果不为空，表示当前持有一份有效的背景缓存 (If not null, a valid background image is stored).
  - `int saveBkX, saveBkY` – 背景快照在屏幕上的保存起始坐标 (The X, Y coordinates of where the background snapshot was taken).
  - `int saveWidth, saveHeight` – 背景快照区域的宽度和高度 (The width and height of the saved background area).
  - `bool hasSnap` – 标记当前是否有有效的背景快照 (Flag indicating whether a valid background snapshot is currently stored).

- `StellarX::RouRectangle rouRectangleSize` – 控件圆角矩形参数。当控件形状为圆角矩形时，使用该结构决定圆角的大小 (Stores the control’s rounded rectangle corner sizes. Used when the control’s shape is a rounded rectangle to determine the curvature radii)。默认圆角宽高为20 (Defaults to 20 for both width and height, as defined in RouRectangle).

- **绘图状态缓存 (Drawing state caches)**: 为了在控件绘制时保存并恢复全局绘图状态，每个控件维护以下当前状态指针：

  - `LOGFONT* currentFont` – 当前字体样式的备份指针 (Pointer to a LOGFONT storing the current font style before control drawing).
  - `COLORREF* currentColor` – 当前文本绘制颜色的备份 (Pointer storing current text color).
  - `COLORREF* currentBkColor` – 当前背景填充颜色的备份 (Pointer storing current background fill color).
  - `COLORREF* currentBorderColor` – 当前边框颜色的备份 (Pointer storing current border color).
  - `LINESTYLE* currentLineStyle` – 当前线型样式的备份 (Pointer storing current line style settings).

  以上指针在控件销毁时会删除以释放资源。调用 `saveStyle()` 时，这些指针会指向保存的全局状态；`restoreStyle()` 则将全局绘图状态恢复并重置这些指针 (These are allocated to hold copies of global drawing settings; they are deleted in the Control destructor to free resources. When `saveStyle()` is called, the current global drawing settings are stored in these objects, and `restoreStyle()` reapplies them and resets the pointers).

- **禁用复制和移动 (Deleted copy/move)**: 为防止误用，Control 明确删除了复制构造和赋值运算符，以及移动构造和赋值 (Copy constructor and assignment operator, as well as move constructor and move assignment, are deleted to prevent copying or moving of controls)。控件对象不可被复制，只能以指针或智能指针方式管理 (Control objects cannot be copied; they should be managed via pointers or smart pointers).

- **构造函数与析构函数 (Constructors & Destructor)**:

  - `Control(int x, int y, int width, int height)` – **受保护构造函数**，使用指定的位置和尺寸初始化控件基本属性。被派生类调用，用于设置控件初始的 local/global 坐标和宽高 (Protected constructor used by subclasses to initialize the control’s position (both local and global) and size).
  - `Control()` – **受保护默认构造函数**，初始化控件位置为 (0,0)，宽高为 (100,100)。通常不会显式使用，除非子类需要默认大小的控件 (Initializes the control at origin (0,0) with a default size of 100×100. Typically invoked by subclass default constructors if needed).
  - `virtual ~Control()` – 析构函数，基类析构确保清理分配的资源。它删除并释放上述 `currentFont` 等绘图状态缓存指针，并调用 `discardBackground()` 释放背景快照 (Base destructor cleans up allocated resources, deleting the stored font, color, and line style objects, setting them to null, and calling `discardBackground()` to release any saved background image)。

**依赖关系 (Dependencies)**:

- **操作系统/图形库**：Control 使用 Win32 API 数据类型（如 `HWND`、`COLORREF` 等）以及 EasyX 图形库进行绘图管理。因此需要 Windows 平台支持 (The Control class includes Windows headers and depends on EasyX graphics library for drawing operations, hence it runs on Windows platform).
- **框架核心类型**：Control 依赖 `CoreTypes.h` 中定义的 StellarX 命名空间枚举和结构，如 LayoutMode、Anchor、ControlShape、RouRectangle、ControlText 等，用于自身属性 (It relies on types defined in CoreTypes, such as LayoutMode, Anchor, ControlShape, RouRectangle, and ControlText for its properties and behavior).
- **子类关系**：Control 是所有具体控件（按钮、标签、文本框、表格等）的基类。比如 **Button**, **Label**, **TextBox**, **Table** 等类都公开继承自 Control (All concrete control classes like Button, Label, TextBox, Table, etc., publicly inherit from Control). Control 提供的接口（draw、handleEvent等）由子类实现，实现各自的外观和行为 (The interfaces provided by Control (draw, handleEvent, etc.) are overridden by these subclasses to implement their specific appearance and behavior).
- **容器关系**：尽管 Control 本身不包含子控件列表，但其派生的容器类（如 **Canvas**、**Window** 等）利用继承的接口管理子控件集合 (Control itself does not store child controls, but derived container classes like Canvas or Window use Control’s interface to manage child controls).

## 派生控件（Derived Controls）

以下类均继承自 **Control** 基类，表示常见的可视控件组件。这些控件实现了 Control 定义的接口，并根据需要添加自己的属性和方法。每个控件的继承关系和特定功能如下： *(The following classes all derive from the **Control** base class, representing common UI components. They implement the interfaces defined by Control and add their own properties and methods as needed. Each control’s inheritance and specific features are detailed below:)*

### Button（按钮控件）

**类名**：`Button`
 **继承**：继承自 Control (`class Button : public Control`)

**简要说明**：多功能按钮控件，支持多种状态和样式。提供完整的按钮交互，包括普通点击、切换开关、禁用等模式，并支持自定义外观（颜色、形状、填充）和鼠标悬停提示等。（*A versatile button control supporting multiple states and styles. It provides full button functionality including normal click, toggle (on/off) mode, and disabled state. The Button supports extensive customization of appearance (colors, shape, fill patterns) and features such as hover tooltips.*）

**公共方法**：

- `Button(int x, int y, int width, int height, const std::string& text, StellarX::ButtonMode mode = NORMAL, StellarX::ControlShape shape = RECTANGLE)` – **构造函数**，创建按钮，指定位置尺寸、显示文本，以及可选的按钮模式和形状。将按钮初始化为默认颜色配置 (Constructs a button at given position and size with the specified label text, and optional ButtonMode and shape. Initializes the button with default color settings for its states).
- `Button(int x, int y, int width, int height, const std::string& text, COLORREF ct, COLORREF cf, StellarX::ButtonMode mode = NORMAL, StellarX::ControlShape shape = RECTANGLE)` – **构造函数**，创建具有自定义“按下/未按”颜色的按钮。参数 `ct` 为按钮被点击时颜色，`cf` 为按钮未点击时颜色 (Constructor to create a button with custom colors for the pressed (`ct`) and unpressed (`cf`) states).
- `Button(int x, int y, int width, int height, const std::string& text, COLORREF ct, COLORREF cf, COLORREF ch, StellarX::ButtonMode mode = NORMAL, StellarX::ControlShape shape = RECTANGLE)` – **构造函数**，创建具有自定义“按下/未按/悬停”颜色的按钮。额外参数 `ch` 指定鼠标悬停时的按钮背景颜色 (Constructor to create a button with custom pressed (`ct`), unpressed (`cf`), and hover (`ch`) colors).
- `~Button()` – **析构函数**，销毁按钮时释放可能加载的图像资源。例如如果按钮使用了自定义填充图像，将在析构时清理 (Releases any resources such as loaded images when the button is destroyed).
- `void draw() override` – 绘制按钮外观。实现包括根据按钮状态（正常、悬停、按下、禁用）设置颜色和填充，并绘制按钮边框和文本等。按钮绘制支持多种形状，如果是圆形或椭圆按钮，会进行鼠标区域判定的特殊处理 (Renders the button. The implementation sets the appropriate colors/fill based on button state (normal, hover, pressed, disabled), draws the button’s border and filled shape, and then draws the text centered on the button. Different shapes (rectangle, rounded rect, circle, ellipse) are handled, including special hit-testing for circular/elliptical shapes).
- `bool handleEvent(const ExMessage& msg) override` – 处理按钮的鼠标事件。包括检测鼠标按下、释放、移入、移出等，以更新按钮的 `click`（按下状态）和 `hover`（悬停状态），并触发相应回调 (Handles mouse events for the button. This includes detecting mouse down/up to update the `click` state (pressed or not), tracking mouse enter/leave to set the `hover` state, and triggering the appropriate callbacks when clicked or toggled).
- 回调设置方法 (Callback setters):
  - `void setOnClickListener(std::function<void()>&& callback)` – 设置按钮在 NORMAL 模式下点击时执行的回调函数。当按钮每次被点击（鼠标按下然后松开）且模式为 NORMAL 时调用此回调 (Assigns a callback to be invoked when the button is clicked in NORMAL mode).
  - `void setOnToggleOnListener(std::function<void()>&& callback)` – 设置按钮在 TOGGLE 模式下从未选中切换为选中状态时的回调。当按钮切换到“按下/选中”状态时调用 (Sets the callback for when a toggle-mode button is toggled on (pressed state)).
  - `void setOnToggleOffListener(std::function<void()>&& callback)` – 设置按钮在 TOGGLE 模式下从选中切换为未选中状态时的回调。当按钮从按下恢复为弹起状态时调用 (Sets the callback for when a toggle-mode button is toggled off (released state)).
- 模式和形状设置 (Mode/Shape settings):
  - `void setbuttonMode(StellarX::ButtonMode mode)` – 设置按钮的工作模式（NORMAL/TOGGLE/DISABLED）。更改模式会影响按钮行为：设置 TOGGLE 会使按钮保持按下状态，设置 DISABLED 会使按钮不可点击 (Changes the button’s operating mode; e.g., setting TOGGLE makes the button stay pressed when clicked, setting DISABLED grays it out and disables interaction).
  - `void setButtonShape(StellarX::ControlShape shape)` – 设置按钮形状（矩形、圆角矩形、圆形、椭圆等）。修改形状会影响按钮绘制的轮廓和鼠标命中区域 (Sets the geometric shape of the button. This changes how the button is drawn (e.g., with rounded corners or as a circle) and how hover/click hit-testing is calculated).
- 外观属性设置 (Appearance setters):
  - `void setROUND_RECTANGLEwidth(int width)` / `void setROUND_RECTANGLEheight(int height)` – 若按钮形状为圆角矩形，设置其圆角椭圆的宽度或高度。用于调整圆角大小 (If the button’s shape is a rounded rectangle, these adjust the horizontal/vertical radius of the corner’s ellipse to change the corner roundness).
  - `void setFillMode(StellarX::FillMode mode)` – 设置按钮背景的填充模式。例如纯色填充、图案填充等 (Sets the fill mode for the button’s background, e.g., solid color, hatched pattern, image pattern, etc.).
  - `void setFillIma(StellarX::FillStyle style)` – 设置按钮背景填充的图案样式。只有当 FillMode 为 Hatched 有效，用于选择具体的填充图案 (Sets the hatched fill pattern for the button’s background, effective if FillMode is set to Hatched. Chooses which hatch style to use).
  - `void setFillIma(std::string imageName)` – 设置按钮背景填充为指定文件的图像。这会加载给定路径的图像用于填充按钮背景（FillMode 应设为 Pattern/DibPattern）(Uses an external image file to fill the button background. This will load the image from the given file path; the FillMode should be set to Pattern or DibPattern to use the image fill).
  - `void setButtonBorder(COLORREF color)` – 设置按钮边框颜色。更改边框绘制使用的颜色 (Sets the border color of the button’s outline).
  - `void setButtonFalseColor(COLORREF color)` – 设置按钮未被按下时的背景颜色。即按钮处于弹起状态的填充色 (Sets the background color for the button’s unpressed (false) state).
  - `void setButtonText(const char* text)` / `void setButtonText(std::string text)` – 设置按钮显示的文本标签。更改按钮上显示的文字内容 (Sets the label text displayed on the button).
  - `void setButtonClick(BOOL clicked)` – 强制设置按钮按下状态。传 true 则使按钮显示为按下状态，false 则恢复未按状态；通常用于程序控制 TOGGLE 模式按钮的状态 (Forces the button’s pressed state. True makes the button appear pressed (selected), false makes it unpressed. This is mainly useful for programmatically controlling the state of a TOGGLE mode button).
- 查询方法 (Getters):
  - `bool isClicked() const` – 返回按钮当前是否处于按下（选中）状态。TOGGLE 模式下按下一次后该状态会保持，NORMAL 模式下每次点击会短暂为 true (Indicates whether the button is currently in the pressed state. In TOGGLE mode, this stays true after being clicked until toggled off; in NORMAL mode it is true only during the click).
  - `std::string getButtonText() const` / `const char* getButtonText_c() const` – 获取按钮的文本标签内容。提供 std::string 和 C 字符串两种形式 (Returns the text label of the button, as a std::string or C-string).
  - `StellarX::ButtonMode getButtonMode() const` – 获取按钮当前的模式（NORMAL/TOGGLE/DISABLED）.
  - `StellarX::ControlShape getButtonShape() const` – 获取按钮当前形状类型.
  - `StellarX::FillMode getFillMode() const` – 获取按钮当前填充模式.
  - `StellarX::FillStyle getFillIma() const` – 获取按钮当前填充图案样式.
  - `IMAGE* getFillImaImage() const` – 如果按钮使用了图像填充，获取当前填充所用的 IMAGE 对象指针.
  - `COLORREF getButtonBorder() const` – 获取按钮边框颜色.
  - `COLORREF getButtonTextColor() const` – 获取按钮文字颜色 (Returns the text color used for the button’s label). *（说明：文字颜色存储在 `ControlText` 的 color 字段中，此方法便于直接获取）*(Note: The text color is actually stored in the `ControlText` struct as part of textStyle; this getter provides convenient access to it.)*.
  - `StellarX::ControlText getButtonTextStyle() const` – 获取按钮的文字样式结构副本。包含字体、字号、颜色等信息 (Returns a copy of the ControlText struct representing the button’s current text style, including font face, size, color, etc.).
- **Tooltip 提示功能方法** (Button 提示工具条 API):
  - `void enableTooltip(bool on)` – 启用或禁用鼠标悬停提示功能。设为 true 则当鼠标悬停在按钮上时显示提示文本 (Enable or disable a tooltip that appears when the mouse hovers over the button). 关闭提示时会立即隐藏当前提示 (Disabling tooltips will hide any currently visible tooltip immediately).
  - `void setTooltipDelay(int ms)` – 设置提示出现的延迟时间（毫秒）。例如 1000 表示鼠标悬停1秒后显示提示 (Sets the delay in milliseconds before the tooltip is shown on hover. e.g., 1000 means the tooltip appears after 1 second of hovering).
  - `void setTooltipFollowCursor(bool on)` – 设置提示框是否跟随鼠标移动。如果为 true，则提示将出现在鼠标附近位置，并随鼠标移动更新 (Determines if the tooltip should follow the cursor. If true, the tooltip will appear near the cursor and move along with it).
  - `void setTooltipOffset(int dx, int dy)` – 设置提示框相对于鼠标位置的偏移。可调整提示框出现的位置，例如 (12, 18) 表示在鼠标坐标基础上右移12，下移18 像素 (Sets an offset for the tooltip’s position relative to the cursor. For example, (12, 18) will position the tooltip 12px to the right and 18px below the cursor).
  - `void setTooltipStyle(COLORREF textColor, COLORREF backgroundColor, bool transparent)` – 设置提示框文本颜色、背景颜色及是否背景透明。若 transparent 为 true，则提示背景不绘制矩形底色 (Configures the tooltip’s text color, background color, and transparency. If `transparent` is true, the tooltip has no opaque background, allowing underlying graphics to show through behind the text).
  - `void setTooltipText(const std::string& text)` – 设置按钮在 NORMAL 模式下的提示文本。当按钮未切换或不在TOGGLE模式时，将显示此提示内容 (Sets the tooltip text for the button (used in NORMAL mode or when the button is not toggling between two states)). 调用该函数会将 tipTextClick 设置为给定内容，并标记用户自定义标志以避免覆盖 (This updates the internal `tipTextClick` and flags that a custom tooltip is set by the user).
  - `void setTooltipTextsForToggle(const std::string& onText, const std::string& offText)` – 设置按钮在 TOGGLE 模式下不同状态的提示文本。第一个参数用于按钮选中(Pressed)时的提示，第二个用于未选中(Released)时的提示 (Defines two tooltip strings for a toggle-mode button: one when the button is toggled on (pressed) and one when toggled off).

**成员变量** *(Button的大部分成员为私有 private)*：

- `std::string text` – 按钮当前显示的文本 (The text label displayed on the button).
- `bool click` – 按钮按下状态标志。true 表示按钮当前呈现为按下（TOGGLE模式下为选中），false 表示未按下。(Indicates if the button is in a pressed state. True when the button is currently pressed (or toggled on), false when not pressed.)
- `bool hover` – 按钮悬停状态标志。true 表示鼠标当前在按钮区域内 (Indicates if the mouse cursor is currently hovering over the button).
- `std::string cutText` – 裁剪后的文本。如果按钮文本过长需要裁剪显示，则存储裁剪结果 (A possibly truncated version of the text used for rendering if the full text does not fit in the button).
- `bool needCutText` – 标志是否需要裁剪文本。当文本长度超出按钮宽度时设为 true (Flag indicating if text needs to be truncated to fit the button width).
- `bool isUseCutText` – 标志当前是否使用裁剪后的文本显示。为 true 则表示 draw 时使用 `cutText` 而非完整 text (Indicates whether the button is currently using the truncated `cutText` for display instead of the full `text`).
- `int padX, padY` – 文本绘制的内边距 (padding)。padX 是左右最小内边距像素，padY 是上下内边距 (Minimum horizontal (padX) and vertical (padY) padding in pixels around the button’s text). 默认 padX=6, padY=4.
- **颜色设置 (Color settings)**:
  - `COLORREF buttonTrueColor` – 按钮被点击（选中）时的填充背景颜色 (Background color when the button is in pressed/true state).
  - `COLORREF buttonFalseColor` – 按钮未被点击时的填充背景颜色 (Background color when the button is in unpressed/false state).
  - `COLORREF buttonHoverColor` – 鼠标悬停在按钮上时的背景颜色 (Background color when the mouse is hovering over the button).
  - `COLORREF buttonBorderColor` – 按钮边框颜色，默认黑色 (The color of the button’s border; default is black).
- `StellarX::ButtonMode mode` – 当前按钮模式（NORMAL/TOGGLE/DISABLED）.
- `StellarX::ControlShape shape` – 当前按钮形状（矩形、圆角矩形、圆形或椭圆等）.
- `StellarX::FillMode buttonFillMode` – 按钮填充模式（纯色、图案等），默认 Solid 纯色填充.
- `StellarX::FillStyle buttonFillIma` – 按钮填充图案样式，默认 BDiagonal（右斜线网格），当 FillMode=Hatched 时使用.
- `IMAGE* buttonFileIMAGE` – 填充按钮背景的图像指针（如使用自定义图片填充），默认为 nullptr.
- **回调函数**：
  - `std::function<void()> onClickCallback` – 按钮点击回调函数指针 (Function to call on button click in normal mode).
  - `std::function<void()> onToggleOnCallback` – 按钮 TOGGLE 模式下切换为选中状态时的回调 (Function to call when a toggle button is toggled on).
  - `std::function<void()> onToggleOffCallback` – 按钮 TOGGLE 模式下切换为未选中状态时的回调 (Function to call when a toggle button is toggled off).
- `StellarX::ControlText oldStyle` – 按钮文本原样式的备份 (Backup of the button’s text style before certain changes). 一些操作如禁用按钮时可能暂时修改文本样式（如加删除线），oldStyle 保存原始样式以便恢复 (For example, when disabling the button, the text might be styled with a strikethrough; `oldStyle` stores the original style so it can be restored when re-enabled).
- `int oldtext_width, oldtext_height` – 记录上一次绘制时文本的宽度和高度 (Stores the width and height of the text from the last draw, used to detect if recalculation or re-cutting is needed).
- `int text_width, text_height` – 当前文本的像素宽度和高度 (The measured pixel width and height of the current text string, used for centering and for truncation logic).
- **Tooltip 提示相关**:
  - `bool tipEnabled` – 是否启用了鼠标悬停提示 (Whether the tooltip feature is enabled for this button).
  - `bool tipVisible` – 当前提示是否正显示 (Whether the tooltip is currently visible on screen).
  - `bool tipFollowCursor` – 提示框是否跟随鼠标移动 (Whether the tooltip should move with the cursor).
  - `bool tipUserOverride` – 用户是否自定义了提示文本 (Flag indicating if the user has overridden the tooltip text via setTooltipText; if false, default texts might be used for toggle states).
  - `int tipDelayMs` – 提示显示延迟毫秒数，默认1000ms (Delay in milliseconds before tooltip appears; default is 1000 ms).
  - `int tipOffsetX, tipOffsetY` – 提示框相对鼠标的偏移 (Tooltip offset from the cursor in X and Y).
  - `ULONGLONG tipHoverTick` – 记录鼠标开始悬停的时间戳 (Timestamp (in GetTickCount units) when the mouse started hovering over the button).
  - `int lastMouseX, lastMouseY` – 记录最近一次鼠标所在的位置，用于确定 tooltip 出现位置 (The last known mouse coordinates over the button, used for positioning the tooltip).
  - `std::string tipTextClick` – NORMAL 模式或 Toggle 未选中状态下使用的提示文本 (Tooltip text used in normal mode or when toggle button is in off state).
  - `std::string tipTextOn` – Toggle 按钮选中状态下使用的提示文本 (Tooltip text when the toggle button is in the on state).
  - `std::string tipTextOff` – Toggle 按钮未选中状态下使用的提示文本 (Tooltip text when the toggle button is in the off state).
  - `Label tipLabel` – 用于显示 tooltip 的 Label 控件对象。内部直接复用一个 Label 来呈现提示文本。tooltip 逻辑通过操作 tipLabel 的显示和内容实现 (A Label object used internally to render the tooltip text. The button’s tooltip functionality is implemented by configuring and showing this label near the cursor when needed).

**依赖**：Button 依赖 **Label** 控件来显示其 Tooltip 提示 (The Button class internally uses a **Label** control to display tooltips). 它还使用框架的 **ControlShape**、**FillMode**、**FillStyle** 等枚举定义不同的外观选项 (It uses framework enums like ControlShape, FillMode, FillStyle, etc., for its appearance settings). Button 是常用控件，通常作为对话框按钮或触发某动作的 UI 元素使用 (Button is a fundamental interactive control, commonly used in dialogs or as a trigger for actions in the UI).

### Label（标签控件）

**类名**：`Label`
 **继承**：继承自 Control (`class Label : public Control`)

**简要说明**：静态文本标签控件，用于显示只读文本内容。Label 不处理用户输入，主要作用是呈现文本，可支持背景透明。（*A static text label control for displaying read-only text. The Label does not handle user input and is mainly for presenting text, with support for transparent background.*）

**公共方法**：

- `Label()` – 默认构造函数，创建一个空文本的标签，使用默认字体颜色黑色、背景白色，默认大小适配文本 (Default constructor that creates a label with default text “标签” (Chinese for “Label”), black text color, white background. The size will be determined based on text content once set).
- `Label(int x, int y, std::string text = "标签", COLORREF textColor = BLACK, COLORREF bkColor = RGB(255,255,255))` – 重载构造函数，在指定位置创建带初始文本的标签，可指定文字颜色和背景色。背景默认白色，text默认“标签” (Overloaded constructor to create a label at given position with initial text and optional text color and background color. The default text is “标签” (label), default text color black, default background white).
- `void draw() override` – 绘制标签内容。实现为：如果 `textBkDisap` 为 false，则先绘制背景矩形（填充 `textBkColor`），然后设置文字颜色和字体，将 `text` 绘制在控件位置。如果 `textBkDisap` 为 true，则不绘制背景使其透明叠加 (Renders the label. If `textBkDisap` (background disappear flag) is false, it fills a rectangle with `textBkColor` as background; then draws the `text` at the label’s position using the current font and text color. If `textBkDisap` is true, no background rectangle is drawn, allowing whatever is behind the label to show through, achieving a transparent background effect). Label 根据其文本和样式自动适应大小 (The label’s drawn size adapts to the text content and style).
- `void hide()` – 将标签隐藏。实现上相当于 `setIsVisible(false)`，并（可能）清除或不再绘制标签内容 (Hides the label, equivalent to calling setIsVisible(false) so it will no longer be drawn. This is a convenience method to quickly make the label invisible).
- `void setTextdisap(bool transparent)` – 设置标签背景是否透明。传入 true 则开启背景透明模式（不绘制背景颜色），false 则有背景色填充 (Enables or disables transparent background for the label. True means the label will not draw any background (transparent), false means it will fill a background with its background color).
- `void setTextBkColor(COLORREF color)` – 设置标签背景颜色。仅当背景不透明时有效，改变 Label 绘制时填充的背景矩形颜色 (Sets the background color of the label. Has effect only if the label is not set to transparent; changes the color used to fill behind the text).
- `void setText(std::string newText)` – 设置标签显示的文本内容。更新文本后会自动调整控件大小以适应新文本长度，并将控件标记为需要重绘 (Changes the text displayed by the label. After setting, the label may resize itself to fit the new text (depending on implementation) and will be marked dirty for redraw).

**事件处理**：Label 重写了 `handleEvent` 但不做任何处理，总是返回 false。这意味着标签不拦截或消费任何事件，事件会传递给底层控件 (The Label overrides `handleEvent` to do nothing and always return false, meaning it does not consume any events and they will pass through it).

**成员变量**：

- `std::string text` – 标签显示的文本内容 (The text string displayed by the label).
- `COLORREF textBkColor` – 文本背景色 (Background color behind the text). 默认情况下在构造时设为白色或指定颜色 (Set in constructor, default white unless specified).
- `bool textBkDisap` – 文本背景是否透明的标志。true 表示背景透明（不绘制背景色），false 表示绘制不透明背景 (Flag indicating if the label’s background is transparent. True means do not draw the background (transparent), false means fill background with `textBkColor`). 默认 false (Default is false, meaning background is drawn).
- `StellarX::ControlText textStyle` – 标签文本的样式 (字体、字号、颜色等)。默认使用黑色字体、默认大小 (Defaults to a standard font (e.g., 12pt “微软雅黑”) and black color, can be modified to change font style or color). 开发者可以直接修改该结构体的字段（如 textStyle.color 等）然后调用 `draw()` 刷新，以改变标签文字外观 (Developers can modify this struct’s fields (e.g., textStyle.color for text color) and then redraw to change the label’s text appearance).

**依赖**：Label 主要依赖 **ControlText** 结构来存储字体和颜色信息，使用 Windows GDI（通过 EasyX）绘制文本。它无子控件且不处理事件，常作为静态显示用途 (The Label uses **ControlText** for font info and EasyX (GDI) to draw text. It has no children and performs no event logic, typically used for static display).

### TextBox（文本框控件）

**类名**：`TextBox`
 **继承**：继承自 Control (`class TextBox : public Control`)

**简要说明**：文本输入框控件，支持用户输入和只读显示两种模式，并提供可选的密码模式。内部集成 EasyX 的输入框功能来实现用户输入。（*A text box control that supports both user input mode and read-only display mode, as well as an optional password mode. It internally leverages EasyX’s input box capabilities for text input.*）

**公共方法**：

- `TextBox(int x, int y, int width, int height, std::string text = "", StellarX::TextBoxmode mode = INPUT_MODE, StellarX::ControlShape shape = RECTANGLE)` – 构造函数，在指定位置创建文本框，初始化显示文本、模式和形状。默认模式为可输入，默认形状为有边框矩形 (Constructs a TextBox at the given position and size, with an initial text (which defaults to empty), a mode (defaults to input mode), and shape (defaults to a bordered rectangle)).
- `void draw() override` – 绘制文本框外观。根据当前形状绘制文本框的边框和背景，然后绘制文本内容。若模式为密码模式，则绘制时会隐藏实际文字（例如用星号替代）(Renders the text box. It draws the rectangle (or chosen shape) for the text box border and background, then draws the text. If the mode is password, the actual characters are hidden (likely rendered as asterisks or bullets)).
- `bool handleEvent(const ExMessage& msg) override` – 处理文本框的鼠标和键盘事件。当检测到鼠标点击文本框时，若模式为可输入，则调用 EasyX 提供的 `InputBox` 弹出文本输入对话框获取用户输入。获取输入后更新内部文本并标记重绘 (Handles events for the text box. If a mouse click is detected within the text box and the mode is INPUT_MODE, it uses EasyX’s InputBox (a system modal input dialog) to get user input. After the user enters text and closes the InputBox, the TextBox updates its internal text with the new value and marks itself dirty for redraw). 在 READONLY_MODE 下，点击不会触发输入框 (In READONLY_MODE, clicking does nothing).
- `void setMode(StellarX::TextBoxmode mode)` – 设置文本框的工作模式（输入或只读或密码）。更改模式会影响交互行为：只读模式下禁止输入，密码模式下输入的文字以特殊符号显示 (Sets the TextBox’s mode. Changing the mode alters its behavior: in READONLY_MODE, user input is disabled; in PASSWORD_MODE, the text is masked when displayed).
- `void setMaxCharLen(size_t len)` – 设置文本框允许输入的最大字符长度。限制用户通过 InputBox 输入的字符数量 (Sets the maximum number of characters that can be input in the text box. This limits the length of text accepted by the InputBox dialog).
- `void setTextBoxshape(StellarX::ControlShape shape)` – 设置文本框的形状类型。可选矩形或圆角矩形等，但一般文本框使用矩形外观 (Sets the shape of the text box (rectangle, rounded rectangle, etc.). Typically text boxes use a rectangular shape).
- `void setTextBoxBorder(COLORREF color)` – 设置文本框边框颜色。改变文本框四周边框的绘制颜色 (Changes the border color of the text box’s outline).
- `void setTextBoxBk(COLORREF color)` – 设置文本框背景填充颜色。 (Sets the background fill color of the text box).
- `void setText(std::string text)` – 设置文本框显示的文本内容。如果在 INPUT_MODE 下，可以程序matically预填内容；在 READONLY_MODE 下用于显示只读文本 (Sets the text content of the text box. In input mode, can be used to pre-fill text; in read-only mode, use it to display static text). 设置后标记控件需要重绘 (After setting, the control is marked dirty for redraw).
- `std::string getText() const` – 获取文本框当前的文本内容。如果处于密码模式，返回的仍是实际文本而非掩码字符 (Returns the current text content of the text box. In password mode, this returns the actual stored text, not the masked display).

**成员变量**：

- `std::string text` – 文本框当前包含的文本内容 (The text currently stored/displayed in the text box).
- `StellarX::TextBoxmode mode` – 文本框当前模式（INPUT_MODE 可输入、READONLY_MODE 只读、PASSWORD_MODE 密码）.
- `StellarX::ControlShape shape` – 文本框外观形状（矩形、圆角矩形等）.
- `bool click` – 文本框点击状态标志。用于指示当前控件是否处于选中/激活（点击后正在输入）的状态。例如，click 为 true 时可能表示输入框正在获取输入 (A flag possibly indicating if the textbox is “active” or clicked. True might mean the text box was clicked and is awaiting input, though the exact usage depends on implementation details).
- `size_t maxCharLen` – 文本框允许输入的最大字符数，默认10。超过此长度的输入会被截断 (Maximum number of characters allowed in the text box. Defaults to 10. Input beyond this length will be truncated or disallowed).
- `COLORREF textBoxBkClor` – 文本框背景颜色，默认白色 (Background color of the text box, default is white).
- `COLORREF textBoxBorderClor` – 文本框边框颜色，默认黑色 (Border color of the text box, default is black).
- `StellarX::ControlText textStyle` – 文本框文本样式。决定文本绘制的字体、颜色等 (The font and text appearance settings for the text content of the text box). 可以调整其中字体大小以改变文本框中文字的显示大小 (Modifying this (e.g., font height) will change how the text appears inside the box).

**依赖**：TextBox 依赖 **CoreTypes** 中的 **TextBoxmode** 枚举来区分模式，还使用 EasyX 的 `InputBox` 函数进行实际文本输入对话框（在 Windows 环境下弹出模态输入窗口）。在图形上，它类似 **Button** 使用 **ControlShape** 控制边框形状。TextBox 提供了简单的文本输入能力，对于复杂文本编辑需要另行扩展 (Internally, the TextBox calls EasyX’s InputBox to get user input in a modal dialog, which is a limitation to Windows platform. Graphically, it uses ControlShape for its outline similar to Button. It is intended for basic text input; more complex text editing would require additional functionality beyond this control).

### Table（表格控件）

**类名**：`Table`
 **继承**：继承自 Control (`class Table : public Control`)

**简要说明**：高级表格控件，用于显示二维表格数据，支持分页显示和大量数据高效渲染。Table 提供表头、数据行分页、翻页按钮等完整功能，适合用于数据报告、记录浏览等场景。（*An advanced table/grid control for displaying tabular data. It supports pagination and efficient rendering of large data sets. The Table provides features such as headers, paginated data rows, navigation buttons for page flips, etc., making it suitable for reports, record browsers, and similar use cases.*）

**主要特性**: 自动分页计算、每页行数可配置、自适应列宽行高、翻页导航按钮、背景缓存优化绘制性能等。（*Key features include automatic pagination and page index calculation, configurable rows per page, auto-calculated column widths and row heights, navigation buttons for paging, and background buffering to optimize rendering performance.*）

**公共方法**：

- `Table(int x, int y)` – 构造函数，在指定位置创建一个空的表格控件。默认表格初始宽高由内容决定（通常在设置数据后计算）。默认每页显示行数为 5。创建后需调用 `setHeaders` 和 `setData` 来填充表头和数据 (Constructs a Table at the given position (top-left). The initial size may be determined after data is provided. By default, it shows 5 rows per page. After construction, one should call `setHeaders` and `setData` to populate the table).
- `~Table()` – 析构函数，清理表格资源 (Destructor to clean up any resources. It will delete any dynamically allocated components, like navigation buttons or labels created internally).
- `void draw() override` – 绘制表格当前页的内容。绘制流程包括表头、数据行以及页脚（页码和翻页按钮）。Table 内部通过辅助的私有 `drawHeader()`, `drawTable()`, `drawPageNum()`, `drawButton()` 方法分别绘制不同部分。(Draws the table for the current page. The drawing routine includes rendering the header row, the data rows for the current page, and the footer (page number display and navigation buttons). Internally it likely uses helper functions like `drawHeader()`, `drawTable()` (for data cells), `drawPageNum()`, and `drawButton()` for different sections.) Table 会根据内容自动计算列宽和行高，并支持隔行变色、高亮当前行等（如果实现了的话）(The Table calculates column widths and row heights based on content, and may support features like alternating row colors or highlighting if implemented).
- `bool handleEvent(const ExMessage& msg) override` – 处理表格的事件。包括翻页按钮的点击事件，可能还有对行的点击选择等 (Handles events for the table, such as detecting clicks on the "Previous" or "Next" page buttons, and potentially row click events for selection if such feature exists). 当点击翻页按钮时，会更新 `currentPage` 并触发重绘显示新页数据 (On clicking navigation buttons, it updates `currentPage` and triggers a redraw to display the new page of data).
- 设置数据与外观的方法 (Data and appearance setters):
  - `void setHeaders(std::initializer_list<std::string> headers)` – 设置表格表头列名。传入一个字符串列表，每个元素作为一列的标题。调用后会自动计算列宽，并标记需要重绘表头 (Sets the table’s header labels. The initializer_list of strings provides a title for each column. This triggers a recalculation of column widths and marks the table for redraw).
  - `void setData(std::vector<std::string> data)` – 设置表格数据。**注意**: 这里参数类型可能是错误，应为 `std::vector<std::vector<std::string>>` 或初始化列表（见下）(Note: The signature in code appears to be `setData(std::vector<std::string>)` which is likely a typo; actual implementation probably expects a 2D structure. Use the initializer_list overload for clarity).
  - `void setData(std::initializer_list<std::vector<std::string>> data)` – 设置表格数据。传入一个列表，每个元素是代表一行的字符串向量。Table 将保存数据并自动分页计算 `totalPages` 和 `currentPage` 等 (Sets the table data by providing an initializer_list of rows, where each row is a vector of strings for the columns. The table stores this data and computes `totalPages` based on rowsPerPage, resetting currentPage to 1).
  - `void setRowsPerPage(int rows)` – 设置每页显示的行数。改变此值会重新计算总页数并调整当前显示 (Sets how many rows are displayed per page. Changing this will recalculate `totalPages` and adjust the current page if needed).
  - `void showPageButton(bool show)` – 设置是否显示翻页按钮。如果数据量小仅一页，开发者可选择隐藏翻页控制 (Determines whether the page navigation buttons (prev/next) are visible. If the data fits on one page, one might hide the navigation controls).
  - `void setTableBorder(COLORREF color)` – 设置表格边框颜色（单元格网格线颜色）。改变表格绘制时单元格边框和外框的颜色 (Sets the color used for the table grid lines and border).
  - `void setTableBk(COLORREF color)` – 设置表格背景颜色。作为单元格背景的底色 (Sets the background color used for the table’s cells).
  - `void setTableFillMode(StellarX::FillMode mode)` – 设置表格单元格背景填充模式。可选择纯色或图案填充 (Sets the fill mode for the table cells background, e.g., solid fill or hatched pattern).
  - `void setTableLineStyle(StellarX::LineStyle style)` – 设置表格线型样式。用于单元格边框线，可选实线、虚线等 (Sets the line style for the table grid lines (cells borders), e.g., solid, dashed).
  - `void setTableBorderWidth(int width)` – 设置表格网格线和外边框的宽度（像素）(Sets the width (thickness) of the table’s grid lines and border in pixels).
  - `void clearHeaders()` – 清空表头。将 headers 列表置空，并标记需要重绘 (Clears all header labels. After calling, the table will have no columns defined until new headers are set).
  - `void clearData()` – 清空表格数据。移除所有数据行，重置当前页等 (Clears all table data rows and resets pagination state).
  - `void resetTable()` – 清空表头和数据。相当于依次调用 clearHeaders() 和 clearData(), 将表格恢复初始空状态 (Clears both headers and data, resetting the table to an initial empty state).
  - `void onWindowResize() override` – 当容器窗口大小变化时调用。实现为丢弃背景快照并标记表格为脏，需要在新尺寸下重绘 (Called when the window is resized. The Table’s implementation likely discards any saved background (if using caching) and marks itself dirty to redraw according to the new size).
- 查询方法 (Getters):
  - `int getCurrentPage() const` – 获取当前页码（从1开始计数）.
  - `int getTotalPages() const` – 获取总页数.
  - `int getRowsPerPage() const` – 获取每页显示的行数设置.
  - `bool getShowPageButton() const` – 获取当前是否设置为显示翻页按钮.
  - `COLORREF getTableBorder() const` – 获取表格边框（网格线）颜色.
  - `COLORREF getTableBk() const` – 获取表格背景颜色.
  - `StellarX::FillMode getTableFillMode() const` – 获取表格当前填充模式.
  - `StellarX::LineStyle getTableLineStyle() const` – 获取表格当前线型样式.
  - `std::vector<std::string> getHeaders() const` – 获取当前表头列表的副本.
  - `std::vector<std::vector<std::string>> getData() const` – 获取当前表格数据的副本.
  - `int getTableBorderWidth() const` – 获取表格边框/网格线宽度.
  - `int getTableWidth() const, int getTableHeight() const` – 获取表格当前总宽度和总高度。这通常对应绘制时计算出的整个表格占用尺寸，包括表头和页脚 (Returns the overall width and height of the table as currently rendered, including header and footer areas. These are typically calculated during the last draw).

**成员变量** *(主要为私有属性)*：

- `std::vector<std::vector<std::string>> data` – 表格数据存储，按行列组织的字符串二维数组。每个内部 `std::vector<std::string>` 代表一行的数据，所有行长度应与 headers 列数匹配 (Stores the table’s data in a 2D vector of strings (rows × columns). Each inner vector represents one row. The number of elements in each row should match the number of header columns).
- `std::vector<std::string> headers` – 表格列标题列表。长度即列数 (List of column headers. The size of this vector defines the number of columns in the table).
- `std::string pageNumtext` – 页码标签文本模版。默认值 "页码标签"，可能用于显示当前页/总页的信息 (A template or base text for the page number label, default "页码标签". This might be combined with current page info to display pagination status like “第 X 页/共 Y 页”).
- `int tableBorderWidth` – 表格边框和网格线宽度，默认值来自 `TABLE_DEFAULT_BORDER_WIDTH` 宏，通常为1 (Thickness of table grid lines and border. Default comes from TABLE_DEFAULT_BORDER_WIDTH, typically 1 pixel).
- `std::vector<int> colWidths` – 每列宽度像素值的数组。绘制时每列的起始位置和单元格矩形基于此计算 (Array of pixel widths for each column. These are calculated from header/text lengths and used to layout the table cells).
- `std::vector<int> lineHeights` – 每行高度像素值的数组。通常各行高度相同，由字体高度和 padding 决定；也可能表头行和数据行区分 (Array of heights for each row. Often uniform for all data rows and possibly a different height for header; determined by text height and vertical padding).
- `int rowsPerPage` – 每页显示行数，默认 5.
- `int currentPage` – 当前页码，默认 1（第一页）.
- `int totalPages` – 总页数，默认 1。根据数据行数和 rowsPerPage 计算得到 (Total number of pages, computed based on number of data rows and rowsPerPage).
- **特性开关**:
  - `bool isShowPageButton` – 是否显示翻页按钮，默认 true.
  - `bool isNeedDrawHeaders` – 是否需要绘制表头标志，暂未使用（代码注释为“暂时废弃”）.
  - `bool isNeedCellSize` – 是否需要计算单元格尺寸标志。可能用于延迟计算优化 (Flag indicating if cell sizes need recalculation. Possibly used to optimize and avoid recalculating cell metrics on every draw).
  - `bool isNeedButtonAndPageNum` – 是否需要计算翻页按钮和页码信息标志。类似地用于控制是否更新页脚 (Flag indicating if navigation buttons and page number label need to be recalculated/positioned. Could be used to skip re-layout of footer if not needed).
- **导航控件**:
  - `Button* prevButton` – “上一页”按钮指针。指向 Table 内部创建的 Button 对象，用于翻页到前一页。nullptr 表示未创建或隐藏 (Pointer to the "Previous Page" button control. Created internally by the Table for pagination. If nullptr, the button is not present or not needed).
  - `Button* nextButton` – “下一页”按钮指针。用于翻到下一页 (Pointer to the "Next Page" button).
  - `Label* pageNum` – 页码显示的标签指针。用于显示当前页/总页信息 (Pointer to a Label that displays the page number text, e.g., "第 X 页/共 Y 页").
- **内部坐标缓存**（用于绘制过程的临时值）:
  - `int dX, dY` – 当前单元格绘制的起始坐标 (likely "draw X/Y": the starting x,y for drawing cells). 初始化为表格区域左上角 (Initialized to x,y of the table). 绘制过程中更新以逐行逐列推进 (These are updated as the table draws each cell to track the current drawing position).
  - `int uX, uY` – 当前单元格绘制的结束坐标 (likely "end X/Y": maybe unused or similar to dX,dY as drawing coordinates).
  - `int pX, pY` – 页码标签 Label 的左上角坐标 (Coordinates for where the page number label is placed).
- **颜色和样式**:
  - `StellarX::FillMode  tableFillMode` – 表格单元格填充模式, 默认 Solid。
  - `StellarX::LineStyle tableLineStyle` – 表格网格线线型, 默认 Solid 实线。
  - `COLORREF tableBorderClor` – 表格边框和网格线颜色, 默认黑色 (RGB(0,0,0))。
  - `COLORREF tableBkClor` – 表格背景底色, 默认白色 (RGB(255,255,255))。
- `StellarX::ControlText textStyle` – 表格内容文本样式。用于设置表格单元格中文字的字体和颜色，默认继承自 ControlText 默认值 (Determines font and color for text drawn in the table’s cells). 可以修改例如字体大小以调整表格内容字号 (Can be adjusted to change text size of table content).

**依赖**：Table 控件内部组合使用 **Button**（翻页按钮）和 **Label**（页码显示）控件来实现完整功能，因此它依赖这些子控件的接口来处理事件和绘制文字。Table 作为容器包含这些控件并在翻页时对它们进行布局和更新 (The Table internally composes **Button** controls for navigation and a **Label** for page number, thus it relies on their functionality for event handling and text rendering. The Table acts as a container for these controls, positioning and updating them as pages change). Table 还会使用 GDI 绘制单元格边框和背景，对性能要求高时利用了背景缓存等优化 (The Table uses GDI (EasyX) to draw cell grids and background, and likely employs background buffering optimizations to handle large data efficiently).

## 复合控件（Composite Controls）

复合控件是指包含其他控件作为子元素，实现更复杂界面功能的控件。例如 TabControl 包含多个页面 Canvas 和对应的按钮，Dialog 包含文本、按钮等控件。下面是这些复合控件的接口说明： *(Composite controls contain other controls as child elements to provide more complex UI functionality. For example, TabControl contains multiple Canvas pages and their respective tab buttons; Dialog contains text and button controls. Below are the interface details of these composite controls:)*

### TabControl（选项卡控件）

**类名**：`TabControl`
 **继承**：继承自 Canvas (`class TabControl : public Canvas`)

**简要说明**：选项卡容器控件，可在单一界面区域内承载多个子页面，通过页签（标签按钮）进行切换。TabControl 提供类似选项卡对话框的界面，每个选项卡有一个标题按钮和对应的内容区域。（*A tabbed container control that can hold multiple child “pages” in the same area and switch between them using tab header buttons. It provides a tabbed dialog-like interface where each tab has a header button and an associated content area.*）

**功能特点**：TabControl 支持将页签栏放置在容器的不同位置（上、下、左、右）。每个选项卡由一个按钮（作为页签）和一个 Canvas 容器（作为页面内容）组成。一次仅显示一个选项卡的内容，其余页面隐藏。TabControl 自动管理页签按钮的布局和当前激活页内容的显示切换。（*The TabControl allows the tab bar to be placed at the top, bottom, left, or right of the control. Each tab consists of a Button (tab header) and a Canvas (page content). Only one page is visible at a time, with others hidden. The TabControl automatically handles laying out the tab header buttons and toggling the visibility of the page content for the active tab.*）

**公共方法**：

- `TabControl()` – 默认构造函数，创建一个默认大小的选项卡控件。内部调用 Canvas 基类构造将位置设为 (0,0), 宽高默认100×100，并设置控件ID为 "TabControl" (Calls the Canvas constructor to initialize a TabControl at (0,0) of default size, and sets its id to "TabControl").
- `TabControl(int x, int y, int width, int height)` – 构造函数，在指定位置和尺寸创建一个选项卡控件。初始化后无选项卡，需要通过 `add` 方法添加页面 (Creates a TabControl at specified position with given width and height. Initially it contains no tabs; use `add()` to add tab pages).
- `void add(std::pair<std::unique_ptr<Button>, std::unique_ptr<Canvas>>&& control)` – **添加选项卡页面**，将一个按钮-页面对添加为新的选项卡。`pair.first` 是页签按钮，`pair.second` 是对应内容 Canvas。调用该方法后，TabControl 会负责管理该按钮和页面的布局与显示 (Adds a new tab to the TabControl given a pair of a Button (tab header) and a Canvas (tab page content). After adding, the TabControl takes ownership and will manage the layout and display of the tab’s button and page). 调用后自动调整所有页签按钮大小并初始化新页面坐标，将新选项卡默认设置为隐藏内容。(The method will recalc the size of all tab buttons, position the new page, and by default hide the new page’s content so that it’s not shown until activated.)
- `void add(std::string tabText, std::unique_ptr<Control> control)` – **向已有选项卡添加子控件**。此重载用于将一个新的控件添加到已有的某个选项卡页面内。`tabText` 指定目标选项卡的标题文本，`control` 是要添加的控件指针。调用时会在匹配标题的选项卡页面 Canvas 内添加该控件，并保持其相对坐标 (Adds a control to one of the existing tab pages. The `tabText` identifies which tab’s page to add to (by matching the tab’s title text), and `control` is the new child control to add. The method sets the new control’s parent to the target page’s Canvas and adjusts its coordinates to be relative to that page). 如果 `tabText` 没找到对应页签，则不执行操作 (If no tab with the given title is found, the call does nothing).
- `void setTabPlacement(StellarX::TabPlacement placement)` – 设置选项卡页签栏的位置。可以在 TabControl 的顶部、底部、左侧或右侧显示页签。调用后立即重新布局页签和页面区域 (Sets the placement of the tabs (Top, Bottom, Left, or Right). Recomputes the layout of tab header buttons and page content areas accordingly).
- `void setTabBarHeight(int height)` – 设置页签栏的高度（或宽度，如果在左/右侧）。调整这个值可以改变页签按钮的大小。默认高度有最小值限制 (Sets the height of the tab bar (if tabs are top/bottom) or width (if tabs are left/right)). This effectively changes the size of each tab button on that dimension. There is an internal minimum enforced for tab button size to ensure usability (e.g., `BUTMINHEIGHT`, `BUTMINWIDTH`).
- `void setActiveIndex(int index)` – 显式设置当前激活的选项卡索引。索引从0开始。若指定索引有效且不等于当前激活，则将对应页签按钮设为点击状态，切换显示该页内容。如果在首次绘制前调用，则会将此索引存为默认激活页 (Sets the active tab by index (0-based). If called after the control is shown and the index is valid (and different from current), it programmatically “clicks” that tab’s button to activate the page. If called before the first draw (initialization phase), the index is stored as the default to activate on first render rather than switching immediately).
- `int getActiveIndex() const` – 获取当前激活的选项卡索引。如果没有页签被选中则可能返回 -1 (Returns the index of the currently active tab. If no tab is active, it returns -1).
- `int count() const` – 获取当前选项卡总数量.
- `int indexOf(const std::string& tabText) const` – 根据页签标题文本查找对应的索引。如果存在多个同名页签，返回第一个匹配的索引；如未找到则返回 -1 (Returns the index of the tab with the given title text. If multiple tabs have the same title, returns the first match. Returns -1 if not found).
- `void setIsVisible(bool visible) override` – 重载自 Canvas：设置整个 TabControl（以及其中所有页签按钮和页面）的可见性。如果设置为可见，则只显示当前激活的页面，但**所有**页签按钮都会显示；如果设置为不可见，则页签和页面全部隐藏。此外，该实现会确保隐藏状态下子页面不会遗留背景快照 (Overrides Canvas::setIsVisible to show/hide the TabControl along with all its tabs and pages. When made visible, all tab header buttons are shown and only the currently active page’s Canvas is shown (others remain hidden); when invisible, all tabs and pages are hidden. It also handles clearing out saved snapshots for hidden pages to prevent stale visuals).
- `void onWindowResize() override` – 当父窗口尺寸改变时调用。实现为：调用 Control 基类的 onWindowResize() 丢弃自身背景，然后重新计算页签栏和页面区域布局。之后对每个页签按钮和页面调用各自 onWindowResize() 使其内容也调整 (On parent window resize, the TabControl’s override first calls Control::onWindowResize() to discard its background and mark dirty. It then recalculates the tab bar and page layout (size and position) for the new control size. Finally it forwards the onWindowResize call to each tab’s Button and Canvas so they can adjust their internals if needed).
- `void setDirty(bool dirty) override` – 重载自 Control：将 TabControl 及其所有子页签按钮和页面的脏标记设为给定值。通常在需要整组重绘时调用 (Overrides Control::setDirty to set the dirty flag for the TabControl itself and all its child tab buttons and page Canvases to the given value (true/false). This is useful to invalidate or validate the entire tab set at once).
- `void requestRepaint(Control* parent) override` – 重载自 Control：处理 TabControl 自身或子控件请求重绘的逻辑。如果 `parent` 参数就是当前 TabControl 本身，则表示一次针对整个选项卡控件的重绘请求；实现中会检查每个子控件（按钮和页面）的 dirty 状态，将需要重绘的子控件各自绘制一遍。否则（parent 不是本控件），调用基类 Canvas 的 onRequestRepaintAsRoot() 让更高层容器处理。(Overrides Control::requestRepaint. If the `parent` passed in is this TabControl itself (meaning a repaint of the whole tab control is requested), the implementation iterates through all tab pairs and if a tab’s Button or Canvas is dirty and visible, it calls their draw() to repaint them. If `parent` is not this (meaning the request is bubbled further up), it defers to the base behavior by calling onRequestRepaintAsRoot().) 这一特殊实现确保当 TabControl 本身重绘时，其子页面内容也会刷新 (This ensures that when the TabControl is repainted, all visible child content is also redrawn).

**事件处理**：TabControl 实现自己的 `handleEvent` 逻辑，将事件分发给当前页的子控件或页签按钮。具体而言，当鼠标点击一个页签按钮时，相应按钮会触发回调使该选项卡切换为激活状态；TabControl::handleEvent 会优先将事件传给所有页签按钮（通常是排列在最上层）检查是否有按钮点击。如果没有页签按钮消费事件，则再把事件传递给当前激活的页面 Canvas 里的子控件。这样实现当用户点击不同区域时由不同元素响应 (The TabControl’s event handling sends incoming messages first to the tab header buttons (which are typically drawn above the page content) to see if any button consumes the event (e.g., a click on a tab). If none of the buttons handle the event, it then forwards the event to the currently visible page’s Canvas (which in turn dispatches to its child controls). This ensures the correct element responds depending on where the user clicked).

**成员变量**：

- `std::vector< std::pair< std::unique_ptr<Button>, std::unique_ptr<Canvas> > > controls` – 存储选项卡的容器，包含多个 (按钮, 页面Canvas) 对。每个 pair 的 first 是页签按钮，second 是对应内容页面 (Vector holding the tab definitions: each element is a pair where first is the tab header Button and second is the page Canvas for that tab). 这个容器是 TabControl 最重要的结构，用于迭代布局和事件处理 (This is central to TabControl’s operation for iterating through tabs for layout, drawing, and event dispatch).
- `int tabBarHeight` – 页签栏高度（或宽度），用于决定页签按钮尺寸。默认值可能为某个常量，例如30像素 (Height of the tab bar. Determines the size of tab header buttons in the minor axis. Default might be around 30px, ensuring a minimum usable size). 可以通过 `setTabBarHeight` 修改。
- `StellarX::TabPlacement tabPlacement` – 当前页签栏位置（Top/Bottom/Left/Right）。默认应该为 Top (Default placement likely Top). 改变该值会在下一次布局时把页签移动到相应边 (Changing this will re-position the tabs on the specified side on next layout recalculation).
- `bool IsFirstDraw` – 标记是否尚未绘制过（首次绘制标志）。初始为 true，第一次调用 draw() 后置 false (A flag indicating whether the TabControl has been drawn for the first time or not. Initialized to true, set to false after the first draw). 用于处理 `defaultActivation` 逻辑，在首次绘制时激活预设的选项卡.
- `int defaultActivation` – 默认激活选项卡索引。初始可能为 -1，表示未指定。如果在控件显示前通过 `setActiveIndex` 调用设置了一个索引，则该值保存那个索引，在首次绘制时相应页签会被激活。(Stores an index of a tab to activate on first draw. Default -1 meaning none specified. If `setActiveIndex` is called before the control is drawn, this captures the desired active tab, and during the first `draw()` call the TabControl will activate that tab).
- **常量**：内部定义的最小按钮宽高常量，如 `BUTMINWIDTH`, `BUTMINHEIGHT`，用于确保无论 TabControl 尺寸多小，每个页签按钮都有合理可点击大小。这些常量可能定义为例如 50 像素宽、20 像素高 (There are internal constants for minimum tab button dimensions (e.g., `BUTMINWIDTH`, `BUTMINHEIGHT`) to guarantee each tab remains a clickable size even if the control is very small).
- *（其余布局相关变量和 Canvas 基类的成员，如 x, y, width, height, 以及 Canvas 提供的 children 列表，都通过继承获得）*(Other layout and inherited members: since TabControl extends Canvas, it inherits all Canvas members like x, y, width, height, and the vector of child controls from Canvas. However, in TabControl, instead of using the inherited `controls` vector from Canvas, it manages its own vector of pair for tabs. The Canvas’s own child list might remain empty or unused in this context.)*

**继承和依赖**：TabControl 直接继承自 **Canvas**（即它本身就是一个可包含控件的画布），这意味着 TabControl 拥有 Canvas 的所有能力（背景绘制、子控件容器等），并进一步组合了多个 Button 和 Canvas 子控件来形成选项卡结构。**Button** 用于页签，**Canvas** 用于页内容，因此 TabControl 强依赖 Button 和 Canvas 的接口 (TabControl inherits from **Canvas**, giving it container capabilities. It composes **Button** controls as tab headers and **Canvas** controls as pages. It heavily relies on the Button interface (for toggling and event callbacks) and Canvas for holding page content). TabControl 本身不依赖外部系统资源，除了通过父 Window 获取尺寸变化事件。作为复杂控件，TabControl 提供了一个在桌面 GUI 中组织多页面内容的便捷组件 (As a composite control, TabControl provides an easy way to organize multi-page content in a desktop GUI, without relying on OS-level tabs).

### Dialog（对话框控件）

**类名**：`Dialog`
 **继承**：继承自 Canvas (`class Dialog : public Canvas`)

**简要说明**：模态/非模态对话框控件，提供标准消息框的界面和功能。Dialog 可以独立弹出，包含标题、消息文本和一组按钮（如确定、取消等），并支持阻塞主窗口（模态）或非阻塞回调两种模式。（*A dialog box control supporting both modal and modeless operation, providing a standard message box UI with a title, message text, and a set of buttons (e.g., OK, Cancel). The Dialog can be shown modally (blocking the main window) or non-modally with a callback for results.*）

**主要特性**: 支持多个预定义类型（MessageBoxType 枚举的六种组合按钮），可以根据需要显示相应按钮组合；支持自动根据消息文本长度换行及调整对话框尺寸；在模态模式下弹出会阻塞调用线程直到对话框关闭，在非模态模式下则通过回调通知结果；对话框绘制时会保存背景并在关闭时恢复，以实现半透明效果或防止闪烁。（*Key features include support for several predefined configurations of buttons (according to the MessageBoxType enum), automatic text wrapping and dialog resizing based on content, modal display (blocking until closed) vs. modeless with callbacks, and background saving/restoring to prevent flicker or allow transparency effects.*）

**公共方法**：

- `Dialog(Window& hWnd, std::string text, std::string message = "对话框", StellarX::MessageBoxType type = OK, bool modal = true)` – 构造函数，创建一个对话框实例。参数：`hWnd` 父窗口引用，用于在该窗口中央显示对话框；`text` 对话框显示的消息内容；`message` 对话框标题文本（默认为“对话框”）；`type` 对话框类型（决定按钮组合，默认为 OK 单按钮）；`modal` 指定是否模态 (true=模态)。构造函数内会根据类型创建对应的按钮集合、关闭按钮等，并计算对话框初始尺寸 (The constructor sets up the dialog: it stores a reference to the parent Window, saves the provided message text and title, creates the appropriate buttons according to the specified type, adds a close (“X”) button, and computes an initial size based on text length and number of buttons. If `modal` is true, the Dialog is configured to block input to the parent window when shown).
- `~Dialog()` – 析构函数，负责销毁对话框内创建的控件（按钮、标签等）并释放背景缓存等资源。若对话框在非模态显示且尚未关闭，析构时应通知父窗口清理引用 (Destroys all child controls created inside the dialog (buttons, labels) and releases any saved background images. If the dialog was modeless and still open, the destructor ensures any references in the parent are cleared).
- `void draw() override` – 绘制对话框。Dialog 继承自 Canvas，draw() 实现包含：绘制对话框背景矩形（使用背景颜色 backgroundColor 和边框颜色 borderColor，通常为一个带边框的圆角矩形或矩形）；绘制标题栏（包括标题文本Label和关闭按钮）；绘制消息文本内容（自动换行后的多行）；绘制底部功能按钮区域（按照 type 创建的确定/取消等按钮）。绘制过程中处理背景快照以在出现对话框时保存被遮挡的内容 (The Dialog’s draw method draws the dialog’s background and border (often a rounded rectangle with borderColor), draws the title bar (title text label and close button at the top), renders the message text (splitting it into lines to fit width), and draws the bottom area with the action buttons as specified by the dialog type. It uses background snapshot routines to save what's behind the dialog when it appears and restore upon closing to prevent artifacts).
- `bool handleEvent(const ExMessage& msg) override` – 处理对话框的事件。包括：点击关闭按钮时将关闭对话框；点击功能按钮（如确定、取消等）时，将根据按钮设定设置结果枚举，并关闭对话框（模态则结束阻塞返回结果，非模态则通过回调）; 以及拖动窗口（如果实现了窗口拖动）(Handles events such as clicks on the close button (triggers Close()), clicks on any of the action buttons (sets the result accordingly and closes the dialog), and possibly window dragging if the title bar is draggable). 在模态情况下，handleEvent 内部可能会自己运行消息循环直到对话框关闭 (In modal mode, handleEvent might not be used in the typical way since control is in a separate loop; but for modeless, it ensures events are passed to child controls).
- **设置和获取**:
  - `void SetTitle(const std::string& title)` – 设置对话框标题文本。更新内部 `titleText` 并刷新标题 Label (Changes the dialog’s title. Updates the internal `titleText` and the title Label control accordingly).
  - `void SetMessage(const std::string& message)` – 设置对话框主消息文本。更新内部 `message` 并重新拆分行、调整对话框大小 (Sets the main message text of the dialog. Updates the internal `message` string, re-splits it into lines, and recalculates dialog size as needed to accommodate the new text).
  - `void SetType(StellarX::MessageBoxType type)` – 设置对话框类型。更改需要显示的按钮组合，例如从 OK 切换为 YesNo 等。调用将移除原有按钮并根据新类型创建新按钮，重新布局底部区域 (Changes which buttons are displayed by altering the MessageBoxType. This will remove any existing action buttons and create new ones for the specified type, then reposition them in the footer).
  - `void SetModal(bool modal)` – 设置对话框的模态属性。true 表示在 Show() 时以模态方式运行（阻塞），false 表示以非模态方式运行 (Configures whether the dialog is modal or not. True means it will run modally (blocking the caller until closed), false means it will be modeless).
  - `void SetResult(StellarX::MessageBoxResult result)` – 人为设置对话框结果。这通常在对话框逻辑内部使用，例如用户点击按钮后调用，以记录选择的结果 (Manually sets the dialog’s result. Typically used internally when a button is clicked to store the user’s choice prior to closing the dialog).
  - `StellarX::MessageBoxResult GetResult() const` – 获取对话框结果。仅在对话框关闭后有意义：模态对话框可以通过此函数获取用户选择的结果枚举值 (Returns the dialog’s result (which button was pressed). This is useful after a modal dialog closes to know what the user selected).
- `bool model() const override` – 返回对话框是否为模态。Dialog 实现此函数，用于区分模态对话框（返回 true）和非模态（返回 false）。父窗口在处理窗口事件时会据此判断是否需要暂停交互 (Indicates if the dialog is modal. The Dialog overrides `model()` to return the value of its modal flag. This can be used by the parent Window to determine whether user interaction with other controls should be blocked).
- `void Show()` – 显示对话框。对于模态对话框，该函数会使对话框在屏幕中央出现并阻塞，直到对话框关闭。对于非模态，则在屏幕中央显示并立即返回 (Displays the dialog. If modal, this will position the dialog (typically centered on parent window) and enter a local event loop, blocking further execution until the dialog is closed by the user. If modeless, it simply makes the dialog visible (likely centered) and returns immediately). Show() 方法内部会将对话框加入父窗口的对话框列表 `Window::dialogs` 并标记窗口存在对话框 (Internally, Show() likely adds this dialog to the parent Window’s `dialogs` vector (for tracking open dialogs) and sets flags like `dialogClose = false` in the window).
- `void Close()` – 关闭对话框。将对话框隐藏并从父窗口管理列表中移除，销毁内部控件。模态情况下Close会结束内部阻塞循环，恢复主窗口事件处理；非模态情况下Close则调用已注册的 resultCallback (Closes the dialog. It hides/removes the dialog from view, removes it from the parent window’s tracking list, and triggers cleanup of its child controls. In modal mode, Close breaks out of the modal loop allowing execution to continue in the caller; in modeless mode, Close will invoke the `resultCallback` (if set) to inform of the result asynchronously).
- `void setInitialization(bool init)` – 设置是否需要初始化标志。这个内部标志 `needsInitialization` 控制对话框仅在首次 Show 时创建按钮等 (Sets the internal `needsInitialization` flag. If true, indicates that the dialog hasn’t yet fully initialized its UI (buttons/labels) and should do so on next show. Once initialized, this can be set false to avoid reinitializing).
- **结果回调**:
  - `std::function<void(StellarX::MessageBoxResult)> resultCallback` – 非模态对话框结果回调函数。在 Dialog 显示为非模态时可以设置此回调，当用户点击某个按钮关闭对话框后将异步调用该函数并传入选择结果 (Callback function that will be called with the dialog’s result when a modeless dialog is closed. This allows handling the user’s choice without blocking).
  - `void SetResultCallback(std::function<void(StellarX::MessageBoxResult)> cb)` – 设置结果回调函数。用于非模态对话框，传入一个函数，当对话框关闭时框架会调用它并将 MessageBoxResult 结果传递进去 (Assigns the callback to call when the dialog is closed in modeless mode, receiving the MessageBoxResult).
- **其他方法**:
  - `void performDelayedCleanup()` – 执行延迟清理。当对话框以非模态显示并关闭时，由 MessageBox 工厂调用，用于实际删除对话框对象 (Performs a delayed cleanup of the dialog. This is used by the MessageBox factory to delete the dialog object some time after closing if it was modeless, ensuring the callback is called before destruction).
  - `std::string GetCaption() const` – 获取对话框标题文本，用于去重检测。MessageBox 工厂在创建非模态对话框时可调用此函数检查是否已有相同标题和消息的对话框未关闭，从而避免重复弹出 (Returns the dialog’s title text (caption). The MessageBox factory uses this to check if a dialog with the same caption/text is already open, to prevent duplicates).
  - `std::string GetText() const` – 获取对话框消息文本，用于去重检测。

**成员变量**：

- `Window& hWnd` – 对话框所属的父窗口引用。Dialog 会在该窗口的客户区显示，并对其事件进行拦截（模态时阻止 hWnd 处理事件）。通过引用可以在需要时调用父窗口的方法，例如 addDialog/removeDialog (Reference to the parent Window on which this dialog is displayed. The dialog is drawn within the parent window’s context. In modal mode, it uses this reference to block input to the window. The reference is also used to register/unregister the dialog in the window’s internal dialog list).
- `int textWidth, textHeight` – 计算得到的对话框消息文本总尺寸（像素）。根据 `message` 内容和预设最大宽度自动换行得出 (The width and height (in pixels) of the message text area after word-wrapping. This is used to size the dialog appropriately).
- `int buttonNum` – 底部功能按钮数量。由 MessageBoxType 决定，例如 OKCancel 类型时为2 (Number of action buttons in the dialog (excluding the close 'X' button). Determined by the MessageBoxType; e.g., OKCancel yields 2).
- `StellarX::MessageBoxType type` – 当前对话框类型。决定对话框包含哪些功能按钮组合 (The type of the dialog, determining which action buttons are present, e.g., OK, YesNo, etc.).
- `std::string titleText` – 对话框标题栏文本。默认 "提示" (Default is "提示", meaning "Prompt" or "Notice").
- `std::unique_ptr<Label> title` – 标题文本的 Label 控件指针。对话框使用一个 Label 来显示标题文字，通常放置在标题栏中央 (Pointer to a Label control that displays the dialog’s title in the title bar area).
- `std::string message` – 对话框主要消息内容文本。由构造函数传入 (The main message text of the dialog to be displayed in the body).
- `std::vector<std::string> lines` – 分割后的消息文本行列表。根据对话框宽度将 message 拆分成若干行存储，用于在 draw 时逐行绘制 (The message text split into multiple lines to fit within the dialog’s width. This is prepared during initialization so that drawing can simply iterate over lines).
- `bool needsInitialization` – 是否需要初始化的标志。初始为 true，表示需要调用 initButtons等初始化UI元素。在第一次 Show 时完成初始化后设为 false (Flag indicating if the dialog’s UI elements (buttons, etc.) still need to be initialized. True initially; after the first show (where `initButtons()`, etc., are called), set to false so as not to duplicate initialization).
- `bool close` – 对话框关闭状态标志。为 true 时表示对话框应关闭，事件循环会检查此值退出 (Likely used to signal that the dialog should close. When set to true (e.g., after a button click), the modal loop will break and the dialog will begin teardown).
- `bool modal` – 是否模态的标志。构造时根据传入参数设置，影响 Show 和 handleEvent 行为 (Indicates if the dialog is modal. Set in constructor based on parameter, determines how Show/handleEvent operate).
- `COLORREF backgroundColor` – 对话框背景颜色，默认 RGB(240,240,240) 浅灰.
- `COLORREF borderColor` – 对话框边框颜色，默认 RGB(100,100,100) 深灰.
- `COLORREF buttonTrueColor, buttonFalseColor, buttonHoverColor` – 对话框底部功能按钮的配色方案。这些颜色用于创建对话框按钮时指定其不同状态颜色：buttonTrueColor（按钮按下时颜色，默认为略深的灰粉色），buttonFalseColor（按钮正常状态颜色，默认为浅灰），buttonHoverColor（按钮悬停颜色，默认为稍亮灰） (Color scheme for the dialog’s action buttons. These are used when creating the Button controls for OK/Cancel/etc: buttonTrueColor for pressed state (default a muted red/pinkish tone, RGB(211,190,190)), buttonFalseColor for unpressed state (default light gray, RGB(215,215,215)), buttonHoverColor for hover state (default slightly brighter gray, RGB(224,224,224))).
- `Button* closeButton` – 对话框右上角的关闭按钮（“X”）指针。点击该按钮相当于取消/关闭对话框 (Pointer to the top-right close Button (with an “X”). Clicking it will close the dialog, typically equivalent to Cancel).
- `StellarX::MessageBoxResult result` – 对话框的结果枚举值。初始默认为 Cancel（假定取消），当用户点击某个功能按钮时被设为对应的结果 (Stores the outcome of the dialog (which button was pressed). Initialized to Cancel by default, and set to the corresponding value when a button is clicked).
- `bool shouldClose` – 标志对话框是否即将关闭。可能与 close 类似，但用于区分真正销毁时机 (Flag indicating the dialog should be closed. This might be used to initiate the shutdown sequence slightly before actual removal).
- `bool isCleaning` – 标志对话框是否正在清理过程中。防止重复清理 (Flag indicating the dialog is currently in the cleanup process, to avoid re-entrance).
- `bool pendingCleanup` – 标志是否需要延迟清理（如等待回调完成）。非模态对话框关闭时可能先不立即销毁对象而设置此标志，稍后由 performDelayedCleanup 真正删除 (Flag indicating that cleanup (object deletion) is pending. For modeless dialogs, after close, the object might not be destroyed immediately to allow the result callback to execute; instead pendingCleanup is set and actual deletion is done via performDelayedCleanup when safe).
- `StellarX::ControlText textStyle` – 对话框文本样式。用于对话框消息文本的字体和颜色 (Font and style for the dialog’s message text). Dialog 通常使用系统默认字体、黑色文本 (Likely default to a standard font and black color).
- **内部方法** *(private, for initialization and layout)*:
  - `void initButtons()` – 根据当前 `type` 创建底部功能按钮（如确定、取消等）并布局它们。同时计算 buttonNum 等 (Creates the action buttons (OK, Cancel, etc.) based on the dialog’s type and positions them appropriately at the bottom. Updates `buttonNum` and likely sizes).
  - `void initCloseButton()` – 创建并设置关闭按钮 (Creates the close “X” button and positions it at the top-right of the dialog).
  - `void initTitle()` – 创建并初始化标题 Label 控件 (Initializes the title Label with the current titleText and positions it in the title bar).
  - `void splitMessageLines()` – 将 `message` 按一定宽度拆分成多行填充到 `lines` 向量中 (Performs word wrapping: splits the message string into multiple lines that fit within the dialog’s width and stores them in `lines`).
  - `void getTextSize()` – 计算消息文本内容在当前字体下需要的宽高 (Calculates the pixel width and height required for the message text given the current font and line breaks).
  - `void initDialogSize()` – 根据内容（文本尺寸和按钮区域）计算对话框的总尺寸并调整自身宽高 (Sets the overall dialog width and height based on the text dimensions and space needed for buttons and title, including margins).
  - `void addControl(std::unique_ptr<Control> control)` – 将给定控件加入对话框的子控件列表 (Adds a new child control (Button/Label) to the Dialog’s Canvas children list and position relative to dialog).
  - `void clearControls()` – 清除并删除对话框内的所有子控件 (Removes all child controls from the dialog, deleting the buttons and labels. Used when resetting or destroying).
  - `std::unique_ptr<Button> createDialogButton(int x, int y, const std::string& text)` – 工具函数，创建一个在对话框底部指定位置的标准按钮。用于 OK/Cancel 等按钮的批量创建 (Utility to create a standard-sized dialog action button at given coordinates with given label text. Used by initButtons to create each required button).
  - `void requestRepaint(Control* parent) override` – 重载自 Control：Dialog 特殊实现覆盖 Canvas 默认行为。如果 parent == this，则仅执行 Canvas::requestRepaint 逻辑；否则调用父类 onRequestRepaintAsRoot() (The Dialog’s override for requestRepaint likely ensures that if a repaint is requested, it properly handles redrawing of itself and perhaps background. It might also intercept calls to avoid propagating beyond the dialog if not needed. The snippet suggests it overrides but possibly just uses default Canvas behavior except in special cases).

**依赖**：Dialog 依赖 **Window** 类来实现模态效果：模态对话框弹出时，会通过 Window 拦截消息循环或标记 `dialogClose` 等使主窗口等待。Dialog 内部组合了 **Label**（标题和消息文本）和 **Button**（操作按钮和关闭按钮）等多个控件，因此其正确运行依赖这些子控件的行为。Dialog 还使用 **MessageBoxType**/**MessageBoxResult** 枚举定义按钮方案和结果值。作为 Canvas 子类，Dialog 继承了绘制和背景处理机制，并在此基础上扩展 (The Dialog works closely with the **Window** class to implement modality: when shown modally, it likely uses Window’s event loop control (like Window::runEventLoop or flags) to block input to other controls. It composes multiple **Label** and **Button** controls and therefore relies on them for display and click handling. It uses **MessageBoxType** to decide what Buttons to create and **MessageBoxResult** to report outcomes. Being a subclass of Canvas, it benefits from Canvas's drawing logic and snapshot management to handle covering the underlying window content smoothly).

### MessageBox（消息框工厂类）

**类名**：`StellarX::MessageBox`
 **继承**：无，所有成员为静态方法 (This is a static utility class within namespace StellarX; it is not a Control and cannot be instantiated).

**简要说明**：消息框对话框的工厂类，提供简化的静态方法来创建和显示 **Dialog** 对象。开发者无需直接操作 Dialog 类，通过 MessageBox 提供的接口即可快捷地显示模态或非模态对话框。（*A factory class for message boxes that provides convenient static methods to create and display **Dialog** instances. It spares developers from dealing with the Dialog class directly by offering easy functions to show modal or modeless dialogs.*）

**公共静态方法**：

- `static MessageBoxResult showModal(Window& wnd, const std::string& text, const std::string& caption = "提示", MessageBoxType type = MessageBoxType::OK)` – 显示一个模态消息框。在窗口 `wnd` 中央弹出一个对话框，显示内容 `text` 和标题 `caption`（默认“提示”），按钮组合由 `type` 指定（默认单个 OK）。该调用会阻塞当前线程，直到用户点击任意按钮关闭对话框。返回值为用户选择的结果（MessageBoxResult 枚举，例如 MessageBoxResult::OK 或 MessageBoxResult::Cancel 等) (Displays a modal message box on the given window `wnd`. It creates a Dialog with the provided message text and caption (default "提示"), with buttons as specified by `type` (default OK only). This call blocks execution until the user closes the dialog by clicking a button. It returns the result as a MessageBoxResult value corresponding to which button was pressed). 典型用法如：`MessageBox::showModal(mainWindow, "操作完成", "通知", MessageBoxType::OKCancel)` 阻塞等待用户点“确定”或“取消” (Example: calling showModal to present an "Operation completed" message with OK and Cancel buttons, and wait for the user to choose).

- `static void showAsync(Window& wnd, const std::string& text, const std::string& caption = "提示", MessageBoxType type = MessageBoxType::OK, std::function<void(MessageBoxResult)> onResult = nullptr)` – 显示一个非模态（异步）消息框。参数同上，但该函数**不会阻塞**。它立即返回，消息框以非模态方式显示在窗口上，用户可以继续与主窗口交互。当用户点击按钮关闭消息框时，如果提供了 `onResult` 回调函数，则异步调用之并传入结果枚举值 (Displays a modeless (non-blocking) message box. Parameters are similar to showModal, but this function returns immediately, allowing the main window to continue running. The message box dialog is shown and will close when the user clicks a button, at which point if an `onResult` callback is provided, it will be invoked with the MessageBoxResult of the user’s choice). 这种用法适合不想打断用户操作的提示，例如:

  ```
  MessageBox::showAsync(mainWindow, "下载已在后台进行", "信息", MessageBoxType::OK,  
                        [](MessageBoxResult res){ /* 回调处理 */ });  
  ```

  (This is useful for notifications that shouldn’t halt the program flow. For example, showing a "Download started in background" info box with just an OK button and using the callback to perform any follow-up if needed).

**实现说明**: MessageBox 的实现会根据提供的参数创建一个 **Dialog** 对象并显示。对于 showModal，它创建 Dialog（模态=true）然后调用 Dialog::Show()，等待获取结果后返回 (In implementation, showModal likely constructs a Dialog with modal=true, then calls its Show() method, and finally returns the Dialog’s result). 对于 showAsync，它创建 Dialog（模态=false），设置 Dialog::resultCallback 为传入的 onResult，然后调用 Dialog::Show() 后直接返回 (For showAsync, it constructs a Dialog with modal=false, sets its resultCallback to the provided function (if any), calls Show(), and returns immediately without waiting). MessageBox 工厂可能维护一个记录以避免同时弹出两个内容相同的非模态对话框（如通过 Dialog::GetCaption()/GetText() 检查去重） (The MessageBox might also ensure no duplicate dialogs are opened for the same text/caption pair, possibly by checking existing open dialogs in Window’s list using Dialog::GetCaption()/GetText()).

**依赖**：MessageBox 工厂严重依赖 **Dialog** 类实现实际对话框，以及 **Window** 提供的容器支持。它只是封装了 Dialog 的构造和显示逻辑 (The MessageBox relies on the **Dialog** class to do the actual work of showing a message box, and on **Window** for placement. It is essentially a thin wrapper that simplifies using Dialog).

## 顶层容器（Top-Level Containers）

顶层容器类在框架中充当应用程序窗口或主要界面容器，能够容纳其他控件。这里包括 Window 和 Canvas 两个类：Window 代表应用主窗口，负责和操作系统交互；Canvas 则是一个通用的控件容器，可以放置在窗口内或其他容器内用于分组子控件。 *(The top-level container classes serve as the application window or primary interface containers that hold other controls. This includes the Window class (representing the main application window, handling OS-level interactions) and the Canvas class (a general-purpose container control that can be placed in windows or other containers to group child controls).)*

### Window（窗口容器）

**类名**：`Window`
 **继承**：无直接继承自 Control，但 Window 包含并管理 Control 子控件的列表 (Window is not a Control itself, but it aggregates controls. It does not inherit from Control, it's an independent class that manages a collection of Control objects).

**简要说明**：应用程序主窗口类，封装 Win32 窗口并提供容器功能，能够添加控件并处理窗口事件循环。Window 结合 EasyX 图形库，实现一个可调整大小、不闪烁的窗口，支持锚定布局和双缓冲绘图，以容纳 GUI 控件。（*The main application window class. It wraps a Win32 window (via EasyX) and provides a container for controls. The Window class is designed to be resizable and minimize flicker, supporting anchored layouts and double-buffered drawing, and serves as the root container for GUI controls.*）

**关键实现**: Window 通过子类化窗口过程（WndProcThunk）拦截窗口消息，如大小调整（WM_SIZING/WM_SIZE）、最小尺寸约束等。它将窗口客户区大小变化解耦为 pending 尺寸记录和统一重绘信号，从而在拖动调整窗口大小时避免频繁重绘抖动。（*Key points: The Window subclasses the window procedure (using a thunk) to intercept important messages like WM_SIZING/WM_SIZE. It clamps minimum client size, defers resize handling by recording pending width/height and uses a flag (needResizeDirty) to trigger a one-time redraw at the end of a resize, preventing flicker during continuous resizing.*）

**公共方法**：

- `Window(int width, int height, int mode)` – 构造函数，创建指定客户区宽高的窗口。`mode` 为 EasyX 初始化模式，如 EX_SHOWCONSOLE（显示控制台）或 EX_TOPMOST（置顶）等。构造函数仅初始化成员，实际创建窗口和子类化窗口过程在 draw() 中完成 (Initializes a Window with given client area width and height, and an EasyX mode flag. It does not immediately open the OS window; it just sets up internal state. Actual window creation and subclassing of WndProc happen in draw()).
- `Window(int width, int height, int mode, COLORREF bkColor)` – 重载构造，额外指定背景颜色。
- `Window(int width, int height, int mode, COLORREF bkColor, std::string headline)` – 重载构造，额外指定窗口标题文本。
- `~Window()` – 析构函数，关闭窗口并清理 (Closes the window (likely via EasyX closegraph or similar) and destroys child controls).
- **绘制与事件循环**:
  - `void draw()` – 绘制窗口界面（无参数版本）。执行 EasyX 初始化（initgraph）创建窗口，设置必要的窗口扩展样式 WS_EX_COMPOSITED（useComposited 控制）减少闪烁。然后填充背景纯色（wBkcolor）或背景图（如果设置了 background image），并绘制所有已添加的控件 (This sets up the actual Win32 window if not done, possibly by calling EasyX’s initgraph using stored `width`, `height` and `windowMode`. It then draws the background: if a background image is loaded (background != nullptr), it draws that, otherwise fills a solid color (wBkcolor). After that, it iterates through the `controls` vector to draw each child control. It also likely draws any non-modal dialogs that are open (from `dialogs` vector) on top).
  - `void draw(std::string pImgFile)` – 绘制窗口界面（带背景图）。该重载加载指定路径的图像作为窗口背景（赋值给 background 指针），然后调用 draw() 主函数 (This variant sets the background image by loading from the provided file path (stores it in `background` image pointer and records `bkImageFile` path), then likely calls the regular draw() routine to render with this image).
  - `int runEventLoop()` – 运行窗口的主事件循环。实现为循环调用 EasyX 的 PeekMessage 获取消息，然后对所有控件调用其 handleEvent() 进行分发，并在必要时调用 draw 进行重绘 (Runs the main GUI loop. Likely uses a loop around `PeekMessage()` (non-blocking message fetch) to continuously handle events. For each message, it passes it to each control’s handleEvent (in reverse order perhaps for proper z-order), tracks if any control became dirty (needResizeDirty usage), and if so, triggers a redraw of all controls at the end of the loop iteration). 特殊处理 WM_SIZING 等消息：当用户拖动调整窗口大小时，临时冻结重绘（isSizing=true），只在拖动结束 WM_EXITSIZEMOVE 时统一更新 (It has logic to handle WM_SIZING by not immediately redrawing (set isSizing true), and only upon WM_SIZE at end (or WM_EXITSIZEMOVE) does it schedule one combined redraw using `needResizeDirty`). runEventLoop 返回值可能是窗口关闭时的退出代码 (The function returns an exit code when the window is closed, possibly 0).
- **背景与标题设置**:
  - `void setBkImage(std::string pImgFile)` – 更换窗口背景图像。加载指定文件为背景 (Loads the specified image file and sets it as the window’s background. After calling, it triggers a redraw of the window to display the new background).
  - `void setBkcolor(COLORREF c)` – 设置窗口背景纯色。将背景色改为 c，并立即触发一次重绘 (Sets the window background color to `c` and immediately triggers a full redraw of the window).
  - `void setHeadline(std::string headline)` – 设置窗口标题文本。更新内部 `headline` 字符串，并调用 Win32 API 更新窗口标题栏显示 (Changes the window’s title text. Updates the internal `headline` and likely calls SetWindowText on the underlying HWND to reflect the new title).
- **控件和对话框管理**:
  - `void addControl(std::unique_ptr<Control> control)` – 将一个新的控件添加到窗口。此控件将被纳入 Window 的 `controls` 列表，在下次 draw 时绘制，并在事件循环中接收事件 (Adds a Control to the window’s control list. The Window takes ownership via the unique_ptr. The new control’s coordinates are typically relative to the window’s client area. It will be drawn on next repaint and will participate in event handling). 对于某些控件，Window 在添加时可能根据当前窗口大小调用其 onWindowResize 以初始化布局 (Window might call the new control’s onWindowResize upon adding to adjust layout if anchors are used).
  - `void addDialog(std::unique_ptr<Control> dialog)` – 将一个非模态对话框控件添加到窗口。Window 将其存入 `dialogs` 列表，该列表专门用于对话框( Adds a non-modal dialog (which is a Control) to the window. It is stored in a separate `dialogs` vector. The window will draw these on top of other controls and give them priority in event handling). 这通常由 MessageBox::showAsync 内部调用，当创建 Dialog 后将其托管给 Window (This is typically called internally by MessageBox::showAsync after creating a Dialog, to ensure the window knows about the open dialog).
  - `bool hasNonModalDialogWithCaption(const std::string& caption, const std::string& message) const` – 检查是否存在某个标题和消息匹配的非模态对话框正在窗口中显示。用于避免重复弹出内容相同的对话框 (Checks the `dialogs` list to see if a dialog with the given caption and message text is currently open. This is used to prevent opening duplicate dialogs with identical content).
- **状态访问器**:
  - `HWND getHwnd() const` – 获取窗口的 Windows 窗口句柄。这允许调用底层 Win32 API，如需要 (Returns the Win32 window handle (HWND) of the EasyX-created window, in case direct Win32 calls are needed).
  - `int getWidth() const` / `int getHeight() const` – 获取窗口当前客户区宽度和高度。注意 Window 自身维护的 width/height 表示当前**有效**客户区尺寸，可能在交互拉伸过程中暂时保持旧值直到松开鼠标统一更新 (Returns the current effective client area width and height of the window. Note that the Window maintains `width`/`height` as the last fully applied size; during interactive resizing, it uses pendingW/H to store the changing values).
  - `std::string getHeadline() const` – 获取窗口标题文本.
  - `COLORREF getBkcolor() const` – 获取窗口背景色.
  - `IMAGE* getBkImage() const` – 获取窗口背景 IMAGE 对象指针（如果有）. 如果未设置背景图则返回 nullptr (Returns the pointer to the background image if one is set, otherwise nullptr).
  - `std::string getBkImageFile() const` – 获取当前背景图片文件路径（如果有）.
  - `std::vector<std::unique_ptr<Control>>& getControls()` – 获取对窗口普通控件列表的非 const 引用。允许遍历或操作已添加的控件 (Provides a reference to the vector of controls (not including dialogs), allowing iteration or manipulation of child controls if needed).
- **尺寸调整**:
  - `void pumpResizeIfNeeded()` – 检查并执行一次延迟的重绘以响应窗口尺寸变化。Window 内部在事件循环每次迭代结尾调用此函数：如果 `needResizeDirty` 标志为 true，则调用一次 draw() 完成所有控件的调整绘制，然后将 `needResizeDirty` 复位 (Executes a deferred full redraw if a resize event has occurred. Inside the event loop, after processing all messages, Window calls this: if `needResizeDirty` is true (meaning a resize has happened and controls might need repositioning), it calls draw() to redraw everything and resets `needResizeDirty`). 这样可以把连续的 WM_SIZE 处理压缩为单次绘制 (This condenses potentially many WM_SIZE events into a single redraw, improving efficiency and reducing flicker).
  - `void scheduleResizeFromModal(int w, int h)` – 当模态对话框关闭时调用，将 pendingW/H 更新为给定值并标记 `needResizeDirty`。这用于在模态对话框可能改变父窗口大小（比如隐藏滚动条之类）后，通知窗口稍后调整布局 (Schedules a resize handling after a modal dialog closes and potentially affected the window size or layout. It sets the pendingW, pendingH (maybe to w,h) and marks needResizeDirty so that pumpResizeIfNeeded will redraw with updated layout).

**成员变量**：

- **尺寸状态**:
  - `int width, height` – 当前有效客户区宽度和高度。这是应用到控件布局的实际尺寸 (The current effective width and height of the client area, which have been applied to control layout).
  - `int localwidth, localheight` – 窗口创建时的初始基准宽度和高度。作为锚定布局计算参考 (The baseline width/height at window creation, used as reference for anchor layout scaling calculations).
  - `int pendingW, pendingH` – 待应用的新宽高。在窗口被拉伸 (WM_SIZING) 过程中，Window 会记录不断变化的尺寸到 pendingW/H，但暂不应用到控件；等到用户释放鼠标（最终 WM_SIZE）时，再将 pendingW/H 赋给 width/height 并触发统一重绘。(The width and height that are pending application. During interactive resizing, as WM_SIZING events provide new sizes, the Window stores them in pendingW/H without immediately adjusting controls. Only when resizing is done (WM_SIZE final) do these get applied to actual width/height with one unified redraw).
  - `int minClientW, minClientH` – 业务设定的最小客户区宽高。Window 初始化时可指定一个最小大小，用于在 WM_GETMINMAXINFO 消息处理中限制窗口不能缩得太小 (Minimum client area width and height set for the window (if any). The Window uses these to enforce a minimum window size by handling WM_GETMINMAXINFO or clamping WM_SIZING).
  - `int windowMode` – EasyX 窗口初始化模式标志。例如 EX_SHOWCONSOLE, EX_TOPMOST 等，通过 EasyX initgraph 用 (The EasyX window mode flags used on creation, such as whether to show a console or make the window topmost, etc.).
  - `bool needResizeDirty` – 统一收口重绘标志。为 true 表示窗口尺寸变动引起的重绘尚未处理，需要在事件循环空闲时统一执行 (Flag indicating that a resize occurred and a full redraw is needed. When true, the event loop will call draw once and then clear this flag. It's set during resizing events to avoid multiple intermediate draws).
  - `bool isSizing` – 是否处于拖拽调整大小的状态。当收到 WM_ENTERSIZEMOVE 消息（用户开始拖拽窗口边框）时设 true，在 WM_EXITSIZEMOVE（拖拽结束）时设 false。(Flag indicating if the user is currently resizing the window (dragging the window border). Set to true on WM_ENTERSIZEMOVE, false on WM_EXITSIZEMOVE. When true, Window may suppress immediate redraws to avoid jitter).
- **原生窗口和子类化**:
  - `HWND hWnd` – EasyX 创建的窗口句柄。Window 类在 draw() 中通过 EasyX 初始化拿到窗口句柄，并安装自定义窗口过程 (The Win32 window handle obtained after EasyX initialization. The Window uses it to subclass the window procedure).
  - `WNDPROC oldWndProc` – 保存的旧窗口过程指针。子类化时，用于在 Window 过程内调用原窗口过程处理未拦截的消息 (The original window procedure pointer saved when subclassing. The custom WndProcThunk will call this (via CallWindowProc) for messages it doesn’t handle).
  - `bool procHooked` – 标志窗口过程是否已子类化，避免重复钩挂.
  - `static LRESULT CALLBACK WndProcThunk(HWND h, UINT m, WPARAM w, LPARAM l)` – **静态窗口过程**，将 Win32 消息分发到特定 Window 实例的成员函数 (A static window procedure that acts as a trampoline to the Window instance’s message handler. It likely retrieves the Window* from a static map or from `GetWindowLongPtr(h,GWLP_USERDATA)` and calls an instance method). Window 在初始化时把该 Thunk 安装到 hWnd (During Window initialization, this function is set as the new window procedure for hWnd).
- **绘制相关**:
  - `bool useComposited` – 是否使用组合窗口扩展风格（WS_EX_COMPOSITED）。默认为 true。打开该选项可减少某些机器上的绘制闪烁（但可能引入一帧延迟）(Whether to use the WS_EX_COMPOSITED extended style for the window, which can reduce flicker by double buffering the entire window. Default true, though on some systems this might introduce a slight frame delay).
  - `std::string headline` – 窗口标题文本。将在窗口标题栏显示 (The text for the window’s title bar).
  - `COLORREF wBkcolor` – 窗口背景纯色。绘制窗口时作为填充底色 (The solid background color used if no background image is set. Default BLACK (0x000000)).
  - `IMAGE* background` – 背景图像指针。如果加载了背景图，则绘制窗口时优先绘制该图像覆盖整个客户区 (Pointer to an IMAGE used as the window’s background. If not null, draw() will tile or stretch/draw this image to cover the client area).
  - `std::string bkImageFile` – 当前背景图像文件路径。记录所加载背景图的位置 (Stores the filename of the currently loaded background image, if any, mainly for reference or reloading purposes).
- **控件/对话框容器**:
  - `std::vector<std::unique_ptr<Control>> controls` – 存放窗口上的普通控件列表。Window 的主要子控件容器。加入此列表的控件会在 draw() 时绘制、在事件循环中接收事件 (Holds all non-dialog child controls added to the window. These are drawn in the window’s draw() and have their events processed in the main loop).
  - `std::vector<std::unique_ptr<Control>> dialogs` – 存放窗口上当前打开的非模态对话框控件列表。这些对话框（Dialog 类实例）在 Window::draw() 中应绘制在普通控件之上，在事件处理时应优先处理 (Holds any modeless dialog controls currently open on this window. In draw(), these are drawn on top of regular controls, and in event handling, they get first chance to consume input, effectively overlaying modal-like behavior for their portion of the window).
- `bool dialogClose` – 项目内使用的标志，指示对话框关闭状态。当窗口存在模态对话框时，该标志或许用于中断主事件循环 (A flag used to indicate a dialog has closed; possibly used internally to break out of a modal loop or signal something related to dialogs. It’s usage is not fully clear but likely interacts with the modal logic).

**依赖**：Window 直接使用 Win32 API（通过 EasyX）来创建窗口和处理消息。因此，它需要 Windows 平台支持。Window 包含框架内所有控件，是 GUI 的根容器：它依赖 **Control** 及其子类（Button 等）的存在，以将它们加入显示列表并调用其绘制和事件处理。Window 也依赖 **Dialog** 用于处理模态对话框逻辑、依赖 **MessageBoxType/Result** 枚举来处理最小化尺寸、结果返回等 (The Window class wraps the OS window and heavily interacts with the Win32 API via EasyX. It serves as the integration point where Control objects meet OS events. It depends on all control classes for populating its interface, and on **Dialog** for handling modal scenarios. It uses enumerated types from CoreTypes (LayoutMode, Anchor) for resizing logic as well). Window 与操作系统的交互如消息循环、最小化最大化等，均在其内部实现，对于开发者而言主要使用其 addControl、draw、runEventLoop 等接口 (All OS-level interactions (minimize, close, etc.) are handled internally by Window’s subclassed WndProc. For the developer, the main usage is adding controls, calling draw to initialize, and runEventLoop to start the GUI loop).

### Canvas（画布容器）

**类名**：`Canvas`
 **继承**：继承自 Control (`class Canvas : public Control`)。Canvas 是 Control 的一个具体实现，主要用于作为容器承载其他控件。

**简要说明**：通用画布控件，可充当容器容纳子控件，并支持绘制背景形状和处理子控件布局。Canvas 通常用作复合控件的基础（如 Dialog 和 TabControl 都继承自 Canvas），也可独立用于在窗口中分组一组控件。（*A general-purpose canvas control that can contain child controls and draw a background shape. Canvas often serves as a base for composite controls (e.g., Dialog and TabControl inherit Canvas), and can also be used on its own to group a set of controls in a window.*）

**功能**:  Canvas 继承自 Control，提供除基本属性外的重要扩展：维护一个子控件列表（controls），并在绘制时能够绘制自身背景（可设置形状、填充模式、颜色等）然后递归绘制其包含的子控件。它也重载了部分 Control 方法以在自身移动或可见性变化时调整子控件。例如，当 Canvas 移动时，会相应偏移所有子控件的位置；当 Canvas 被隐藏时，自动隐藏子控件。（*Canvas extends Control by maintaining a list of child controls and by drawing a background (with configurable shape, fill mode, colors) before drawing its children. It overrides certain Control methods to handle child elements: for instance, moving a Canvas shifts all its children’s positions accordingly; hiding a Canvas also hides its children.*）

**公共方法**：

- `Canvas()` – 默认构造函数，将 Canvas 初始放置在 (0,0) 且尺寸为 100×100。调用 Control 基类构造，并设置 id 为 "Canvas" (Initializes a Canvas at origin with a default size of 100 by 100. Sets the control’s id to "Canvas").
- `Canvas(int x, int y, int width, int height)` – 构造函数，在指定坐标创建给定尺寸的 Canvas。设置 id 为 "Canvas"。初始时背景形状默认矩形，填充模式、颜色等使用默认值 (Creates a Canvas at (x,y) with specified width and height. Id set to "Canvas". By default, its shape is rectangle with default fill mode and colors).
- `void setX(int x) override` / `void setY(int y) override` – 重载移动 Canvas 的方法。Canvas 实现除更新自身位置外，会遍历所有子控件，将每个子控件的绝对坐标随之偏移（新的 X = 子控件原 localX + Canvas 新的 x 等）并调用子控件的 `onWindowResize()` 让其自行处理 (When moving the Canvas, it not only updates its own x (or y) but also adjusts all child controls: it calls each child’s `onWindowResize()` and sets the child’s X coordinate to its stored localX plus the new Canvas X. This effectively keeps children at the same relative position inside the Canvas even as the Canvas moves. Similarly for Y. Also marks itself dirty for redraw).
- `void setWidth(int width) override` / `void setHeight(int height) override` – 默认继承 Control 的行为，只调整自身尺寸并标记 dirty。Canvas 没有特别重载这两个，但在 Canvas 内部，调整尺寸通常伴随需要重新布局子控件，此时 Canvas 的 onWindowResize 可能会被显式调用 (Canvas uses the base implementation for setWidth/Height: update and mark dirty. However, resizing a Canvas usually would imply repositioning children if anchors are set; such logic might rely on explicit call to onWindowResize to recalculation local positions if needed).
- `void clearAllControls()` – 清空 Canvas 容器中的所有子控件。实现为简单调用 `controls.clear()` 删除子控件智能指针列表 (Removes all child controls from the Canvas’s vector, destroying them. After calling this, the Canvas will have no child controls).
- `void draw() override` – 绘制 Canvas 及其子控件。实现逻辑：
  1. 若 Canvas 自身不是 dirty 或处于隐藏状态（show=false），则跳过自身重绘，仅检查并绘制特殊控件子类如 Table 等需要持续绘制 (If the Canvas itself is not marked dirty or is not visible, it will skip redrawing its background. However, it still iterates children: if a child is of a special type (like Table) that manages its own content and may need continuous drawing, it calls that child’s draw anyway to update dynamic content).
  2. 如果需要重绘：先调用 `saveStyle()` 保存当前全局绘图状态。然后设置绘图颜色和样式：边框色 = `canvasBorderClor`，填充色 = `canvasBkClor`（前提 FillMode 不是 Null），填充模式 = `canvasFillMode`，线型 = `canvasLineStyle`，线宽 = `canvaslinewidth`。
  3. 绘制 Canvas 背景形状：根据 `shape` 不同，调用 EasyX 不同函数绘制矩形、圆角矩形、圆形或椭圆等。如果 shape 带 B_ 前缀表示无边框版本，则使用 solidrectangle/solidroundrect 等无边框绘制，否则用 fillrectangle/fillroundrect 绘制带边框。圆角尺寸由 rouRectangleSize 提供 (Draw the canvas background shape: depending on `shape`, it chooses the appropriate function: e.g., fillrectangle for a filled rectangle with border, solidrectangle for borderless, fillroundrect/solidroundrect for rounded corners with/without border, etc. The rouRectangle struct provides the corner ellipse size for roundrect shapes).
  4. 依次绘制所有子控件：遍历 `controls` 列表，对每个子控件设置 dirty=true（确保其自身 draw 刷新），然后调用其 `draw()`。这样保证当 Canvas 重绘时，所有子控件都重绘 (Iterates through each child control, marks it dirty (setDirty(true)), and calls its draw() to render it).
  5. 调用 `restoreStyle()` 恢复全局绘图状态，标记 Canvas 自身 dirty=false 表示绘制已完成 (Finally restores the saved drawing state and marks itself not dirty).
- `bool handleEvent(const ExMessage& msg) override` – 处理事件，将事件分发给子控件。实现为从后向前遍历子控件列表（Z序从顶开始）调用每个子控件的 handleEvent。如有子控件消费事件则记录 firstConsumer 并设置 consumed=true。遍历完后，如果有控件消费且该事件不属于“噪音”消息（如 WM_MOUSEMOVE），则打印调试日志 (It iterates over children in reverse order (so the topmost drawn control gets event first) calling each child’s handleEvent(msg). If a child returns true (event consumed) and firstConsumer is not yet set, it stores that pointer. After checking all, if an event was consumed and is not a “noisy” event like WM_MOUSEMOVE, it logs a debug message indicating Canvas consumed the event via a child).
   然后，如果发现任意子控件 `isDirty()==true` 则设 anyDirty=true。若 anyDirty 且消息非噪音，则打印日志并调用 `requestRepaint(parent)` 通知父容器需要重绘 (It also checks if any child became dirty during event handling; if yes, for non-mouse-move events it logs and calls `requestRepaint(parent)` to ask the parent to repaint this Canvas). 最后返回 consumed (Finally returns whether any child consumed the event).
- 管理子控件:
  - `void addControl(std::unique_ptr<Control> control)` – 将一个子控件添加到 Canvas。实现：把 control 的 local 坐标 (getLocalX/Y) 转换为全局坐标，即加上 Canvas 自身的 x,y；将 control.parent 设为 this；记录日志说明添加 (Logs the addition with both local and computed global coords)。然后将 control 转移到 Canvas.controls 列表末尾，并将 Canvas 标记 dirty=true (Then moves the control into the Canvas’s controls list and marks the Canvas dirty). 这样下次 Canvas 重绘时会绘制该子控件 (The new control will be rendered on the next draw call of the Canvas).
  - `void setShape(StellarX::ControlShape shape)` – 设置 Canvas 背景形状。允许在矩形、圆角矩形、圆形、椭圆几种外观之间切换。只接受枚举定义的四边形/圆形形状 (Sets the shape used to draw the Canvas background (rectangle, round-rectangle, circle, ellipse with or without border). Only the shapes defined in ControlShape are accepted). 实现中，对传入 shape 进行 switch，但实际上任何受支持值都会执行 (The implementation likely just switches to the provided shape if valid and sets dirty true).
  - `void setCanvasfillMode(StellarX::FillMode mode)` – 设置 Canvas 背景填充模式（注意代码拼写大小写，此应为 setCanvasFillMode）。(Sets the fill mode for the Canvas background). 例如可将 Canvas 背景设为图案填充等。实现：赋值 canvasFillMode = mode 并标记 dirty (It assigns the new fill mode and marks the Canvas dirty).
  - `void setBorderColor(COLORREF color)` – 设置 Canvas 边框颜色。实现：canvasBorderClor = color，dirty=true.
  - `void setCanvasBkColor(COLORREF color)` – 设置 Canvas 背景填充颜色。实现：canvasBkClor = color，dirty=true.
  - `void setCanvasLineStyle(StellarX::LineStyle style)` – 设置 Canvas 边框线型样式。实现：canvasLineStyle = style，dirty=true.
  - `void setLinewidth(int width)` – 设置 Canvas 边框线宽。实现：canvaslinewidth = width，dirty=true.
- `void setIsVisible(bool visible) override` – 重载 Control::setIsVisible。Canvas 将自身 show 标志设为 visible 并置 dirty=true。与 Control 默认实现相比没有额外逻辑 (Canvas does not add extra logic here beyond base: it sets its own `show` to the given value and marks itself dirty. It does not explicitly hide children here – however, when Canvas’s draw runs and show=false, children won’t be drawn and events won’t be passed if IsVisible returns false). 注：TabControl 等子类会进一步重载实现子控件可见性同步 (Note: As seen, TabControl overrides setIsVisible to handle children, but base Canvas doesn’t).
- `void setDirty(bool dirty) override` – 重载 Control::setDirty。实现：设置自身 dirty 标志，并对所有子控件也递归设置 dirty= true。这确保当 Canvas 需要重绘时，其包含的子控件也会重绘 (Marks the canvas and all its children as dirty when dirty=true is set. This ensures a full redraw of all content. If dirty=false, it similarly clears the dirty flag on children).

**成员变量**：

- `std::vector< std::unique_ptr<Control> > controls` – Canvas 的子控件列表 (The container of child controls within this Canvas). 这些控件的坐标是**全局**坐标（已经加上 Canvas.x,y），Canvas 同时可能存储子控件的局部坐标用于布局 (Children are stored with their absolute positions already offset by Canvas’s position. The Canvas itself may not separately store local coords for them, as each Control already knows its local coordinates relative to parent from when it was added).
- `StellarX::ControlShape shape` – Canvas 背景形状类型 (The shape used for this Canvas’s background. Default likely RECTANGLE).
- `StellarX::FillMode canvasFillMode` – Canvas 背景填充模式 (Fill mode for the background; default Solid).
- `StellarX::LineStyle canvasLineStyle` – Canvas 边框线型 (Border line style; default Solid).
- `int canvaslinewidth` – Canvas 边框线宽度（像素）(Border line width; default maybe 1).
- `COLORREF canvasBorderClor` – Canvas 边框颜色 (Border color; default black RGB(0,0,0)).
- `COLORREF canvasBkClor` – Canvas 背景填充颜色 (Background fill color; default white RGB(255,255,255)).

*(默认值参考 TABLE_DEFAULT_BG_COLOR/DEFAULT_BORDER_COLOR 等相似定义推测)*

**依赖**：Canvas 作为 Control 子类，不直接依赖 OS 资源，但它依赖 **ControlShape**、**FillMode**、**LineStyle** 等 CoreTypes 定义来渲染外观。Canvas 常被其他控件继承或组合使用，例如 **TabControl** 和 **Dialog** 就利用 Canvas 来实现多控件布局和背景绘制功能 (Canvas relies on the core enums for shapes and styles. It is often used as a base or a component in composite controls: e.g., **TabControl** inherits Canvas to manage pages, **Dialog** inherits Canvas to manage dialog content. Thus, Canvas forms a foundational block for any container-like control in the framework). 开发者可直接使用 Canvas 来划分界面区域或自定义绘制形状和放置子控件 (A developer might use a Canvas directly in a Window to group controls or draw a colored panel background). Canvas 无窗口句柄，不处理独立消息循环，其事件处理和重绘完全依赖父 Window 传递 (Canvas doesn’t have its own window handle; it relies on the parent Window to dispatch events and refresh calls to it, just like any other Control).

## 工具类（Utility Classes）

工具类提供辅助功能，非 GUI 控件但为框架的开发和调试提供支持。例如日志记录类 SxLog 用于输出调试信息。下面对框架的日志系统做简要说明： *(Utility classes offer supporting functionality outside of the UI controls, such as logging or debug output. For instance, the SxLog logging system is used for debug and information logging. Below is an overview of the framework’s logging utility:)*

### 日志系统 SxLog（Logging Utility）

**类名**：`StellarX::SxLog` *(实际上由多个类和宏组成，包括 SxLogger, FileSink, SxLogLine, SxLogScope 等；对外主要以宏接口使用)* (Implemented by several classes like SxLogger, FileSink, SxLogLine, SxLogScope, but primarily exposed via macros).

**简要说明**：StellarX 框架的日志记录工具，支持中英双语日志输出、日志分级过滤、文件输出及滚动等功能。通过一组宏（如 `SX_LOGD`, `SX_LOGI`, `SX_LOGW`, `SX_LOGE` 分别用于调试、信息、警告、错误级别）和辅助宏 `SX_T`（将中英文字符串选择输出）实现简洁的日志记录接口。（*The logging system for the StellarX framework, providing bilingual log output, log level filtering, and file output with rotation. It is accessed via a set of macros (e.g., `SX_LOGD`, `SX_LOGI`, `SX_LOGW`, `SX_LOGE` for debug, info, warning, error levels respectively) and a helper macro `SX_T` (for bilingual text selection) to offer a simple logging interface.*）

**使用方法**:
 在代码中，可通过例如：

```
SX_LOGD("Event") << SX_T("Canvas 消耗消息: ","Canvas consumed: msg=") << msg.message;  
```

将会打印一条调试日志。在日志输出中，`"Event"` 为日志标签，SX_T 宏包含了中文和英文两段字符串，日志系统会根据当前语言设置选取相应语言输出。以上示例最终可能输出：“Canvas 消耗消息: msg=0x0201 子控件 id=Button1”（当语言设为中文）或 “Canvas consumed: msg=0x0201 by child id=Button1”（设为英文）(In practice, using the macros as shown will produce log lines with a tag and bilingual text. The system chooses Chinese or English text in SX_T based on the current language setting. For example, the above might log “Canvas 消耗消息: msg=...” in Chinese mode, or “Canvas consumed: msg=...” in English mode).

**日志级别**: SxLog 定义了日志级别枚举 SxLogLevel（例如 Debug, Info, Warn, Error 等）。通过全局日志器 SxLogger，可以设置最低输出级别。当使用 `SX_LOGx` 宏记录时，低于当前最低级别的日志不会输出 (The SxLog system has log levels (likely Debug=0, Info=1, etc.). The global logger SxLogger has a `setMinLevel(SxLogLevel level)` function to set the minimum level of messages to actually output. Logging calls via `SX_LOGx` macros with a level below this threshold are no-ops).

**语言控制**: 默认情况下日志语言为中文（ZhCN）。可以通过 `SxLogger::setLanguage(SxLogLanguage lang)` 切换语言，例如设为 EnUS 输出英文日志 (By default, the log language is Chinese (ZhCN). The language can be changed by calling `SxLogger::setLanguage(SxLogLanguage)` to switch to English (EnUS) or other supported languages. Only the text provided via SX_T macro is affected by this setting).

**输出方式**: 日志既可以输出到调试控制台，也可以输出到文件。通过 `SxLogger::enableFile(const std::string& path, bool append, size_t rotateBytes)` 可以启用日志文件输出。参数 path 为文件路径，append 指定是否追加模式写入，rotateBytes 则如果>0表示启用文件滚动，当文件达到指定大小字节时自动改名备份并开启新文件 (The log can be configured to write to a file by calling `SxLogger::enableFile(path, append, rotateBytes)`. If `rotateBytes` is set (non-zero), the logger will automatically rotate (rename and start a new log file) when the current file exceeds that size).

**实现细节**:

- 日志系统由以下组件构成：
  - **FileSink**：负责文件输出的类，封装文件打开、写入、关闭及文件滚动逻辑。
  - **SxLogger**：日志核心单例类（Get() 获取实例），管理日志级别、语言和输出目的地。SxLogger 提供 `shouldLog(level)` 判断是否应输出某级别日志，`logLine(level, tag, message)` 统一将一行日志送往所有目的（控制台、文件）。
  - **SxLogLine**：使用 RAII 的技巧实现流式日志，将 `SX_LOGx` 宏展开后生成的对象在析构时自动调用 SxLogger 提交日志。这保证了一条日志完整输出在一行结束。
  - **SxLogScope**：用于计算一段作用域执行时间的辅助类。在构造时记录时间戳，析构时计算耗时并输出日志，可用于性能分析。
- **日志宏**:
  - `SX_LOGD(tag)`, `SX_LOGI(tag)`, `SX_LOGW(tag)`, `SX_LOGE(tag)` 分别创建一个 SxLogLine 对象用于调试(D)、信息(I)、警告(W)、错误(E)级别日志。参数 `tag` 是短字符串标签，用于标识日志分类。后续可以用流插入操作符 `<<` 将消息内容流入日志对象。每条日志宏调用最终在一行输出，并以换行结束 (These macros create an SxLogLine internally for the specified level with the given tag, then the code uses `<<` to append message parts. When the statement ends, the SxLogLine’s destructor is invoked, which sends the accumulated text to SxLogger to output with a newline).
  - `SX_T(chinese, english)` 宏用于处理内嵌在日志中的中英文双语字符串。运行时根据当前语言，只选择其中一个串输出。例如 `SX_T("失败","Failure")` 若语言为中文则产出 "失败"，英文则产出 "Failure" (The SX_T macro is a translation helper: it takes a Chinese string and an English string, and at runtime includes only the one matching the current language setting in the log. It allows log messages to be bilingual without duplicating code).

**使用示例**:

```
SX_LOGI("Init") << SX_T("窗口创建成功，大小=","Window created, size=")  
               << wnd.getWidth() << "x" << wnd.getHeight();  
```

如果当前日志级别 <= Info，则将输出类似：“窗口创建成功，大小=800x600” 或 “Window created, size=800x600”。否则不会输出任何信息 (If the current log level is Info or lower, this will output the message in the appropriate language. If the level is higher (e.g., Warning), nothing is output).

**依赖**：日志系统与 GUI 框架其他部分松耦合。它使用 C++11 线程安全特性（如 `std::call_once` 设置控制台 CodePage, `std::mutex` 来序列化日志输出）。日志宏在 GUI 控件代码中被调用以记录调试信息，但它本身不依赖 GUI 类。**EasyX** 控制台输出在 Windows 默认是 GBK，所以 SxLogger 在初始化时尝试设置控制台 CodePage 为 936 (GBK) 以正确显示中文日志(The logging system is mostly self-contained, using standard C++ for file and console operations. In the context of GUI, it’s used by controls for debug output but does not interact with GUI components. Notably, it sets the Windows console code page to 936 (GBK) once to ensure Chinese characters are properly displayed on the console, as noted in the comments).

通过 SxLog 工具类，开发者可以方便地输出框架内部运行状况信息，这对调试 GUI 行为、事件处理流程等非常有帮助。（*The SxLog utility enables developers to conveniently output internal state and flow information from the framework, which is invaluable for debugging GUI behavior and event handling.*）