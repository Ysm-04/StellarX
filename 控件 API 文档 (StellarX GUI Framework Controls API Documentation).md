# StellarX GUI Framework 控件 API 文档 / Controls API Documentation

版本 / Version: v4.1.0
日期 / Date: 2026-10-31

本文档以 StellarX v4.1.0 发布源码为准，覆盖核心类型、控件所有权、布局与托管重绘、逻辑焦点、原生输入路由、UTF-16 TextBox、IME、日志系统和当前限制。

This document follows the StellarX v4.1.0 release source. It covers core types, control ownership, layout and managed repaint, logical focus, native input routing, the UTF-16 TextBox and IME pipeline, logging, and current limitations.

## 1. 全局约定 / Global Conventions

### 1.1 包含方式 / Include

```cpp
#include "StellarX.h"
```

`StellarX.h` 是推荐的主入口头文件。v4.1.0 发布 18 个公共头：`Button.h`、`Canvas.h`、`Control.h`、`CoreTypes.h`、`Dialog.h`、`FocusManager.h`、`InputEvents.h`、`KeyboardRouting.h`、`Label.h`、`MessageBox.h`、`StellarX.h`、`SxLog.h`、`TabControl.h`、`Table.h`、`TextBox.h`、`TextInputRouting.h`、`TextSelectionRouting.h` 和 `Window.h`。业务代码通常只包含 `StellarX.h`，不应依赖其内部包含顺序。

`StellarX.h` is the recommended entry header. v4.1.0 ships 18 public headers: `Button.h`, `Canvas.h`, `Control.h`, `CoreTypes.h`, `Dialog.h`, `FocusManager.h`, `InputEvents.h`, `KeyboardRouting.h`, `Label.h`, `MessageBox.h`, `StellarX.h`, `SxLog.h`, `TabControl.h`, `Table.h`, `TextBox.h`, `TextInputRouting.h`, `TextSelectionRouting.h`, and `Window.h`. Applications normally include only `StellarX.h` and must not depend on its internal include order.

发布源码还包含 22 个 `src/*.cpp` 和 12 个 `src/internal/*.h`。私有头只供框架实现使用；直接集成源码时必须完整保留这些文件并配置私有包含目录，业务代码不得包含它们。更新版本时应整体替换这三组源码，不要只覆盖新增文件。

The source release also contains 22 `src/*.cpp` files and 12 `src/internal/*.h` files. Private headers are implementation-only. Keep the complete set and configure the private include directory when integrating source, but never include them from application code. Replace all three source groups as a unit when upgrading instead of copying only newly added files.

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

### 1.3 事件、输入与绘制 / Events, Input, and Drawing

- 每个具体控件实现 `draw()` 和 Pointer 兼容入口 `handleEvent(const ExMessage&)`。
- 键盘、正式文字、IME 临时组合和文字选区分别通过 `handleKeyEvent()`、`handleTextInputEvent()`、`handleCompositionEvent()` 和 `handleTextSelectionGesture()` 定向投递。
- WndProc 采集并规范化原生输入，`NativeInputQueue` 按严格序号跨线程交付；EasyX `EX_KEY` / `EX_MOUSE` 队列继续排空，但不再作为业务输入源。
- 键盘沿逻辑焦点父链冒泡；正式文字和 IME 组合只投递给绑定的焦点叶节点。窗口失活时暂停键盘业务投递，但保留可恢复的逻辑焦点。
- v4.1.0 继续采用托管重绘与延迟结构变更：事件阶段优先修改状态、登记脏区并排队控件树变更，`Window` 在安全点统一提交。
- `discardBackground()` 表示回贴并释放背景快照；`invalidateBackgroundSnapshot()` 表示仅作废快照，不回贴旧背景。

- Every concrete control implements `draw()` and the Pointer-compatible `handleEvent(const ExMessage&)` entry.
- Keyboard, committed text, transient IME composition, and text-selection gestures use the directed `handleKeyEvent()`, `handleTextInputEvent()`, `handleCompositionEvent()`, and `handleTextSelectionGesture()` entries.
- WndProc normalizes native input and `NativeInputQueue` delivers it across threads in strict sequence. EasyX `EX_KEY` / `EX_MOUSE` queues are still drained but are no longer business-input sources.
- Keyboard events bubble through the logical focus-parent chain; committed text and IME composition are delivered only to the bound focus leaf. Window deactivation suspends keyboard business routing while preserving restorable logical focus.
- v4.1.0 retains managed repaint and deferred structural mutation: event handling mutates state, records dirty coverage, and queues control-tree changes; `Window` commits them at a safe point.
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
    std::string faceName = "微软雅黑";
    COLORREF color = RGB(0, 0, 0);
    int nEscapement = 0;
    int nOrientation = 0;
    int nWeight = 0;
    bool bItalic = false;
    bool bUnderline = false;
    bool bStrikeOut = false;

    bool operator==(const ControlText& text) const;
    bool operator!=(const ControlText& text) const;
};

struct RouRectangle
{
    int ROUND_RECTANGLEwidth = 20;
    int ROUND_RECTANGLEheight = 20;
};
```

`ControlText` 用于 Button、Label、TextBox、Table 等控件文本样式。`faceName` 由对象拥有，不再保存外部字符指针。`RouRectangle` 用于圆角矩形角部椭圆尺寸。

`ControlText` is used by controls such as Button, Label, TextBox, and Table. `faceName` is owned by the object instead of storing an external character pointer. `RouRectangle` controls corner ellipse size for rounded rectangles.

### 2.4 稳定控件句柄 / Stable Control Handle

```cpp
struct ControlHandle
{
    std::uint64_t value = 0;

