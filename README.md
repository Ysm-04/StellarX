# 星垣 (StellarX) GUI Framework

![Version](https://img.shields.io/badge/Version-1.0.0-brightgreen.svg) <!-- 新增版本徽章 -->
![Download](https://img.shields.io/badge/Download-1.0.0_Release-blue.svg) <!-- 新增下载徽章 -->

![C++](https://img.shields.io/badge/C++-17+-00599C?logo=cplusplus&logoColor=white)
![Windows](https://img.shields.io/badge/Platform-Windows-0078D6?logo=windows)
![EasyX](https://img.shields.io/badge/Based_on-EasyX-00A0EA)
![License](https://img.shields.io/badge/License-MIT-blue.svg)
![Architecture](https://img.shields.io/badge/Architecture-Modular%20OOP-brightgreen)
![CMake](https://img.shields.io/badge/Build-CMake-064F8C?logo=cmake)

> **「繁星为界，轻若尘埃」** —— 一个为Windows平台打造的、极致轻量级、高度模块化的C++原生GUI框架。

`星垣 (StellarX)` 诞生于对现代GUI框架"过度臃肿"的反抗。它拒绝动辄数百MB的依赖、漫长的编译时间和复杂的学习曲线，选择回归本质：用最精简的代码、最清晰的架构和最高的效率，解决桌面应用开发的核心需求。

它是一个**纯粹的教学级、工具级框架**，旨在让开发者深入理解GUI原理，并快速构建轻量级Windows工具。

---

## 📦 项目结构与设计哲学

星垣框架采用经典的**面向对象**和**模块化**设计，项目结构清晰规范：

```
StellarX/
├── include/               # 头文件目录
│   └── StellarX/          # 框架头文件
│       ├── StellarX.h     # 主包含头文件 - 一键引入整个框架
│       ├── CoreTypes.h    # ★ 核心 ★ - 所有枚举、结构体的唯一定义源
│       ├── Control.h      # 抽象基类 - 定义所有控件的统一接口
│       ├── Button.h       # 按钮控件
│       ├── Window.h       # 窗口管理
│       ├── Label.h        # 标签控件
│       ├── TextBox.h      # 文本框控件
│       ├── Canvas.h       # 画布容器
│       └── Table.h        # 表格控件
├── src/                   # 源文件目录
│   ├── Control.cpp
│   ├── Button.cpp
│   ├── Window.cpp
│   ├── Label.cpp
│   ├── TextBox.cpp
│   ├── Canvas.cpp
│   └── Table.cpp
├── examples/              # 示例代码目录
│   └── demo.cpp          # 基础演示
├── docs/                  # 文档目录
│   └── CODE_OF_CONDUCT.md # 行为准则
├── CMakeLists.txt        # CMake 构建配置
├── CONTRIBUTING.md       # 贡献指南
├── CHANGELOG.md          # 更新日志
├── Doxyfile              # Doxygen 配置
├── LICENSE               # MIT 许可证
└── README.md             # 项目说明
```

**设计理念：**
1.  **单一职责原则 (SRP)**: 每个类/文件只负责一件事。
2.  **依赖倒置原则 (DIP)**: 高层模块（如`Window`）不依赖低层模块（如`Button`），二者都依赖其抽象（`Control`）。
3.  **开闭原则 (OCP)**: 通过继承`Control`基类，可以轻松扩展新的控件，而无需修改现有代码。
4.  **一致性**: 所有控件共享统一的`draw()`和`handleEvent()`接口。

## 🚀 核心特性

-   **极致的轻量级**: 核心库编译后仅 ~1.2MB，无任何外部依赖。生成的应用程序小巧玲珑。
-   **清晰的模块化架构**: 使用`CoreTypes.h`统一管理所有类型，消除重复定义，极大提升可维护性。
-   **原生C++性能**: 直接基于EasyX和Win32 API，提供接近原生的执行效率，内存占用极低（通常<10MB）。
-   **丰富的控件库**: 提供按钮、标签、文本框、表格、画布等常用控件，满足基本桌面应用需求。
-   **高度可定制化**: 从控件颜色、形状（矩形、圆角、圆形、椭圆）到填充模式、字体样式，均有详尽枚举支持，可轻松定制。
-   **简洁直观的API**: 采用经典的面向对象设计，代码即文档，学习成本极低。
-   **标准项目结构**: 采用标准的include/src分离结构，支持CMake构建，易于集成和使用。

## ⚡ 快速开始（5分钟上手）

> **🎯 最新版本下载**
> 从 [GitHub Releases](https://github.com/Ysm-04/StellarX/releases/latest) 下载预编译的库文件和头文件，即可快速集成到你的项目中。

### 环境要求

-   **操作系统**: Windows 10 或更高版本
-   **编译器**: 支持C++17的编译器 (如: **Visual Studio 2019+**)
-   **图形库**: [EasyX](https://easyx.cn/) (2022版本或更高，安装时请选择与您编译器匹配的版本)
-   **构建工具**: CMake 3.12+ (可选，推荐使用)

### 安装 EasyX
1. 访问 [EasyX 官网](https://easyx.cn/) 下载最新版本
2. 运行安装程序，选择与您的 Visual Studio 版本匹配的版本
3. 安装完成后，无需额外配置，星垣框架会自动链接 EasyX

### 方法一：使用CMake构建（推荐）

1.  **克隆项目**:
    ```bash
    git clone https://github.com/Ysm-04/StellarX.git
    cd StellarX
    ```

2.  **生成构建系统**:
    ```bash
    mkdir build
    cd build
    cmake ..
    ```

3.  **编译项目**:
    ```bash
    cmake --build .
    ```

4.  **运行示例**:
    ```bash
    ./examples/Demo
    ```

### 方法二：手动集成到现有项目

1.  **将include和src目录复制**到您的项目中
2.  **配置包含路径**，确保编译器可以找到`include/StellarX/`目录
3.  **将所有.cpp文件**添加到您的项目中编译

### 创建你的第一个星垣应用

```cpp
// 只需包含这一个头文件即可使用所有功能
#include "StellarX.h"

// 程序入口点（请使用WinMain以获得更好的兼容性）
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {

	// 1. 创建一个640x480的窗口，背景为白色，标题为"我的应用"
	Window mainWindow(640, 480, 0, RGB(255, 255, 255), "我的第一个星垣应用");

	// 2. 创建一个按钮 (使用智能指针管理)
	auto myButton = std::make_unique<Button>(
		250, 200, 140, 40, // x, y, 宽度, 高度
		"点击我",           // 按钮文本
		StellarX::ButtonMode::NORMAL,
		StellarX::ControlShape::ROUND_RECTANGLE
	);

	// 3. 为按钮设置点击事件（使用Lambda表达式）
	myButton->setOnClickListener([]() {
		MessageBox(nullptr, "Hello, 星垣！",  "问候", MB_OK | MB_ICONINFORMATION);
		});

	// 4. (可选)设置按钮样式
	myButton->textStyle.nHeight = 20;
	myButton->textStyle.color = RGB(0, 0, 128); // 深蓝色文字
	myButton->setButtonBorder(RGB(0, 128, 255)); // 蓝色边框

	// 5. 将按钮添加到窗口
	mainWindow.addControl(std::move(myButton));

	// 6. 绘制窗口
	mainWindow.draw();

	// 7. 进入消息循环，等待用户交互
	mainWindow.runEventLoop();

	return 0;
}
```

5.  **编译并运行！** 您将看到一个带有蓝色圆角按钮的窗口，点击它将会弹出消息框。

## 📚 核心类型详解 (`CoreTypes.h`)

星垣框架的所有视觉和行为属性都通过`CoreTypes.h`中定义的精美枚举和结构体来控制。

### 枚举类型 (Enums)

| 枚举类型           | 描述         | 常用值                                                       |
| :----------------- | :----------- | :----------------------------------------------------------- |
| **`ControlShape`** | 控件几何形状 | `RECTANGLE`, `B_RECTANGLE`, `ROUND_RECTANGLE`, `CIRCLE`, `ELLIPSE`等 |
| **`ButtonMode`**   | 按钮行为模式 | `NORMAL`（普通）, `TOGGLE`（切换）, `DISABLED`（禁用）       |
| **`TextBoxMode`**  | 文本框模式   | `INPUT_MODE`（输入）, `READONLY_MODE`（只读）                |
| **`FillMode`**     | 图形填充模式 | `SOLID`（实心）, `NULL`（空心）, `HATCHED`（图案）等         |
| **`FillStyle`**    | 图案填充样式 | `HORIZONTAL`（水平线）, `CROSS`（十字线）等                  |
| **`LineStyle`**    | 边框线型     | `SOLID`（实线）, `DASH`（虚线）, `DOT`（点线）等             |

### 结构体 (Structs)

| 结构体            | 描述                                                         |
| :---------------- | :----------------------------------------------------------- |
| **`ControlText`** | 封装了所有文本样式属性，包括字体、大小、颜色、粗体、斜体、下划线、删除线等。 |

**使用示例：**
```cpp
// 创建一个复杂的文本样式
StellarX::ControlText myStyle;
myStyle.nHeight = 25; // 字体高度
myStyle.lpszFace = _T("微软雅黑"); // 字体
myStyle.color = RGB(255, 0, 0); // 红色
myStyle.nWeight = FW_BOLD; // 粗体
myStyle.bUnderline = true; // 下划线

// 应用于控件
myLabel->textStyle = myStyle;
myButton->textStyle = myStyle;
```

## 🧩 控件库大全

### 1. 基础控件

| 控件        | 头文件      | 描述          | 关键特性                                                |
| :---------- | :---------- | :------------ | :------------------------------------------------------ |
| **Button**  | `Button.h`  | 多功能按钮    | 支持多种模式/形状/状态，可设置悬停/点击颜色，自定义回调 |
| **Label**   | `Label.h`   | 文本标签      | 支持背景透明/不透明，自定义字体样式                     |
| **TextBox** | `TextBox.h` | 输入框/显示框 | 支持输入和只读模式，集成EasyX的`InputBox`               |

### 2. 容器控件

| 控件       | 头文件     | 描述                                                     |
| :--------- | :--------- | :------------------------------------------------------- |
| **Canvas** | `Canvas.h` | 容器控件，可作为其他控件的父容器，支持自定义边框和背景。 |
| **Window** | `Window.h` | 顶级窗口，所有控件的最终容器，负责消息循环和调度。       |

### 3. 高级控件

| 控件      | 头文件    | 描述     | 关键特性                                                     |
| :-------- | :-------- | :------- | :----------------------------------------------------------- |
| **Table** | `Table.h` | 数据表格 | **框架功能亮点**，支持分页显示、自定义表头和数据、自动计算列宽、翻页按钮。 |

**表格控件示例：**
```cpp
// 创建一个表格
auto myTable = std::make_unique<StellarX::Table>(50, 50);

// 设置表头
myTable->setHeaders({ "ID", "姓名", "年龄", "职业" });

// 添加数据行
myTable->addDataRow({ "1", "张三", "25", "工程师" });
myTable->addDataRow({ "2", "李四", "30", "设计师" });
myTable->addDataRow({ "3", "王五", "28", "产品经理" });

// 设置每页显示2行
myTable->setRowsPerPage(2);

// 设置表格样式
myTable->textStyle.nHeight = 16;
myTable->setTableBorder(RGB(50, 50, 50));
myTable->setTableBackground(RGB(240, 240, 240));

// 添加到窗口
mainWindow.addControl(std::move(myTable));
```

## 🔧 高级主题与最佳实践

### 1. 自定义控件
您可以通过继承`Control`基类来创建自定义控件。只需实现`draw()`和`handleEvent()`两个纯虚函数即可。

```cpp
class MyCustomControl : public StellarX::Control {
public:
    MyCustomControl(int x, int y) : Control(x, y, 100, 100) {}
    void draw() override {
        // 您的自定义绘制逻辑
        setfillcolor(RGB(255, 100, 100));
        fillrectangle(x, y, x + width, y + height);
    }
    void handleEvent(const ExMessage& msg) override {
        // 您的自定义事件处理逻辑
        if (msg.message == WM_LBUTTONDOWN && isInControl(msg.x, msg.y)) {
            // 处理点击
        }
    }
};
```

### 2. 布局管理
当前版本星垣主要采用**绝对定位**。对于简单布局，您可以通过计算坐标来实现。对于复杂布局，可以考虑：
- 在`Canvas`中嵌套控件，实现相对定位。
- 自行实现简单的流式布局或网格布局管理器。

### 3. 性能优化
- **脏矩形渲染**: 框架内部已实现，控件状态改变时`dirty=true`，仅在需要时重绘。
- **图像资源**: 使用`IMAGE`对象加载图片后，可重复使用，避免多次加载。
- **减少循环内操作**: 在`draw()`和`handleEvent()`中避免进行重型计算。

## ⚠️ 重要限制与适用场景

**星垣框架的设计目标是轻便、清晰和教学价值，因此它明确** **不适用于** **以下场景：**

-   **高性能游戏或复杂动画**: 渲染基于EasyX的CPU软件渲染，性能有限。
-   **需要高DPI缩放的应用**: 对高DPI显示器的支持有限，界面可能显示不正确。
-   **需要无障碍功能的应用**: 未提供对屏幕阅读器等辅助技术的支持。
-   **跨平台应用**: 深度依赖Windows API和EasyX，无法直接在Linux/macOS上运行。
-   **复杂的商业软件前端**: 缺乏高级控件（如树形图、富文本框、选项卡、高级列表等）和成熟的自动布局管理器。

**如果您需要开发上述类型的应用，请考虑使用以下成熟方案：**
-   **Qt**: 功能极其强大，跨平台，适合大型商业应用。
-   **wxWidgets**: 原生外观，跨平台。
-   **Dear ImGui**: 即时模式GUI，非常适合工具和调试界面。
-   **Web技术栈 (Electron/CEF)**: 适合需要Web技术的场景。

## 📜 许可证 (License)

本项目采用 **MIT 许可证**。

您可以自由地：
-   使用、复制、修改、合并、出版发行、散布、再授权及销售本框架的副本。
-   将其用于私人或商业项目。

唯一要求是：
-   请在您的项目中保留原始的版权声明。

详见项目根目录的 [LICENSE](LICENSE) 文件。

## 👥 贡献指南 (Contributing)

我们欢迎任何形式的贡献！如果您想为星垣框架添砖加瓦，请阅读以下指南：

1.  **代码风格**: 请遵循现有的Google C++规范风格（使用空格缩进，大括号换行等）。
2.  **新增功能**: 必须提供**示例代码**，并更新本README文档的相关部分。
3.  **提交PR**: 请确保您的代码在提交前已经过测试，并描述清楚您的更改内容和动机。
4.  **问题反馈**: 如果您发现了Bug或有新的想法，欢迎在GitHub提交Issue。

详细贡献指南请参阅 [CONTRIBUTING.md](CONTRIBUTING.md)。

## 🙏 致谢 (Acknowledgements)

-   感谢 [EasyX Graphics Library](https://easyx.cn/) 为这个项目提供了简单易用的图形基础，使得用C++教学GUI编程成为可能。
-   感谢所有追求**简洁、高效、清晰**编码理念的开发者，你们是"星垣"诞生的灵感源泉。

---

**星辰大海，代码为舟。**

愿 `星垣 (StellarX)` 能成为您探索GUI世界的一颗可靠基石，无论是用于学习、教学还是创造实用的工具。

## 📞 支持与反馈

如果您在使用过程中遇到问题或有任何建议：
1. 查看 [示例代码](examples/) 获取使用参考
2. 查阅 [更新日志](CHANGELOG.md) 了解最新变化
3. 在GitHub仓库提交Issue反馈问题

---
*星垣框架 - 轻若尘埃，繁星为界*