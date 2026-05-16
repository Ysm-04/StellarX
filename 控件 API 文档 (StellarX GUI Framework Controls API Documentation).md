# StellarX GUI Framework 控件 API 文档 / Controls API Documentation

版本 / Version: v3.1.0
日期 / Date: 2026-05-15

本文档基于当前最新源码与旧版 API 文档全量审校后整理，覆盖 StellarX v3.1.0 的核心类型、控件 API、布局语义、托管重绘语义、日志系统和当前限制。

This document is revised from a full audit of the current source code and the previous API documentation. It covers core types, control APIs, layout semantics, managed repaint semantics, logging utilities, and current limitations in StellarX v3.1.0.

## 1. 全局约定 / Global Conventions

### 1.1 包含方式 / Include

```cpp
#include "StellarX.h"
```

`StellarX.h` 是推荐的主入口头文件。当前源码实际包含 `CoreTypes.h`、`SxLog.h`、`Control.h`、`Canvas.h`、`Window.h`、`Button.h`、`Label.h`、`TextBox.h`、`Table.h`、`Dialog.h`、`MessageBox.h`、`TabControl.h`；该顺序属于框架内部实现，用户代码不应依赖内部包含顺序。

`StellarX.h` is the recommended main include header. The current source includes `CoreTypes.h`, `SxLog.h`, `Control.h`, `Canvas.h`, `Window.h`, `Button.h`, `Label.h`, `TextBox.h`, `Table.h`, `Dialog.h`, `MessageBox.h`, and `TabControl.h`; this order is an internal implementation detail and user code should not depend on it.

### 1.2 坐标与几何 / Coordinates and Geometry

- `localx/localy/localWidth/localHeight`：设计态父局部矩形，是布局解算基线。
- `x/y/width/height`：当前运行态绘制矩形。
- `Window` 顶层控件的父局部坐标等同于窗口客户区坐标。
- `Canvas`、`TabControl`、`Dialog` 等容器内部子控件使用父局部坐标添加，并由父容器映射到窗口世界坐标。
- 普通 resize、父容器重排、统一布局解算默认不自动回写 `local*`。
- 显式调用 `commitCurrentGeometryAsDesignRect()` 才会把当前运行态矩形提交为新的设计基线。

- `localx/localy/localWidth/localHeight`: design-time parent-local rectangle and layout baseline.
- `x/y/width/height`: current runtime drawing rectangle.
- Top-level controls under `Window` use the window client coordinate space as parent-local space.
- Children under `Canvas`, `TabControl`, and `Dialog` are added in parent-local coordinates and mapped to world coordinates by the parent container.
- Normal resize, parent relayout, and unified layout solving do not write back to `local*` by default.
- Call `commitCurrentGeometryAsDesignRect()` explicitly when the current runtime rectangle should become the new design baseline.

### 1.3 事件与绘制 / Events and Drawing

- 每个控件实现 `draw()` 和 `handleEvent(const ExMessage&)`。
- `handleEvent()` 返回 `true` 表示事件已被消费，父容器不应继续向下层分发。
- v3.1.0 采用托管重绘思路：事件处理阶段优先修改状态和登记脏区，绘制提交由 `Window` 在事件尾统一收口。
- `discardBackground()` 表示回贴并释放背景快照；`invalidateBackgroundSnapshot()` 表示仅作废快照，不回贴旧背景。

- Every control implements `draw()` and `handleEvent(const ExMessage&)`.
- `handleEvent()` returns `true` when the event is consumed.
- v3.1.0 uses managed repaint semantics: event handling primarily mutates state and registers dirty coverage; `Window` commits repaint work at the end of the event round.
- `discardBackground()` restores and releases the snapshot; `invalidateBackgroundSnapshot()` only invalidates the snapshot.

## 2. CoreTypes 基础类型 / Core Types

### 2.1 绘制样式枚举 / Drawing Style Enums

| 类型 / Type | 值 / Values | 说明 / Description |
| --- | --- | --- |
| `FillStyle` | `Horizontal`, `Vertical`, `FDiagonal`, `BDiagonal`, `Cross`, `DiagCross` | 图案填充样式 / hatch fill styles |
| `FillMode` | `Solid`, `Null`, `Hatched`, `Pattern`, `DibPattern` | 背景填充模式 / background fill modes |
| `LineStyle` | `Solid`, `Dash`, `Dot`, `DashDot`, `DashDotDot`, `Null` | 边框线型 / border line styles |
| `ControlShape` | `RECTANGLE`, `B_RECTANGLE`, `ROUND_RECTANGLE`, `B_ROUND_RECTANGLE`, `CIRCLE`, `B_CIRCLE`, `ELLIPSE`, `B_ELLIPSE` | 控件几何形状，`B_` 前缀表示无边框版本 / control geometry; `B_` means borderless |

### 2.2 控件行为枚举 / Control Behavior Enums