    explicit operator bool() const noexcept;
    bool operator==(const ControlHandle& other) const noexcept;
    bool operator!=(const ControlHandle& other) const noexcept;
};
```

- `value == 0` 表示无效句柄；非零值只保证当前进程本次运行期间的身份稳定。
- 句柄可用于 `removeControl()`、`findControl()` 和 `getControlHandles()`，但不证明对象仍然存活。
- 控件移除后旧句柄仍可安全比较或查询，`findControl()` 会返回 `nullptr`。

- `value == 0` is invalid; nonzero identity is stable only for the current process run.
- Use handles with `removeControl()`, `findControl()`, and `getControlHandles()`, but do not treat a handle as proof of object lifetime.
- A removed handle remains safe to compare or query; `findControl()` returns `nullptr`.

### 2.5 布局类型 / Layout Types

旧接口保留：

```cpp
enum class LayoutMode { Fixed, AnchorToEdges };
enum class Anchor { NoAnchor = 0, Left = 1, Right, Top, Bottom };
```

自 v4.0.0 起推荐的接口：

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

### 2.6 焦点身份与焦点结果 / Focus Identity and Results

```cpp
struct FocusTargetId
{
    ControlHandle owner{};
    std::uint32_t subNode = 0;
    explicit operator bool() const noexcept;
};

struct FocusScopeId
{
    std::uint64_t value = 0;
    explicit operator bool() const noexcept;
};

enum class FocusPolicy : std::uint8_t
{
    None = 0,
    Programmatic = 1u << 0,
    Pointer = 1u << 1,
    Tab = 1u << 2
};

enum class FocusReason : std::uint8_t
{
    Programmatic,
    Pointer,
    TabTraversal,
    TabRoving,
    ScopeActivation,
    ScopeRestore,
    InvalidationFallback
};

enum class FocusError : std::uint8_t
{
    None,
    InvalidTarget,
    NotFocusable,
    NotVisible,
    Disabled,
    OutsideActiveScope,
    InvalidScope,
    ReentrancyLimit
};

struct FocusChangeResult
{
    bool changed = false;
    FocusError error = FocusError::None;
    FocusTargetId focused{};
};
```

- `FocusTargetId` 是焦点参与节点的稳定身份；v4.1.0 内置控件均使用 `subNode == 0`，调用方不应自行发明非零节点编号。
- `FocusTargetSnapshot` 是采集时刻的可见性、启用状态、策略、Tab 顺序、文字能力和 IME caret 锚点快照。任何用户回调结束后都必须重新解析，不能长期缓存其中的状态或裸指针。
- `FocusScopeId` 用于 Dialog 等受限焦点区域。焦点请求返回可断言的 `FocusChangeResult`；重入活性保护会报告 `FocusError::ReentrancyLimit`。

- `FocusTargetId` is stable focus-node identity. All built-in v4.1.0 controls use `subNode == 0`; applications must not invent nonzero node numbers.
- `FocusTargetSnapshot` captures visibility, enabled state, policy, Tab order, text capabilities, and IME caret geometry at one instant. It must be re-resolved after user callbacks; never retain snapshot state or borrowed pointers.
- `FocusScopeId` identifies constrained focus regions such as Dialogs. Focus requests return an assertable `FocusChangeResult`; reentrancy liveliness protection reports `FocusError::ReentrancyLimit`.

### 2.7 规范化原生输入 / Normalized Native Input

`InputEvents.h` 定义 `KeyEvent`、`TextInputEvent`、`CompositionEvent`、`WindowActivationEvent`、`PointerEvent` 和 `NativeInputEvent`。关键语义如下：

- `NativeInputEvent::sequence` 在所有输入类型之间严格递增，消费者按该顺序处理。
- `KeyEvent` 同时保留原始 `virtualKey`、物理左右修饰键和采集时 IME 状态。`VK_PROCESSKEY` 表示单键归 IME 所有，不能据此伪造一个持久组合生命周期。
- `TextInputEvent::text` 与 `CompositionEvent::compositionText` 均为 `std::u16string`。正式文字和临时组合视图是两条独立路径。
- `CompositionStage` 包含 `Start`、`Update`、`Commit`、`Cancel`、`End`；正式提交仍由 `TextInputEvent` 写入编辑模型。
- 每个事件携带采集时的窗口激活快照，防止排队输入跨越失活/恢复边界后误投递。

`InputEvents.h` defines `KeyEvent`, `TextInputEvent`, `CompositionEvent`, `WindowActivationEvent`, `PointerEvent`, and `NativeInputEvent`:

- `NativeInputEvent::sequence` is strictly increasing across all input kinds and defines consumer order.
- `KeyEvent` preserves the raw `virtualKey`, physical left/right modifiers, and capture-time IME state. `VK_PROCESSKEY` denotes ownership of one key and must not fabricate persistent composition state.
- `TextInputEvent::text` and `CompositionEvent::compositionText` use `std::u16string`. Committed text and transient composition view are separate paths.
- `CompositionStage` contains `Start`, `Update`, `Commit`, `Cancel`, and `End`; committed text still enters the model through `TextInputEvent`.
- Each event carries a capture-time activation snapshot so delayed input cannot cross a deactivate/reactivate boundary.

### 2.8 定向路由事件 / Directed Routing Events

- `RoutedKeyEvent` 携带规范化按键、原始焦点和当前冒泡节点；`KeyRouteResult::stopBubble` 与 `preventFrameworkDefault` 是彼此独立的裁决。
- `RoutedTextInputEvent` 和 `RoutedCompositionEvent` 只在同步回调期间借用 UTF-16 视图；需要保留时必须复制。
- `RoutedTextSelectionGestureEvent` 使用 HWND 客户区坐标，并以 `Begin`、`Update`、`End`、`Cancel` 表达完整拖选生命周期。

- `RoutedKeyEvent` carries the normalized key, original focus leaf, and current bubbling node. `KeyRouteResult::stopBubble` and `preventFrameworkDefault` are independent decisions.
- `RoutedTextInputEvent` and `RoutedCompositionEvent` borrow UTF-16 views only for the synchronous callback; copy data that must outlive the call.
- `RoutedTextSelectionGestureEvent` uses HWND client coordinates and models a complete drag-selection lifecycle with `Begin`, `Update`, `End`, and `Cancel`.

## 3. Control 控件基类 / Base Control

`Control` 是所有控件的抽象基类，不直接实例化。

`Control` is the abstract base class of all controls and is not instantiated directly.

### 3.1 必须由派生类实现 / Required Overrides

```cpp
virtual void draw() = 0;
virtual bool handleEvent(const ExMessage& msg) = 0;
virtual bool model() const = 0;
```

`handleEvent()` 现在承接由定向 Pointer 管线转换出的兼容 `ExMessage`；EasyX `EX_KEY` 不再投递到该入口。派生控件的键盘业务应实现下面的定向接口。

`handleEvent()` now receives compatibility `ExMessage` values translated by the directed Pointer pipeline. EasyX `EX_KEY` is no longer delivered here; derived controls implement keyboard behavior through the directed APIs below.

### 3.2 焦点与定向输入 / Focus and Directed Input

```cpp
virtual StellarX::KeyRouteResult handleKeyEvent(
    StellarX::RoutedKeyEvent& event);
