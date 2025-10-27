# Changelog

All notable changes to the StellarX project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

[中文文档](CHANGELOG.md)

## [v2.1.0] - 2025-10-27

**Focus**: Resizable/maximized window (EasyX reinforced by Win32), first-phase layout manager (HBox/VBox), early Tabs control. We also fixed black borders, maximize “ghosts”, flicker, and the issue where controls only appeared after interaction. Control-level **background snapshot/restore**, **single-line truncation**, and **tooltips** are now standardized.

### ✨ Added

- **Bilingual API Documentation (Chinese and English)**
  - The documentation provides a detailed introduction of each class, including API descriptions, functionalities, and points to note, with a comprehensive explanation of each control.

- **Window resize/maximize reinforcement (EasyX + Win32)**
  - `Window::enableResize(bool enable, int minW, int minH)`; toggle at runtime and set min track size.
  - Subclassed WndProc for `WM_GETMINMAXINFO / WM_SIZE / WM_EXITSIZEMOVE / WM_ERASEBKGND / WM_PAINT` with `WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS`.
  - **Full-surface background paint** in `WM_PAINT` (solid/image), removing black borders and maximize ghosts.
  - **One-shot redraw** on resize with `pendingW/H + needResizeDirty` coalescing.
- **Layout manager (phase 1)**
  - On `Canvas`: `LayoutKind::{Absolute, HBox, VBox, Grid(placeholder), Flow(placeholder), Stack(placeholder)}` with `LayoutParams` (`margins`, `fixedW/fixedH(-1=same)`, `weight`, `Align{Start,Center,End,Stretch}`).
  - Implemented **HBox/VBox** auto layout inside containers; containers stay absolutely positioned; nesting supported.
- **Tabs control (early)**
  - Decoupled tab strip and page container; background snapshot for transparent themes.
- **Button single-line truncation (MBCS; CN/EN aware)**
  - Pixel-width threshold with `...` avoiding half-glyph artifacts.
- **Hover tooltip**
  - Implemented via `Label` with delay & auto-hide; per-control background snapshot/restore; customizable text with sensible fallback.

### 🔧 Changed

- **Window rendering path**
  - Reduced reliance on “offscreen frame blit”; in `WM_PAINT` use **GDI full background + EasyX batch drawing** (`BeginBatchDraw/EndBatchDraw + FlushBatchDraw`) to suppress flicker.
  - On resize, only rebuild the scaled background (`zoomBackground`); actual painting happens next frame.
- **Control base**
  - Standardized **captureBackground/restoreBackground**; transparent/stacked visuals are stable.
  - Unified `dirty`: containers paint their own background when dirty; **children still evaluate/draw** as needed each frame.
- **Table**
  - In transparent mode, pagination widgets now inherit table text/fill style.
  - Reworked **pagination math + block centering** (header included in available width).
  - Fixed background snapshot sizing (header inclusion) that caused failed restores.
- **Event loop**
  - Coalesced `WM_SIZE` to the loop tail to avoid redraw storms and pointer-hover lag.

### ✅ Fixed

- **Black borders / maximize ghost / flicker**: blocked system background erase; full-surface paint in `WM_PAINT`; cleared clipping to prevent stale fragments.
- **Containers not drawn; controls only after interaction**: first-frame & post-input full-tree dirty; children draw even if the container isn’t dirty.
- **Table pagination overlap & transparent shadowing**: corrected snapshot area; recomputed coordinates; instant restore + redraw after paging.

### ⚠️ Breaking

- If external code accessed `Window` private members (e.g., `dialogs`), use `getControls()` / `getdialogs()`.
- Pagination math & header inclusion may shift hard-coded offsets in custom renderers.
- Custom controls that don’t restore `SetWorkingImage(nullptr)` before drawing should be reviewed.

### 📌 Upgrade notes

1. Migrate manual `cleardevice()+putimage` paths to unified **full-surface background** in `WM_PAINT`.
2. For transparent controls, `captureBackground()` before first draw; `restoreBackground()` when hiding/overdrawing.
3. For layout, set container `layout.kind = HBox/VBox` and child `LayoutParams` (`margin/fixed/weight/align`).
4. Keep a **single** truncation pass for buttons to avoid duplicate `...`.
5. Prefer built-in table pagination.

## [v2.0.1] - 2025-10-03

### Added

- New example: 32-bit register viewer tool implemented based on StellarX (supports bit inversion, left shift, right shift, hexadecimal/decimal signed/unsigned toggle, grouped binary display).
  - Example path: `examples/register-viewer/`
- `TextBox` added `setText` API, allowing external setting of text box content
- `TextBox::setText` API modified: immediately calls `draw` method to redraw after setting text
- `Button` added `setButtonClick` API, allowing external functions to modify button click state and execute corresponding callback functions
- ==All documents updated with corresponding English versions(.en)==

## [v2.0.0] - 2025-09-21

### Overview
v2.0.0 is a major release. This release introduces dialog and message box factory (Dialog / MessageBox), with several important fixes and improvements to event distribution, API semantics, and internal resource management.

Some APIs/behaviors have breaking changes that are not backward compatible.

