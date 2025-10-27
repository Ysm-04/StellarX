# 更新日志

StellarX 项目所有显著的变化都将被记录在这个文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
并且本项目遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

[English document](CHANGELOG.en.md)

## [v2.1.0] - 2025-10-27

**重点**：窗口可拉伸/最大化补强（EasyX + Win32）、布局管理器（HBox/VBox 第一阶段）、选项卡控件雏形；系统性修复黑边、最大化残影、频闪与“控件需交互才出现”等历史问题。并统一了**背景快照/恢复**、**按钮单行截断**、**Tooltip** 的机制。

### ✨ 新增

- **窗口拉伸 / 最大化补强（在 EasyX 基础上用 Win32 加固）**
  - 新增 `Window::enableResize(bool enable, int minW, int minH)`；运行时开关可拉伸并设置最小跟踪尺寸。
  - 子类化窗口过程：处理 `WM_GETMINMAXINFO / WM_SIZE / WM_EXITSIZEMOVE / WM_ERASEBKGND / WM_PAINT`，并启用 `WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS`，解决 EasyX 窗口原生不可拉伸问题。
  - **整窗背景绘制**：在 `WM_PAINT` 用 GDI 直写客户区（纯色/图片均支持），彻底消除黑边与最大化残影。
  - **一次性重绘**：合并连续 `WM_SIZE`，使用 `pendingW/H + needResizeDirty` 在主循环尾部统一置脏重绘，避免死循环与抖动。
- **布局管理器（第一阶段）**
  - 在 `Canvas` 引入布局元数据：`LayoutKind::{Absolute, HBox, VBox, Grid(预留), Flow(预留), Stack(预留)}` 与 `LayoutParams`（`margin{L,R,T,B}`、`fixedW/fixedH(-1=沿用)`、`weight`、`Align{Start,Center,End,Stretch}`）。
  - 实装 **HBox/VBox** 自动布局（容器内部自动排布；容器本身仍绝对定位，可嵌套）。
- **选项卡（Tabs）控件雏形**
  - 页签条与页面容器解耦；多页签切换；为透明主题提供**背景快照**避免叠影。
- **按钮文本单行截断（MBCS：中/英分治）**
  - 基于像素宽度阈值的 `...` 截断，避免半字节/半汉字。
- **悬停提示（Tooltip）**
  - 以 `Label` 为载体，支持延时出现、自动隐藏、自定义文案（默认回退到按钮文本）；使用控件级**背景快照/恢复**。

### 🔧 变更

- **Window 渲染路径**
  - 弱化“离屏 frame→整屏贴图”的依赖；在 `WM_PAINT` 走 **GDI 整窗背景 + EasyX 批量绘制**（`BeginBatchDraw/EndBatchDraw + FlushBatchDraw`）以抑制频闪。
  - 尺寸变化仅重建 `zoomBackground`（图片背景的缩放副本），显示延后到下一帧。
- **Control 基类**
  - 标准化 **captureBackground/restoreBackground**；统一透明/叠放控件行为。
  - 统一 `dirty` 语义：容器自身脏才重画背景，但**子控件每帧评估并按需绘制**，不再出现“容器不脏→子控件不画”的空窗。
- **Table**
  - 透明模式下，分页按钮与页码标签跟随表格的文本/填充风格，避免风格漂移。
  - **分页度量与整体居中**重做：可用宽度含表头；“上一页/下一页+页码”作为**一个块**水平居中。
  - 修复**背景快照区域**未计入表头导致恢复失败的问题。
- **事件循环**
  - 合并 `WM_SIZE` 到循环尾统一处理，降低输入延迟与重绘风暴。

### ✅ 修复

- **黑边 / 最大化残影 / 频闪**：`WM_ERASEBKGND` 返回 1 禁止系统擦背景；`WM_PAINT` 全面覆盖；清空裁剪区防止旧帧残留。
- **容器不画、控件需交互后才出现**：首帧与输入后触发**全树置脏**；子控件在容器未脏时也能正常绘制。
- **Table 翻页重叠与透明叠影**：修正快照区域、重算坐标并即时恢复+重绘。

