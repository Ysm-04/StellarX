# 中文 API 文档

[English API Documentation](English API Documentation.md)

下面是 **StellarX GUI 框架** 各主要类和函数的 API 文档。每个类的用途、接口、参数和返回值等详细信息如下：

### Control 类 (抽象基类)

**描述：** `Control` 是所有控件的抽象基类，定义了通用的属性和接口，包括位置、尺寸、重绘标记等。它提供了绘图状态保存与恢复机制，确保控件绘制不影响全局状态。`Control` 本身不能直接实例化。

- **主要属性：**
  - `x, y`：控件左上角坐标。
  - `width, height`：控件宽度和高度。
  - `dirty`：是否需要重绘的标志。
  - `show`：控件是否可见。
  - `rouRectangleSize`：`StellarX::RouRectangle` 结构，保存圆角矩形控件圆角的宽高半径。
  - **注意：** `Control` 会在内部维护当前绘图状态（字体、颜色、线型等）的备份指针，用于绘制前保存状态、绘制后恢复。
- **主要接口方法：**
  - `virtual void draw() = 0;`
     **描述：** 纯虚函数，绘制控件内容。具体控件类需实现自己的绘制逻辑。
  - `virtual bool handleEvent(const ExMessage& msg) = 0;`
     **描述：** 纯虚函数，处理事件消息（如鼠标、键盘事件）。返回值表示事件是否被该控件消费（`true` 则不再向其他控件传播）。
  - `void saveBackground(int x, int y, int w, int h);`
     **描述：** 保存控件区域 `(x, y, w, h)` 的背景图像快照，以便需要时恢复背景。常用于控件需要暂时覆盖背景（如弹出框）。
  - `void restBackground();`
     **描述：** 恢复最近保存的背景快照，将之前保存的背景图像绘制回控件原位置。典型用于控件隐藏或重绘前清除旧内容。
  - `void discardBackground();`
     **描述：** 丢弃当前保存的背景快照并释放相关资源。当窗口重绘或尺寸变化导致背景失效时调用，避免错误使用旧快照。
  - *属性访问和设置：*
    - `int getX() const, getY() const, getWidth() const, getHeight() const` 等：获取控件位置和尺寸各属性。
    - `int getRight() const, getBottom() const`：获取控件右边界和下边界坐标（`x + width`，`y + height`）。
    - `void setX(int nx), setY(int ny), setWidth(int w), setHeight(int h)`：设置控件位置或尺寸，对应属性改变后自动将控件标记为需要重绘 (`dirty = true`)。
    - `void setDirty(bool d)`：手动设置控件的重绘标志。
    - `void setShow(bool visible)`：设置控件可见性。如果设为 `false`，则控件将不绘制自身内容。
    - `bool isVisible() const`：返回控件当前可见状态（`show` 标志）。**注意：** 控件被标记为隐藏时，其 `draw()` 通常不会被调用。
  - *其他：*
    - `virtual bool model() const = 0;`
       **描述：** 纯虚函数，用于检查控件是否为“模态”。仅对话框控件需要实现（模态对话框返回 `true`），其他非对话框控件可忽略（返回 `false`）。窗口事件循环根据此函数区分模态对话框与普通控件的事件处理优先级。
    - `void saveStyle(), restoreStyle();` *(受保护方法)*
       **描述：** 保存当前全局绘图样式（字体、颜色、线型等）并恢复。控件在 `draw()` 中应先调用 `saveStyle()` 备份当前样式，绘制完毕后调用 `restoreStyle()` 以免影响其他绘制操作。

### Window 类 (应用主窗口)

**描述：** `Window` 类表示应用程序的主窗口，负责窗口的创建、消息循环、控件管理和整体渲染。一个应用通常创建一个 Window 实例作为 GUI 的根容器。

- **构造函数：**
  - `Window(int width, int height, int mode, COLORREF bkColor = ..., std::string headline = "窗口")`
     创建一个指定宽、高的主窗口。`mode` 参数指定图形模式（如是否双缓冲，EasyX 使用 NULL 或 `INIT_RENDERMANUAL` 等模式），`bkColor` 是窗口背景色，`headline` 是窗口标题文本。构造时不会立即显示窗口，需调用 `draw()`。
- **主要方法：**
  - `void draw();`
     **描述：** 初始化并显示窗口，创建绘图界面。将使用设定的模式创建绘图窗口（调用 EasyX 的 `initgraph`），并应用窗口标题和背景色。然后绘制已添加的控件。通常在创建 Window 后立即调用一次。
  - `void draw(std::string pImgFile);`
     **描述：** 与无参版本类似，但使用指定路径的图像文件作为窗口背景图。该方法会加载图像并按照窗口尺寸绘制为背景，然后再绘制子控件。
  - `void runEventLoop();`
     **描述：** 进入窗口消息处理循环。该循环持续获取用户输入事件 (`peekmessage`) 并将其分发给子控件或对话框处理：
    - 优先将事件传递给所有已打开的非模态对话框（`Dialog`，`model() == false` 且 `isVisible() == true`）。一旦某个对话框的 `handleEvent` 返回 `true`（事件被消费），停止继续传递。
    - 若事件未被对话框消费，再按添加顺序将事件传递给普通控件列表中的各控件的 `handleEvent`（通常 Container 控件会向其子控件继续传播）。
    - 处理完事件后，如果有对话框正在显示（或刚关闭)，或者 `dialogClose` 标志为真，则强制重绘一帧界面：包括所有普通控件和对话框，以确保界面更新。
    - 循环每帧暂停 10 毫秒，防止 CPU 占用过高。
    - **注意：** 该函数在调用后不会返回，直到窗口接收到关闭消息 (`WM_CLOSE`)，此时函数内部会跳出循环结束。
  - `void setBkImage(IMAGE* img)` / `void setBkImage(std::string filePath);`
     **描述：** 更改窗口背景图像。可以直接传入一个已加载的 `IMAGE` 指针，或提供图像文件路径让函数内部加载。调用此函数会触发窗口重绘当前所有控件和对话框以应用新的背景。
  - `void setBkcolor(COLORREF c);`
     **描述：** 设置窗口背景颜色，并立即用该颜色清除屏幕背景（不销毁已有背景图，仅覆盖绘制）。
  - `void setHeadline(std::string title);`
     **描述：** 设置窗口标题文本。窗口创建后可随时更改标题；如果窗口已显示，则通过 WinAPI `SetWindowText` 立即更新窗口标题栏。
  - `void addControl(std::unique_ptr<Control> control);`
     **描述：** 向窗口添加一个普通控件。Window 维护一个控件列表，`draw()` 和 `runEventLoop()` 会据此绘制及派发事件。此方法接收智能指针，调用后 Window 接管控件的生命周期。
  - `void addDialog(std::unique_ptr<Control> dialog);`
     **描述：** 向窗口添加一个对话框控件（一般为 `Dialog` 类型）。对话框与普通控件分开管理，其事件和绘制有独立逻辑。
  - `bool hasNonModalDialogWithCaption(const std::string& caption) const;`
     **描述：** 检查当前是否已有**非模态**对话框，其标题（caption）与给定字符串匹配。如果存在返回 `true`。通常用于避免重复打开相同对话框。
     **重载版本：** `bool hasNonModalDialogWithCaption(const std::string& caption, const std::string& text) const;`
     除了标题，还同时比较对话框内容文本，以更严格地判断重复。`MessageBox::showAsync` 内部使用此方法防止弹出重复的提示框。
  - *信息获取方法：*
    - `HWND getHwnd() const;` 获取底层窗口句柄（EasyX 初始化返回的 HWND）。
    - `int getWidth() const, getHeight() const;` 获取窗口宽度和高度。
    - `std::string getHeadline() const;` 获取当前窗口标题。
    - `COLORREF getBkcolor() const;` 获取窗口背景色。
    - `IMAGE* getBkImage() const;` 获取当前窗口背景图像对象指针（如果有）。
    - `std::vector<std::unique_ptr<Control>>& getControls();` 引用返回窗口维护的控件列表（可以对其遍历，但一般不手动修改）。

### Canvas 类 (容器控件)

**描述：** `Canvas` 是一种可包含子控件的画布容器，用于将其他控件分组、统一背景和边框样式，以及实现复杂布局。Canvas 自身也是一个控件，继承自 `Control`。