virtual void handleTextInputEvent(
    const StellarX::RoutedTextInputEvent& event);
virtual void handleCompositionEvent(
    const StellarX::RoutedCompositionEvent& event);
virtual bool handleTextSelectionGesture(
    const StellarX::RoutedTextSelectionGestureEvent& event);

StellarX::FocusChangeResult requestFocus(
    StellarX::FocusReason reason = StellarX::FocusReason::Programmatic);
void setFocusPolicy(StellarX::FocusPolicy policy);
StellarX::FocusPolicy getFocusPolicy() const noexcept;
void setTabIndex(std::int32_t value);
std::int32_t getTabIndex() const noexcept;
bool hasLogicalFocus() const noexcept;
```

默认定向处理器不消费输入。可聚焦控件通过 `setFocusPolicy()` 声明程序、Pointer 和 Tab 入口；`requestFocus()` 由所属 `Window` 的 `FocusManager` 执行。键盘路由沿稳定焦点身份的逻辑父链冒泡，文字与组合事件不冒泡。派生复合控件如需暴露内部焦点节点，应重写受保护的焦点查询、收集、应用与定向投递协议，且不得让解析得到的裸指针逃逸同步调用。

Directed handlers do nothing by default. Focusable controls declare programmatic, Pointer, and Tab entry paths with `setFocusPolicy()`; `requestFocus()` is executed by the owning Window's `FocusManager`. Keyboard routes bubble through the stable logical focus-parent chain, while text and composition do not bubble. Composite controls exposing internal focus nodes override the protected query, collection, application, and directed-dispatch protocol without allowing resolved raw pointers to escape the synchronous call.

### 3.3 几何读取 / Geometry Getters

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

### 3.4 公开几何 setter / Public Geometry Setters

```cpp
virtual void setX(int x);
virtual void setY(int y);
virtual void setWidth(int width);
virtual void setHeight(int height);
```

公开 setter 面向用户使用，默认只修改运行态矩形，不自动回写 `local*`。容器或特殊控件可以在 override 中维护自己的运行态子树一致性。

Public setters are user-facing runtime geometry APIs. They do not automatically write back to `local*`. Containers and specialized controls may override them to keep their runtime subtree consistent.

### 3.5 可见性、dirty 与事件瞬时状态 / Visibility, Dirty State, Transient Mouse State

```cpp
virtual void setIsVisible(bool show);
virtual void setDirty(bool dirty);
bool IsVisible() const;
bool isDirty();
std::string getId() const;
StellarX::ControlHandle getHandle() const noexcept;
bool didEventAffectVisual() const;
virtual bool clearTransientMouseState();
virtual void tick(ULONGLONG nowMs);
```

`clearTransientMouseState()` 用于清理 hover、Tooltip、临时按下态等鼠标瞬时状态。它不是新的事件分发入口，也不做新命中判断。

`clearTransientMouseState()` clears transient mouse states such as hover, Tooltip visibility, and temporary pressed state. It is not an event dispatch entry and does not perform new hit-testing.

`getHandle()` 返回控件稳定身份；`tick()` 由 Window 使用单调时钟驱动，默认实现为空，延时提示等控件可重写。

`getHandle()` returns stable control identity. `tick()` is driven by Window with a monotonic clock; its default implementation is empty and timed controls may override it.

### 3.6 父子关系和宿主窗口 / Parent and Host Window

```cpp
void setParent(Control* parent);
virtual void setHostWindow(Window* host);
Window* getHostWindow() const;
```

### 3.7 布局接口 / Layout APIs

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

### 3.8 托管重绘相关 / Managed Repaint APIs

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
- `runEventLoop()` 驱动消息循环、严格有序的原生输入排空、焦点与路由、resize 收口和托管重绘提交。

- `draw()` initializes or redraws the full scene.
- `runEventLoop()` drives the message loop, strictly ordered native-input drain, focus and routing, resize consolidation, and managed repaint commits.

### 4.3 背景与标题 / Background and Title

```cpp
void setBkImage(std::string pImgFile);
void setBkcolor(COLORREF c);
void setHeadline(std::string headline);
```

### 4.4 控件与 Dialog 管理 / Control and Dialog Management

```cpp
StellarX::ControlHandle addControl(std::unique_ptr<Control> control);
bool removeControl(StellarX::ControlHandle handle);
void clearControls();
Control* findControl(StellarX::ControlHandle handle);
const Control* findControl(StellarX::ControlHandle handle) const;
std::vector<StellarX::ControlHandle> getControlHandles() const;
StellarX::FocusManager& getFocusManager();
const StellarX::FocusManager& getFocusManager() const;
void addDialog(std::unique_ptr<Control> dialogs);
bool hasNonModalDialogWithCaption(const std::string& caption, const std::string& message) const;
```

- `addControl()` 总会立即返回新控件句柄；事务期间对象先进入 pending 队列，`findControl()` 在正式提交前不会返回它。
- `removeControl()` 返回目标是否命中且删除请求已排队/提交。`clearControls()` 同样会在事务期间延迟。
- `getControlHandles()` 返回当前已提交控件的句柄快照，不暴露容器所有权。

- `addControl()` always returns the new handle immediately. During a transaction, the object stays pending and `findControl()` cannot see it until commit.
- `removeControl()` reports whether the target was found and its removal was queued/committed. `clearControls()` is also deferred during transactions.
- `getControlHandles()` returns a snapshot of committed handles without exposing container ownership.

### 4.5 FocusManager

每个 `Window` 唯一拥有一个 `FocusManager`：

```cpp
StellarX::FocusManager& focus = window.getFocusManager();