| 类型 / Type | 值 / Values | 说明 / Description |
| --- | --- | --- |
| `TextBoxmode` | `INPUT_MODE`, `READONLY_MODE`, `PASSWORD_MODE` | 文本框输入、只读、密码模式 / TextBox input, read-only, password modes |
| `ButtonMode` | `NORMAL`, `TOGGLE`, `DISABLED` | 普通、切换、禁用按钮模式 / normal, toggle, disabled button modes |
| `MessageBoxType` | `OK`, `OKCancel`, `YesNo`, `YesNoCancel`, `RetryCancel`, `AbortRetryIgnore` | 消息框按钮组合 / message box button groups |
| `MessageBoxResult` | `OK=1`, `Cancel=2`, `Abort=3`, `Retry=4`, `Ignore=5`, `Yes=6`, `No=7` | 消息框返回结果 / message box result |
| `TabPlacement` | `Top`, `Bottom`, `Left`, `Right` | 页签栏位置 / tab bar placement |

### 2.3 文本与形状结构 / Text and Shape Structs

```cpp
struct ControlText
{
    int nHeight = 0;
    int nWidth = 0;
    LPCTSTR lpszFace = "微软雅黑";
    COLORREF color = RGB(0, 0, 0);
    int nEscapement = 0;
    int nOrientation = 0;
    int nWeight = 0;
    bool bItalic = false;
    bool bUnderline = false;
    bool bStrikeOut = false;
};

struct RouRectangle
{
    int ROUND_RECTANGLEwidth = 20;
    int ROUND_RECTANGLEheight = 20;
};
```

`ControlText` 用于 Button、Label、TextBox、Table 等控件文本样式。`RouRectangle` 用于圆角矩形角部椭圆尺寸。

`ControlText` is used by controls such as Button, Label, TextBox, and Table. `RouRectangle` controls corner ellipse size for rounded rectangles.

### 2.4 布局类型 / Layout Types

旧接口保留：

```cpp
enum class LayoutMode { Fixed, AnchorToEdges };
enum class Anchor { NoAnchor = 0, Left = 1, Right, Top, Bottom };
```

v3.1.0 推荐接口：

```cpp
enum class AxisSizePolicy
{
    Stretch,
    FixedSize
};

enum class AxisAlignPolicy
{
    Start,
    End,
    Center,
    Proportional
};

struct AxisLayoutSpec
{
    bool anchorStart = false;
    bool anchorEnd = false;
    AxisSizePolicy sizePolicy = AxisSizePolicy::FixedSize;
    AxisAlignPolicy alignPolicy = AxisAlignPolicy::Start;
};

struct LayoutSpec
{
    AxisLayoutSpec horizontal{};
    AxisLayoutSpec vertical{};
};

struct LayoutCapability
{
    bool allowStretchX = true;
    bool allowStretchY = true;
};

struct ResolvedLayoutRect
{
    int localX = 0;
    int localY = 0;
    int width = 0;
    int height = 0;
    int worldX = 0;
    int worldY = 0;
};
```

说明：

- `AxisSizePolicy` 只决定尺寸是否变化。
- `AxisAlignPolicy` 只在固定尺寸时决定位置如何求解。
- `Proportional` 是位移策略，不是尺寸缩放策略。
- `LayoutCapability` 是控件硬能力边界，不等于默认策略。

Notes:

- `AxisSizePolicy` controls only whether size changes.
- `AxisAlignPolicy` controls positioning only when size is fixed.
- `Proportional` is a positioning strategy, not a scaling strategy.
- `LayoutCapability` is a hard capability boundary, not the default strategy.

## 3. Control 控件基类 / Base Control

`Control` 是所有控件的抽象基类，不直接实例化。

`Control` is the abstract base class of all controls and is not instantiated directly.

### 3.1 必须由派生类实现 / Required Overrides

```cpp
virtual void draw() = 0;
virtual bool handleEvent(const ExMessage& msg) = 0;
virtual bool model() const = 0;
```

### 3.2 几何读取 / Geometry Getters

```cpp
int getX() const;
int getY() const;
int getWidth() const;
int getHeight() const;
int getRight() const;
int getBottom() const;

int getLocalX() const;
int getLocalY() const;
int getLocalWidth() const;
int getLocalHeight() const;
int getLocalRight() const;
int getLocalBottom() const;
RECT getBoundsRect() const;
```

### 3.3 公开几何 setter / Public Geometry Setters

```cpp
virtual void setX(int x);
virtual void setY(int y);
virtual void setWidth(int width);
virtual void setHeight(int height);
```

公开 setter 面向用户使用，默认只修改运行态矩形，不自动回写 `local*`。容器或特殊控件可以在 override 中维护自己的运行态子树一致性。

Public setters are user-facing runtime geometry APIs. They do not automatically write back to `local*`. Containers and specialized controls may override them to keep their runtime subtree consistent.

### 3.4 可见性、dirty 与事件瞬时状态 / Visibility, Dirty State, Transient Mouse State

```cpp
virtual void setIsVisible(bool show);
virtual void setDirty(bool dirty);
bool IsVisible() const;
bool isDirty();
std::string getId() const;
bool didEventAffectVisual() const;
virtual bool clearTransientMouseState();
```

`clearTransientMouseState()` 用于清理 hover、Tooltip、临时按下态等鼠标瞬时状态。它不是新的事件分发入口，也不做新命中判断。