- **特性：**
  - 支持四种矩形形状背景（普通矩形/圆角矩形，各有无边框和有边框版本），可通过 `setShape` 设置。
  - 可自定义容器背景颜色 (`canvasBkColor`)、边框颜色 (`canvasBorderColor`)、边框线型 (`canvasLineStyle`)、填充模式 (`canvasFillMode`，纯色/图案/无填充) 等。
  - Canvas 能自动管理子控件的生命周期（使用 `addControl` 添加的子控件在 Canvas 销毁时自动释放）。
  - Canvas 会在绘制时遍历并绘制所有添加的子控件，并在处理事件时优先将事件传递给子控件列表中最后添加的控件（Z 顺序）。
- **重要数据成员：**
  - `std::vector<std::unique_ptr<Control>> controls;` 子控件列表。
  - `StellarX::ControlShape shape;` 容器背景形状（默认矩形）。
  - `StellarX::FillMode canvasFillMode;` 背景填充模式（默认实色填充）。
  - `StellarX::LineStyle canvasLineStyle;` 边框线型（默认实线）。
  - `int canvaslinewidth;` 边框线宽像素值。
  - `COLORREF canvasBorderColor, canvasBkColor;` 容器边框颜色与背景色。
  - `StellarX::LayoutKind Kind;` （预留）布局管理类型，当前未深入实现，可用于标识布局策略（如绝对布局、水平/垂直布局等）。
  - **注意：** Canvas 重载了 `isVisible()` 始终返回 false，因为 Canvas 通常不作为独立可见单元（事件循环中不直接处理 Canvas，而由其子控件处理事件）。但这不影响 Canvas 作为容器的绘制和子控件事件派发。
- **主要方法：**
  - `Canvas(); Canvas(int x, int y, int w, int h);`
     **描述：** 构造一个 Canvas 容器，可以指定位置 `(x,y)` 及初始尺寸。
  - `void addControl(std::unique_ptr<Control> control);`
     **描述：** 添加子控件到 Canvas。被添加控件将成为 Canvas 内容的一部分，其坐标相对于 Canvas 左上角（Canvas 并不主动调整子控件位置，需在添加前设置好子控件位置）。添加后 Canvas 会将自身标记为需要重绘。
  - `void setShape(StellarX::ControlShape shape);`
     **描述：** 设置 Canvas 背景形状。支持 `RECTANGLE`、`B_RECTANGLE`（无边框矩形）、`ROUND_RECTANGLE`（圆角矩形）、`B_ROUND_RECTANGLE`（无边框圆角矩形）。如果传入圆形或椭圆形（CIRCLE/ELLIPSE等），Canvas 不支持，将自动按矩形处理。
  - `void setCanvasFillMode(StellarX::FillMode mode);`
     **描述：** 设置背景填充模式，如纯色 (`Solid`)、无填充 (`Null`)、预定义图案填充 (`Hatched`) 或自定义图案/图像填充 (`Pattern/DibPattern`)，需结合 `setfillstyle` 使用。默认为纯色。
  - `void setBorderColor(COLORREF color);` / `void setCanvasBkColor(COLORREF color);` / `void setCanvasLineStyle(StellarX::LineStyle style);` / `void setLinewidth(int width);`
     **描述：** 分别设置容器边框颜色、背景色、边框线型和线宽。这些改变都会使 Canvas 需要重绘。
  - `void draw() override;`
     **描述：** 绘制 Canvas 背景和其所有子控件。Canvas 绘制步骤：
    1. 根据设定的背景颜色/填充模式和形状绘制容器背景（例如填充矩形或圆角矩形区域）。
    2. 遍历子控件列表，调用每个子控件的 `draw()` 方法绘制它们。绘制前对每个子控件先设置其 `dirty = true`，确保子控件按需重绘。
    3. 恢复绘图样式，标记自身为已绘制（`dirty = false`）。
        *实现细节：* Canvas 使用 `saveStyle()/restoreStyle()` 保护全局绘图状态，并依赖 EasyX 的填充/绘制函数根据 `shape` 绘制矩形或圆角矩形背景。
  - `bool handleEvent(const ExMessage& msg) override;`
     **描述：** 将事件按照子控件的添加顺序逆序传递（后添加的子控件先处理）。对 `controls` 列表从末尾向前调用每个子控件的 `handleEvent`，若某子控件返回 `true`（表示事件已被处理），则停止传递并返回 `true`；若所有子控件都未消费事件，则返回 `false` 表示未处理。
     *用途：* 使得堆叠在顶层的子控件（通常列表后面的）有更高优先权处理事件，例如多个重叠控件的情形。
  - `void clearAllControls();` *(protected 方法)*
     **描述：** 清除并移除所有子控件。会释放智能指针列表中所有控件对象。一般在容器销毁或重置时使用。普通情况下不直接调用，由容器析构自动完成。

### Label 类 (静态文本标签)

**描述：** `Label` 用于显示一段静态文本，可设置文字颜色、背景透明/不透明及样式等。Label 不处理用户输入事件（纯展示）。

- **构造函数：**
  - `Label(); Label(int x, int y, std::string text = "标签", COLORREF textColor = BLACK, COLORREF bkColor = WHITE);`
     **描述：** 创建一个 Label 控件。可指定初始位置 `(x,y)`、显示文本内容和文本颜色、背景色等。默认文本为“标签”，默认文字黑色、背景白色。
- **主要属性：**
  - `std::string text;` 显示的文字内容。
  - `COLORREF textColor;` 文本颜色。
  - `COLORREF textBkColor;` 文本背景色（在背景不透明时生效）。
  - `bool textBkDisap;` 是否背景透明显示文本。若为 `true`，Label 绘制文字时使用透明背景模式。
  - `StellarX::ControlText textStyle;` 文字样式结构，包括字体、高度、粗细等属性（继承自 Control 默认值，例如 `字体：微软雅黑`）。
  - **注意：** Label 大小 (`width, height`) 初始为 0，通常 Label 大小根据文本自动适应，无需手动设置宽高。Label 绘制时会截取背景图像以覆盖文字区域刷新。
- **主要方法：**
  - `void setTextdisap(bool transparent);`
     **描述：** 设置文本背景是否透明显示。传入 `true` 则文字背景透明，显示时不遮挡底下背景；`false` 则文字背景为 `textBkColor` 实色矩形。
  - `void setTextColor(COLORREF color);` / `void setTextBkColor(COLORREF color);`
     **描述：** 设置文字颜色或背景色。更改后会标记 Label 需要重绘。
  - `void setText(std::string text);`
     **描述：** 更改显示的文本内容，并标记需要重绘。支持中文字符串。
  - `void draw() override;`
     **描述：** 绘制文本标签。根据 `textBkDisap` 决定调用 EasyX 的 `setbkmode(TRANSPARENT)` 或 `OPAQUE`，并设置背景色 `setbkcolor(textBkColor)`。然后设置文本颜色和样式，调用 `outtextxy(x, y, text)` 输出文字。为防止文字重绘留痕迹，`Label::draw` 会先保存文字区域背景 (`saveBackground`)，绘制完成后再恢复样式。首次绘制后将 `dirty` 置为 false。
  - `bool handleEvent(...) override;`
     **描述：** Label 不处理任何事件，始终返回 false。
  - `void hide();`
     **描述：** 将 Label 内容隐藏。其实现是调用 `restBackground()` 恢复先前保存的背景，然后 `discardBackground()` 释放快照，并将 `dirty` 设为 false。这通常在 Label 用作 Tooltip 提示框时，由宿主控件调用，用于移除提示文本的显示而不重绘整个界面。
- **使用场景：** Label 常用于界面中的静态说明文字、标题、状态栏信息等。通过配合 `textStyle` 可以修改字体、大小、是否加粗等。默认情况下 Label 背景不透明白底，如需与窗口背景融合可调用 `setTextdisap(true)` 使背景透明。

### Button 类 (按钮控件)

**描述：** `Button` 提供普通按钮和切换按钮（两态）的功能，可响应点击和悬停事件。支持设置各种样式（形状、颜色）和点击回调，是交互的主要控件之一。

- **工作模式：** 由 `StellarX::ButtonMode` 枚举定义：

  - `NORMAL` 普通按钮：每次点击都会触发一次动作，按钮本身不保持按下状态。
  - `TOGGLE` 切换按钮：每次点击切换自身状态（选中/未选中），并触发不同的回调。
  - `DISABLED` 禁用按钮：不响应用户点击，显示为灰色且文字带删除线。
  - 可通过 `setButtonMode(ButtonMode mode)` 改变按钮模式。