focus.requestFocus(target, StellarX::FocusReason::Programmatic);
focus.clearFocus();
focus.focusedTarget();
focus.hasFocus(target);
focus.collectTabStops();
focus.advanceTabFocus(false); // false: forward, true: reverse

StellarX::FocusScopeId scope = focus.pushScope(scopeRoot, initialFocus);
focus.popScope(scope);
```

- 焦点管理器只长期保存稳定身份，不拥有 `Control`，也不缓存可跨回调使用的裸指针。
- 焦点回调中的重入请求使用当前事务回合的 latest-wins 单槽；超过 64 轮会返回 `ReentrancyLimit`。
- Tab 遍历每次重新收集当前作用域内的有效停靠点，不缓存首次绘制前的空结果。
- 控件进入待删除队列时，所属子树会在物理析构前立即从焦点模型失效。
- `pushScope()` / `popScope()` 为 Dialog 等受限区域提供焦点约束；普通业务通常通过 Dialog 自动管理作用域。

- The manager stores only stable identity, owns no Controls, and never retains raw pointers across callbacks.
- Reentrant focus requests use a latest-wins single slot for the current transaction round. More than 64 rounds returns `ReentrancyLimit`.
- Tab traversal recollects valid stops in the active scope on every call and does not cache an empty pre-draw result.
- A subtree becomes logically invalid as soon as removal is queued, before physical destruction.
- `pushScope()` / `popScope()` constrain focus for regions such as Dialogs; ordinary applications normally let Dialog manage the scope.

### 4.6 查询 / Getters

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

### 4.7 resize 与托管重绘 / Resize and Managed Repaint

```cpp
void pumpResizeIfNeeded();
void scheduleResizeFromModal(int w, int h);
bool isManagedDispatchActive() const;
void requestManagedRepaint(Control* source);
void requestManagedRepaint(Control* source, const RECT& previousCoverage);
void flushManagedRepaint();
void beginMutationScope();
void endMutationScope();
bool isMutationTransactionActive() const;
void requestFullSceneRedraw();
```

这些接口属于高级/内部路径。业务代码一般只需要调用 `draw()` 和 `runEventLoop()`。带 `previousCoverage` 的重载用于在 hover / Tooltip 等临时状态清理后保留清理前覆盖范围，避免上层 overlay 补画判断丢失旧区域。

These are advanced/internal path APIs. Application code normally only calls `draw()` and `runEventLoop()`. The overload with `previousCoverage` preserves the pre-cleanup coverage of transient states such as hover / Tooltip, preventing upper overlay repaint checks from losing the old affected area.

`beginMutationScope()` / `endMutationScope()` 允许嵌套且必须成对。最外层结束后才允许提交 pending mutation；`requestFullSceneRedraw()` 把无法安全局部修复的变化推迟到下一安全点整场景重绘。普通业务代码通常不需要直接调用这些接口。

`beginMutationScope()` / `endMutationScope()` may nest and must be paired. Pending mutations commit only after the outermost scope ends. `requestFullSceneRedraw()` defers changes that cannot be repaired locally to the next safe full-scene redraw. Normal application code rarely calls these APIs directly.

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

### 5.3 焦点与键盘 / Focus and Keyboard

```cpp
StellarX::KeyRouteResult handleKeyEvent(
    StellarX::RoutedKeyEvent& event) override;
