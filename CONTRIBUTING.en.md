# Contributing to StellarX

Thank you for helping improve StellarX. StellarX is a C++17 GUI framework for Windows built on EasyX. Before changing code, identify the behavioral boundary and keep source, examples, and documentation consistent.

## Development Environment

- Visual Studio 2019 or later with the Desktop development with C++ workload
- An EasyX installation matching Visual Studio; set `EASYX_ROOT` if CMake cannot find `EasyXa.lib`
- CMake 3.15 or later
- Git

Use an out-of-source build directory:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DSTELLARX_BUILD_EXAMPLES=ON `
  -DSTELLARX_USE_STATIC_MSVC_RUNTIME=ON `
  -DEASYX_ROOT="D:/path/to/EasyX"
cmake --build build --config Release
```

Select the matching generator when using Visual Studio 2019. Do not commit local CMake caches, temporary test results, or debug logs.

## Reporting Issues and Suggesting Changes

Search existing Issues before filing a new one. A useful bug report includes:

- StellarX, Visual Studio, EasyX, and Windows versions;
- minimal reproducible code, exact steps, and expected behavior;
- actual behavior, diagnostics, screenshots, or logs;
- for input-method issues, the IME name, display scaling, and multi-monitor setup.

Feature proposals should explain the use case, the limitation of current behavior, and the compatibility boundary that should be preserved.

## Submitting a Pull Request

1. Fork the repository and create a focused branch from the latest `master`.
2. Keep the change scoped; avoid unrelated formatting and generated files.
3. Build affected targets and run proportional automated and manual checks.
4. Update affected examples, README files, API documentation, or changelog entries.
5. Use descriptive commits and open the Pull Request against the original `master` branch.

The Pull Request description should state the motivation, observable behavior changes, verification performed, untested environments, and any source or binary compatibility impact.

## Source Layout

- `include/StellarX/`: public headers available to applications; public API changes must be reflected in the API documentation.
- `src/`: framework implementation files.
- `src/internal/`: implementation-only private headers; applications must not include them.
- `examples/`: independently buildable examples and their documentation.
- `CMakeLists.txt`: authoritative source, header, and build-option inventory.

When adding, removing, or renaming framework sources, update the CMake inventory and verify that Git recorded filename case changes correctly on case-insensitive Windows filesystems.

## Code and Comment Style

- Follow the formatting and naming used by surrounding code; do not reformat unrelated files.
- Types use PascalCase, while functions and local variables generally use camelCase. Preserve historical public API names for compatibility instead of renaming them solely for consistency.
- Follow the indentation, line-break, and brace style of the file. The project does not require a repository-wide four-space conversion or a uniform member prefix.
- Prefer RAII, owning strings, smart pointers, and stable handles for ownership and lifetime.
- Every public interface must have a Chinese declaration comment describing its purpose, parameters, or important semantics. Complex implementation comments should explain rationale, constraints, and invariants rather than restating code or referring to temporary development phases.
- User-facing comments describe observable behavior; internal comments describe threading, reentrancy, focus, IME, and deferred-mutation boundaries that are easy to misuse.
- Do not retain an unvalidated raw pointer across a control callback. Store long-lived identity as `ControlHandle` or `FocusTargetId`.
- Implement keyboard behavior through directed routing APIs such as `handleKeyEvent()`; do not reintroduce `handleEvent(EX_KEY)` as a business-input path.
- Text editing uses UTF-16 internally. Windows ACP APIs are compatibility boundaries, not a new storage model.

## Verification

At minimum:

- build the Release configuration without introducing warnings;
- launch, interact with, and close affected examples;
- run relevant automated tests and manual IME checks for focus, keyboard, pointer, TextBox, or IME changes;
- keep version numbers, API names, commands, and limitations aligned across Chinese and English documentation;
- run `git diff --check` and confirm that no accidental binaries or machine-specific paths are included.

State any untested environment explicitly in the Pull Request. Do not report an untested path as passing.

## Need Help?

If an API boundary or verification method is unclear, open an Issue for discussion before beginning a broad implementation.