`clearTransientMouseState()` clears transient mouse states such as hover, Tooltip visibility, and temporary pressed state. It is not an event dispatch entry and does not perform new hit-testing.

### 3.5 父子关系和宿主窗口 / Parent and Host Window

```cpp
void setParent(Control* parent);
virtual void setHostWindow(Window* host);
Window* getHostWindow() const;
```

### 3.6 布局接口 / Layout APIs

```cpp
void setLayoutMode(StellarX::LayoutMode layoutMode_);
void setAnchor(StellarX::Anchor anchor_1, StellarX::Anchor anchor_2);
StellarX::Anchor getAnchor_1() const;
StellarX::Anchor getAnchor_2() const;
StellarX::LayoutMode getLayoutMode() const;

void setHorizontalLayoutSpec(const StellarX::AxisLayoutSpec& spec);
void setVerticalLayoutSpec(const StellarX::AxisLayoutSpec& spec);
void setHorizontalAnchors(bool left, bool right);
void setVerticalAnchors(bool top, bool bottom);
void setHorizontalSizePolicy(StellarX::AxisSizePolicy policy);
void setVerticalSizePolicy(StellarX::AxisSizePolicy policy);
void setHorizontalAlignPolicy(StellarX::AxisAlignPolicy policy);
void setVerticalAlignPolicy(StellarX::AxisAlignPolicy policy);

StellarX::AxisLayoutSpec getHorizontalLayoutSpec() const;
StellarX::AxisLayoutSpec getVerticalLayoutSpec() const;
const StellarX::LayoutSpec& getLayoutSpec() const;
const StellarX::LayoutCapability& getLayoutCapability() const;
void commitCurrentGeometryAsDesignRect();
```

旧 API 只作为兼容输入层。新代码建议直接使用 `setHorizontal*` / `setVertical*` 系列接口。

Legacy APIs are compatibility inputs. New code should prefer the `setHorizontal*` / `setVertical*` APIs.

### 3.7 托管重绘相关 / Managed Repaint APIs

```cpp
virtual RECT getManagedRepaintCoverageRect() const;
virtual RECT getManagedRepaintPersistentCoverageRect() const;
Control* getManagedRepaintRoot();
Control* getManagedRepaintDirectBranch(Control* root);
bool hasValidBackgroundSnapshot() const;
virtual bool hasManagedDirtySubtree() const;
virtual bool canCommitManagedPartialRepaint() const;
virtual void commitManagedRepaint();
void invalidateBackgroundSnapshot();
virtual void onWindowResize();
```

这些接口主要服务框架内部的托管重绘链路。普通用户通常只需要设置控件状态、调用 `setDirty(true)` 或依赖控件自身 setter。

These APIs primarily serve the internal managed repaint path. Normal users usually only change control state, call `setDirty(true)`, or use control-specific setters.

## 4. Window 窗口 / Window

`Window` 不是 `Control` 派生类，而是顶层窗口与控件管理器。

`Window` is not derived from `Control`. It is the top-level window and control manager.

### 4.1 构造 / Constructors

```cpp
Window(int width, int height, int mode);
Window(int width, int height, int mode, COLORREF bkcloc);
Window(int width, int height, int mode, COLORREF bkcloc, std::string headline);
~Window();
```

### 4.2 绘制与事件循环 / Drawing and Event Loop

```cpp
void draw();
void draw(std::string pImgFile);
int runEventLoop();
```

- `draw()` 初始化或重绘窗口全场景。
- `runEventLoop()` 驱动消息循环、事件分发、resize 收口和托管重绘提交。

### 4.3 背景与标题 / Background and Title

```cpp
void setBkImage(std::string pImgFile);
void setBkcolor(COLORREF c);
void setHeadline(std::string headline);
```

### 4.4 控件与 Dialog 管理 / Control and Dialog Management

```cpp
void addControl(std::unique_ptr<Control> control);
void addDialog(std::unique_ptr<Control> dialogs);
bool hasNonModalDialogWithCaption(const std::string& caption, const std::string& message) const;
std::vector<std::unique_ptr<Control>>& getControls();
```

### 4.5 查询 / Getters

```cpp
HWND getHwnd() const;
int getWidth() const;
int getHeight() const;
int getPendingWidth() const;
int getPendingHeight() const;
std::string getHeadline() const;
COLORREF getBkcolor() const;
IMAGE* getBkImage() const;
std::string getBkImageFile() const;
```

`getWidth()` / `getHeight()` 返回当前已生效客户区尺寸；`getPendingWidth()` / `getPendingHeight()` 返回 resize 收口前记录的待应用尺寸。

`getWidth()` / `getHeight()` return the effective client size. `getPendingWidth()` / `getPendingHeight()` return pending size values recorded before resize consolidation.

### 4.6 resize 与托管重绘 / Resize and Managed Repaint

```cpp
void pumpResizeIfNeeded();
void scheduleResizeFromModal(int w, int h);
bool isManagedDispatchActive() const;
void requestManagedRepaint(Control* source);
void requestManagedRepaint(Control* source, const RECT& previousCoverage);
void flushManagedRepaint();
```