```

- 启用状态的 Button 支持程序、Pointer 和 Tab 聚焦；`DISABLED` 模式会使当前焦点立即失效并触发同作用域回退。
- Space 按下时显示键盘按压态，抬起且焦点仍有效时激活；Enter 首次按下立即激活，不显示持续按压态。
- 自动重复的 Enter 不重复激活；窗口失活、焦点离开或控件禁用会清理尚未释放的 Space 按压态。
- 键盘按压态与 NORMAL 鼠标瞬时态、TOGGLE 持久状态相互独立。

- Enabled Buttons support programmatic, Pointer, and Tab focus. `DISABLED` invalidates current focus and triggers in-scope fallback.
- Space shows an armed state on key-down and activates on key-up while focus remains valid. Enter activates on the first key-down without a sustained pressed visual.
- Enter auto-repeat does not reactivate. Window deactivation, focus loss, or disabling clears an outstanding Space armed state.
- Keyboard armed state is independent from the NORMAL mouse transient state and the TOGGLE persistent state.

### 5.4 外观和状态 / Appearance and State

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

### 5.5 查询 / Getters

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

### 5.6 Tooltip

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

TextBox 是单行 UTF-16 自绘编辑器，支持输入、只读和密码模式。文字、caret、选区、临时组合串与水平滚动均在控件表面绘制，不再弹出 EasyX `InputBox`。

TextBox is a self-rendered single-line UTF-16 editor with input, read-only, and password modes. Text, caret, selection, transient composition, and horizontal scrolling are drawn on the control surface; EasyX `InputBox` is no longer used.

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
StellarX::KeyRouteResult handleKeyEvent(
    StellarX::RoutedKeyEvent& event) override;
void handleTextInputEvent(
    const StellarX::RoutedTextInputEvent& event) override;
void handleCompositionEvent(
    const StellarX::RoutedCompositionEvent& event) override;
bool handleTextSelectionGesture(
    const StellarX::RoutedTextSelectionGestureEvent& event) override;
void tick(ULONGLONG nowMs) override;
void onWindowResize() override;

void setMode(StellarX::TextBoxmode mode);
void setMaxCharLen(size_t len);
void setTextBoxshape(StellarX::ControlShape shape);
void setTextBoxBorder(COLORREF color);
void setTextBoxBk(COLORREF color);

[[nodiscard]] bool setTextUtf16(std::u16string text);
const std::u16string& getTextUtf16() const noexcept;

void setText(std::string text);
std::string getText() const;
bool tryGetText(std::string& output) const;
```

`setTextUtf16()` / `getTextUtf16()` 是无损 Unicode 接口。`setText(std::string)`、`getText()` 和 `tryGetText()` 保留为 Windows ACP 兼容边界：设置转换失败时不修改原值；`getText()` 转换失败返回空串；`tryGetText()` 转换失败返回 `false` 且保持输出参数不变。

`setTextUtf16()` / `getTextUtf16()` are lossless Unicode APIs. `setText(std::string)`, `getText()`, and `tryGetText()` remain Windows ACP compatibility boundaries: failed input conversion preserves the old value; `getText()` returns an empty string on failed output conversion; `tryGetText()` returns `false` without changing its output argument.

`setMaxCharLen()` 按 Unicode scalar 数量限制正式文本，移动、删除和选区不会拆开代理项对；传入 0 时保持当前限制不变。

`setMaxCharLen()` limits committed text by Unicode scalar count. Movement, deletion, and selection never split a surrogate pair; passing zero keeps the current limit.

`setTextBoxshape()` 当前只支持 `RECTANGLE`、`B_RECTANGLE`、`ROUND_RECTANGLE`、`B_ROUND_RECTANGLE`；传入圆形或椭圆形状会回退为 `RECTANGLE`。

`setTextBoxshape()` currently supports only `RECTANGLE`, `B_RECTANGLE`, `ROUND_RECTANGLE`, and `B_ROUND_RECTANGLE`; circle or ellipse shapes fall back to `RECTANGLE`.

### 7.3 编辑、选区与剪贴板 / Editing, Selection, and Clipboard

- 支持 Left/Right、Home/End、Backspace、Delete、Shift 扩展选区和 `Ctrl+A/C/X/V`。
- 鼠标按下、拖动和释放通过定向选区手势处理；选区和 caret 位置使用 Unicode scalar 安全边界。
- 只读模式允许聚焦、定位 caret、选择和复制，但拒绝正式文字、删除、剪切和粘贴修改。
- 密码模式使用掩码绘制，并阻止复制和剪切真实内容；程序仍可通过公开文本接口读取或设置值。
- 焦点离开会取消旧的临时编辑视觉并收口选区显示；再次获得焦点时以当前正式模型重新建立 caret 与水平滚动。

- Left/Right, Home/End, Backspace, Delete, Shift selection extension, and `Ctrl+A/C/X/V` are supported.
- Pointer press, drag, and release use directed selection gestures; selection and caret positions observe Unicode-scalar-safe boundaries.
- Read-only mode remains focusable and allows caret positioning, selection, and copy while rejecting committed text, deletion, cut, and paste mutations.
- Password mode renders a mask and blocks copying or cutting the real content. Applications may still read or set the value through public APIs.
- Focus loss retires transient editing visuals and selection display; focus gain rebuilds caret and horizontal scroll from the current committed model.

### 7.4 IME 与当前边界 / IME and Current Boundaries