- **外观形状：** 由 `StellarX::ControlShape` 定义：

  - 支持矩形 (`RECTANGLE`/`B_RECTANGLE`)、圆角矩形 (`ROUND_RECTANGLE`/`B_ROUND_RECTANGLE`)、圆形 (`CIRCLE`/`B_CIRCLE`)、椭圆 (`ELLIPSE`/`B_ELLIPSE`) 八种形状（B_ 前缀表示无边框填充）。通过 `setButtonShape(ControlShape shape)` 设置按钮形状。
  - 注意：在切换 `ControlShape` 时，某些尺寸下圆形/椭圆以控件的 `width` 和 `height` 计算，圆形半径取较小的半径，椭圆按照矩形外接框绘制。

- **主要可配置属性：**

  - 颜色：`buttonTrueColor`（被点击时颜色），`buttonFalseColor`（常态颜色），`buttonHoverColor`（鼠标悬停颜色），`buttonBorderColor`（边框颜色）。
  - 填充：`buttonFillMode`（填充模式，实色/图案/图像等），`buttonFillIma`（图案填充样式，如果 `FillMode == Hatched`），`buttonFileIMAGE`（指向自定义填充图像的指针，如果 `FillMode == DibPattern`）。
  - 文本：按钮显示的文字 `text` 及其样式 `textStyle`（字体、字号、颜色等）。另外提供 `cutText` 字符串用于文本过长时显示裁剪的内容。
  - Tooltip：`tipEnabled`（是否启用悬停提示），`tipTextClick`（NORMAL模式提示文本），`tipTextOn/Off`（TOGGLE模式下两种状态的提示文本），`tipFollowCursor`（提示框是否跟随鼠标移动），`tipDelayMs`（提示延迟毫秒），`tipOffsetX/Y`（提示框相对位置偏移），`tipLabel`（内部使用的 Label 控件呈现提示文本）。
  - 圆角大小：通过继承自 `Control` 的 `rouRectangleSize` 成员控制圆角矩形圆角的半径大小。提供 `setRoundRectangleWidth(int)` 和 `setRoundRectangleHeight(int)` 接口修改。

- **主要方法：**

  - `void setOnClickListener(function<void()>&& callback);`
     **描述：** 设置按钮普通点击时的回调函数。当按钮模式为 NORMAL，在每次鼠标释放点击时调用该回调；当模式为 TOGGLE，则仅在每次点击状态发生变化时调用（取决于 toggle 逻辑）。

  - `void setOnToggleOnListener(...)` / `void setOnToggleOffListener(...);`
     **描述：** 设置按钮在 TOGGLE 模式下由未选中变为选中、以及由选中变为未选中时分别触发的回调函数。当按钮模式为 NORMAL 或 DISABLED 时，这两个回调不会被调用。

  - `void setButtonText(const char* text)` / `void setButtonText(std::string text);`
     **描述：** 更改按钮上的显示文本。支持 C 风格字符串或 `std::string`。更改文本后会重新计算文本宽高，并标记按钮需要重绘。

  - `void setButtonBorder(COLORREF border)` / `void setButtonFalseColor(COLORREF color)`
     **描述：** 设置按钮边框颜色、常态下背景填充颜色。

  - `void setFillMode(FillMode mode)` / `void setFillPattern(FillStyle pattern)` / `void setFillImage(const std::string& path)`
     **描述：** 设置按钮背景填充方式：

    - `setFillMode` 改变填充模式（纯色、图案、位图等）。
    - 若填充模式为图案 (`Pattern`)，需调用 `setFillPattern` 设置具体的图案样式 (`StellarX::FillStyle`)。
    - 若填充模式为位图 (`DibPattern`)，可调用 `setFillImage` 加载指定路径的图像并用作填充（会自动调整图像大小与按钮一致）。

  - `void setButtonClick(bool click);`
     **描述：** 外部设置按钮的“点击状态”。对于 TOGGLE 模式，`setButtonClick(true)` 会将按钮设为选中状态并触发 onToggleOn 回调，`false` 则触发 onToggleOff 回调。对于 NORMAL 模式，传入 true 将临时触发一次 onClick 回调（随后状态立即复位为未点击）。无论哪种模式，调用都会引起按钮重绘。**注意：** DISABLED 模式调用此函数没有效果。

  - `bool isClicked() const;`
     **描述：** 返回按钮当前是否处于按下状态（仅对 TOGGLE 模式有意义，表示按钮选中状态）。

  - 其他状态获取方法如 `getButtonText()`, `getButtonMode()`, `getButtonShape()`, `getFillMode()`, `getFillPattern()`, `getFillImage()`, `getButtonBorder()`, `getButtonTextColor()`, `getButtonTextStyle()`, `getButtonWidth()`, `getButtonHeight()` 一一提供，返回对应属性值。

  - `void draw() override;`
     **描述：** 绘制按钮外观。根据按钮当前状态选择填充颜色：若禁用则用 `DISABLEDCOLOUR` 灰色并给文字加删除线效果，否则 `click` 状态优先，其次 `hover` 状态，最后默认未点击颜色。然后设置透明背景、边框颜色和文本样式，计算文字应绘制的位置并绘制文字。
     如果按钮文本长度超出按钮宽度，`draw()` 会自动调用 `cutButtonText()` 对文本进行裁剪，并用省略号表示超出部分，以确保文字在按钮内完整显示。绘制结束后恢复样式并将 `dirty` 置为 false。

  - `bool handleEvent(const ExMessage& msg) override;`
     **描述：** 处理与按钮相关的鼠标事件，包括鼠标按下、弹起、移动等：

    - 检测鼠标是否悬停在按钮区域内（根据按钮形状调用内部 `isMouseInCircle/Ellipse` 辅助函数）。
    - 处理 `WM_LBUTTONDOWN`（鼠标按下）：若按钮未禁用且鼠标位于按钮上，在 NORMAL 模式下将按钮标记为点击状态（按下），在 TOGGLE 模式下不改变状态（等待弹起时处理）。
    - 处理 `WM_LBUTTONUP`（鼠标弹起）：若鼠标在按钮上且按钮可点击：
      - NORMAL 模式：如果之前已记录按下，则认为一次有效点击，调用 `onClickCallback`，然后清除点击状态。
      - TOGGLE 模式：切换按钮状态 `click = !click`。根据切换后的状态调用 `onToggleOnCallback` 或 `onToggleOffCallback`。
      - 每次有效点击后，都会通过 `flushmessage(EX_MOUSE|EX_KEY)` 清空输入消息队列，防止此次点击产生的消息（例如弹起）重复被处理。
    - 处理鼠标移出（在 `WM_MOUSEMOVE` 中检测）：如果 NORMAL 模式下拖出按钮区域则取消按下状态，任何模式下 hover 状态变化都将使按钮重绘。
    - Tooltip 提示：如果启用了 `tipEnabled`，`handleEvent` 内会管理 `tipLabel` 的显示：
      - 当鼠标首次移入按钮区域，启动一个定时（根据 `tipDelayMs`）。
      - 当鼠标离开或按钮点击后，立即隐藏提示（调用 `hideTooltip()`）。
      - 当鼠标在按钮上停留超过延迟且按钮仍显示，则设置 `tipVisible = true` 并初始化 `tipLabel` 的文本和位置准备显示提示。`tipFollowCursor` 决定提示框锚定鼠标位置还是按钮下方。
      - `refreshTooltipTextForState()` 用于当按钮为 TOGGLE 模式且未自定义提示文本时，选择显示 `tipTextOn` 或 `tipTextOff`。
      - 每帧事件处理后，如提示应显示，则调用 `tipLabel.draw()` 绘制提示文本。
    - 函数最后，如果按钮 `hover` 状态或 `click` 状态有变化，则将自身标记为 `dirty` 并立即调用 `draw()` 重绘自身，实现交互的即时反馈。

    `handleEvent` 返回值为 `true` 当且仅当按钮按下鼠标弹起事件被处理时，表示该点击事件已被消费，其它控件不应再处理。