这些接口属于高级/内部路径。业务代码一般只需要调用 `draw()` 和 `runEventLoop()`。带 `previousCoverage` 的重载用于在 hover / Tooltip 等临时状态清理后保留清理前覆盖范围，避免上层 overlay 补画判断丢失旧区域。

These are advanced/internal path APIs. Application code normally only calls `draw()` and `runEventLoop()`. The overload with `previousCoverage` preserves the pre-cleanup coverage of transient states such as hover / Tooltip, preventing upper overlay repaint checks from losing the old affected area.

## 5. Button 按钮 / Button

按钮支持普通、TOGGLE、禁用状态，多种形状、颜色、填充模式、文本截断和 Tooltip。

Button supports normal, toggle, disabled states, multiple shapes, colors, fill modes, text truncation, and Tooltip.

### 5.1 构造 / Constructors

```cpp
Button(int x, int y, int width, int height, const std::string text,
       StellarX::ButtonMode mode = StellarX::ButtonMode::NORMAL,
       StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE);

Button(int x, int y, int width, int height, const std::string text,
       COLORREF ct, COLORREF cf,
       StellarX::ButtonMode mode = StellarX::ButtonMode::NORMAL,
       StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE);

Button(int x, int y, int width, int height, const std::string text,
       COLORREF ct, COLORREF cf, COLORREF ch,
       StellarX::ButtonMode mode = StellarX::ButtonMode::NORMAL,
       StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE);
~Button();
```

`ct` 表示点击/选中态颜色，`cf` 表示默认态颜色，`ch` 表示悬停态颜色。

`ct` is pressed/selected color, `cf` is normal color, and `ch` is hover color.

### 5.2 事件回调 / Event Callbacks

```cpp
void setOnClickListener(std::function<void()> callback);
void setOnToggleOnListener(std::function<void()> callback);
void setOnToggleOffListener(std::function<void()> callback);
```

### 5.3 外观和状态 / Appearance and State

```cpp
void setbuttonMode(StellarX::ButtonMode mode);
void setROUND_RECTANGLEwidth(int width);
void setROUND_RECTANGLEheight(int height);
void setFillMode(StellarX::FillMode mode);
void setFillIma(StellarX::FillStyle ima);
void setFillIma(std::string imaName);
void setButtonBorder(COLORREF Border);
void setButtonFalseColor(COLORREF color);
void setButtonText(const char* text);
void setButtonText(std::string text);
void setButtonShape(StellarX::ControlShape shape);
void setButtonClick(BOOL click);
```

`setButtonClick(TRUE)` 在 NORMAL 模式下会触发 `onClickCallback`；在 TOGGLE 模式下，只有状态实际变化时才触发 `onToggleOnCallback` / `onToggleOffCallback`，状态不变不会重复触发。

`setButtonClick(TRUE)` triggers `onClickCallback` in NORMAL mode. In TOGGLE mode, callbacks are fired only when the state actually changes: `onToggleOnCallback` / `onToggleOffCallback`; no repeated callback is emitted when the state is unchanged.

### 5.4 查询 / Getters

```cpp
bool isClicked() const;
std::string getButtonText() const;
const char* getButtonText_c() const;
StellarX::ButtonMode getButtonMode() const;
StellarX::ControlShape getButtonShape() const;
StellarX::FillMode getFillMode() const;
StellarX::FillStyle getFillIma() const;
IMAGE* getFillImaImage() const;
COLORREF getButtonBorder() const;
COLORREF getButtonTextColor() const;
StellarX::ControlText getButtonTextStyle() const;
```

### 5.5 Tooltip

```cpp
void enableTooltip(bool on);
void setTooltipDelay(int ms);
void setTooltipFollowCursor(bool on);
void setTooltipOffset(int dx, int dy);
void setTooltipStyle(COLORREF text, COLORREF bk, bool transparent);
void setTooltipText(const std::string& s);
void setTooltipTextsForToggle(const std::string& onText, const std::string& offText);
```

当前 Tooltip 已完成显示、隐藏、清理和托管重绘 coverage 收口，但尚未实现智能选位。

`setTooltipOffset(dx, dy)` 当前仅在 `setTooltipFollowCursor(true)` 时生效；不跟随鼠标时，Tooltip 的 X 使用当前鼠标 X，Y 位于按钮底部。当前版本不会自动避让窗口边界、上层控件或 Dialog 遮挡。

Tooltip display, hide, cleanup, and managed repaint coverage are stabilized. Intelligent placement is not implemented yet.

`setTooltipOffset(dx, dy)` currently takes effect only when `setTooltipFollowCursor(true)` is enabled. When cursor following is disabled, Tooltip X uses the current mouse X and Tooltip Y is placed at the bottom of the button. This version does not automatically avoid client edges, upper controls, or Dialog occlusion.

## 6. Label 标签 / Label

Label 用于静态文本显示，不消费事件。

Label is for static text display and does not consume events.

### 6.1 构造 / Constructors

```cpp
Label();
Label(int x, int y, std::string text = "标签",
      COLORREF textcolor = BLACK,
      COLORREF bkColor = RGB(255, 255, 255));
```

### 6.2 接口 / APIs

```cpp
void draw() override;
void hide();
void setDirty(bool dirty) override;
void setTextdisap(bool key);
void setTextBkColor(COLORREF color);
void setText(std::string text);
```

