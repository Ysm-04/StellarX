# StellarX 示例程序

本目录收录 9 个相互独立的 StellarX 示例。每个子目录均以单独的 `main.cpp` 作为程序入口，不应把多个示例源码同时加入同一个可执行项目。

## 示例列表与来源

| 目录 | 展示内容 | 来源 |
| --- | --- | --- |
| `basic-window` | Window、Button、模态 MessageBox | 项目作者提供 |
| `login-interface` | 表单、密码框、Canvas 显隐、TabControl | 项目作者提供 |
| `register-viewer` | 32 位切换、位运算、多进制显示 | 项目作者提供 |
| `sxlog-demo` | 日志级别、双语文本、计时和 Tag 过滤 | 项目作者提供 |
| `basketball-scoreboard` | 篮球计分牌及控件联动 | 熊猫老师反馈并提供 |
| `guess-number` | 猜数字、随机数和输入校验 | 熊猫老师反馈并提供 |
| `bmi-calculator` | BMI 计算、范围校验和图片背景 | 熊猫老师反馈并提供 |
| `temperature-converter` | 摄氏度与华氏度双向转换 | 熊猫老师反馈并提供 |
| `quote-viewer` | 固定数据集合和前后翻页 | 熊猫老师反馈并提供 |

## 直接运行

每个目录内的同名 `.exe` 是使用 Visual Studio 2022、C++17、x64 Release 和静态运行库 `/MT` 构建的预览程序，仅适用于 64 位 Windows。这些程序未进行数字签名，Windows SmartScreen 可能显示安全提示。

仓库中的现有 `.exe` 仍是 v3.1.0 历史预览产物，本次 v4.0.0 源码更新未重新生成它们。验证新版源码时请按下文重新构建，不要用现有 `.exe` 判断 v4.0.0 行为。

源码统一保存为 UTF-8；鉴于当前 StellarX/EasyX 使用窄字符串绘制中文，MSVC 构建会通过 `/execution-charset:.936` 生成 GBK 字符串，避免中文控件乱码。

`login-interface` 和 `bmi-calculator` 依赖各自的 `assets/` 目录；移动 EXE 时应同时复制资源目录。资源缺失时，这两个示例会退回纯色背景，不影响其余交互。

## 使用 CMake 构建

安装 Visual Studio 2022 与匹配架构的 EasyX 后，在仓库根目录执行：

```powershell
cmake -S . -B build -A x64 -DSTELLARX_BUILD_EXAMPLES=ON -DSTELLARX_USE_STATIC_MSVC_RUNTIME=ON
cmake --build build --config Release
```

CMake 构建输出位于 `build/bin/Release/`。