- **使用说明：**
   开发者通常通过设置按钮的各种监听器来处理点击事件。例如：

  ```
  auto btn = std::make_unique<Button>(50, 50, 80, 30, "确定");
  btn->setOnClickListener([](){ /* 点击时执行 */ });
  ```

  对于 TOGGLE 按钮：

  ```
  btn->setButtonMode(ButtonMode::TOGGLE);
  btn->setOnToggleOnListener([](){ /* 开启时执行 */ });
  btn->setOnToggleOffListener([](){ /* 取消时执行 */ });
  ```

  也可以控制按钮的外观，如：

  ```
  btn->textStyle.color = RGB(255,255,255);             // 白色文字
  btn->setButtonFalseColor(RGB(100,150,200));          // 默认背景色
  btn->setButtonBorder(RGB(80,80,80));                 // 边框灰色
  btn->setButtonShape(StellarX::ControlShape::RECTANGLE);
  ```

  若要提示说明按钮功能，可：

  ```
  btn->tipEnabled = true;
  btn->tipTextClick = "点击执行确定操作";
  // (TOGGLE模式则设置 tipTextOn/off)
  ```

### TextBox 类 (文本框控件)

**描述：** `TextBox` 用于显示和输入单行文本。可配置为可编辑或只读模式。TextBox 提供基本的文本输入框功能，当用户点击输入模式的文本框时，会弹出一个输入对话框接受用户输入（利用 EasyX 的 `InputBox` 实现），并将结果显示在文本框中。

- **模式：** 由 `StellarX::TextBoxmode` 指定：
  - `INPUT_MODE` 可输入模式：用户点击文本框可输入新内容。
  - `READONLY_MODE` 只读模式：用户点击不会更改文本，典型地可用于展示不可修改的信息。
- **外观：**
  - 支持矩形和圆角矩形四种形状（无边框或有边框），通过 `setTextBoxShape(ControlShape)` 设置。圆形/椭圆对文本框来说不常用，一律按矩形处理。
  - 文本框默认有黑色边框和白色背景，可用 `setTextBoxBorder` 和 `setTextBoxBk` 改变。
  - 文字样式通过 `textStyle` 调整，包含字体、大小、颜色等属性。
- **主要属性：**
  - `std::string text;` 当前文本框显示的文本内容。
  - `TextBoxmode mode;` 当前模式（决定是否允许输入）。
  - `ControlShape shape;` 当前文本框边框形状（矩形或圆角矩形，默认 RECTANGLE）。
  - `bool click;` 内部使用标志，表示是否刚被点击（用于触发输入弹窗，处理完后即重置）。
  - `size_t maxCharLen;` 最大允许输入字符数。默认 255。
  - `COLORREF textBoxBorderColor, textBoxBkColor;` 边框颜色和背景填充颜色。
  - `StellarX::ControlText textStyle;` 文本样式（高度、宽度、字体、颜色等）。
- **主要方法：**
  - `void setMode(StellarX::TextBoxmode mode);`
     **描述：** 切换文本框模式。可选 INPUT_MODE 或 READONLY_MODE。更改模式不会清除已有文本，但只读模式下用户输入将被忽略。
  - `void setMaxCharLen(size_t len);`
     **描述：** 设置文本框最大字符长度。仅当 `len > 0` 时有效。超出长度的输入会被截断。
  - `void setTextBoxShape(StellarX::ControlShape shape);`
     **描述：** 设置文本框形状。支持矩形和圆角矩形，与 Button 类似。如果传入非法形状（圆/椭圆），会自动改为矩形。设置后文本框将 `dirty`，下次绘制按新形状渲染。
  - `void setTextBoxBorder(COLORREF color);` / `void setTextBoxBk(COLORREF color);`
     **描述：** 设置边框颜色和背景颜色。
  - `void setText(std::string text);`
     **描述：** 更新文本框内容文本。如果新文本长度超过 `maxCharLen`，将自动截断。设置后标记 `dirty` 并立即调用 `draw()` 重绘，使更改即时可见。
  - `std::string getText() const;`
     **描述：** 获取当前文本框内容字符串。
  - `void draw() override;`
     **描述：** 绘制文本框背景和文字。根据 `textStyle` 设置字体，如字体高度/宽度大于控件尺寸则裁剪适应；使用透明背景模式绘制文字，以便背景色通过 `fill...` 函数填充。
    - 绘制顺序：先 `fillrectangle/roundrect` 绘制背景（按 `textBoxBkColor`），然后 `outtextxy` 绘制文本，文本在 X 方向留出 10 像素左右内边距，Y 方向垂直居中。
    - 如果文本长度超出控件宽度，不自动裁剪（与 Button 不同），而是可能显示不下或被遮盖。通常应通过设置 `maxCharLen` 控制输入长度，或设计足够宽度的文本框。
    - 绘制完成后 `dirty` 置 false。
  - `bool handleEvent(const ExMessage& msg) override;`
     **描述：** 处理鼠标事件：
    - 判断鼠标是否在文本框区域 (`hover`)。
    - 当 `WM_LBUTTONUP` 且鼠标在区域内发生：若模式为 INPUT_MODE，则调用 EasyX 的 `InputBox` 弹出输入对话框，允许用户输入文本（初始值为当前文本框内容），用户提交后 `InputBox` 返回新文本，将此文本赋值并标记 `dirty` 以便重绘显示。若模式为 READONLY_MODE，则弹出一个只读提示框（不会改变文本），并不修改 `dirty` 状态。
    - 其它鼠标事件不改变文本框外观，仅根据需要返回 false（未消费事件）。
    - 当完成一次输入或点击，文本框会调用自身 `draw()` 更新显示。READONLY_MODE 下点击会简单弹出提示（使用 InputBox 显示原文本并提示输入无效）。
    - 函数返回 true 表示在 INPUT_MODE 下点击事件被消费（因为弹出了输入框），READONLY_MODE 下也返回 true（提示已显示），从而阻止事件继续传播。
- **用法注意：**
  - 由于 EasyX 的 `InputBox` 是模态阻塞函数，`runEventLoop` 本身是非阻塞循环，**在 UI 线程直接调用 InputBox 会暂停整个消息循环** 直到输入完成。StellarX 目前采用这种简单方式处理文本输入（不是异步 UI 输入）。因此在 `TextBox::handleEvent` 中使用 `InputBox` 来得到用户输入。这简化了实现，但 UI 线程会暂时挂起。开发者应避免在非常频繁交互时滥用 InputBox。
  - 可以通过 `maxCharLen` 控制可输入最大字符数，例如对于数值输入框可以设定长度防止超出范围。
  - 使用 `textStyle` 可调整显示字体（如等宽字体用于代码输入等），`textStyle.color` 也可单独设置文字颜色。

### Dialog 类 (对话框控件)

**描述：** `Dialog` 提供模态/非模态对话框功能，可在应用中弹出提示、确认或输入等对话界面。Dialog 本质上是一个特殊的 Canvas 容器，内部包含文本消息和按钮等子控件，并与 Window 紧密配合实现模态阻塞效果。

- **创建与模式：**

  - 构造函数 `Dialog(Window& parent, std::string title, std::string message = "对话框", MessageBoxType type = OK, bool modal = true)`
     **描述：** 创建一个关联到 `parent` 窗口的对话框。`title` 将作为对话框标题文本显示在对话框顶部，`message` 是正文消息内容。`type` 参数指定对话框包含的按钮组合类型（见 MessageBoxType 枚举），例如 OK 只有一个确定按钮，YesNo 包含“是”和“否”两个按钮等。`modal` 决定对话框是模态还是非模态。构造后对话框初始为隐藏状态，需要调用 `show()` 显示。
  - `MessageBoxType` 枚举定义了常见的对话框按钮组合：
    - `OK`：一个“确定”按钮。
    - `OKCancel`：“确定”和“取消”按钮。
    - `YesNo`：“是”和“否”按钮。
    - `YesNoCancel`：“是”、“否”和“取消”按钮。
    - `RetryCancel`：“重试”和“取消”。
    - `AbortRetryIgnore`：“中止”、“重试”、“忽略”按钮。
  - 模态 (`modal=true`) 对话框：调用 `show()` 时会阻塞当前线程运行，直到对话框关闭后才返回。这期间应用的其他部分暂停响应输入。适用于必须先处理用户响应的关键情景。模态对话框关闭时，`show()` 返回后可通过 `getResult()` 获取用户点击结果。
  - 非模态 (`modal=false`) 对话框：调用 `show()` 则立即返回，应用程序继续运行，用户可以在背景窗口和对话框之间自由交互。需要通过回调函数或轮询 `getResult()` 来获取对话框的结果。