### 6.3 内容驱动尺寸 / Content-driven Size

- `setText()` 会刷新运行态尺寸。
- `textStyle` 是公开字段，便于用户直接修改字体样式。
- 直接修改 `textStyle` 后，如需立即刷新运行态尺寸，请调用 `setDirty(true)`。
- `draw()` 只消费当前运行态矩形，不再作为新的几何决策入口。
- 统一布局器只负责安置 Label 位置，Label 宽高仍由内容驱动路径决定。

- `setText()` refreshes runtime size.
- `textStyle` is public for direct style changes.
- After direct `textStyle` changes, call `setDirty(true)` when immediate runtime size refresh is required.
- `draw()` consumes the current runtime rectangle and is no longer a geometry-decision entry.
- The unified layout solver positions Label; its size is still content-driven.

## 7. TextBox 文本框 / TextBox

TextBox 支持输入、只读和密码模式。当前输入仍基于 EasyX `InputBox`。

TextBox supports input, read-only, and password modes. Current input still relies on EasyX `InputBox`.

### 7.1 构造 / Constructor

```cpp
TextBox(int x, int y, int width, int height,
        std::string text = "",
        StellarX::TextBoxmode mode = StellarX::TextBoxmode::INPUT_MODE,
        StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE);
```

### 7.2 接口 / APIs

```cpp
void draw() override;
bool handleEvent(const ExMessage& msg) override;
void setMode(StellarX::TextBoxmode mode);
void setMaxCharLen(size_t len);
void setTextBoxshape(StellarX::ControlShape shape);
void setTextBoxBorder(COLORREF color);
void setTextBoxBk(COLORREF color);
void setText(std::string text);
std::string getText() const;
```

`setMaxCharLen()` 当前按 `std::string::size()` / 输入缓冲区字节长度处理，不等同于 Unicode 字符数；中文和其他多字节文本会按底层编码占用多个字节计算。

`setMaxCharLen()` currently uses `std::string::size()` / input-buffer byte length, not Unicode code-point count. Chinese and other multibyte text count by their encoded byte length.

`setTextBoxshape()` 当前只支持 `RECTANGLE`、`B_RECTANGLE`、`ROUND_RECTANGLE`、`B_ROUND_RECTANGLE`；传入圆形或椭圆形状会回退为 `RECTANGLE`。

`setTextBoxshape()` currently supports only `RECTANGLE`, `B_RECTANGLE`, `ROUND_RECTANGLE`, and `B_ROUND_RECTANGLE`; circle or ellipse shapes fall back to `RECTANGLE`.

### 7.3 当前边界 / Current Boundary

- 当前版本未引入焦点系统。
- 当前版本未引入键盘事件系统。
- TextBox 自绘输入体系尚未重写。

- Focus system is not implemented yet.
- Keyboard event system is not implemented yet.
- The self-rendered TextBox input pipeline is not rewritten yet.

## 8. Canvas 容器 / Canvas

Canvas 是通用容器控件，负责绘制自身背景并托管子控件。

Canvas is a general container control. It draws its own background and owns child controls.

### 8.1 构造 / Constructors

```cpp
Canvas();
Canvas(int x, int y, int width, int height);
~Canvas();
```

### 8.2 几何与绘制 / Geometry and Drawing

```cpp
void setX(int x) override;
void setY(int y) override;
void setWidth(int width) override;
void setHeight(int height) override;
void draw() override;
bool handleEvent(const ExMessage& msg) override;
bool clearTransientMouseState() override;
```

Canvas 的几何 setter 会同步重算受管理子控件的世界坐标。

Canvas geometry setters recalculate managed children world coordinates.

### 8.3 子控件与样式 / Children and Style

```cpp
void addControl(std::unique_ptr<Control> control);
void setShape(StellarX::ControlShape shape);
void setCanvasfillMode(StellarX::FillMode mode);
void setBorderColor(COLORREF color);
void setCanvasBkColor(COLORREF color);
void setCanvasLineStyle(StellarX::LineStyle style);
void setLinewidth(int width);
std::vector<std::unique_ptr<Control>>& getControls();
```

### 8.4 可见性、resize、托管重绘 / Visibility, Resize, Managed Repaint

```cpp
void setIsVisible(bool visible) override;
void setDirty(bool dirty) override;
void onWindowResize() override;
void requestRepaint(Control* parent) override;
bool hasManagedDirtySubtree() const override;
RECT getManagedRepaintCoverageRect() const override;
RECT getManagedRepaintPersistentCoverageRect() const override;
bool canCommitManagedPartialRepaint() const override;
void commitManagedRepaint() override;
```

说明：当前版本仍保留 Canvas 对 Table 的历史兜底绘制逻辑，后续等 Table 内部局部重绘体系完成后再移除。

Note: the current version still keeps the historical Canvas fallback special case for Table. It will be removed only after Table internal partial repaint is completed.

## 9. TabControl 选项卡 / TabControl

TabControl 管理“页签按钮 + 页面 Canvas”。外层接入统一布局，内部页签栏和页面区仍由自身专用逻辑管理。

