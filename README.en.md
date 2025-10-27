# StellarX GUI Framework

[中文文档](README.md)

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

> **"Bound by Stars, Light as Dust"** — A native C++ GUI framework for Windows, featuring extreme lightness and high modularity.

`StellarX` was born from resistance against bloat. It avoids huge dependencies, long build times, and steep learning curves, returning to the essence: using concise code, clear architecture, and high efficiency to solve desktop GUI needs.

It is a **pure teaching/tool-level framework** that helps developers understand GUI principles and quickly build lightweight Windows tools.

---

## 🆕 What's New in v2.1.0

**Bilingual API Documentation (Chinese and English)**

- The documentation provides a detailed introduction of each class, including API descriptions, functionalities, and points to note, with a comprehensive explanation of each control.

- **Resizable/Maximizable Window (EasyX + Win32 reinforcement)**  
  Subclassed WndProc with `WM_GETMINMAXINFO / WM_SIZE / WM_EXITSIZEMOVE / WM_ERASEBKGND / WM_PAINT` and window styles `WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS`. Full-surface background paint in `WM_PAINT` eliminates black borders and maximize “ghosts”. Coalesced resize with one-shot redraw.
- **Layout Manager (Phase 1)**  
  `Canvas` now supports `LayoutKind::{Absolute, HBox, VBox, Grid(placeholder), Flow(placeholder), Stack(placeholder)}` with `LayoutParams` (`margins`, `fixedW/fixedH`, `weight`, `Align{Start,Center,End,Stretch}`). Implemented **HBox/VBox** auto layout (containers remain absolutely positioned; nesting supported).
- **Tabs (early)**  
  Tab strip + page container separated; background snapshot avoids trails on transparent themes.
- **Button Single-Line Truncation (MBCS, CN/EN aware)**  
  Pixel-width based `...` truncation prevents half-glyph artifacts.
- **Hover Tooltip**  
  Implemented via `Label`, supports delay and auto-hide; per-control background snapshot/restore.

See details in `CHANGELOG.en.md`.

---

## 📦 Project Structure & Design Philosophy

StellarX adopts classic **OOP** and **modular** design with a clear structure:

```markdown
StellarX/
├── include/
│   └── StellarX/
│       ├── StellarX.h
│       ├── CoreTypes.h       # Single source of truth for enums/structs
│       ├── Control.h
│       ├── Button.h
│       ├── Window.h
│       ├── Label.h
│       ├── TextBox.h
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
│   ├── Table.cpp
│   ├── Dialog.cpp            
│   └── MessageBox.cpp        
├── examples/
│   └── demo.cpp
├── docs/
│   └── CODE_OF_CONDUCT.md
├── CMakeLists.txt
├── CONTRIBUTING.md
├── CHANGELOG.en.md
├── CHANGELOG.md
├── Doxyfile
├── LICENSE
├──API 文档.md
├──API Documentation.en.md
└── README.en.md
```

### Design Philosophy

1. **SRP**: each class/file does one thing.
2. **DIP**: high-level modules depend on abstractions (`Control`), not concrete controls.
3. **OCP**: new controls extend `Control` without touching existing code.
4. **Consistency**: all controls expose `draw()` and `handleEvent()`.

## 🚀 Core Features

- **Extreme Lightweight**: tiny footprint with zero external dependencies beyond EasyX.
- **Clear Modular Architecture**: `CoreTypes.h` centralizes types and enums.
- **Native C++ Performance**: EasyX + Win32 API for near-native speed, low memory (<10MB typical).
- **Complete Control Set**: Button, Label, TextBox, Canvas, Table, Dialog, and MessageBox factory.
- **Highly Customizable**: colors, shapes (rectangle/rounded/circle/ellipse), fill modes, fonts via enums.
- **Simple, Intuitive API**: OOP-oriented, code as documentation.
- **Standard Project Layout**: include/src split, CMake build.
- **Enhanced Event System**: `handleEvent` returns `bool` for consumption (since v2.0.0).
- **Dialog System**: modal and modeless; auto background save/restore.

------

## ⚡ Quick Start (5 Minutes)