- **组成元素：**

  - 标题文本（使用内部的 Label 控件 `title` 实现，显示在对话框顶部）。
  - 消息正文（支持多行文本，`Dialog` 自动按行分割存储在 `lines` 向量中，并在绘制时逐行居中输出）。
  - 功能按钮（根据 `MessageBoxType`，包含1到3个 Button 控件，位置自动计算居于对话框底部）。
  - 关闭按钮（右上角的 “×” 按钮，一个小的 Button 控件，点击相当于取消/关闭）。
  - 背景样式：Dialog 采用圆角矩形背景（如 Canvas shape = ROUND_RECTANGLE）并有半透明效果（具体通过 saveBackground 恢复背景实现视觉模态效果，但在 EasyX 简化模型下实际只是截屏背景，无模糊透明实现）。

- **主要属性：**

  - `bool modal;` 是否模态对话框。
  - `titleText`：标题栏文本，`message`：消息内容文本。
  - `MessageBoxType type;` 对话框类型（按钮组合）。
  - `std::vector<std::string> lines;` 分割后的多行消息文本。
  - `std::unique_ptr<Label> title;` 用于显示标题的 Label 控件。
  - `Button* closeButton;` 关闭按钮指针（“×”）。
  - `Window& hWnd;` 引用父窗口，用于操作对话框关闭后刷新背景等。
  - `MessageBoxResult result;` 用户选择结果，枚举值对应哪个按钮被按下。常用值：OK=1, Cancel=2, Yes=6, No=7 等（见 CoreTypes 定义）。
  - `std::function<void(MessageBoxResult)> resultCallback;` 非模态对话框的结果回调函数指针。如果设置，将在对话框关闭且 `modal == false` 时被调用，参数为用户最终点击结果。

- **主要方法：**

  - `void show();`
     **描述：** 显示对话框。如果是模态对话框，此函数将阻塞，内部运行一个事件处理循环，使该对话框独占输入焦点，直到对话框关闭为止；如果是非模态，则此函数立即返回（但对话框窗口已可见，输入由主 Window 继续分发）。`show()` 不返回任何值，但可以在模态情况下函数返回后调用 `getResult()` 获取结果。

  - `void closeDialog();`
     **描述：** 关闭对话框。会将对话框隐藏并清理资源。如果有设置非模态回调且对话框为非模态，则在此过程中调用 `resultCallback(result)` 通知结果。通常不直接调用，由内部逻辑根据按钮按下触发。

  - `MessageBoxResult getResult() const;`
     **描述：** 获取对话框的返回结果（用户按下了哪个按钮）。对模态对话框，`show()` 返回后调用此函数获取结果；对非模态，可在回调或稍后某时获取。

  - `void setTitle(const std::string& title);` / `void setMessage(const std::string& msg);`
     **描述：** 更改对话框标题或内容文本。更改内容会重新分割 `lines` 并调整文本区域大小，需要重新布局。

  - `void setType(MessageBoxType type);`
     **描述：** 更改对话框类型（按钮组合）。将重新创建按钮布局。

  - `void setModal(bool modal);`
     **描述：** 切换对话框模态属性（必须在 show() 前调用）。一般不会在运行中切换。

  - `void setResult(MessageBoxResult res);`
     **描述：** 设置对话框结果代码。通常由内部在按钮按下时调用，开发者无需手动调用。

  - `void setInitialization(bool init);`
     **描述：** 若传入 true，则提前计算和设置对话框尺寸，并保存背景快照。**用途：** 在通过 `MessageBox` 静态函数创建对话框时，会先调用此函数以准备好对话框以便正确显示。

  - `void setResultCallback(function<void(MessageBoxResult)> cb);`
     **描述：** 为非模态对话框设置结果回调函数。用户关闭对话框时将异步调用此函数，将结果传出。

  - *内部实现在 `Dialog` 类私有方法中：*

    - `initButtons()`：根据 `type` 创建对应数量的 Button 控件并设置其回调，将它们添加到 Dialog 容器。
    - `initCloseButton()`：创建右上角关闭按钮（文本为 “×”），点击后触发取消逻辑。
    - `initTitle()`：创建标题 Label 控件。
    - `splitMessageLines()`：将 `message` 根据换行符分割填充到 `lines` 数组。
    - `getTextSize()`：计算 `lines` 中最长行宽度和单行高度，以确定文本区域所需尺寸。
    - `initDialogSize()`：综合计算对话框宽高（考虑文本区域和按钮区域的宽度、高度），然后居中放置对话框于窗口。最后调用上述 init** 方法创建各子控件。
    - `performDelayedCleanup()`：用于延迟销毁对话框时释放资源。Dialog 设计上为了避免背景快照不匹配，需要在对话框关闭且完全隐去后再恢复背景并释放子控件资源；此函数执行该清理流程，由 Window 的事件循环或 `handleEvent` 在适当时机调用。

  - `void draw() override;`
     **描述：** 绘制对话框界面。Dialog 继承自 Canvas，通过调用 `Canvas::draw()` 绘制背景和子控件框架，然后另外绘制消息文本内容。绘制步骤：

    1. 如果首次显示，对话框先保存覆盖区域的屏幕背景（便于关闭时恢复）。
    2. 设置 Canvas 的背景颜色、边框等样式为对话框特定值（通常圆角矩形）。
    3. 将所有子控件标记为 `dirty` 并调用 `Canvas::draw()` 绘制对话框背景和子控件（包括按钮、标题Label等）。
    4. 使用对话框的 `textStyle` 设置文字样式，然后将每行 `lines` 文本居中绘制在对话框内部合适的位置（计算基于对话框宽度）。
    5. 恢复样式，设置 `dirty = false`。

    对话框作为一个整体，由 Window 管理，其绘制顺序通常在 Window::runEventLoop 强制刷新时绘制。模态对话框在自己的 loop 内绘制。

  - `bool handleEvent(const ExMessage& msg) override;`
     **描述：** 处理对话框相关事件：

    - 如果对话框未显示 (`show == false`)，则检查 `pendingCleanup` 标志，如果有延迟清理任务且当前未在清理，则调用 `performDelayedCleanup()` 执行清理，然后不消费事件直接返回 false。
    - 如果对话框正在清理或标记延迟清理，则不处理任何事件直接返回 false。
    - 若为模态对话框 (`modal == true`)，并检测到用户点击了对话框区域外部（窗口背景区域）的鼠标弹起事件，则发出提示声音 (`\a`)并返回 true（**吞噬事件**），防止用户点击窗体其他部分。
    - 将事件传递给 Dialog 继承的 Canvas（即其子控件）处理：`consume = Canvas::handleEvent(msg)`，这样对话框内部的按钮可以响应事件。如果子控件处理了事件则 consume 为 true。
    - 最后，如果对话框有延迟清理任务，则调用 `performDelayedCleanup()` 处理。返回值为 whether 消费了事件。
    - **模态对话框特别说明：** Window 主循环在有模态对话框显示时，会将背景点击等事件吃掉（上面逻辑已实现），同时对话框 `show()` 方法内部启动自己的循环处理事件，使模态对话框独占用户输入。非模态对话框则依赖 Window 主循环分发事件，由 handleEvent 参与处理。

- **使用示例：**
   `Dialog` 通常不直接使用，而是通过 `StellarX::MessageBox` 工具调用快速创建并显示：

  - 模态对话框：

    ```
    StellarX::MessageBoxResult res = StellarX::MessageBox::showModal(window, 
                             "文件已保存。是否关闭程序？", "提示", 
                             StellarX::MessageBoxType::YesNoCancel);
    if (res == StellarX::MessageBoxResult::Yes) { /* 用户选择是 */ }
    ```

    `showModal` 内部创建 `Dialog` 并以模态方式显示，直到用户点击“是/否/取消”其中一个按钮，函数返回对应的枚举值。

  - 非模态对话框：

    ```
    StellarX::MessageBox::showAsync(window, 
                      "处理完成！", "通知", 
                      StellarX::MessageBoxType::OK, 
                      [](StellarX::MessageBoxResult){ /* 用户点了OK后的处理 */ });
    ```

    这样弹出的对话框不会阻塞程序，它包含一个“确定”按钮。用户点击后，对话框关闭并自动调用提供的 lambda 回调。Window 的事件循环会继续正常运行。

  开发者也可自行实例化 Dialog：

  ```
  Dialog dlg(mainWindow, "标题", "内容……", StellarX::MessageBoxType::OKCancel, false);
  dlg.setResultCallback([](MessageBoxResult res) { /* 处理结果 */ });
  dlg.show();
  ```

  以上将创建一个非模态对话框。一般推荐使用 `MessageBox` 封装函数更方便。