TabControl manages tab buttons and page canvases. The outer control participates in unified layout, while internal tab bar and page area remain self-managed.

### 9.1 构造 / Constructors

```cpp
TabControl();
TabControl(int x, int y, int width, int height);
~TabControl();
```

### 9.2 几何、绘制、事件 / Geometry, Drawing, Events

```cpp
void setX(int x) override;
void setY(int y) override;
void setWidth(int width) override;
void setHeight(int height) override;
void draw() override;
bool handleEvent(const ExMessage& msg) override;
bool clearTransientMouseState() override;
```

### 9.3 页签管理 / Tab Management

```cpp
void add(std::pair<std::unique_ptr<Button>, std::unique_ptr<Canvas>>&& control);
void add(std::string tabText, std::unique_ptr<Control> control);
void setTabPlacement(StellarX::TabPlacement placement);
void setTabBarHeight(int height);
void setIsVisible(bool visible) override;
void onWindowResize() override;
int getActiveIndex() const;
void setActiveIndex(int idx);
int count() const;
int indexOf(const std::string& tabText) const;
```

`indexOf()` 找到匹配页签返回索引，未找到返回 `-1`。

`indexOf()` returns the matching index, or `-1` when not found.

### 9.4 dirty 与托管重绘 / Dirty State and Managed Repaint

```cpp
void setDirty(bool dirty) override;
void requestRepaint(Control* parent) override;
bool hasManagedDirtySubtree() const override;
RECT getManagedRepaintCoverageRect() const override;
RECT getManagedRepaintPersistentCoverageRect() const override;
bool canCommitManagedPartialRepaint() const override;
void commitManagedRepaint() override;
```

当前版本不重写 TabControl 内部页签栏/页面区布局应用路径，仅收口外层职责边界。

The current version does not rewrite TabControl internal tab/page layout application paths; it only consolidates the outer responsibility boundary.

## 10. Table 表格 / Table

Table 支持表头、数据、分页、页码和分页按钮。

Table supports headers, data rows, pagination, page labels, and page buttons.

### 10.1 构造 / Constructor

```cpp
Table(int x, int y);
~Table();
```

### 10.2 几何、绘制、事件 / Geometry, Drawing, Events

```cpp
void setX(int x) override;
void setY(int y) override;
void setWidth(int width) override;
void setHeight(int height) override;
void draw() override;
bool handleEvent(const ExMessage& msg) override;
bool clearTransientMouseState() override;
RECT getManagedRepaintCoverageRect() const override;
RECT getManagedRepaintPersistentCoverageRect() const override;
void onWindowResize() override;
```

当前版本 `setWidth()` 支持横向变化；`setHeight()` 是行为边界，明确不启用纵向 Stretch。

In the current version, `setWidth()` supports horizontal changes. `setHeight()` is a behavior boundary and does not enable vertical Stretch.

当前 `Table::draw()` 仍可能触发 `initTextWaH()`，刷新表格结构尺寸并同步 `localWidth/localHeight`。这是当前版本保留的技术债，后续版本应前移到数据变更或显式刷新阶段。

Currently, `Table::draw()` may still call `initTextWaH()`, refreshing structural size and synchronizing `localWidth/localHeight`. This is a known technical debt retained in this version; a later version should move it to data-change or explicit-refresh stages.

### 10.3 数据 / Data APIs

```cpp
void setHeaders(std::initializer_list<std::string> headers);
void setData(std::vector<std::string> data);
void setData(std::initializer_list<std::vector<std::string>> data);
void clearHeaders();
void clearData();
void resetTable();
```

`setData(std::vector<std::string>)` 表示单行追加；`setData(std::initializer_list<std::vector<std::string>>)` 表示多行追加，不是覆盖。若需要覆盖数据，请先调用 `clearData()` 或 `resetTable()`。

`setData(std::vector<std::string>)` appends a single row. `setData(std::initializer_list<std::vector<std::string>>)` appends multiple rows and does not overwrite existing data. To replace data, call `clearData()` or `resetTable()` first.

### 10.4 分页与样式 / Pagination and Style

```cpp
void setRowsPerPage(int rows);
void showPageButton(bool isShow);
void setTableBorder(COLORREF color);
void setTableBk(COLORREF color);
void setTableFillMode(StellarX::FillMode mode);
void setTableLineStyle(StellarX::LineStyle style);
void setTableBorderWidth(int width);
```

### 10.5 查询 / Getters

```cpp
int getCurrentPage() const;
int getTotalPages() const;
int getRowsPerPage() const;
bool getShowPageButton() const;
COLORREF getTableBorder() const;
COLORREF getTableBk() const;
StellarX::FillMode getTableFillMode() const;
StellarX::LineStyle getTableLineStyle() const;
std::vector<std::string> getHeaders() const;
std::vector<std::vector<std::string>> getData() const;
int getTableBorderWidth() const;
int getTableWidth() const;
int getTableHeight() const;
```

### 10.6 当前边界 / Current Boundaries

