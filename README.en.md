# StellarX GUI Framework

[中文文档](README.md)

------

![GitHub all releases](https://img.shields.io/github/downloads/Ysm-04/StellarX/total)

[![Star GitHub Repo](https://img.shields.io/github/stars/Ysm-04/StellarX.svg?style=social&label=Star%20This%20Repo)](https://github.com/Ysm-04/StellarX)

![Version](https://img.shields.io/badge/Version-2.0.0-brightgreen.svg) 
![Download](https://img.shields.io/badge/Download-2.0.0_Release-blue.svg) 

![C++](https://img.shields.io/badge/C++-17+-00599C?logo=cplusplus&logoColor=white)
![Windows](https://img.shields.io/badge/Platform-Windows-0078D6?logo=windows)
![EasyX](https://img.shields.io/badge/Based_on-EasyX-00A0EA)
![License](https://img.shields.io/badge/License-MIT-blue.svg)
![Architecture](https://img.shields.io/badge/Architecture-Modular%20OOP-brightgreen)
![CMake](https://img.shields.io/badge/Build-CMake-064F8C?logo=cmake)

> **"Bound by Stars, Light as Dust"** — A native C++ GUI framework for Windows platform, featuring extreme lightweight and high modularity.

`StellarX` was born from resistance against "overly bloated" modern GUI frameworks. It rejects dependencies that often reach hundreds of MB, long compilation times, and steep learning curves, choosing to return to the essence: solving core desktop application development needs with the most concise code, clearest architecture, and highest efficiency.

It is a **pure teaching-level, tool-level framework** designed to help developers deeply understand GUI principles and quickly build lightweight Windows tools.

---



## 📦 Project Structure & Design Philosophy

StellarX framework adopts classic **Object-Oriented** and **modular** design with a clear and standardized project structure:

```markdown

StellarX/
├── include/               # Header files directory
│   └── StellarX/          # Framework header files
│       ├── StellarX.h     # Main include header - one-click import of entire framework
│       ├── CoreTypes.h    # ★ Core ★ - Single source of truth for all enums and structs
│       ├── Control.h      # Abstract base class - defines unified interface for all controls
│       ├── Button.h       # Button control
│       ├── Window.h       # Window management
│       ├── Label.h        # Label control
│       ├── TextBox.h      # Text box control
│       ├── Canvas.h       # Canvas container
│       ├── Dialog.h       # Dialog control (new in v2.0.0)
│       ├── MessageBox.h   # Message box factory (new in v2.0.0)
│       └── Table.h        # Table control
├── src/                   # Source files directory
│   ├── Control.cpp
│   ├── Button.cpp
│   ├── Window.cpp
│   ├── Label.cpp
│   ├── TextBox.cpp
│   ├── Canvas.cpp
│   ├── Table.cpp     
│   ├── Dialog.cpp        # v2.0.0 new
│   └── MessageBox.cpp    # v2.0.0 new
├── examples/              # Example code directory
│   └── demo.cpp          # Basic demonstration
├── docs/                  # Documentation directory
│   └── CODE_OF_CONDUCT.md # Code of Conduct
├── CMakeLists.txt        # CMake build configuration
├── CONTRIBUTING.md       # Contribution guide
├── CHANGELOG.md          # Changelog
├── Doxyfile              # Doxygen configuration
├── LICENSE               # MIT License
└── README.md             # Project description
```

### **Design Philosophy:**

1. **Single Responsibility Principle (SRP)**: Each class/file is responsible for one thing only.
2. **Dependency Inversion Principle (DIP)**: High-level modules (like `Window`) don't depend on low-level modules (like `Button`), both depend on abstractions (`Control`).
3. **Open/Closed Principle (OCP)**: New controls can be easily extended by inheriting the `Control` base class without modifying existing code.
4. **Consistency**: All controls share unified `draw()` and `handleEvent()` interfaces.

## 🚀 Core Features

- **Extreme Lightweight**: Core library compiles to only ~1.2MB, with zero external dependencies. Generated applications are compact.
- **Clear Modular Architecture**: Uses `CoreTypes.h` to uniformly manage all types, eliminating duplicate definitions and greatly improving maintainability.
- **Native C++ Performance**: Built directly on EasyX and Win32 API, providing near-native execution efficiency with very low memory footprint (typically <10MB).
- **Complete Control System**: Button, Label, TextBox, Canvas, Table, Dialog, and MessageBox factory.
- **Highly Customizable**: From control colors, shapes (rectangle, rounded, circle, ellipse) to fill modes and font styles, all have detailed enum support for easy customization.
- **Simple & Intuitive API**: Uses classic object-oriented design, code as documentation, low learning curve.
- **Standard Project Structure**: Adopts standard include/src separation structure, supports CMake build, easy to integrate and use.
- **Enhanced Event System**: v2.0.0 introduces event consumption mechanism, all `handleEvent` methods return `bool` indicating whether event was consumed, supporting finer-grained event propagation control.
- **Dialog System**: New complete dialog support, including modal and non-modal dialogs, automatically handling background saving and restoration.

## ⚡ Quick Start (5 Minutes to Get Started)

> **🎯 Latest Version Download**
> Download pre-compiled library files and header files from [GitHub Releases](https://github.com/Ysm-04/StellarX/releases/latest) for quick integration into your project.

### Environment Requirements

- **OS**: Windows 10 or higher
- **Compiler**: C++17 supported compiler (e.g., **Visual Studio 2019+**)
- **Graphics Library**: [EasyX](https://easyx.cn/) (2022 version or higher, please select the version matching your compiler during installation)
- **Build Tool**: CMake 3.12+ (optional, recommended)

### Install EasyX

1. Visit [EasyX official website](https://easyx.cn/) to download the latest version
2. Run the installer, select the version matching your Visual Studio version
3. After installation, no additional configuration is needed, StellarX framework will automatically link EasyX

### Method 1: Using CMake Build (Recommended)

1. **Clone the project**:

   ```bash
   git clone https://github.com/Ysm-04/StellarX.git
   cd StellarX
   ```

2. **Generate build system**:

   ```bash
   mkdir build
   cd build
   cmake ..
   ```

3. **Compile the project**:

   ```bash
   cmake --build .
   ```

4. **Run the example**:

   ```bash
   ./examples/Demo
   ```

### Method 2: Manual Integration into Existing Project

1. **Copy the include and src directories** to your project
2. **Configure include paths** to ensure the compiler can find the `include/StellarX/` directory
3. **Add all .cpp files** to your project for compilation

### Create Your First StellarX Application

```cpp
// Just include this one header to use all features
#include "StellarX.h"

// Program entry point (use WinMain for better compatibility)
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    // 1. Create a 640x480 window with white background, titled "My App"
    Window mainWindow(640, 480, 0, RGB(255, 255, 255), "My First StellarX App");

    // 2. Create a button (managed with smart pointer)
    auto myButton = std::make_unique<Button>(
        250, 200, 140, 40, // x, y, width, height
        "Click Me",         // button text
        StellarX::ButtonMode::NORMAL,
        StellarX::ControlShape::ROUND_RECTANGLE
    );

    // 3. Set click event for the button (using Lambda expression)
    myButton->setOnClickListener([&mainWindow]() {
        // Use message box factory to create modal dialog
        auto result = StellarX::MessageBox::ShowModal(
            mainWindow, 
            "Welcome to StellarX GUI\r\nAuthor: Ysm-04", 
            "Greeting", 
            StellarX::MessageBoxType::OKCancel
        );
           // Handle dialog result
        if (result == StellarX::MessageBoxResult::OK) {
            // User clicked OK button
        }
    });
        
     

    // 4. (Optional) Set button style
    myButton->textStyle.nHeight = 20;
    myButton->textStyle.color = RGB(0, 0, 128); // Dark blue text
    myButton->setButtonBorder(RGB(0, 128, 255)); // Blue border

    // 5. Add button to window
    mainWindow.addControl(std::move(myButton));

    // 6. Draw the window
    mainWindow.draw();

    // 7. Enter message loop, wait for user interaction
    mainWindow.runEventLoop();

    return 0;
}
```

1. **Compile and run!** You'll see a window with a blue rounded button, clicking it will pop up a message box.

## 📚 Core Types Detailed Explanation (`CoreTypes.h`)

All visual and behavioral properties of the StellarX framework are controlled through the elegant enums and structs defined in `CoreTypes.h`.

### Enum Types (Enums)

| Enum Type              | Description             | Common Values                                                |
| :--------------------- | :---------------------- | :----------------------------------------------------------- |
| **`ControlShape`**     | Control geometric shape | `RECTANGLE`, `B_RECTANGLE`, `ROUND_RECTANGLE`, `CIRCLE`, `ELLIPSE`, etc. |
| **`ButtonMode`**       | Button behavior mode    | `NORMAL`, `TOGGLE`, `DISABLED`                               |
| **`TextBoxMode`**      | Text box mode           | `INPUT_MODE`, `READONLY_MODE`                                |
| **`FillMode`**         | Graphics fill mode      | `SOLID`, `NULL`, `HATCHED`, etc.                             |
| **`FillStyle`**        | Pattern fill style      | `HORIZONTAL`, `CROSS`, etc.                                  |
| **`LineStyle`**        | Border line style       | `SOLID`, `DASH`, `DOT`, etc.                                 |
| **`MessageBoxType`**   | Message box type        | `OK`, `OKCancel`, `YesNo`, `YesNoCancel`, `RetryCancel`, `AbortRetryIgnore` |
| **`MessageBoxResult`** | Message box result      | `OK`, `Cancel`, `Yes`, `No`, `Abort`, `Retry`, `Ignore`      |

### Structs (Structs)

| Struct             | Description                                                  |
| :----------------- | :----------------------------------------------------------- |
| **`ControlText`**  | Encapsulates all text style attributes, including font, size, color, bold, italic, underline, strikethrough, etc. |
| **`RouRectangle`** | Defines rounded rectangle corner ellipse dimensions, contains width and height properties. |

**Usage Example:**

```c++
// Create a complex text style
StellarX::ControlText myStyle;
myStyle.nHeight = 25; // Font height
myStyle.lpszFace = _T("Microsoft YaHei"); // Font
myStyle.color = RGB(255, 0, 0); // Red color
myStyle.nWeight = FW_BOLD; // Bold
myStyle.bUnderline = true; // Underline

// Apply to controls
myLabel->textStyle = myStyle;
myButton->textStyle = myStyle;
```

## 🧩 Complete Control Library

### 1. Basic Controls

| Control     | Header File | Description           | Key Features                                                 |
| :---------- | :---------- | :-------------------- | :----------------------------------------------------------- |
| **Button**  | `Button.h`  | Multi-function button | Supports multiple modes/shapes/states, hover/click colors, custom callbacks |
| **Label**   | `Label.h`   | Text label            | Supports transparent/opaque background, custom font styles   |
| **TextBox** | `TextBox.h` | Input box/Display box | Supports input and read-only modes, integrates EasyX's `InputBox` |

### 2. Container Controls

| Control    | Header File | Description                                                  |
| :--------- | :---------- | :----------------------------------------------------------- |
| **Canvas** | `Canvas.h`  | Container control, can serve as parent container for other controls, supports custom borders and background. |
| **Window** | `Window.h`  | Top-level window, ultimate container for all controls, responsible for message loop and dispatching. |

### 3. Advanced Controls

| Control    | Header File | Description  | Key Features                                                 |
| :--------- | :---------- | :----------- | :----------------------------------------------------------- |
| **Table**  | `Table.h`   | Data table   | **Framework highlight feature**, supports paginated display, custom headers and data, automatic column width calculation, page turn buttons. |
| **Dialog** | `Dialog.h`  | Dialog class | Implements complete dialog functionality, supports multiple button combinations and asynchronous result callbacks. Automatically handles layout, background save/restore and lifecycle management. |

**Table Control Example:**

```c++
// Create a table
auto myTable = std::make_unique<Table>(50, 50);

// Set headers
myTable->setHeaders({ "ID", "Name", "Age", "Occupation" });

// Add data rows
myTable->setData({ "1", "Zhang San", "25", "Engineer" });
myTable->setData({ "2", "Li Si", "30", "Designer" });
myTable->setData({ "3", "Wang Wu", "28", "Product Manager" });

// Set rows per page
myTable->setRowsPerPage(2);

// Set table style
myTable->textStyle.nHeight = 16;
myTable->setTableBorder(RGB(50, 50, 50));
myTable->setTableBk(RGB(240, 240, 240));

// Add to window
mainWindow.addControl(std::move(myTable));
```

### 4. Static Factory Class

| Control        | Header File    | Description          | Key Features                                                 |
| :------------- | :------------- | :------------------- | :----------------------------------------------------------- |
| **MessageBox** | `MessageBox.h` | Dialog factory calls | Static method calls, no instantiation needed, automatically handles modal and non-modal logic differences, integrated into window's dialog management system, provides deduplication mechanism to prevent duplicate dialogs |

**MessageBox Usage Example:**

```c++
// Modal message box (blocks until closed)
auto result = StellarX::MessageBox::ShowModal(
    mainWindow, 
    "Confirm to perform this operation?", 
    "Confirmation", 
    StellarX::MessageBoxType::YesNo
);

if (result == StellarX::MessageBoxResult::Yes)
{
    // User selected "Yes"
}

// Non-modal message box (asynchronous callback)
StellarX::MessageBox::ShowAsync(
    mainWindow,
    "Operation completed", 
    "Notification", 
    StellarX::MessageBoxType::OK,
    [](StellarX::MessageBoxResult result) {
        // Asynchronously handle result
    }
);
```

## Examples

- **Register Viewer (≈450 lines)** — An interactive 32-bit register visualization tool implemented based on StellarX (supports bit inversion, left/right shift, hex/decimal conversion, signed/unsigned toggle, binary grouping display).
  Path: `examples/register-viewer/`

## 🔧 Advanced Topics & Best Practices

### 1. Custom Controls

You can create custom controls by inheriting from the `Control` base class. Just implement the two pure virtual functions `draw()` and `handleEvent()`.

```c++
class MyCustomControl : public Control {
public:
    MyCustomControl(int x, int y) : Control(x, y, 100, 100) {}
    
    void draw() override {
        saveStyle();
        // Your custom drawing logic
        setfillcolor(RGB(255, 100, 100));
        fillrectangle(x, y, x + width, y + height);
        restoreStyle();
    }
    
    bool handleEvent(const ExMessage& msg) override {
        // Your custom event handling logic
        if (msg.message == WM_LBUTTONDOWN && 
            msg.x > x && msg.x < x + width && 
            msg.y > y && msg.y < y + height) {
            // Handle click
            return true; // Event consumed
        }
        return false; // Event not consumed
    }
    
    bool IsVisible() const override { return true; }
    bool model() const override { return false; }
};
```

### 2. Layout Management

Current version of StellarX primarily uses **absolute positioning**. For simple layouts, you can achieve this by calculating coordinates. For complex layouts, consider:

- Nesting controls in `Canvas` to achieve relative positioning.
- Implementing simple flow layout or grid layout managers yourself.

### 3. Performance Optimization

- **Dirty Rectangle Rendering**: Implemented internally in the framework, controls set `dirty=true` when state changes, only redraw when necessary.
- **Image Resources**: Use `IMAGE` objects to load images, then reuse them to avoid multiple loads.
- **Reduce Operations in Loops**: Avoid heavy calculations in `draw()` and `handleEvent()`.
- **Event Consumption Mechanism**: Use event consumption return values appropriately to avoid unnecessary event propagation.

### 4. Dialog Usage Tips

- **Modal Dialogs**: Use `ShowModal` method, blocks current thread until dialog closes, suitable for important operations requiring user confirmation.
- **Non-modal Dialogs**: Use `ShowAsync` method, doesn't block main thread, suitable for informational prompts or background tasks.
- **Dialog Deduplication**: Framework has built-in non-modal dialog deduplication mechanism to prevent multiple dialogs for the same message from appearing simultaneously.

## ⚠️ Important Limitations & Suitable Scenarios

**StellarX framework's design goals are lightness, clarity, and teaching value, therefore it explicitly is NOT suitable for the following scenarios:**

- **High-performance games or complex animations**: Rendering is based on EasyX's CPU software rendering, performance is limited.
- **Applications requiring high DPI scaling**: Limited support for high DPI displays, interface may not display correctly.
- **Applications requiring accessibility features**: No support for screen readers and other assistive technologies.
- **Cross-platform applications**: Deeply dependent on Windows API and EasyX, cannot run directly on Linux/macOS.
- **Complex commercial software frontends**: Lacks advanced controls (like tree views, rich text boxes, tabs, advanced lists) and mature automatic layout managers.

**If you need to develop the aforementioned types of applications, please consider the following mature solutions:**

- **Qt**: Extremely powerful, cross-platform, suitable for large commercial applications.
- **wxWidgets**: Native appearance, cross-platform.
- **Dear ImGui**: Immediate mode GUI, very suitable for tools and debug interfaces.
- **Web Technology Stack (Electron/CEF)**: Suitable for scenarios requiring web technologies.

## 📜 License

This project uses the **MIT License**.

You are free to:

- Use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the framework.
- Use it for private or commercial projects.

The only requirement is:

- Please retain the original copyright notice in your projects.

See the [LICENSE](https://license/) file in the project root directory for details.

## 👥 Contribution Guide

We welcome all forms of contributions! If you want to contribute to the StellarX framework, please read the following guidelines:

1. **Code Style**: Please follow the existing Google C++ style guide (use spaces for indentation, braces on new lines, etc.).
2. **New Features**: Must provide **example code** and update relevant parts of this README document.
3. **Submitting PRs**: Please ensure your code is tested before submission and clearly describe your changes and motivation.
4. **Issue Reporting**: If you find a bug or have new ideas, welcome to submit Issues on GitHub.

Detailed contribution guide please refer to [CONTRIBUTING.md](CONTRIBUTING.en.md).

## 🙏 Acknowledgements

- Thanks to [EasyX Graphics Library](https://easyx.cn/) for providing the simple and easy-to-use graphics foundation for this project, making C++ GUI programming education possible.
- Thanks to all developers pursuing the **concise, efficient, clear** coding philosophy, you are the inspiration for StellarX's birth.

------

**Star Ocean Vast, Code as Boat.**

May `StellarX` become a reliable cornerstone for your exploration of the GUI world, whether for learning, teaching, or creating practical tools.

## 📞 Support & Feedback

If you encounter problems during use or have any suggestions:

1. Check [Example Code](examples/) for usage reference
2. Check [Changelog](CHANGELOG.en.md) to learn about latest changes
3. Submit Issues on GitHub repository to report problems

------

*StellarX Framework - Light as Dust, Bound by Stars*

