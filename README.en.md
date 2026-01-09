# StellarX GUI Framework README

[中文README](README.md)

------

![GitHub all releases](https://img.shields.io/github/downloads/Ysm-04/StellarX/total)
 [![Star GitHub Repo](https://img.shields.io/github/stars/Ysm-04/StellarX.svg?style=social&label=Star%20This%20Repo)](https://github.com/Ysm-04/StellarX)

![Version](https://img.shields.io/badge/Version-2.3.2-brightgreen.svg)
 ![Download](https://img.shields.io/badge/Download-2.3.2_Release-blue.svg)

![C++](https://img.shields.io/badge/C++-17+-00599C?logo=cplusplus&logoColor=white)
 ![Windows](https://img.shields.io/badge/Platform-Windows-0078D6?logo=windows)
 ![EasyX](https://img.shields.io/badge/Based_on-EasyX-00A0EA)
 ![License](https://img.shields.io/badge/License-MIT-blue.svg)
 ![Architecture](https://img.shields.io/badge/Architecture-Modular%20OOP-brightgreen)
 ![CMake](https://img.shields.io/badge/Build-CMake-064F8C?logo=cmake)

> **“Bounded by the stars, light as dust.”** — An ultra-lightweight, highly modular, native C++ GUI framework for Windows.

`StellarX` rejects bloat: no hundreds-of-MB dependencies, no marathon builds, and no steep learning curve. Back to the essence—clean code, clear architecture, and high efficiency to solve the core needs of desktop app development.

This is a **teaching-grade and tooling-grade** framework that helps developers understand GUI fundamentals and quickly build lightweight utilities.

------

### 🆕V3.0.0 - Major Update

[CHANGELOG.en.md](CHANGELOG.en.md)

### ✨ New Features

- **SxLog**: A lightweight logging system with support for log levels, tag filtering, bilingual (Chinese/English) output, and console/file logging with optional file rolling.
- **TabControl**: Improved tab switching logic to ensure the current tab is closed before the target tab is opened.
- **Improved Setters**: Setters now only update state and mark as dirty, with drawing handled by the unified redraw flow.

### ⚙️ Changes

- **TabControl Default Active Tab**: Default active tab logic clarified. First, set the active index without immediate drawing; after the first draw, the tab is activated.

### ✅ Fixes

- **TabControl::setActiveIndex Crash**: Fixed crash when setting the default active tab before the first draw.
- **TabControl Rendering Glitch**: Fixed issue where non-active tabs were incorrectly drawn when switching visibility.

### ⚠️ Breaking Changes

- **Button Size APIs Removed**: `getButtonWidth()` and `getButtonHeight()` removed; use `getWidth()` and `getHeight()` instead.
- **No Immediate Drawing for Setters**: Setters like `setText()` no longer trigger immediate drawing.

### 📌 Upgrade Guide

- **Button**: Replace `getButtonWidth()` / `getButtonHeight()` with `getWidth()` / `getHeight()`.
- **Setters**: Ensure a redraw mechanism after calling setters like `setText()`.

## 📦 Project Structure & Design Philosophy

StellarX adopts classic **OOP** and **modular** design with a clear structure:

```
StellarX/
├── include/
│   └── StellarX/
│       ├── StellarX.h
│       ├── CoreTypes.h      # single source of truth (enums/structs)
│       ├── Control.h
│       ├── Button.h
│       ├── Window.h
│       ├── Label.h
│       ├── TextBox.h
│       ├── TabControl.h #v2.2.0
│       ├── Canvas.h
│       ├── Dialog.h         
│       ├── MessageBox.h     
│       └── Table.h
├── src/
│   ├── Control.cpp
│   ├── Button.cpp
│   ├── Window.cpp
│   ├── Label.cpp
│   ├── TextBox.cpp
│   ├── Canvas.cpp
│   ├── TabControl.cpp #v2.2.0
│   ├── Table.cpp
│   ├── Dialog.cpp           
│   └── MessageBox.cpp       
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

**Design Philosophy:**

1. **Single Responsibility (SRP):** each class/file does exactly one thing.
2. **Dependency Inversion (DIP):** high-level modules depend on abstractions (`Control`), not concrete controls.
3. **Open/Closed (OCP):** extend by inheriting from `Control` without modifying existing code.
4. **Consistency:** unified `draw()` / `handleEvent()` across all controls.

## 🚀 Core Features

- **Ultra-lightweight:** no heavyweight external dependencies besides EasyX.
- **Clear modules:** `CoreTypes.h` unifies types and enums.
- **Native performance:** EasyX + Win32 for efficient execution and low memory (often <10 MB).
- **Complete control set:** Button, Label, TextBox, Canvas, Table, Dialog, MessageBox, **TabControl**.
- **Highly customizable:** colors; shapes (rectangle/rounded/circle/ellipse); fills; fonts—switchable via enums.
- **Simple, intuitive API:** OOP design with clear semantics—code as documentation.
- **Standard project layout:** split `include/src`, CMake-friendly, easy to integrate or use out of the box.

------

## ⚡ Quick Start (5 minutes)

> Get the prebuilt package from [Releases](https://github.com/Ysm-04/StellarX/releases/latest).

### Requirements

- **OS:** Windows 10+
- **Compiler:** C++17 (e.g., VS 2019+)
- **Graphics:** [EasyX](https://easyx.cn/) 2022+ (matching your compiler)
- **Build:** CMake 3.12+ (optional)

### Install EasyX

1. Download the latest EasyX
2. Install components matching your Visual Studio version
3. The framework links automatically—no extra config needed

### Build with CMake (recommended)

```
git clone https://github.com/Ysm-04/StellarX.git
cd StellarX
mkdir build && cd build
cmake ..
cmake --build .
./examples/Demo
```

### Manual Integration

- Copy `include` and `src`
- Add header search path: `include/StellarX/`
- Add all `.cpp` files to your project

### First Resizable Window

```
#include "StellarX.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) 
{
    // Resizing enabled by default; current size is the minimum size
    Window mainWindow(800, 600, 0, RGB(255,255,255), "My StellarX App");
    mainWindow.draw();

    // Add your controls...
    // mainWindow.addControl(std::move(btn));

    mainWindow.runEventLoop();
    return 0;
}
```

> Implementation note: perform **full-window background drawing** (solid/image) during `WM_PAINT`, and combine with EasyX batch drawing to suppress flicker and black edges.

------

## 📚 Core Types (excerpt from `CoreTypes.h`)

### Enums

| Enum               | Description      | Common values                                                |
| ------------------ | ---------------- | ------------------------------------------------------------ |
| `ControlShape`     | Geometric shape  | `RECTANGLE`, `B_RECTANGLE`, `ROUND_RECTANGLE`, `CIRCLE`, `ELLIPSE` |
| `ButtonMode`       | Button behavior  | `NORMAL`, `TOGGLE`, `DISABLED`                               |
| `TextBoxMode`      | TextBox mode     | `INPUT_MODE`, `READONLY_MODE`                                |
| `FillMode`         | Fill mode        | `SOLID`, `NULL`, `HATCHED`                                   |
| `FillStyle`        | Pattern style    | `HORIZONTAL`, `CROSS`                                        |
| `LineStyle`        | Line style       | `SOLID`, `DASH`, `DOT`                                       |
| `MessageBoxType`   | Message box type | `OK`, `OKCancel`, `YesNo`, ...                               |
| `MessageBoxResult` | Result           | `OK`, `Cancel`, `Yes`, `No`, `Abort`, `Retry`, `Ignore`      |
| `TabPlacement`     | Tab position     | `Top`, `Bottom`, `Left`, `Right`                             |

| Enum         | Description            | Common values                                |
| ------------ | ---------------------- | -------------------------------------------- |
| `LayoutMode` | 窗口布局模式           | `Fixed`,  `AnchorToEdges`                    |
| `Anchor`     | 控件相对于父容器的锚点 | `NoAnchor` ,`Left` , `Right`, `Top`,`Bottom` |

### Structs

| Struct         | Description                                          |
| -------------- | ---------------------------------------------------- |
| `ControlText`  | Font/size/color/bold/italic/underline/strike-through |
| `RouRectangle` | Corner ellipse size for rounded rectangles           |

------

## 🧩 Controls Library

### 1) Basic Controls

| Control | Header      | Description       | Key Points                                                   |
| ------- | ----------- | ----------------- | ------------------------------------------------------------ |
| Button  | `Button.h`  | Versatile button  | Shapes/modes; hover/pressed colors; callbacks; **single-line truncation** + **Tooltip** (v2.1.0) |
| Label   | `Label.h`   | Text label        | Transparent/opaque background; custom fonts                  |
| TextBox | `TextBox.h` | Input/display box | Input/readonly; integrates EasyX `InputBox`                  |

### 2) Container Controls

| Control | Header     | Description                                                  |
| ------- | ---------- | ------------------------------------------------------------ |
| Canvas  | `Canvas.h` | Parent container with custom border/background; **built-in HBox/VBox auto layout** (v2.1.0) |
| Window  | `Window.h` | Top-level container with message loop and dispatch; **resizable** (v2.1.0) |

### 3) Advanced Controls

| Control    | Header         | Description | Key Points                                                   |
| ---------- | -------------- | ----------- | ------------------------------------------------------------ |
| Table      | `Table.h`      | Data grid   | Paging/header/auto column width; fixed page-control overlap/ghosting (v2.1.0) |
| Dialog     | `Dialog.h`     | Dialog      | Modal/non-modal; auto layout; background save/restore        |
| TabControl | `TabControl.h` | Tabs        | One-click add of “tab + page” pair (pair), or add child controls to a page; uses relative coordinates |

### 4) Static Factory

| Control    | Header         | Description         | Key Points                                   |
| ---------- | -------------- | ------------------- | -------------------------------------------- |
| MessageBox | `MessageBox.h` | Message-box factory | Static API; modal/non-modal; de-dup built in |

------

## 📐 Layout Management (HBox/VBox)

==Reserved, to be implemented==

------

## 🗂 Tabs (TabControl)

- Tab strip (button group) + page container (`Canvas`)
- For transparent themes: **background snapshot** switching in the page area to avoid ghosting
- API: **see the API documentation**

------

## ✂️ Single-line Text Truncation & Button Tooltip

- **Button truncation:** separate handling for CJK/Latin under MBCS; append `...` based on pixel-width threshold
- **Tooltip:** delayed show and auto-hide; default text = button text; customizable; uses control-level **background snapshot/restore**

------

## 🧊 Transparent Background & Background Snapshots

- **General convention:** `captureBackground(rect)` before the first draw; `restoreBackground()` before hiding/covering
- **Table:** snapshot region **includes the header**; after page switch, restore immediately + redraw; paging controls centered

------

## 🔧 Advanced Topics & Best Practices

- Custom controls: inherit from `Control`, implement `draw()` / `handleEvent()`
- Performance:
  - **Dirty rectangles:** set `dirty=true` on state changes for on-demand redraw
  - **Avoid extra `cleardevice()`**: background is centrally handled in `WM_PAINT`
  - Ensure `SetWorkingImage(nullptr)` before drawing so output goes to the screen
- Event consumption: return `true` after handling to stop propagation

------

## ⚠️ Applicability & Limits

- Not suitable for high-performance games or complex animation; re-verify metrics under extreme DPI
- No accessibility support yet
- Windows-only, not cross-platform
- For complex commercial front-ends, consider Qt / wxWidgets / ImGui / Electron

------

## 📜 License

MIT (see `LICENSE`).

## 👥 Contributing Guidelines

- Follow the existing C++ style
- New features should include examples and README updates
- Self-test before submitting and explain the motivation for changes
- For bugs/ideas, please open an Issue

## 🙏 Acknowledgements

- Thanks to [EasyX](https://easyx.cn/)
- Thanks to developers who value **simplicity/efficiency/clarity**

------

**Stars and seas, code as the vessel.**

## 📞 Support & Feedback

- See [examples/](examples/)
- Read the [CHANGELOG](CHANGELOG.md / CHANGELOG.en.md)
- Submit an Issue on GitHub