> Download prebuilt headers/libs from [Releases](https://github.com/Ysm-04/StellarX/releases/latest).

### Requirements

- **OS**: Windows 10+
- **Compiler**: C++17 (e.g., Visual Studio 2019+)
- **Graphics**: [EasyX](https://easyx.cn/) 2022+ (match your compiler)
- **Build**: CMake 3.12+ (optional)

### Install EasyX

1. Download latest EasyX
2. Install matching your Visual Studio
3. No extra configuration needed for StellarX

### CMake Build (Recommended)

```
git clone https://github.com/Ysm-04/StellarX.git
cd StellarX
mkdir build && cd build
cmake ..
cmake --build .
./examples/Demo
```

### Manual Integration

- Copy `include` and `src` into your project
- Add include paths for `include/StellarX/`
- Add all `.cpp` files to your build

### Your First App (now Resizable)

```
#include "StellarX.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Window mainWindow(800, 600, 0, RGB(255,255,255), "StellarX App");
    mainWindow.draw();

    // Enable resize + minimum track size
    mainWindow.enableResize(true, 480, 360);

    // Add your controls here...
    // mainWindow.addControl(std::move(btn));

    mainWindow.runEventLoop();
    return 0;
}
```

> Internally, we paint the **entire client area** in `WM_PAINT` (solid/image) and use EasyX batch drawing to avoid flicker and black borders.

------

## 📚 Core Types (`CoreTypes.h`)

### Enums (selection)

| Enum               | Description | Values                                                       |
| ------------------ | ----------- | ------------------------------------------------------------ |
| `ControlShape`     | geometry    | `RECTANGLE`, `B_RECTANGLE`, `ROUND_RECTANGLE`, `CIRCLE`, `ELLIPSE`, ... |
| `ButtonMode`       | behavior    | `NORMAL`, `TOGGLE`, `DISABLED`                               |
| `TextBoxMode`      | modes       | `INPUT_MODE`, `READONLY_MODE`                                |
| `FillMode`         | fill        | `SOLID`, `NULL`, `HATCHED`, ...                              |
| `FillStyle`        | pattern     | `HORIZONTAL`, `CROSS`, ...                                   |
| `LineStyle`        | border      | `SOLID`, `DASH`, `DOT`, ...                                  |
| `MessageBoxType`   | dialog      | `OK`, `OKCancel`, `YesNo`, ...                               |
| `MessageBoxResult` | result      | `OK`, `Cancel`, `Yes`, `No`, `Abort`, `Retry`, `Ignore`      |

### Structs

| Struct         | Description                                  |
| -------------- | -------------------------------------------- |
| `ControlText`  | font/size/color/bold/italic/underline/strike |
| `RouRectangle` | rounded-rectangle corner ellipse size        |

------

## 🧩 Control Library

### 1) Basic

| Control | Header      | Description             | Key Points                                                   |
| ------- | ----------- | ----------------------- | ------------------------------------------------------------ |
| Button  | `Button.h`  | multi-functional button | shapes/modes, hover/click colors, callbacks, **single-line truncation** + **tooltip** (v2.1.0) |
| Label   | `Label.h`   | text label              | transparent/opaque backgrounds, custom font styles           |
| TextBox | `TextBox.h` | input/display           | input & read-only, integrates EasyX `InputBox`               |

### 2) Containers

| Control | Header     | Description                                                  |
| ------- | ---------- | ------------------------------------------------------------ |
| Canvas  | `Canvas.h` | parent container, custom border/background, **auto layout HBox/VBox** (v2.1.0) |
| Window  | `Window.h` | top-level container, message loop/dispatch, **resizable** (v2.1.0) |

### 3) Advanced

| Control | Header     | Description | Key Points                                                   |
| ------- | ---------- | ----------- | ------------------------------------------------------------ |
| Table   | `Table.h`  | data table  | pagination, header/data, auto column widths, page controls fixed overlap/centering (v2.1.0) |
| Dialog  | `Dialog.h` | dialogs     | modal/modeless, layout & background save/restore             |

### 4) Static Factory

| Control    | Header         | Description    | Key Points                                  |
| ---------- | -------------- | -------------- | ------------------------------------------- |
| MessageBox | `MessageBox.h` | dialog factory | static API; modal/modeless; dedup mechanism |

------

## 📐 Layout Management (HBox/VBox)

- **Container-level**: `Canvas::layout.kind = HBox / VBox / Absolute (default)`; optional container `padding`.
- **Child-level**: `LayoutParams`
  - `margin{L,R,T,B}`
  - `fixedW/fixedH` (`-1` = keep current)
  - `weight` (distribute along main axis; width in HBox, height in VBox)
  - `alignX/alignY` = `Start/Center/End/Stretch` (cross-axis)

**Example** (3 items HBox, center vertically, stretch height):

```
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

## 🗂 Tabs (Early)

- Tab strip (buttons) + page container (`Canvas`)
- Transparent themes: page area uses **background snapshot** on switch
- API sketch: `tabs.addPage(title, canvasPtr); tabs.setActive(index);`

------

## ✂️ Single-Line Truncation & 🫧 Tooltip

- **Button truncation**: MBCS CN/EN aware pixel-width threshold + `...`
- **Tooltip**: delayed show, auto-hide; default text = button label; customizable; uses per-control **background snapshot/restore**.

------

## 🧊 Transparent Background & Background Snapshot

- **Convention**: `captureBackground(rect)` before first draw; `restoreBackground()` before hiding/overdraw.
- **Table**: snapshot area now **includes header**; pagination re-centers controls and restores backgrounds immediately.

------

## 🔧 Advanced Topics & Best Practices

- Custom controls: inherit `Control` and implement `draw()` / `handleEvent()`.
- Performance:
  - **Dirty rendering**: set `dirty=true` when state changes; framework repaints as needed.
  - **Avoid extra `cleardevice()`**: background is already handled in `WM_PAINT` full-surface path.
  - Use `SetWorkingImage(nullptr)` before drawing to the screen.
- Event consumption: return `true` if handled to stop propagation.

------

## ⚠️ Limitations & When Not To Use

- High-performance games or complex animations
- High DPI scaling extremes (re-verify coordinates/metrics)
- Accessibility requirements
- Cross-platform targets (Windows-only)
- Heavy enterprise frontends (consider Qt/wxWidgets/ImGui/Electron)

------

## 📜 License

MIT License (see `LICENSE`).

## 👥 Contributing

- Follow the existing C++ style (spaces, brace on new line).
- New features should include an example and README update.
- Test before PR; describe changes and motivation.
- Use Issues for bugs/ideas.

## 🙏 Acknowledgements

- Thanks to [EasyX](https://easyx.cn/).
- Thanks to developers who value **concise, efficient, clear** code.

------

**Star Ocean Vast, Code as Boat.**

## 📞 Support & Feedback

- See [examples/](examples/)
- See [Changelog](CHANGELOG.en.md)
- Open Issues on GitHub