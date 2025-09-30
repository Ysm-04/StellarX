# 更新日志

StellarX 项目所有显著的变化都将被记录在这个文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
并且本项目遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [v2.0.1] - 2025 - 09 - 30

### 新增

- 文本框新增`setText`接口，可在外部设置文本框内容

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