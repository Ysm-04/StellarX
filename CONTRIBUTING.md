# 为 StellarX 做贡献

感谢您有兴趣为 StellarX 做出贡献！本文档提供了贡献的指南和步骤。

StellarX 是一个为 **Windows 平台**打造的 C++ GUI 框架，基于 EasyX 图形库。

## 开发环境设置

1. 安装 Visual Studio 2019 或更高版本
2. 安装对应版本的 EasyX 图形库
3. 安装 CMake 3.12 或更高版本
4. 克隆项目仓库
5. 使用 CMake 生成解决方案并编译

## 如何贡献

### 报告错误
1.  检查 [Issues](../../issues) 看看这个错误是否已经被报告过。
2.  如果没有，请创建一个新的 Issue。
3.  使用 "Bug 报告" 模板。
4.  提供**清晰的标题和描述**。
5.  包括相关的代码片段、截图或重现问题的步骤。

### 建议功能增强
1.  检查现有的 Issue 中是否有你的想法。
2.  使用 "功能请求" 模板创建一个新的 Issue。
3.  清晰地描述新功能，并解释它为何有用。

### 提交代码更改 (拉取请求 Pull Requests)
1.  **Fork** GitHub 上的仓库。
2.  **Clone** 你 fork 的仓库到本地机器。
3.  为你的功能或错误修复创建一个**新分支** (`git checkout -b my-feature-branch`)。
4.  **进行你的更改**。确保你的代码遵循项目的风格（见下文）。
5.  **提交你的更改**，并附上清晰描述性的提交信息。
6.  **Push** 你的分支到你 fork 的 GitHub 仓库 (`git push origin my-feature-branch`)。
7.  针对原始 StellarX 仓库的 `main` 分支发起一个**拉取请求 (Pull Request)**。

## 代码风格指南

*   遵循项目中现有的代码格式和命名约定。
*   为变量、函数和类使用有意义的名称。
*   在必要时注释你的代码，特别是复杂的逻辑。
*   确保你的代码编译时没有警告。
*   彻底测试你的更改。
*   使用 **4个空格** 进行缩进（不要使用制表符）
- 类名使用 **PascalCase**（如 `ClassName`）
- 函数和变量使用 **camelCase**（如 `functionName`, `variableName`）
- 常量使用 **UPPER_CASE**（如 `CONSTANT_VALUE`）
- 成员变量以 **m_** 前缀（如 `m_memberVariable`）
- 控件属性使用有意义的名称，避免缩写
- 为所有公共接口添加详细的注释
- 遵循 RAII 原则管理资源

##  示例代码风格

```c++

// 好的示例
class MyControl : public Control {
public:
    MyControl(int x, int y, int width, int height) 
        : Control(x, y, width, height), m_isActive(false) {}
    
    void draw() override {
        // 绘制逻辑
    }
    
private:
    bool m_isActive;
};

// 不好的示例
class my_control: public Control{
public:
    my_control(int x,int y,int w,int h):Control(x,y,w,h),active(false){}
    void Draw() override{
        // 绘制逻辑
    }
private:
    bool active;
};
```

## 项目结构

请遵循项目的目录结构：
- 头文件放在 `include/StellarX/` 目录
- 实现文件放在 `src/` 目录
- 示例代码放在 `examples/` 目录

## 问题？

如果你有任何问题，可以随时开一个 Issue 或联系维护者。