- X 轴可拉伸，Y 轴固定。
- 当前仍以整表正确性优先，未实现内部局部重绘体系。
- `draw()` 内仍可能触发结构尺寸刷新，这是后续 Table 专项技术债。
- 后续如果实现 Table 纵向拉伸，建议优先固定字体/行高，再根据目标高度推导 `rowsPerPage` 和页脚布局，而不是优先做字体同步缩放。

- X-axis stretch is supported, Y-axis size is fixed.
- Correctness is still prioritized through whole-table repaint behavior; internal partial repaint is not implemented yet.
- `draw()` may still trigger structural size refresh. This is deferred as Table-specific technical debt.
- If Table vertical stretch is implemented later, prefer fixed font/row height and derive `rowsPerPage` and footer layout from target height instead of scaling fonts first.

## 11. Dialog 对话框 / Dialog

Dialog 继承自 `Canvas`，支持模态和非模态对话框。

Dialog derives from `Canvas` and supports modal and modeless dialogs.

### 11.1 构造 / Constructor

```cpp
Dialog(Window& hWnd,
       std::string text,
       std::string message = "对话框",
       StellarX::MessageBoxType type = StellarX::MessageBoxType::OK,
       bool modal = true);
~Dialog();
```

### 11.2 结果与内容 / Result and Content

```cpp
void SetResultCallback(std::function<void(StellarX::MessageBoxResult)> cb);
std::string GetCaption() const;
std::string GetText() const;
StellarX::MessageBoxResult GetResult() const;

void SetTitle(const std::string& title);
void SetMessage(const std::string& message);
void SetType(StellarX::MessageBoxType type);
void SetModal(bool modal);
void SetResult(StellarX::MessageBoxResult result);
```

### 11.3 显示与生命周期 / Display and Lifecycle

```cpp
void draw() override;
bool handleEvent(const ExMessage& msg) override;
bool model() const override;
void Show();
void Close();
void recenterInHostWindow();
void performDelayedCleanup();
```

### 11.4 当前语义 / Current Semantics

- Dialog 在宿主窗口 resize、最大化、还原时只重新居中，不参与拉伸。
- Dialog 标题由自身直接绘制，不再依赖独立 Label 子控件。
- `Dialog::setInitialization(bool)` 已移除，请使用 `SetTitle()`、`SetMessage()`、`SetType()` 等公开接口驱动内容变化。
- 非模态 Dialog 遮挡区域会阻止底层控件误响应鼠标移动、hover、click 和 Tooltip。
- Dialog 关闭后会同步底层鼠标状态，清理旧 Tooltip 并恢复当前光标所在控件的 hover 状态。

- Dialog recenters on host resize/maximize/restore and does not stretch.
- Dialog title is drawn directly by Dialog itself and no longer depends on a separate Label child.
- `Dialog::setInitialization(bool)` has been removed. Use `SetTitle()`, `SetMessage()`, `SetType()`, and other public APIs to drive content updates.
- A modeless Dialog blocks underlying controls from false mouse move, hover, click, and Tooltip responses in its occluded area.
- After Dialog closes, underlying mouse state is synchronized, stale Tooltip is cleared, and hover is restored for the control currently under the cursor.

## 12. MessageBox 消息框工厂 / MessageBox Factory

`MessageBox` 是静态工厂类，不是控件。

`MessageBox` is a static factory class, not a control.

```cpp
static StellarX::MessageBoxResult showModal(
    Window& wnd,
    const std::string& text,
    const std::string& caption = "提示",
    StellarX::MessageBoxType type = StellarX::MessageBoxType::OK);

static void showAsync(
    Window& wnd,
    const std::string& text,
    const std::string& caption = "提示",
    StellarX::MessageBoxType type = StellarX::MessageBoxType::OK,
    std::function<void(StellarX::MessageBoxResult)> onResult = nullptr);
```

- `showModal()` 阻塞直到用户关闭消息框，并返回 `MessageBoxResult`。
- `showAsync()` 立即返回，用户选择通过回调异步传出。
- 非模态消息框带去重机制，避免相同标题和正文的对话框重复打开。

- `showModal()` blocks until the user closes the message box and returns `MessageBoxResult`.
- `showAsync()` returns immediately and reports the result through callback.
- Modeless message boxes include de-duplication to avoid opening identical caption/text dialogs repeatedly.

## 13. SxLog 日志系统常用用法 / Logging Utility Common Usage

SxLog 是框架的调试日志系统，使用宏 + RAII 提交方式。本节记录常用配置和宏用法，不是完整公开接口清单。

SxLog is the framework logging utility using macros and RAII-based line submission. This section documents common configuration and macro usage, not the complete public API list.

### 13.1 日志级别 / Log Levels

```cpp
enum class SxLogLevel
{
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
    Off
};
```

### 13.2 语言与 Tag 过滤 / Language and Tag Filter

```cpp
enum class SxLogLanguage { ZhCN, EnUS };
enum class SxTagFilterMode { None, Whitelist, Blacklist };
```

### 13.3 配置 / Configuration

```cpp
StellarX::SxLogger& logger = StellarX::SxLogger::Get();
logger.setMinLevel(StellarX::SxLogLevel::Debug);
logger.setLanguage(StellarX::SxLogLanguage::ZhCN);
logger.setTagFilter(StellarX::SxTagFilterMode::Whitelist, { "Layout", "Event" });
logger.clearTagFilter();
logger.enableConsole(true);
logger.enableFile("stellarx.log", true, 1024 * 1024);
logger.disableFile();
```

