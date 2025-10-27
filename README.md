# 星垣 (StellarX) GUI Framework

[English document](README.en.md)

------

![GitHub all releases](https://img.shields.io/github/downloads/Ysm-04/StellarX/total)
[![Star GitHub Repo](https://img.shields.io/github/stars/Ysm-04/StellarX.svg?style=social&label=Star%20This%20Repo)](https://github.com/Ysm-04/StellarX)

![Version](https://img.shields.io/badge/Version-2.1.0-brightgreen.svg)
![Download](https://img.shields.io/badge/Download-2.1.0_Release-blue.svg)

![C++](https://img.shields.io/badge/C++-17+-00599C?logo=cplusplus&logoColor=white)
![Windows](https://img.shields.io/badge/Platform-Windows-0078D6?logo=windows)
![EasyX](https://img.shields.io/badge/Based_on-EasyX-00A0EA)
![License](https://img.shields.io/badge/License-MIT-blue.svg)
![Architecture](https://img.shields.io/badge/Architecture-Modular%20OOP-brightgreen)
![CMake](https://img.shields.io/badge/Build-CMake-064F8C?logo=cmake)

> **「繁星为界，轻若尘埃」** —— 一个为 Windows 平台打造的、极致轻量级、高度模块化的 C++ 原生 GUI 框架。

`星垣 (StellarX)` 反对臃肿，拒绝动辄数百 MB 的依赖、漫长编译与高门槛学习曲线，回归本质：以精简代码、清晰架构与高效率，解决桌面应用开发的核心需求。

这是一个**教学级、工具级**框架，帮助开发者深入理解 GUI 原理，并快速构建轻量工具。

---

## 🆕 v2.1.0 有何变化

- **中英文双语 API文档**
  - 文档详细介绍了每个类，以及API描述、功能和需要注意的地方，详细介绍了每个控件


- **窗口可拉伸/最大化补强（基于 EasyX + Win32）**  
  子类化窗口过程，处理 `WM_GETMINMAXINFO / WM_SIZE / WM_EXITSIZEMOVE / WM_ERASEBKGND / WM_PAINT`，启用 `WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS`。在 `WM_PAINT` 期进行**整窗背景绘制**（纯色/图片），彻底消除黑边与最大化“残影”；合并拉伸为**一次性重绘**。
- **布局管理器（第一阶段）**  
  `Canvas` 支持 `LayoutKind::{Absolute, HBox, VBox, Grid(预留), Flow(预留), Stack(预留)}` 与 `LayoutParams`（边距、固定尺寸、`weight`、`Align{Start,Center,End,Stretch}`）；实装 **HBox/VBox** 自动布局（容器仍绝对定位，可嵌套）。
- **选项卡（Tabs）雏形**  
  页签条与页面容器分离；透明主题下使用**背景快照**避免叠影。
- **按钮文本单行截断（MBCS：中/英分治）**  
  基于像素宽度阈值的 `...` 截断，杜绝半字符。
- **悬停提示（Tooltip）**  
  以 `Label` 实现，支持延时出现、自动隐藏；控件级**背景快照/恢复**。

详见 `CHANGELOG.md / CHANGELOG.en.md`。

---

## 📦 项目结构与设计哲学

星垣采用经典 **OOP** 与 **模块化** 设计，结构清晰：

```markdown
StellarX/
├── include/
│   └── StellarX/
│       ├── StellarX.h
│       ├── CoreTypes.h      # 唯一定义源（枚举/结构体）
│       ├── Control.h
│       ├── Button.h
│       ├── Window.h
│       ├── Label.h
│       ├── TextBox.h
│       ├── Canvas.h
│       ├── Dialog.h         # v2.0.0
│       ├── MessageBox.h     # v2.0.0
│       └── Table.h
├── src/
│   ├── Control.cpp
│   ├── Button.cpp
│   ├── Window.cpp
│   ├── Label.cpp
│   ├── TextBox.cpp
│   ├── Canvas.cpp
│   ├── Table.cpp
│   ├── Dialog.cpp           # v2.0.0
│   └── MessageBox.cpp       # v2.0.0
├── examples/
│   └── demo.cpp
├── docs/
│   └── CODE_OF_CONDUCT.md
├── CMakeLists.txt
├── CONTRIBUTING.md
├── CHANGELOG.md
├── CHANGELOG.en.md
├── Doxyfile
├── LICENSE
├──API 文档.md
├──API Documentation.en.md
└── README.md
```

**设计理念：**

1. **单一职责（SRP）**：每个类/文件只做一件事。
2. **依赖倒置（DIP）**：高层模块依赖抽象（`Control`），而非具体控件。
3. **开闭原则（OCP）**：继承 `Control` 可扩展新控件，无需修改既有代码。
4. **一致性**：所有控件统一 `draw()` / `handleEvent()` 接口。

## 🚀 核心特性

- **极致轻量**：除 EasyX 外无外部重量级依赖。
- **模块清晰**：`CoreTypes.h` 统一类型与枚举。
- **原生性能**：EasyX + Win32，执行高效、内存低占用（常见 <10MB）。
- **控件齐全**：Button、Label、TextBox、Canvas、Table、Dialog、MessageBox。
- **高度自定义**：颜色、形状（矩形/圆角/圆/椭圆）、填充、字体等皆有枚举。
- **简单直观 API**：OOP 设计，代码即文档。
- **标准工程结构**：include/src 分离，支持 CMake。
- **增强事件系统**：`handleEvent` 返回是否消费（自 v2.0.0）。
- **对话框系统**：支持模态/非模态，自动背景保存与恢复。

------

## ⚡ 快速开始（5 分钟上手）

> 从 [Releases](https://github.com/Ysm-04/StellarX/releases/latest) 获取预编译包。

### 环境要求

- **系统**：Windows 10+
- **编译器**：C++17（如 VS 2019+）
- **图形库**： [EasyX](https://easyx.cn/) 2022+（与编译器匹配）
- **构建**：CMake 3.12+（可选）

### 安装 EasyX

1. 下载 EasyX 最新版
2. 按 VS 版本安装匹配组件
3. 框架会自动链接，无需额外配置

### CMake 构建（推荐）

```
git clone https://github.com/Ysm-04/StellarX.git
cd StellarX
mkdir build && cd build
cmake ..
cmake --build .
./examples/Demo
```

### 手动集成

- 拷贝 `include` 与 `src`
- 配置头文件搜索路径：`include/StellarX/`
- 将全部 `.cpp` 加入工程

### 第一个可拉伸窗口

```c++
#include "StellarX.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) 
{
    //默认启动拉伸，当前尺寸为最小尺寸
    Window mainWindow(800, 600, 0, RGB(255,255,255), "我的星垣应用");
    mainWindow.draw();

    // 添加你的控件...
    // mainWindow.addControl(std::move(btn));

    mainWindow.runEventLoop();
    return 0;
}
```

> 实现要点：在 `WM_PAINT` 期进行**整窗背景绘制**（纯色/图片），并配合 EasyX 批量绘制抑制频闪与黑边。

------

## 📚 核心类型（`CoreTypes.h` 摘要）

### 枚举

| 枚举               | 描述       | 常用值                                                       |
| ------------------ | ---------- | ------------------------------------------------------------ |
| `ControlShape`     | 几何形状   | `RECTANGLE`, `B_RECTANGLE`, `ROUND_RECTANGLE`, `CIRCLE`, `ELLIPSE` |
| `ButtonMode`       | 按钮行为   | `NORMAL`, `TOGGLE`, `DISABLED`                               |
| `TextBoxMode`      | 文本框模式 | `INPUT_MODE`, `READONLY_MODE`                                |
| `FillMode`         | 填充模式   | `SOLID`, `NULL`, `HATCHED`                                   |
| `FillStyle`        | 图案样式   | `HORIZONTAL`, `CROSS`                                        |
| `LineStyle`        | 线型       | `SOLID`, `DASH`, `DOT`                                       |
| `MessageBoxType`   | 消息框类型 | `OK`, `OKCancel`, `YesNo`, ...                               |
| `MessageBoxResult` | 结果       | `OK`, `Cancel`, `Yes`, `No`, `Abort`, `Retry`, `Ignore`      |

### 结构体

| 结构体         | 描述                                   |
| -------------- | -------------------------------------- |
| `ControlText`  | 字体/字号/颜色/粗体/斜体/下划线/删除线 |
| `RouRectangle` | 圆角矩形的角部椭圆尺寸                 |

------

## 🧩 控件库

### 1) 基础控件

| 控件    | 头文件      | 描述        | 关键点                                                       |
| ------- | ----------- | ----------- | ------------------------------------------------------------ |
| Button  | `Button.h`  | 多功能按钮  | 形状/模式、悬停/点击色、回调，**单行截断** + **Tooltip**（v2.1.0） |
| Label   | `Label.h`   | 文本标签    | 透明/不透明背景，自定义字体                                  |
| TextBox | `TextBox.h` | 输入/显示框 | 输入/只读，整合 EasyX `InputBox`                             |

### 2) 容器控件

| 控件   | 头文件     | 描述                                                         |
| ------ | ---------- | ------------------------------------------------------------ |
| Canvas | `Canvas.h` | 父容器，自定义边框/背景，**内置 HBox/VBox 自动布局**（v2.1.0） |
| Window | `Window.h` | 顶层容器，消息循环与分发，**可拉伸**（v2.1.0）               |

### 3) 高级控件

| 控件   | 头文件     | 描述     | 关键点                                                    |
| ------ | ---------- | -------- | --------------------------------------------------------- |
| Table  | `Table.h`  | 数据表格 | 分页/表头/列宽自动、翻页控件重叠/透明叠影已修复（v2.1.0） |
| Dialog | `Dialog.h` | 对话框   | 支持模态/非模态，自动布局与背景保存/恢复                  |

### 4) 静态工厂

| 控件       | 头文件         | 描述       | 关键点                              |
| ---------- | -------------- | ---------- | ----------------------------------- |
| MessageBox | `MessageBox.h` | 消息框工厂 | 静态 API；支持模态/非模态；内置去重 |

------

## 📐 布局管理（HBox/VBox）

- **容器级**：`Canvas::layout.kind = HBox / VBox / Absolute (默认)`；可选容器 `padding`。
- **子项级**：`LayoutParams`
  - `margin{L,R,T,B}`
  - `fixedW/fixedH`（`-1` 表示沿用控件当前尺寸）
  - `weight`（主轴分配权重：HBox→宽、VBox→高）
  - `alignX/alignY` = `Start/Center/End/Stretch`（用于次轴方向对齐/拉伸）

**示例**（三列水平自适应，整体垂直拉伸）：

```c++
Canvas row(20, 20, 760, 120);
row.layout.kind = LayoutKind::HBox;

auto mk = [](int w, int h, float weight){
    auto c = std::make_unique<Control>(0,0,w,h);
    c->layout.marginL = 8; c->layout.marginR = 8;
    c->layout.alignY = LayoutParams::Stretch;
    c->layout.weight = weight;
    return c;
};
row.addControl(mk(100,40, 1.f));
row.addControl(mk(100,40, 2.f));
row.addControl(mk(100,40, 1.f));
```

------

## 🗂 选项卡（Tabs，早期版本）

- 页签条（按钮组） + 页面容器（`Canvas`）
- 透明主题：页面区域**背景快照**切换，避免叠影
- API 草图：`tabs.addPage(title, canvasPtr); tabs.setActive(index);`

------

## ✂️ 文本单行截断 & 🫧 Tooltip

- **按钮截断**：多字节字符集下**中/英分治**，基于像素宽度阈值追加 `...`
- **Tooltip**：延时出现、自动隐藏；默认文字=按钮文本，可自定义；使用控件级**背景快照/恢复**

------

## 🧊 透明背景与背景快照

- **通用约定**：首绘前 `captureBackground(rect)`，隐藏/覆盖前 `restoreBackground()`
- **Table**：快照区域**包含表头**；翻页后立即恢复 + 重绘，分页控件整体居中

------

## 🔧 高级主题与最佳实践

- 自定义控件：继承 `Control`，实现 `draw()` / `handleEvent()`
- 性能：
  - **脏矩形**：状态改变时置 `dirty=true`，按需重绘
  - **避免额外 `cleardevice()`**：背景已由 `WM_PAINT` 统一处理
  - 绘制前确保 `SetWorkingImage(nullptr)` 将输出落到屏幕
- 事件消费：处理后返回 `true` 终止传播

------

## ⚠️ 适用与限制

- 不适合高性能游戏/复杂动画；极端 DPI 需复核度量
- 暂无无障碍能力
- Windows 专用，不跨平台
- 复杂商业前端建议用 Qt / wxWidgets / ImGui / Electron

------

## 📜 许可证

MIT（见 `LICENSE`）。

## 👥 贡献指南

- 遵循现有 C++ 风格
- 新特性需附示例与 README 更新
- 提交前请自测，并说明变更动机
- Bug/想法请提 Issue

## 🙏 致谢

- 感谢 [EasyX](https://easyx.cn/)
- 感谢推崇**简洁/高效/清晰**的开发者

------

**星辰大海，代码为舟。**

## 📞 支持与反馈

- 查看 [examples/](examples/)
- 查阅 [更新日志](CHANGELOG.md / CHANGELOG.en.md)
- 在 GitHub 提交 Issue