- TextBox 内联绘制 IME 临时组合串；正式结果只由 `TextInputEvent` 提交一次。
- caret 与当前文本行以 HWND 客户区物理像素发布给窗口属主线程，用于微软拼音、搜狗拼音候选窗定位。
- 焦点转移、窗口失活、输入资格丢失或目标删除会取消旧组合，旧组合轮次不得惰性绑定到新的 TextBox。
- 当前只提供单行编辑，不包含多行排版、撤销/重做、富文本或 TSF 编辑器能力。

- TextBox renders transient IME composition inline; committed results are applied exactly once through `TextInputEvent`.
- Caret and current-line geometry are published in HWND client physical pixels for Microsoft Pinyin and Sogou Pinyin candidate positioning.
- Focus transfer, window deactivation, loss of input eligibility, or target removal cancels the old composition. An old composition session is never rebound lazily to a new TextBox.
- The current editor is single-line and does not provide multiline layout, undo/redo, rich text, or TSF editor services.

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
void tick(ULONGLONG nowMs) override;
```

Canvas 的几何 setter 会同步重算受管理子控件的世界坐标。

Canvas geometry setters recalculate managed children world coordinates.

### 8.3 子控件与样式 / Children and Style

```cpp
StellarX::ControlHandle addControl(std::unique_ptr<Control> control);
bool removeControl(StellarX::ControlHandle handle);
void clearControls();
Control* findControl(StellarX::ControlHandle handle);
const Control* findControl(StellarX::ControlHandle handle) const;
std::vector<StellarX::ControlHandle> getControlHandles() const;
void setShape(StellarX::ControlShape shape);
void setCanvasfillMode(StellarX::FillMode mode);
void setBorderColor(COLORREF color);
void setCanvasBkColor(COLORREF color);
void setCanvasLineStyle(StellarX::LineStyle style);
void setLinewidth(int width);
```

Canvas 的句柄与 pending 语义和 Window 一致。`findControl()` 只查询直接子控件且不返回尚未提交的对象；事务中失败或尚未命中的查询应在提交后重试。

Canvas follows the same handle and pending semantics as Window. `findControl()` searches direct children only and does not expose uncommitted objects; retry a lookup after transaction commit when needed.

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

### 8.5 焦点与路由 / Focus and Routing

Canvas 自身默认不成为 Tab stop，但会递归暴露当前已提交、可见且有效的子焦点节点。焦点查询和键盘、文字、组合、选区投递都按 `FocusTargetId` 重新解析；`findControl()` 只查直接子级，不能替代这套递归协议。单击 Canvas 内未命中任何子控件的空白区域会清除当前逻辑焦点。

Canvas is not a Tab stop by default, but recursively exposes committed, visible, valid child focus nodes. Focus queries and keyboard, text, composition, and selection dispatch re-resolve `FocusTargetId`; direct-child-only `findControl()` is not a substitute for this recursive protocol. Clicking blank Canvas space clears logical focus.

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
void tick(ULONGLONG nowMs) override;
```

### 9.3 页签管理 / Tab Management

```cpp
size_t addTab(std::unique_ptr<Button> button, std::unique_ptr<Canvas> page);
std::unique_ptr<Control> tryAddToPage(size_t pageIndex, std::unique_ptr<Control> control);
std::unique_ptr<Control> tryAddToPage(const std::string& tabText, std::unique_ptr<Control> control);
void setTabPlacement(StellarX::TabPlacement placement);
void setTabBarHeight(int height);
void setIsVisible(bool visible) override;
void onWindowResize() override;
int getActiveIndex() const;
bool setActiveIndex(int idx);
int count() const;
int indexOf(const std::string& tabText) const;
```

`addTab()` 的参数为空时返回 `size_t(-1)`；事务期间返回包含 pending 页签的逻辑索引，因此可立即用于同一事务中的 `tryAddToPage()`。`tryAddToPage()` 成功时返回空指针，失败时原样返还控件所有权。文本重载遇到重复标题时只匹配第一个页签。

`addTab()` returns `size_t(-1)` for null input. During a transaction it returns a logical index that includes pending tabs, so the same transaction may pass it directly to `tryAddToPage()`. `tryAddToPage()` returns null on success and returns ownership unchanged on failure. The text overload matches only the first duplicate caption.

`setActiveIndex()` 首次绘制前只记录默认索引；运行期索引有效或重复激活当前页时返回 `true`，非法索引返回 `false`。`indexOf()` 未命中返回 `-1`。

Before the first draw, `setActiveIndex()` records the default index. At runtime it returns `true` for a valid index or the already-active page and `false` for an invalid index. `indexOf()` returns `-1` when not found.

### 9.4 焦点与键盘 / Focus and Keyboard

- 页签 Button 与活动页面内控件使用各自真实 `ControlHandle` 进入同一焦点树，不创建重复身份。
- Tab 从当前作用域进入一个页签停靠点；Left/Right 仅在原始焦点位于页签 Button 时切换页签并移动焦点。
- 页签切换后，非活动页面不接收键盘、文字、组合或选区事件；当前页每次按最新控件树重新解析。
- 首次 `draw()` 前页签和页面的内部几何尚未完全物化；调用方不应缓存这一时刻的空遍历结果。

- Tab buttons and controls on the active page enter one focus tree with their real `ControlHandle` values; no duplicate identity is created.
- Tab enters one tab-strip stop for the composite control. Left/Right switches tabs and focus only when the original focus is a tab Button.
- After switching, inactive pages receive no keyboard, text, composition, or selection events; the active page is re-resolved from the latest tree.
- Internal tab/page geometry is not fully materialized before the first `draw()`; callers must not cache an empty traversal result from that moment.