### ⚠️ 可能不兼容

- 若外部代码直接访问 `Window` 私有成员（如 `dialogs`），请改用 `getControls()` / `getdialogs()`。
- `Table` 的分页与表头度量变化可能影响外部硬编码偏移；需要对齐新公式。
- 自定义控件若未遵循绘制前 `SetWorkingImage(nullptr)` 的约定，请自查。

### 📌 升级指引

1. **窗口背景**：将手工 `cleardevice()+putimage` 迁移到 `WM_PAINT` 的**整窗覆盖**流程。
2. **透明控件**：首绘前 `captureBackground()`，隐藏/覆盖时 `restoreBackground()`。
3. **布局**：容器设 `layout.kind = HBox/VBox`；子项用 `LayoutParams`（`margin/fixed/weight/align`）。
4. **按钮截断**：保持单次截断，避免重复 `...`。
5. **表格**：移除自绘分页，使用内置导航。

## [v2.0.1] - 2025 - 10 - 03

### 新增

- 新增示例：基于 StellarX 实现的 32 位寄存器查看工具（支持位取反、左移、右移，十六进制/十进制带符号/无符号切换，分组二进制显示）。
  - 示例路径：`examples/register-viewer/`
- `TextBox`新增`setText`API，可在外部设置文本框内容
- `TextBox::setText`API修改：在设置文本后立即调用`draw`方法重绘
- `Button`新增`setButtonClick`API,允许通过外部函数修改按钮的点击状态，并执行相应的回调函数
- ==所有文档更新对应英文版本==

## [v2.0.0] - 2025-09-21

### 概述
v2.0.0 为一次重大升级（major release）。本次发布新增对话框与消息框工厂（Dialog / MessageBox），并对事件分发、API语义与内部资源管理做了若干重要修复和改进。

部分 API/行为发生不向后兼容的变化（breaking changes）。

### 新增
- **对话框系统**:
  - 新增 `Dialog` 类，继承自 `Canvas`，用于构建模态与非模态对话框
  - 新增 `MessageBox` 工厂类，提供 `ShowModal`（同步阻塞）与 `ShowAsync`（异步回调）两种便捷API
  - 支持六种标准消息框类型：`OK`, `OKCancel`, `YesNo`, `YesNoCancel`, `RetryCancel`, `AbortRetryIgnore`
  - 自动处理对话框布局、背景保存与恢复、事件传播和生命周期管理

- **事件系统增强**:
  - 所有控件的 `handleEvent` 方法现在返回 `bool` 类型，表示是否消费了事件
  - 引入事件消费机制，支持更精细的事件传播控制
  - Window 类的事件循环现在优先处理对话框事件

- **控件状态管理**:
  - Control 基类新增 `dirty` 标志和 `setDirty()` 方法，统一管理重绘状态
  - 所有控件现在都实现了 `IsVisible()` 和 `model()` 方法

- **API 增强**:
  - Button 类新增 `setButtonFalseColor()` 方法
  - TextBox 类的 `setMaxCharLen()` 现在接受 `size_t` 类型参数
  - Window 类新增对话框管理方法和去重检测机制

### 重大变更（Breaking Changes）
- **API 签名变更**:
  - 所有控件的 `handleEvent(const ExMessage& msg)` 方法从返回 `void` 改为返回 `bool`
  - Control 基类新增纯虚函数 `IsVisible() const` 和 `model() const`，所有派生类必须实现

- **资源管理变更**:
  - Control 基类的样式保存从栈对象改为堆对象，使用指针管理
  - 增强了资源释放的安全性，所有资源都在析构函数中正确释放