## MessageBox 工具类

**描述：** `StellarX::MessageBox` 提供静态方法方便地创建标准对话框，无需直接操作 Dialog 类。类似 Windows API 的 MessageBox，但这里可以指定按钮组合类型，并支持非模态调用。

- **静态函数：**

  - `static MessageBoxResult showModal(Window& wnd, const std::string& text, const std::string& caption = "提示", MessageBoxType type = MessageBoxType::OK);`
     **描述：** 弹出一个模态对话框，在窗口 `wnd` 上显示，标题为 `caption`，正文为 `text`，按钮组合由 `type` 指定。此函数会阻塞当前执行直到用户在对话框上点击按钮关闭对话框。返回值为 `MessageBoxResult`，表示用户点击了哪个选项（例如 `MessageBoxResult::OK` 或 `Cancel` 等）。开发者可根据返回值执行不同逻辑。
  - `static void showAsync(Window& wnd, const std::string& text, const std::string& caption = "提示", MessageBoxType type = MessageBoxType::OK, std::function<void(MessageBoxResult)> onResult = nullptr);`
     **描述：** 在窗口 `wnd` 弹出一个非模态对话框。参数类似 `showModal`，但 `onResult` 是可选的回调函数。如果提供了回调函数，当用户关闭对话框时将异步调用该函数并传入用户选择结果。该函数本身立即返回，不阻塞程序流程。如果在 `wnd` 中已经存在一个内容和标题都相同的非模态对话框未关闭，则此函数会发出警报音并不创建新的对话框，以防止重复（内部使用 `Window::hasNonModalDialogWithCaption(caption,text)` 检查)。
     **注意：** 如果不提供 `onResult` 回调，那么需要其他方式得知对话框结果（例如通过 Dialog 的公有接口或者全局状态），但通常建议提供回调以处理结果。

- **实现细节：** `showModal` 和 `showAsync` 内部都会创建一个 `Dialog` 对象，并根据 `type` 设置相应按钮逻辑：

  - `showModal` 创建 Dialog 后，会调用 `dlg.setInitialization(true)` 准备好对话框尺寸，然后调用 `dlg.show()` 以模态方式显示，并阻塞等待结果，最后返回 `dlg.getResult()`。
  - `showAsync` 则创建 Dialog 后，同样初始化，若指定回调则通过 `dlgPtr->setResultCallback(onResult)` 注册，然后把对话框通过 `wnd.addDialog()` 加入主窗口的对话框列表，最后调用 `dlgPtr->show()` 非模态显示。非模态显示不会阻塞，因此 `showAsync` 很快返回；对话框的关闭与结果传递在回调中完成。

- **典型用途：** MessageBox 提供与系统 MessageBox 类似的简明接口：

  - 显示一条提示信息并等待确认：
     `MessageBox::showModal(mainWindow, "操作成功完成！", "提示", MessageBoxType::OK);`

  - 提示加确认选择：
     `auto res = MessageBox::showModal(mainWindow, "确定要删除记录吗？", "请确认", MessageBoxType::YesNo); if (res == MessageBoxResult::Yes) { ... }`

  - 非模态通知：
     `MessageBox::showAsync(mainWindow, "下载已在后台进行。", "通知", MessageBoxType::OK); // 用户点OK时对话框将自行关闭`

  - 非模态询问并通过回调获取结果：

    ```
    MessageBox::showAsync(mainWindow, "发现新版本，是否现在更新？", "更新提示", MessageBoxType::YesNo,
        [](MessageBoxResult res) {
            if (res == MessageBoxResult::Yes) startUpdate();
        });
    ```

  通过这些静态方法，可以很方便地融合对话框提示流程，无需手动管理 Dialog 对象的生命周期和事件处理逻辑。

## Table 类 (表格控件)