### 9.5 dirty 与托管重绘 / Dirty State and Managed Repaint

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

### 10.6 焦点与分页按钮 / Focus and Pagination Buttons

上一页和下一页是 Table 内部持有的真实 Button，并以各自句柄参与 Tab 遍历和 Space/Enter 激活。到达首页或尾页后，被禁用按钮会立即失去焦点资格；FocusManager 优先在当前 Table/作用域中寻找仍有效的遍历回退目标。分页按钮在首次绘制完成后才物化，Tab 遍历因此每次重新收集，不保存旧的空快照。

Previous and Next are real Button objects owned by Table and participate in Tab traversal and Space/Enter activation with their own handles. At the first or last page, a disabled button immediately loses focus eligibility; FocusManager searches for a still-valid traversal fallback in the current Table/scope. Pagination buttons materialize after first draw, so Tab traversal recollects targets instead of retaining an old empty snapshot.

### 10.7 当前边界 / Current Boundaries

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

### 11.4 焦点、键盘与模态循环 / Focus, Keyboard, and Modal Loop

- 模态与非模态 Dialog 都建立独立焦点作用域，Tab/Shift+Tab 只在 Dialog 内部功能按钮间循环，不会进入底层窗口。
- 显式默认功能按钮可用 Enter 激活；聚焦按钮也支持 Space。Escape 执行对话框取消/关闭默认动作，但控件可通过路由裁决阻止框架默认动作。
- 模态循环持续排空原生输入、推进时钟和提交托管重绘，因此 Button 的 hover、按下态和焦点框保持更新。
- Dialog 关闭或重建内部 chrome 时，会先让旧焦点身份失效，再延迟释放控件，避免悬空焦点。
- 当前公开 Dialog 只管理标准标题、正文和功能按钮，不提供任意自定义控件挂载接口。

- Modal and modeless Dialogs create independent focus scopes. Tab/Shift+Tab cycles only through Dialog function buttons and never enters the underlying window.
- Enter activates the explicit default function button; focused buttons also support Space. Escape performs the Dialog cancel/close default, unless a control prevents the framework default through routing.
- The modal loop continues draining native input, advancing timers, and committing managed repaints so Button hover, pressed visuals, and focus rings remain current.
- Closing a Dialog or rebuilding its chrome invalidates old focus identity before deferred control destruction.
- The public Dialog currently manages standard title, message, and function buttons only; it does not expose arbitrary custom-control mounting.

### 11.5 当前语义 / Current Semantics

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
StellarX::SxLogConfig config = logger.getConfigCopy();
bool applied = logger.setConfig(config);
```

`setConfig()` 整体替换配置并返回是否成功。启用文件输出时，候选 sink 必须先成功打开；失败会返回 `false` 并保留旧配置和旧 sink。禁用文件输出、修改路径、append 模式或滚动阈值都会同步更新 sink。

`setConfig()` replaces the complete configuration and reports success. When file output is requested, the candidate sink must open first; failure returns `false` and preserves the previous configuration and sink. Disabling file output or changing path, append mode, or rotation threshold updates the sink accordingly.

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

### 14.2 稳定句柄与控件移除 / Stable Handles and Removal

```cpp
auto button = std::make_unique<Button>(20, 20, 120, 36, "Run");
const StellarX::ControlHandle handle = window.addControl(std::move(button));

if (Control* control = window.findControl(handle))
    control->setIsVisible(false);

window.removeControl(handle);
```

容器持有控件所有权。外部长期保存句柄，只有需要访问时才临时查询裸指针；请求删除后不要继续使用该指针。

Containers own controls. Store handles long-term and obtain a raw pointer only for temporary access; do not use that pointer after requesting removal.

### 14.3 Label 字体样式修改

```cpp
auto label = std::make_unique<Label>(20, 20, "状态", BLACK, RGB(255, 255, 255));
label->textStyle.nHeight = 24;
label->textStyle.faceName = "微软雅黑";
label->textStyle.color = RGB(200, 0, 0);
label->setDirty(true); // 触发内容驱动尺寸刷新 / refresh content-driven runtime size
```

### 14.4 焦点与 UTF-16 TextBox / Focus and UTF-16 TextBox

```cpp
auto input = std::make_unique<TextBox>(20, 80, 260, 36);
input->setTabIndex(10);
input->setMaxCharLen(64);
input->setTextUtf16(u"初始文本");

TextBox* inputView = input.get(); // 只在构造阶段临时使用
const StellarX::ControlHandle inputHandle = window.addControl(std::move(input));

window.getFocusManager().requestFocus(
    { inputHandle, 0 },
    StellarX::FocusReason::Programmatic);

std::u16string value;
if (auto* control = dynamic_cast<TextBox*>(window.findControl(inputHandle)))
    value = control->getTextUtf16();
