# StellarX GUI Framework

[中文 README](README.md)

Official website: https://stellarx-gui.top
Blog: https://blog.stellarx-gui.top

> Visit the official website for framework information and quick start instructions. For detailed tutorials, see the StellarX page on the personal blog.

> This repository is the main **StellarX** repository. Development and Issue/PR discussions happen on GitHub. GitCode is only a read-only mirror. Please report issues on GitHub: https://github.com/Ysm-04/StellarX

[![Mirror on GitCode](https://img.shields.io/badge/Mirror-GitCode-blue)](https://gitcode.com/Ysm-04/StellarX)

![GitHub all releases](https://img.shields.io/github/downloads/Ysm-04/StellarX/total)
[![Star GitHub Repo](https://img.shields.io/github/stars/Ysm-04/StellarX.svg?style=social&label=Star%20This%20Repo)](https://github.com/Ysm-04/StellarX)

![Version](https://img.shields.io/badge/Version-3.1.0-brightgreen.svg)
![Download](https://img.shields.io/badge/Download-3.1.0_Release-blue.svg)
![C++](https://img.shields.io/badge/C++-17+-00599C?logo=cplusplus&logoColor=white)
![Windows](https://img.shields.io/badge/Platform-Windows-0078D6?logo=windows)
![EasyX](https://img.shields.io/badge/Based_on-EasyX-00A0EA)
![License](https://img.shields.io/badge/License-MIT-blue.svg)
![Architecture](https://img.shields.io/badge/Architecture-Modular%20OOP-brightgreen)
![CMake](https://img.shields.io/badge/Build-CMake-064F8C?logo=cmake)

> **“Bounded by the stars, light as dust.”** — A lightweight, modular native C++ GUI framework for Windows.

`StellarX` avoids heavy dependencies, long builds, and steep learning curves. Its goal is to solve common GUI needs for lightweight desktop tools with compact code, clear architecture, and straightforward APIs. It is especially suitable for teaching, utilities, demos, lightweight visualization, and developers who want to understand GUI drawing, event dispatch, layout, and repaint mechanisms.

## UI / Framework Preview

<table>
  <tr>
    <td colspan="2" align="center">
      <strong>KEY5: Layout and Managed Repaint</strong><br>
      <img src="image/KEY5.png" alt="KEY5 layout and managed repaint preview" width="80%">
    </td>
  </tr>
  <tr>
    <td align="center">
      <strong>KEY2: New Layout API Migration</strong><br>
      <img src="image/KEY2.png" alt="KEY2 new layout API preview" width="100%">
    </td>
    <td align="center">
      <strong>KEY4: Dialog / MessageBox</strong><br>
      <img src="image/KEY4.png" alt="KEY4 Dialog and MessageBox preview" width="100%">
    </td>
  </tr>
</table>

## Latest Release

### v3.1.0 (2026-05-15)

v3.1.0 is a layout and managed repaint stabilization release. It turns the capabilities landed in the previous development phase into documented, verifiable, and migratable public semantics.

- **New layout API finalized**: four-edge anchors, per-axis `AxisSizePolicy`, per-axis fixed-size `AxisAlignPolicy`, while keeping legacy `setLayoutMode()` / `setAnchor()` as compatibility inputs.
- **Clear geometry semantics**: `localx/localy/localWidth/localHeight` are design-time parent-local rectangles, while `x/y/width/height` are current runtime drawing rectangles. Normal resize and layout solving do not write back to the design baseline.
- **Managed repaint stabilization**: Window / Canvas / TabControl / Table / Dialog dirty coverage, overlay, and Tooltip transient-layer paths are consolidated to reduce ghosting, z-order errors, and stale hover state.
- **Dialog / MessageBox stability retained**: Dialog recenters during host-window resize and does not stretch. MessageBox remains a thin factory layer.
- **Current boundaries clarified**: Table supports X-axis stretch and fixed Y-axis size in this version. Tooltip intelligent placement, focus, and keyboard event systems are deferred.

Full changelog: [CHANGELOG.en.md](CHANGELOG.en.md)

## Project Structure and Design Philosophy

StellarX uses classic OOP and modular design. `include/src` are separated, `CoreTypes.h` is the central type definition entry, and `StellarX.h` is the main include header.

```text
StellarX/
├── include/
│   └── StellarX/
│       ├── StellarX.h
│       ├── CoreTypes.h
│       ├── SxLog.h
│       ├── Control.h
│       ├── Window.h
│       ├── Button.h
│       ├── Label.h
│       ├── TextBox.h
│       ├── Canvas.h
│       ├── TabControl.h
│       ├── Table.h
│       ├── Dialog.h
│       └── MessageBox.h
├── src/
│   ├── Control.cpp
│   ├── Window.cpp
│   ├── Button.cpp
│   ├── Label.cpp
│   ├── TextBox.cpp
│   ├── Canvas.cpp
│   ├── TabControl.cpp
│   ├── Table.cpp
│   ├── Dialog.cpp
│   ├── MessageBox.cpp
│   └── SxLog.cpp
├── examples/
├── docs/
├── CMakeLists.txt
├── CHANGELOG.md
├── CHANGELOG.en.md
├── README.md
├── README.en.md
├── 控件 API 文档 (StellarX GUI Framework Controls API Documentation).md
└── LICENSE
```

Design principles:

1. **Single Responsibility**: each control owns its drawing, event, and state boundary.
2. **Dependency Inversion**: containers such as Window, Canvas, and TabControl depend on the `Control` abstraction.
3. **Open/Closed**: add new controls by deriving from `Control` and implementing `draw()` and `handleEvent()`.
4. **Consistency**: drawing, events, dirty state, visibility, snapshots, and layout APIs follow common conventions.
5. **Lightweight first**: keep the EasyX + C++ low-friction workflow without heavyweight UI dependencies.

## Core Features

- **Light dependencies**: no heavyweight GUI runtime besides EasyX.
- **Native performance**: EasyX / Win32 drawing and message loop for lightweight tools and teaching scenarios.
- **Common controls**: Button, Label, TextBox, Canvas, TabControl, Table, Dialog, and MessageBox.
- **Unified layout semantics**: four-edge anchors, per-axis Stretch/FixedSize, and Start/End/Center/Proportional alignment.
- **Nested containers**: Canvas and TabControl map parent-local child coordinates to window-world coordinates.
- **Managed repaint**: event handling mutates state and records dirty coverage; Window commits repaint work in a centralized pass.
- **Dialog system**: Dialog supports modal/modeless usage; MessageBox provides a standard factory interface.
- **Debug logging**: SxLog supports console/file output, levels, tag filters, file rotation, and bilingual text selection.

## Quick Start

Get the prebuilt package from [Releases](https://github.com/Ysm-04/StellarX/releases/latest), or integrate the source directly.

### Requirements

- Windows 10+
- Visual Studio 2019+ or a compatible MSVC toolchain
- C++17
- EasyX 2022+ matching your Visual Studio version
- CMake 3.12+, optional

### Install EasyX

1. Download the latest EasyX.
2. Install the components matching your Visual Studio version.
3. Verify that your sample project can include `easyx.h` and link EasyX.

### Build with CMake

```bash
git clone https://github.com/Ysm-04/StellarX.git
cd StellarX
mkdir build
cd build
cmake ..
cmake --build .
```

### Manual Integration

- Copy `include` and `src` into your project.
- Add include path: `include/StellarX/`.
- Add all `.cpp` files under `src` to your build.
- Include the main header in your code: `#include "StellarX.h"`.

### First Window

```cpp
#include "StellarX.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Window mainWindow(800, 600, EX_SHOWCONSOLE, RGB(255, 255, 255), "My StellarX App");

    auto button = std::make_unique<Button>(30, 30, 160, 42, "Click Me");
    button->setOnClickListener([]()
    {
        SX_LOGI("Demo") << SX_T("按钮被点击", "Button clicked");
    });

    mainWindow.addControl(std::move(button));
    mainWindow.draw();
    return mainWindow.runEventLoop();
}
```

## CoreTypes Summary

### Common Enums

| Enum | Description | Common values |
| --- | --- | --- |
| `ControlShape` | Control geometry | `RECTANGLE`, `B_RECTANGLE`, `ROUND_RECTANGLE`, `B_ROUND_RECTANGLE`, `CIRCLE`, `B_CIRCLE`, `ELLIPSE`, `B_ELLIPSE` |
| `ButtonMode` | Button behavior | `NORMAL`, `TOGGLE`, `DISABLED` |
| `TextBoxmode` | TextBox mode | `INPUT_MODE`, `READONLY_MODE`, `PASSWORD_MODE` |
| `FillMode` | Fill mode | `Solid`, `Null`, `Hatched`, `Pattern`, `DibPattern` |
| `FillStyle` | Hatch fill style | `Horizontal`, `Vertical`, `FDiagonal`, `BDiagonal`, `Cross`, `DiagCross` |
| `LineStyle` | Border line style | `Solid`, `Dash`, `Dot`, `DashDot`, `DashDotDot`, `Null` |
| `MessageBoxType` | Message box button group | `OK`, `OKCancel`, `YesNo`, `YesNoCancel`, `RetryCancel`, `AbortRetryIgnore` |
| `MessageBoxResult` | Message box result | `OK=1`, `Cancel=2`, `Abort=3`, `Retry=4`, `Ignore=5`, `Yes=6`, `No=7` |
| `TabPlacement` | Tab bar placement | `Top`, `Bottom`, `Left`, `Right` |
| `LayoutMode` | Legacy layout mode | `Fixed`, `AnchorToEdges` |
| `Anchor` | Legacy anchor input | `NoAnchor`, `Left`, `Right`, `Top`, `Bottom` |
| `AxisSizePolicy` | New per-axis size policy | `Stretch`, `FixedSize` |
| `AxisAlignPolicy` | New fixed-size alignment policy | `Start`, `End`, `Center`, `Proportional` |

### Common Structs

| Struct | Description |
| --- | --- |
| `ControlText` | Font face, height, width, color, rotation, weight, italic, underline, and strikeout |
| `RouRectangle` | Corner ellipse size for rounded rectangles |
| `AxisLayoutSpec` | Per-axis anchors, size policy, and alignment policy |
| `LayoutSpec` | Horizontal and vertical layout specs |
| `LayoutCapability` | Whether a control allows X/Y axis Stretch |
| `ResolvedLayoutRect` | Parent-local and world rectangles after layout solving |

## Controls Library

### Basic Controls

| Control | Header | Description | Key points |
| --- | --- | --- | --- |
| `Button` | `Button.h` | Versatile button | NORMAL / TOGGLE / DISABLED, shapes, colors, Tooltip, truncation, callbacks |
| `Label` | `Label.h` | Text label | Transparent/opaque background, public `textStyle`, content-driven runtime size |
| `TextBox` | `TextBox.h` | Text box | Input, read-only, password mode, lightweight EasyX InputBox integration |

### Container Controls

| Control | Header | Description | Key points |
| --- | --- | --- | --- |
| `Window` | `Window.h` | Top-level window | Background, event loop, resize consolidation, managed repaint entry |
| `Canvas` | `Canvas.h` | General container | Parent-local child mapping, nested containers, managed partial repaint root |
| `TabControl` | `TabControl.h` | Tab container | Outer control uses unified layout; tab bar/page internals are self-managed |

### Advanced Controls

| Control | Header | Description | Key points |
| --- | --- | --- | --- |
| `Table` | `Table.h` | Data table | Headers, data, pagination, page label; X-axis stretch and fixed Y-axis size |
| `Dialog` | `Dialog.h` | Dialog container | Modal/modeless, derives from Canvas, recenters instead of stretching |

### Static Factory

| Control | Header | Description | Key points |
| --- | --- | --- | --- |
| `MessageBox` | `MessageBox.h` | Message box factory | `showModal()` blocks and returns result; `showAsync()` uses callback |

## Layout System

v3.1.0 recommends the new per-axis layout API instead of extending the old two-anchor model further.

### Core Semantics

- `localx/localy/localWidth/localHeight`: design-time parent-local rectangle and layout baseline.
- `x/y/width/height`: current runtime drawing rectangle.
- Normal resize, container relayout, and unified layout solving do not automatically write back to `local*`.
- Use `commitCurrentGeometryAsDesignRect()` when the current runtime geometry should become the new design baseline.

### Example

```cpp
auto panel = std::make_unique<Canvas>(20, 20, 760, 500);
panel->setHorizontalAnchors(true, true);
panel->setVerticalAnchors(true, true);
panel->setHorizontalSizePolicy(StellarX::AxisSizePolicy::Stretch);
panel->setVerticalSizePolicy(StellarX::AxisSizePolicy::Stretch);

auto rightButton = std::make_unique<Button>(620, 30, 120, 36, "Right");
rightButton->setHorizontalAnchors(false, true);
rightButton->setHorizontalSizePolicy(StellarX::AxisSizePolicy::FixedSize);
rightButton->setHorizontalAlignPolicy(StellarX::AxisAlignPolicy::End);
```

### Common Combinations

| Combination | Behavior |
| --- | --- |
| Left + Right + Stretch | Keep left/right margins and stretch width |
| Left only + FixedSize + Start | Keep left margin and fixed size |
| Right only + FixedSize + End | Keep right margin and fixed size |
| Top + Bottom + Stretch | Keep top/bottom margins and stretch height |
| None + FixedSize + Center | Fixed size, position by center relation |
| None + FixedSize + Proportional | Fixed size, position by design-time position ratio |

### Control Capability Boundaries

- `Canvas`: X/Y stretch by default.
- `TabControl`: outer X/Y stretch; internal tab bar and page area remain self-managed.
- `Table`: X stretch and fixed Y size in the current version.
- `TextBox`: usually X stretch and fixed Y size for visual consistency.
- `Label`: does not default to two-axis Stretch; content changes may drive runtime size.

## TabControl

- Tab bar supports `Top`, `Bottom`, `Left`, and `Right`.
- Use `add(std::string tabText, std::unique_ptr<Control> control)` to add a child to a named tab page.
- `indexOf(tabText)` returns the matching index or `-1` when not found.
- This version only consolidates outer layout and repaint responsibilities. It does not rewrite the internal tab bar/page layout mechanism.

## Button Text Truncation and Tooltip

- Button truncates overly long single-line text based on pixel width.
- Tooltip supports delay, cursor following, offset, transparent/opaque background, and TOGGLE state text; offset currently takes effect only in cursor-following mode.
- Tooltip display, hide, snapshot cleanup, and managed coverage are stabilized in this version.
- Tooltip intelligent placement is not implemented yet; it will not automatically reposition when outside the client area or covered by upper controls or Dialogs. When cursor following is disabled, Tooltip uses the current mouse X and the button bottom Y for placement.

## Transparent Background and Background Snapshots

- `discardBackground()`: restores old background and releases the snapshot.
- `invalidateBackgroundSnapshot()`: invalidates the snapshot without restoring old pixels.
- Resize paths invalidate snapshots instead of restoring old-size pixels into new regions.
- Tooltip transient layers are excluded from persistent coverage to avoid polluting sibling background snapshots.
- Table still keeps whole-table repaint behavior and the historical Canvas fallback special case for correctness; `Table::draw()` may still call `initTextWaH()` to refresh structural size, which remains a deferred Table-specific debt.

## SxLog

SxLog provides console/file logging, levels, tag filters, file rotation, and bilingual text selection.

```cpp
StellarX::SxLogger::Get().setMinLevel(StellarX::SxLogLevel::Debug);
StellarX::SxLogger::Get().enableFile("stellarx.log", true, 1024 * 1024);

SX_LOGI("Init") << SX_T("窗口创建成功", "Window created");
SX_TRACE_SCOPE("Layout", "resolve layout");
```

If console output causes visible delay during resize or maximize operations, lower the log level, disable console output, or switch to file output. SxLog macros currently use an `if/else` short-circuit form; avoid placing them naked in single-line `if/else` branches without braces.

## Advanced Topics and Best Practices

- Custom controls should derive from `Control` and implement `draw()` and `handleEvent()`.
- Return `true` when an event is consumed to stop propagation.
- Ensure the EasyX output target is correct before drawing; use `SetWorkingImage(nullptr)` when drawing directly to the screen.
- Prefer marking dirty and requesting managed repaint after state changes; avoid arbitrary direct drawing inside event handlers.
- Public setters are user-facing. The unified layout solver uses internal controlled paths to apply runtime rectangles.
- After directly modifying `Label::textStyle`, call `setDirty(true)` if immediate content-size refresh is required.

## Applicability and Limits

Good fits:

- C++/EasyX teaching projects.
- Lightweight desktop utilities.
- GUI mechanism demos.
- Small and medium status panels, configuration panels, and data tables.

Use with caution or avoid for:

- High-performance games or complex animations.
- Complex high-DPI and multi-monitor scaling scenarios.
- Large commercial front-end projects with complex accessibility requirements.
- Cross-platform GUI applications.

## Current Known Limitations

- Focus system is not implemented yet.
- Keyboard event system is not implemented yet.
- TextBox still uses EasyX InputBox; the self-rendered input pipeline is not rewritten.
- Table internal partial repaint is not complete yet.
- Table does not support vertical Stretch yet.
- Table structural size refresh may still happen in the draw path.
- The historical Canvas fallback special case for Table remains.
- TabControl internal layout application path is not fully consolidated yet.
- Tooltip intelligent placement is not implemented yet.
- Global automatic font scaling is not implemented yet.

## License

MIT. See [LICENSE](LICENSE).

## Contributing Guidelines

- Follow the existing C++ style.
- New features should include examples, documentation, and regression notes.
- Self-test before submitting and explain the motivation for changes.
- Submit bugs or ideas as GitHub Issues.

## Acknowledgements

- Thanks to [EasyX](https://easyx.cn/).
- Thanks to developers who value simple, efficient, and clear GUI design.

**Stars and seas, code as the vessel.**

## Support and Feedback

- See [examples/](examples/).
- Read [CHANGELOG.en.md](CHANGELOG.en.md).
- Submit an Issue on GitHub: https://github.com/Ysm-04/StellarX/issues