**描述：****Table** 类是一个高级表格控件，支持分页显示和大数据量展示[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/include/StellarX/table.h#L2-L10)。它提供完整的数据表格功能，包括表头、数据行和分页导航等，实现类似电子表格的呈现。Table 控件可自动计算列宽和行高，支持自定义边框样式、填充模式以及文本字体样式，从而适应不同的UI设计需求。通过内置的分页机制，Table 能有效展示大量数据而不影响界面性能，并使用背景缓存来优化渲染，避免重绘闪烁。

**主要特性：**

- **自动分页：** 根据数据行数和每页行数自动计算总页数和当前页显示内容[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/include/StellarX/table.h#L8-L16)。提供 **上一页/下一页** 按钮和页码指示，方便用户浏览分页数据。
- **自定义配置：** 开发者可配置每页行数、是否显示翻页按钮，设置表格边框颜色、背景色，选择填充模式（实色填充或透明）和边框线型等，满足不同视觉风格要求。
- **高效渲染：** Table 通过缓存背景图像来提高渲染效率。在表格内容变化或首次绘制时缓存底图，绘制时仅更新差异部分，减少重复绘制带来的闪烁[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L210-L219)[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L241-L249)。
- **典型场景：** 适用于数据列表、报表、记录浏览等需要表格式展示的场景[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/include/StellarX/table.h#L14-L17)。例如显示数据库查询结果、日志记录、统计报表等，让用户以行列形式查看信息。

### 公共成员函数

#### Table(int x, int y)

**函数原型：** `Table(int x, int y)`
 **参数：**

- `x`：表格左上角的 X 坐标位置。
- `y`：表格左上角的 Y 坐标位置。
   **返回值：** 无（构造函数无返回值）。
   **功能描述：** Table 类的构造函数。在指定的坐标位置创建一个新的表格控件实例[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L164-L171)。初始状态下表格没有设置表头和数据，宽度和高度初始为0，会在设置数据后根据内容自动计算调整。`x` 和 `y` 参数用于将表格定位在窗口中的起始位置。
   **使用场景：** 当需要在界面上显示表格数据时，首先调用此构造函数实例化一个 Table 对象。例如，在创建主窗口后，可以创建 `Table table(50, 50);` 来放置一个表格控件，然后再设置表头和数据。

#### ~Table()

**函数原型：** `~Table()`
 **参数：** 无。
 **返回值：** 无。
 **功能描述：** Table 类的析构函数。在对象销毁时自动调用，负责释放表格内部分配的资源。如表格内部创建的翻页按钮、页码标签以及背景缓存图像都将在析构函数中删除以防止内存泄漏[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L169-L178)。
 **使用场景：** 一般不直接调用，由系统自动管理。当 Table 对象的生命周期结束（例如其所在窗口关闭或程序结束）时，析构函数确保清理内存。不需开发者手动调用，但了解其行为有助于避免重复释放资源。

#### draw()

**函数原型：** `void draw() override`
 **参数：** 无。
 **返回值：** 无。
 **功能描述：** 重写自 `Control` 基类的绘制函数，用于将表格绘制到屏幕[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L185-L194)[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L252-L260)。调用 `draw()` 时，Table 根据当前状态绘制表头、当前页的数据行、页码标签和翻页按钮（如果启用）。绘制过程中会应用先前设置的文本样式、边框颜色、背景色等属性。为避免频繁重绘，Table 内部维护一个`dirty`标志，只有当表格内容或外观需更新时（例如调用了设置函数后）`draw()` 才会实际重绘内容[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L187-L196)[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L258-L266)。
 **使用场景：** 通常在设置完表头和数据、或修改了样式后调用，令表格按新内容渲染。一般框架的窗口在刷新时会自动调用各控件的 `draw()` 方法，开发者也可在需要立即刷新表格时手动调用它。例如，当表格数据更新且需要即时反映在界面上时，可以设置相应属性并调用 `draw()` 触发重绘。

#### handleEvent(const ExMessage& msg)

**函数原型：** `bool handleEvent(const ExMessage& msg) override`
 **参数：**

- `msg`：事件消息对象（EasyX库的 `ExMessage` 结构），包含了键盘或鼠标事件的信息。
   **返回值：** 布尔值，表示事件是否被表格控件处理。返回 `true` 表示事件已被消耗，不再需要向其他控件传播；返回 `false` 则表示表格未处理该事件。
   **功能描述：** 重写自 `Control` 基类的事件处理函数，用于处理与表格相关的用户交互事件。对于 Table 而言，`handleEvent` 主要用于处理翻页按钮的点击事件。当 `isShowPageButton` 为真（显示翻页按钮）时，该方法会将传入的事件传递给内部的 “上一页” 和 “下一页” 按钮进行处理[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L274-L283)。如果用户点击了上一页或下一页按钮，按钮的内部逻辑会更新当前页码并调用 `draw()` 重绘表格，`handleEvent` 随后返回 `true` 表示事件已处理。若翻页按钮未启用（例如只有一页数据或通过 `showPageButton(false)` 隐藏了按钮），此函数直接返回 `false`[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L276-L283)，不对事件进行处理。
   **使用场景：** 框架的主事件循环会调用控件的 `handleEvent` 方法来分发用户输入。当使用 Table 控件时，无需手动调用此函数，但了解其行为有助于处理自定义事件。例如，可在Window的事件循环中将鼠标点击事件传给表格控件，使其内部的翻页按钮响应点击，从而实现分页浏览的数据切换。

#### setHeaders(std::initializer_liststd::string headers)

**函数原型：** `void setHeaders(std::initializer_list<std::string> headers)`
 **参数：**

- `headers`：表头文本字符串的初始化列表，每个字符串对应表格一列的列名。
   **返回值：** 无。
   **功能描述：** 设置表格的列标题。调用此函数会清除当前已有的表头并使用提供的字符串列表作为新表头[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L288-L296)。在设置表头后，Table 会将内部状态标记为需要重新计算单元格尺寸和重新绘制表头[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L291-L295)。列标题用于在表格顶部显示，表示每列数据的含义。表头数量确定了表格的列数，后续添加的数据应与列数匹配。
   **使用场景：** 通常在创建 Table 实例后、添加数据之前调用，用来定义表格的列结构。例如：`table.setHeaders({"姓名", "年龄", "职业"});` 将表格设置为包含“姓名”、“年龄”、“职业”三列。设置表头后即可调用 `setData` 添加对应列的数据行。

#### setData(std::vectorstd::string data) / setData(std::initializer_list<std::vectorstd::string> data)

**函数原型1：** `void setData(std::vector<std::string> data)`
 **参数：**

- `data`：包含单行表格数据的字符串向量，每个元素对应一列的内容。
   **返回值：** 无。
   **功能描述：** 将一个数据行添加到表格中。若传入的这行数据列数少于当前表头列数，则函数会自动在行数据末尾补空字符串，使其列数与表头一致[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L300-L308)。然后将该行数据追加到内部数据集合 `data` 中，并更新表格的总页数 (`totalPages`) 属性[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L302-L308)。更新页数时根据当前总行数和每页行数计算页数，确保至少为1页[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L303-L308)。函数调用后会标记表格需要重新计算单元格尺寸，并在下次绘制时包含新添加的数据。
   **使用场景：** 可用于逐行构建表格数据，例如从数据源读取记录并一条条加入表格。适合数据逐步到达或者需要动态添加行的情形。每次调用都会在末页附加一行，并相应更新分页显示。

**函数原型2：** `void setData(std::initializer_list<std::vector<std::string>> data)`
 **参数：**

- `data`：一个由多行数据组成的初始化列表，每个内部的 `std::vector<std::string>` 代表一行的数据。
   **返回值：** 无。
   **功能描述：** 批量添加多行数据到表格中。对于提供的每一行数据，函数会检查其列数是否与表头列数匹配；若不足则在该行末尾补充空字符串以对齐列数[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L313-L321)。然后将处理后的整行数据依次追加到表格内部的数据集中。添加完所有行后，函数重新计算总数据行数下的总页数，并至少确保一页[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L322-L328)。与单行版本类似，此方法也会触发单元格尺寸重新计算和脏标记，使表格内容在下次绘制时更新。
   **使用场景：** 适用于一次性导入整批数据的情形，例如应用初始化时加载整个数据表。调用此函数可以方便地用初始化列表直接提供多行初始数据，而无需多次重复调用单行的 `setData`。在需要清空已有数据并填入新数据时，可以在调用此方法前手动清除旧数据（例如重新创建Table或使用返回的data容器修改）。

#### setRowsPerPage(int rows)

**函数原型：** `void setRowsPerPage(int rows)`
 **参数：**

- `rows`：每页显示的数据行数（正整数）。
   **返回值：** 无。
   **功能描述：** 设置表格分页时每页包含的数据行数。调用此函数会修改内部的 `rowsPerPage` 属性，并根据当前总数据行数重新计算总页数 `totalPages`[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L332-L338)。如果重新计算后总页数低于1，则强制设为1页（避免出现0页的非法状态）。改变每页行数将影响表格的高度和分页布局，因此会标记需要重新计算单元格大小以及重新绘制。在下次 `draw()` 时，表格将按照新的分页大小重新呈现内容。
   **使用场景：** 当需要调整表格分页密度时使用。例如默认每页显示5行数据，可以通过 `table.setRowsPerPage(10);` 改为每页10行，以减少总页数。常用于根据用户偏好或窗口大小调整每页显示量。应注意，如果当前页码在调整后超出新的总页数范围，表格在下次翻页时会自动纠正到最后一页。

#### showPageButton(bool isShow)

**函数原型：** `void showPageButton(bool isShow)`
 **参数：**

- `isShow`：布尔值，指定是否显示翻页按钮。`true` 表示显示 “上一页” 和 “下一页” 按钮，`false` 则隐藏它们。
   **返回值：** 无。
   **功能描述：** 控制表格分页导航按钮的可见性。调用此函数可以在界面上隐藏或显示表格底部的翻页按钮。如果设置为隐藏 (`false`)，则用户将无法通过界面按钮切换页码（此时表格将始终停留在当前页）；当再次显示按钮 (`true`) 时，用户可以使用按钮查看其他页的数据。该函数仅影响按钮的绘制和事件处理状态，不改变当前页码等分页数据。
   **使用场景：** 适用于当数据量较少仅一页即可显示完毕，或开发者希望以其他方式控制分页时。例如，当表格内容只有一页数据时，可以隐藏翻页按钮以简化界面。需要再次启用分页功能时再调用 `showPageButton(true)` 显示按钮。

#### setTableBorder(COLORREF color)

**函数原型：** `void setTableBorder(COLORREF color)`
 **参数：**

- `color`：表格边框颜色，类型为 `COLORREF`（Windows RGB 颜色值，例如 `RGB(0,0,0)` 表示黑色）。
   **返回值：** 无。
   **功能描述：** 设置表格单元格边框线的颜色。调用该方法会将 Table 内部存储的边框颜色更新为指定的 `color`，并将表格标记为需要重绘。在随后的 `draw()` 调用中，表格的线条（包括单元格边框和外围边框）将使用新的颜色绘制[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L192-L199)。默认的边框颜色为黑色，如需与界面主题匹配可使用此函数调整。
   **使用场景：** 当需要改变表格线条颜色以配合UI配色方案时使用。例如，将表格边框设为浅灰色以获得柔和的视觉效果：`table.setTableBorder(RGB(200,200,200));`。在深色模式下，可将边框设为亮色以提高对比度。调用后下一次重绘将体现颜色变化。

#### setTableBk(COLORREF color)

**函数原型：** `void setTableBk(COLORREF color)`
 **参数：**

- `color`：表格背景填充颜色，类型为 `COLORREF`。
   **返回值：** 无。
   **功能描述：** 设置表格单元格的背景颜色。调用该函数会更新 Table 内部的背景色属性，并触发重绘标记。在绘制表格时，每个单元格将使用指定的背景颜色进行填充（当填充模式为实色时）[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L192-L199)。默认背景颜色为白色，如需改变单元格底色（例如间隔行着色等）可以使用不同的颜色多次调用或自定义绘制。
   **使用场景：** 用于调整表格的底色风格。例如，将表格背景设为浅黄色以突出显示内容区域：`table.setTableBk(RGB(255, 255, 224));`。结合填充模式使用，可以实现透明背景等效果。注意背景颜色变化在填充模式为Solid时可见，若填充模式为Null则背景不绘制实色。

#### setTableFillMode(StellarX::FillMode mode)

**函数原型：** `void setTableFillMode(StellarX::FillMode mode)`
 **参数：**

- `mode`：填充模式，枚举类型 `StellarX::FillMode`。可选值包括 Solid（实色填充）、Null（不填充）、Hatched/Pattern 等。
   **返回值：** 无。
   **功能描述：** 设置表格的填充模式，即单元格背景的绘制方式。常用的模式有 Solid（纯色填充背景）或 Null（透明背景不填充）。当前版本中，仅对 Solid 和 Null 模式提供完全支持：如果传入其他模式值，内部会默认退回使用 Solid 模式[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L360-L369)。调用此方法会应用新的填充模式，并将此模式同步到表格的翻页按钮和页码标签控件上，使它们风格一致[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L365-L373)。例如，当设置为 Null 模式时，表格单元格背景将不被填充颜色，呈现透明效果，同时页码标签也设置为透明背景显示。改变填充模式会将表格及相关控件标记为需重绘。
   **使用场景：** 根据界面设计需要调整表格背景绘制策略。例如，为实现镂空透明效果，可使用 `table.setTableFillMode(StellarX::FillMode::Null);` 使表格仅绘制文字和边框而无底色。默认使用 Solid 模式填充背景，实现传统表格外观。当需要图案填充等高级效果时，当前版本可能不支持，仍会以纯色替代。

#### setTableLineStyle(StellarX::LineStyle style)

**函数原型：** `void setTableLineStyle(StellarX::LineStyle style)`
 **参数：**

- `style`：边框线型，枚举类型 `StellarX::LineStyle`，如 Solid（实线）、Dash（虚线）、Dot（点线）等。
   **返回值：** 无。
   **功能描述：** 设置表格网格线（单元格边框线条）的样式。传入不同的线型枚举值可以改变表格绘制时边框的样式，例如使用虚线或点线来绘制单元格边框。调用该方法将更新内部存储的线型并标记表格需要重绘，下次绘制时新的线型将生效。需注意实际效果取决于绘图库对线型的支持。
   **使用场景：** 当希望改变表格外观以区分层次或提升美观时，可修改线型。例如，`table.setTableLineStyle(StellarX::LineStyle::Dash);` 将表格边框改为虚线风格，以在视觉上弱化表格线对界面的干扰。此功能在打印报表或特殊主题界面中会很有用。

#### setTableBorderWidth(int width)

**函数原型：** `void setTableBorderWidth(int width)`
 **参数：**

- `width`：边框线宽度（像素值，正整数）。
   **返回值：** 无。
   **功能描述：** 设置表格边框线条的粗细（线宽）。默认情况下表格边框宽度为1像素，调用此函数可使所有单元格的边框线加粗或变细。更新线宽后表格将在下次重绘时使用新的线宽绘制边框[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L198-L201)。如果设置的宽度大于1，为避免粗线覆盖过多背景，Table 在绘制时会调整背景区域的恢复以适应粗线描边[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L220-L228)[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L240-L248)。
   **使用场景：** 在需要强调表格结构或适应高分辨率显示时，可增加线宽。例如，`table.setTableBorderWidth(2);` 将表格线条加粗为2像素，使表格框架更加醒目。若要获得细网格效果，可以将线宽设置为1以下（不过线宽最小为1像素）。改变线宽后请确保颜色、线型等设置搭配以获得预期视觉效果。

### 获取属性方法（Getters）

以下函数用于获取 Table 当前的状态或配置信息：

- **int getCurrentPage() const**：获取当前显示的页码索引。第一页为1，第二页为2，以此类推[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L390-L398)。可用于在界面上显示当前页码或在程序逻辑中判断分页位置。
- **int getTotalPages() const**：获取根据当前数据行数和每页行数计算出的总页数[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L394-L402)。通常在设置完数据或调整分页参数后调用，以了解数据被分页成多少页。
- **int getRowsPerPage() const**：获取当前每页显示的数据行数设置[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L400-L408)。可用于在界面上提示用户分页大小，或在逻辑中根据该值调整分页处理。
- **bool getShowPageButton() const**：获取当前翻页按钮的显示状态（true 为显示，false 为隐藏）[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L404-L412)。可根据此值判断界面上是否提供了翻页导航。
- **COLORREF getTableBorder() const**：获取当前表格边框颜色[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L410-L418)（返回一个COLORREF值）。可以用于在需要时与其他控件颜色保持一致或者保存当前配置。
- **COLORREF getTableBk() const**：获取当前表格背景填充颜色[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L414-L422)。
- **StellarX::FillMode getTableFillMode() const**：获取当前表格的填充模式设置[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L420-L428)。返回值为 `StellarX::FillMode` 枚举，可判断是实色填充、透明等模式。
- **StellarX::LineStyle getTableLineStyle() const**：获取当前表格边框线型设置[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L424-L432)。返回 `StellarX::LineStyle` 枚举值，如实线、虚线等。
- **std::vectorstd::string getHeaders() const**：获取当前表格的表头列表副本[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L430-L438)。返回一个字符串向量，其中包含每一列的标题。可以使用此函数查看或保存表格的列标题设置。
- **std::vector<std::vectorstd::string> getData() const**：获取当前表格的所有数据[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L434-L442)。返回一个二维字符串向量，其中每个内部向量代表表格中的一行数据（各元素对应该行每列的值）。这允许访问或遍历表格内容，例如用于导出数据。
- **int getTableBorderWidth() const**：获取当前设置的表格边框线宽度（像素）[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L440-L446)。

以上获取函数均为 `const` 成员函数，不会修改 Table 的状态，只返回相应属性的值。开发者可根据需要随时调用这些函数来查询表格状态，例如在界面其他区域显示统计信息或在调试时验证设置是否正确。

### 公共成员变量

- **StellarX::ControlText textStyle**：文本样式结构体，表示表格中文字内容的字体和样式设置[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/include/StellarX/CoreTypes.h#L132-L140)。通过修改该结构体的成员，可以自定义表格文本的外观，例如字体字号、高度、宽度、字体名称 (`lpszFace`)，文字颜色 (`color`)，以及是否加粗 (`nWeight`)、斜体 (`bItalic`)、下划线 (`bUnderline`) 等[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/include/StellarX/CoreTypes.h#L134-L142)。Table 默认使用框架的默认字体（例如微软雅黑）和颜色绘制文字。开发者可以直接访问并调整 `textStyle` 的属性，然后调用 `draw()` 重新绘制，使新的文本样式生效。例如，`table.textStyle.nHeight = 20; table.textStyle.color = RGB(0,0,128);` 将文字调大并改为深蓝色。值得注意的是，Table 内部的翻页按钮和页码标签也会同步使用 Table 的 `textStyle` 设置，以保持表格组件整体风格的一致。

### 示例

下面是一个使用 Table 控件的示例代码[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/README.md#L268-L276)[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/README.md#L278-L286)，演示如何创建表格、设置表头和数据、配置分页，并将其添加到窗口中显示：

```
// 创建一个表格控件，位置在 (50, 50)
auto myTable = std::make_unique<Table>(50, 50);

// 设置表头（列名称）
myTable->setHeaders({ "ID", "姓名", "年龄", "职业" });

// 添加数据行
myTable->setData({ "1", "张三", "25", "工程师" });
myTable->setData({ "2", "李四", "30", "设计师" });
myTable->setData({ "3", "王五", "28", "产品经理" });

// 设置每页显示2行数据
myTable->setRowsPerPage(2);

// 设置表格文本和样式
myTable->textStyle.nHeight = 16;                      // 文本字体高度
myTable->setTableBorder(RGB(50, 50, 50));             // 表格边框颜色（深灰）
myTable->setTableBk(RGB(240, 240, 240));              // 表格背景色（浅灰）

// 将表格控件添加到主窗口（假设 mainWindow 已创建）
mainWindow.addControl(std::move(myTable));
```

上述代码首先创建了一个 Table 实例，并依次设置表头和三行示例数据。接着将每页行数设为2行，这意味着数据将分页为两页（3条数据在每页2行的设置下将分两页显示）。然后调整了文本样式和颜色，将字体高度设为16像素，边框颜色设为灰色，背景色设为浅灰，以定制表格外观。最后，将配置好的表格控件添加到主窗口中，使其随着窗口一起绘制和响应用户操作。在实际应用中，添加控件后通常需要调用主窗口的 `draw()` 来绘制界面，及调用 `runEventLoop()` 进入消息循环以响应用户交互。通过上述流程，开发者即可在应用程序窗口中成功集成并显示一个功能完善的分页表格控件。

以上即为 StellarX 框架各主要组件的接口说明和使用方法。通过这些类和函数，开发者可以组合出完整的 GUI 应用。请注意，StellarX 基于 EasyX 实现，适用于 Windows 平台的教学和轻量工具开发场景。使用时需要确保 EasyX 图形库已初始化，并在应用结束前调用 `closegraph()` 释放图形资源。