### 13.4 日志宏 / Logging Macros

```cpp
SX_LOG_TRACE("Tag") << "trace message";
SX_LOGD("Tag") << "debug message";
SX_LOGI("Tag") << SX_T("中文日志", "English log");
SX_LOGW("Tag") << "warning";
SX_LOGE("Tag") << "error";
SX_LOGF("Tag") << "fatal";
SX_TRACE_SCOPE("Layout", "resolve layout");
```

`SX_T(zh, en)` 只做运行时文本选择，不做编码转换。控制台是否乱码取决于终端 codepage、字体和运行环境。

`SX_T(zh, en)` only selects text at runtime and does not perform encoding conversion. Console display depends on terminal codepage, font, and environment.

日志宏当前采用 `if/else` 短路形式；不建议裸放在未加大括号的 `if/else` 单行分支中，避免 C/C++ dangling-else 绑定歧义。

The logging macros currently use an `if/else` short-circuit form. Avoid placing them naked in single-line `if/else` branches without braces to prevent C/C++ dangling-else ambiguity.

## 14. 推荐用法 / Recommended Usage

### 14.1 新布局 API

```cpp
auto canvas = std::make_unique<Canvas>(20, 20, 760, 500);
canvas->setHorizontalAnchors(true, true);
canvas->setVerticalAnchors(true, true);
canvas->setHorizontalSizePolicy(StellarX::AxisSizePolicy::Stretch);
canvas->setVerticalSizePolicy(StellarX::AxisSizePolicy::Stretch);

auto button = std::make_unique<Button>(620, 30, 120, 36, "Right");
button->setHorizontalAnchors(false, true);
button->setHorizontalSizePolicy(StellarX::AxisSizePolicy::FixedSize);
button->setHorizontalAlignPolicy(StellarX::AxisAlignPolicy::End);
```

### 14.2 Label 字体样式修改

```cpp
auto label = std::make_unique<Label>(20, 20, "状态", BLACK, RGB(255, 255, 255));
label->textStyle.nHeight = 24;
label->textStyle.color = RGB(200, 0, 0);
label->setDirty(true); // 触发内容驱动尺寸刷新 / refresh content-driven runtime size
```

### 14.3 MessageBox

```cpp
auto result = StellarX::MessageBox::showModal(
    wnd,
    "是否继续？",
    "确认",
    StellarX::MessageBoxType::YesNo);

StellarX::MessageBox::showAsync(
    wnd,
    "后台任务已开始",
    "提示",
    StellarX::MessageBoxType::OK,
    [](StellarX::MessageBoxResult result)
    {
        SX_LOGI("MessageBox") << "result=" << static_cast<int>(result);
    });
```

## 15. 当前限制与技术债 / Current Limitations and Technical Debt

以下事项属于后续版本专题，不在 v3.1.0 中实现：

The following items are deferred to future versions and are not implemented in v3.1.0:

- 焦点系统 / focus system.
- 键盘事件系统 / keyboard event system.
- TextBox 自绘输入体系 / self-rendered TextBox input pipeline.
- Table 内部局部重绘体系 / internal partial repaint system for Table.
- Table 纵向 Stretch / vertical Stretch for Table.
- Table 结构尺寸刷新从 draw 流程前移：当前 `Table::draw()` 仍可能触发 `initTextWaH()` 并刷新结构尺寸 / moving Table structural size refresh out of the draw path; currently `Table::draw()` may still call `initTextWaH()` and refresh structural size.
- 移除 Canvas 对 Table 的历史兜底特判 / removing the historical Canvas fallback special case for Table.
- TabControl 内部布局应用路径收口 / TabControl internal layout application path cleanup.
- Tooltip 完整智能选位 / full intelligent Tooltip placement.
- 全局字体自动缩放 / global automatic font scaling.
- 更完整的示例和 API 文档体系 / more complete examples and API documentation system.

## 16. 版本迁移提示 / Migration Notes

从 v3.0.2 迁移到 v3.1.0：

When migrating from v3.0.2 to v3.1.0:

- 旧 `setLayoutMode()` / `setAnchor()` 可继续使用，但推荐逐步迁移到新布局 API。
- 不要依赖普通 resize 自动回写 `local*`。
- 如果程序直接修改 `Label::textStyle`，请在需要立即刷新尺寸时调用 `setDirty(true)`。
- 不要调用已移除的 `Dialog::setInitialization(bool)`。
- 不要假设 Table 支持 Y 轴 Stretch。
- 不要依赖 Tooltip 自动避让窗口边界或上层控件。

- Legacy `setLayoutMode()` / `setAnchor()` still work, but new layout APIs are recommended.
- Do not rely on normal resize to write back to `local*`.
- If code directly modifies `Label::textStyle`, call `setDirty(true)` when immediate size refresh is needed.
- Do not call the removed `Dialog::setInitialization(bool)`.
- Do not assume Table supports Y-axis Stretch.
- Do not rely on Tooltip auto-avoidance for client edges or upper controls.
