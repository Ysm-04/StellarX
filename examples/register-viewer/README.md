# Register Viewer (StellarX example)

**A 32-bit register visualizer built with StellarX** (Windows + EasyX).  
Features:
- 32 toggle bits (MSB→LSB = 31..0)
- Range invert, logical left/right shift
- Hex/Decimal display with signed/unsigned toggle
- Grouped binary view with "last/current" snapshots
- One-click set all 0 / all 1

## Build & Run
1. Ensure StellarX and EasyX are in your include/lib paths.
2. Build as a normal C++ Win32 desktop app (e.g., VS 2019+).
3. Run the executable.

This example demonstrates StellarX APIs with a single `main.cpp` (~450 LOC):
`Window / Canvas / Button / Label / TextBox` + simple event callbacks.