```

长期保存句柄或 `FocusTargetId`，需要访问时重新查询。不要跨控件回调保留 `inputView` 这类裸指针。新 Unicode 代码优先使用 UTF-16 接口；窄字符串接口只用于兼容既有 Windows ACP 数据。

Store handles or `FocusTargetId` values long-term and resolve the control when needed. Do not retain raw pointers such as `inputView` across control callbacks. New Unicode code should use the UTF-16 APIs; narrow-string APIs exist for existing Windows ACP data.

### 14.5 MessageBox

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

以下事项属于后续版本专题，不在 v4.1.0 中实现：

The following items are deferred to future versions and are not implemented in v4.1.0:

- Table 内部局部重绘体系 / internal partial repaint system for Table.
- Table 纵向 Stretch / vertical Stretch for Table.
- Table 结构尺寸刷新从 draw 流程前移：当前 `Table::draw()` 仍可能触发 `initTextWaH()` 并刷新结构尺寸 / moving Table structural size refresh out of the draw path; currently `Table::draw()` may still call `initTextWaH()` and refresh structural size.
- 移除 Canvas 对 Table 的历史兜底特判 / removing the historical Canvas fallback special case for Table.
- TabControl 内部布局应用路径收口 / TabControl internal layout application path cleanup.
- Tooltip 完整智能选位 / full intelligent Tooltip placement.
- 全局字体自动缩放 / global automatic font scaling.
- TextBox 多行、撤销/重做、富文本和 TSF 编辑器能力 / multiline TextBox, undo/redo, rich text, and TSF editor services.
- Dialog 任意自定义控件挂载接口 / arbitrary custom-control mounting in Dialog.
- 多显示器混合 DPI 人工兼容性保证；当前仅完成单屏 100%/125%/150% 回归 / manual mixed-DPI multi-monitor guarantee; current evidence covers one monitor at 100%/125%/150%.

## 16. 版本迁移提示 / Migration Notes

从 v4.0.0 迁移到 v4.1.0：

When migrating from v4.0.0 to v4.1.0:

- `Control` 新增焦点、键盘、文字、组合串、选区和路由暂停/恢复虚接口，`Window` 与多个控件的对象布局也发生变化。所有静态库、应用程序和二进制派生控件必须使用同一版本源码与头文件重新编译。
- `handleEvent(EX_KEY)` 不再接收业务键盘消息。自定义控件应覆盖 `handleKeyEvent()`；文字控件还需声明文字/组合/选区能力并实现对应定向入口。
- TextBox 不再弹出 EasyX `InputBox`。无损 Unicode 代码应改用 `setTextUtf16()` / `getTextUtf16()`；旧 `std::string` API 继续按 Windows ACP 工作。
- 内置 Button、TextBox、Table 分页按钮、TabControl 页签和 Dialog 现在参与统一焦点树。自定义控件若不需要焦点，保持默认 `FocusPolicy::None`。
- Window 与 Canvas 空白处单击会清除焦点；Dialog 使用独立作用域；窗口失活会暂停输入路由而不销毁逻辑焦点。
- 直接源码集成必须整体更新 18 个公共头、12 个私有头和 22 个 `.cpp`，不能只复制 v4.1.0 新增文件。
- v4.1.0 需要链接 `imm32`；使用仓库 CMake 配置时会自动完成。

- `Control` adds virtual focus, keyboard, text, composition, selection, and routing suspend/resume APIs, while `Window` and several controls change object layout. Rebuild every static library, application, and binary-derived control with one consistent source/header version.
- `handleEvent(EX_KEY)` no longer receives business keyboard input. Custom controls override `handleKeyEvent()`; text controls also declare text/composition/selection capabilities and implement the matching directed entries.
- TextBox no longer opens EasyX `InputBox`. Use `setTextUtf16()` / `getTextUtf16()` for lossless Unicode; legacy `std::string` APIs remain Windows ACP boundaries.
- Built-in Button, TextBox, Table pagination buttons, TabControl tabs, and Dialog now participate in one focus tree. Custom controls that need no focus retain the default `FocusPolicy::None`.
- Clicking blank Window or Canvas space clears focus; Dialog uses an independent scope; window deactivation suspends input routing without destroying logical focus.
- Direct source integration must update all 18 public headers, 12 private headers, and 22 `.cpp` files together, not only files newly added in v4.1.0.
- v4.1.0 links `imm32`; the repository CMake configuration does this automatically.

从 v3.1.0 迁移到 v4.0.0：

When migrating from v3.1.0 to v4.0.0:

- `Window::getControls()`、`Canvas::getControls()` 已移除，使用句柄和 add/remove/clear/find/handles 接口。
- `Canvas::clearAllControls()` 改为 `clearControls()`。
- `ControlText::lpszFace` 改为拥有型字符串 `faceName`，赋值时不再传入临时 `c_str()`。
- `TabControl::add(pair)` 改为 `addTab(button, page)`；按页追加控件改用 `tryAddToPage(index, control)`。
- `TabControl::setActiveIndex()` 与 `SxLogger::setConfig()` 现在返回 `bool`。
- 回调中请求删除控件后不要继续使用裸指针；删除会在最外层事务结束时提交。
- 旧布局 API 仍可使用，Table 仍不支持 Y 轴 Stretch，Tooltip 仍不自动避让边界或上层控件。

- `Window::getControls()` and `Canvas::getControls()` were removed; use handles and the add/remove/clear/find/handles APIs.
- Replace `Canvas::clearAllControls()` with `clearControls()`.
- Replace `ControlText::lpszFace` with the owning `faceName` string; do not assign a temporary `c_str()`.
- Replace `TabControl::add(pair)` with `addTab(button, page)` and use `tryAddToPage(index, control)` for page children.
- `TabControl::setActiveIndex()` and `SxLogger::setConfig()` now return `bool`.
- Do not use a raw pointer after requesting removal inside a callback; removal commits when the outermost transaction ends.
- Legacy layout APIs remain available. Table still does not support Y-axis Stretch, and Tooltip still does not auto-avoid boundaries or upper controls.