- **事件处理逻辑**:
  - Window 的 `runEventLoop()` 方法完全重写，现在优先处理对话框事件
  - 引入了事件消费机制，事件被消费后不会继续传播

### 修复（Fixed）
- **内存管理**:
  - 修复 `Button::setFillIma()` 的内存泄漏问题
  - 修复 Control 基类析构函数中的资源释放问题
  - 修复 Dialog 类背景图像资源管理问题

- **布局与渲染**:
  - 修复 `Table` 组件的分页计算、列宽和行高越界问题
  - 修复 `Table` 中 `pX` 坐标累加错误导致的布局错乱
  - 修复 `Canvas::draw()` 中导致子控件不被绘制的 dirty 判定问题
  - 修复 `TextBox::draw()` 和 `restoreStyle()` 的不对称调用问题

- **事件处理**:
  - 修复窗口事件分发逻辑，确保对话框/顶层控件优先处理事件
  - 修复鼠标移出按钮区域时状态更新不及时的问题
  - 修复非模态对话框事件处理中的竞争条件

- **其他问题**:
  - 修复文本测量和渲染中的潜在错误
  - 修复对话框关闭后背景恢复不完全的问题
  - 修复多对话框场景下的 z-order 管理问题

### 改进（Changed）
- **代码质量**:
  - 重构了大量内部 API，增强异常安全性与可维护性
  - 使用智能指针和现代 C++ 特性替代裸 new/delete
  - 统一了代码风格和命名约定

- **性能优化**:
  - 优化了事件处理流程，减少不必要的重绘
  - 改进了对话框背景保存和恢复机制
  - 减少了内存分配和拷贝操作

- **文档与示例**:
  - 为所有新增功能添加了详细的使用示例
  - 完善了代码注释和 API 文档
  - 更新了 README 和 CHANGELOG 反映最新变化

## [1.1.0] - 2025-09-08

### 新增
- **Window 类 API 增强**:
  - 添加了完整的获取器(getter)方法集，提高类的封装性和可用性
  - `getHwnd()` - 获取窗口句柄，便于与原生 Windows API 集成
  - `getWidth()` - 获取窗口宽度
  - `getHeight()` - 获取窗口高度
  - `getHeadline()` - 获取窗口标题
  - `getBkcolor()` - 获取窗口背景颜色
  - `getBkImage()` - 获取窗口背景图片指针
  - `getControls()` - 获取控件管理容器的引用，允许迭代和操作已添加的控件

### 改进
- **API 一致性**: 为所有重要属性提供了对称的设置器(setter)和获取器(getter)方法
- **代码文档**: 进一步完善了类注释，使其更加清晰和专业

## [1.0.0] - 2025-09-08

### 发布摘要
首个稳定版本 (Stable Release)

### 新增

- StellarX 框架的第一个稳定版本
- 完整的控件库：按钮、标签、文本框、画布、表格和窗口
- 基于 CMake 的构建系统
- 详细的文档和示例代码
- **明确声明：本框架专为 Windows 平台设计**

### Released
-   **首次发布预编译的二进制库文件**，方便用户快速集成，无需从源码编译。
-   提供的发布包包括：
    -   `StellarX-v1.0.0-x64-Windows-msvc-x64.zip`
        -   **编译环境**: Visual Studio 2022 (MSVC v143)
        -   **架构**: x64 (64位)
        -   **运行时库**: `/MD` 
        -   **构建模式**: `Release | Debug`
        -   **内容**: 包含所有必要的头文件(`include/`)和静态库文件(``lib/StellarX-Debug.lib StellarX-Release.lib`)

### 核心特性
- 模块化设计，遵循 SOLID 原则
- 统一的控件接口（draw() 和 handleEvent()）
- 支持多种控件形状和样式
- 自定义事件处理回调
- 轻量级设计，无外部依赖

## [0.1.0] - 2025-08-15
### 新增
- 初始项目结构和核心架构
- Control 基类和基本事件处理系统
- 基础示例和文档设置