### Added
- **Dialog System**:
  - Added `Dialog` class, inheriting from `Canvas`, for building modal and non-modal dialogs
  - Added `MessageBox` factory class, providing two convenient APIs: `ShowModal` (synchronous blocking) and `ShowAsync` (asynchronous callback)
  - Supports six standard message box types: `OK`, `OKCancel`, `YesNo`, `YesNoCancel`, `RetryCancel`, `AbortRetryIgnore`
  - Automatically handles dialog layout, background saving and restoration, event propagation, and lifecycle management

- **Enhanced Event System**:
  - All controls' `handleEvent` methods now return `bool` type, indicating whether the event was consumed
  - Introduced event consumption mechanism, supporting finer-grained event propagation control
  - Window class event loop now prioritizes dialog event processing

- **Control State Management**:
  - Control base class added `dirty` flag and `setDirty()` method, uniformly managing redraw state
  - All controls now implement `IsVisible()` and `model()` methods

- **API Enhancements**:
  - Button class added `setButtonFalseColor()` method
  - TextBox class `setMaxCharLen()` now accepts `size_t` type parameter
  - Window class added dialog management methods and duplicate detection mechanism

### Breaking Changes
- **API Signature Changes**:
  - All controls' `handleEvent(const ExMessage& msg)` method changed from returning `void` to returning `bool`
  - Control base class added pure virtual functions `IsVisible() const` and `model() const`, all derived classes must implement them

- **Resource Management Changes**:
  - Control base class style saving changed from stack objects to heap objects, managed using pointers
  - Enhanced resource release safety, all resources are properly released in destructors

- **Event Handling Logic**:
  - Window's `runEventLoop()` method completely rewritten, now prioritizes dialog events
  - Introduced event consumption mechanism, events do not continue propagating after being consumed

### Fixed
- **Memory Management**:
  - Fixed memory leak issue in `Button::setFillIma()`
  - Fixed resource release issues in Control base class destructor
  - Fixed background image resource management issues in Dialog class

- **Layout and Rendering**:
  - Fixed pagination calculation, column width, and row height boundary issues in `Table` component
  - Fixed layout disorder caused by `pX` coordinate accumulation error in `Table`
  - Fixed dirty determination issue in `Canvas::draw()` that prevented child controls from being drawn
  - Fixed asymmetric call issues between `TextBox::draw()` and `restoreStyle()`

- **Event Handling**:
  - Fixed window event distribution logic to ensure dialog/top-level controls prioritize event processing
  - Fixed delayed state updates when mouse moves out of button area
  - Fixed race conditions in non-modal dialog event handling

- **Other Issues**:
  - Fixed potential errors in text measurement and rendering
  - Fixed incomplete background restoration after dialog closure
  - Fixed z-order management issues in multi-dialog scenarios

### Changed
- **Code Quality**:
  - Refactored numerous internal APIs, enhancing exception safety and maintainability
  - Used smart pointers and modern C++ features to replace raw new/delete
  - Unified code style and naming conventions

- **Performance Optimization**:
  - Optimized event processing flow, reducing unnecessary redraws
  - Improved dialog background saving and restoration mechanism
  - Reduced memory allocation and copy operations

- **Documentation and Examples**:
  - Added detailed usage examples for all new features
  - Improved code comments and API documentation
  - Updated README and CHANGELOG to reflect latest changes

## [1.1.0] - 2025-09-08

### Added
- **Window Class API Enhancements**:
  - Added complete getter method set, improving class encapsulation and usability
  - `getHwnd()` - Get window handle for integration with native Windows API
  - `getWidth()` - Get window width
  - `getHeight()` - Get window height
  - `getHeadline()` - Get window title
  - `getBkcolor()` - Get window background color
  - `getBkImage()` - Get window background image pointer
  - `getControls()` - Get reference to control management container, allowing iteration and manipulation of added controls

### Improved
- **API Consistency**: Provided symmetric setter and getter methods for all important attributes
- **Code Documentation**: Further improved class comments, making them clearer and more professional

## [1.0.0] - 2025-09-08

### Release Summary
First stable release

### Added

- First stable version of StellarX framework
- Complete control library: Button, Label, TextBox, Canvas, Table, and Window
- CMake-based build system
- Detailed documentation and example code
- **Explicit declaration: This framework is specifically designed for Windows platform**

### Released
-   **First release of pre-compiled binary library files**, facilitating quick integration without compiling from source
-   Provided release packages include:
    -   `StellarX-v1.0.0-x64-Windows-msvc-x64.zip`
        -   **Build Environment**: Visual Studio 2022 (MSVC v143)
        -   **Architecture**: x64 (64-bit)
        -   **Runtime Library**: `/MD`
        -   **Build Modes**: `Release | Debug`
        -   **Contents**: Includes all necessary header files (`include/`) and static library files (`lib/StellarX-Debug.lib StellarX-Release.lib`)

### Core Features
- Modular design following SOLID principles
- Unified control interface (`draw()` and `handleEvent()`)
- Support for multiple control shapes and styles
- Custom event handling callbacks
- Lightweight design with no external dependencies

## [0.1.0] - 2025-08-15
### Added
- Initial project structure and core architecture
- Control base class and basic event handling system
- Basic examples and documentation setup