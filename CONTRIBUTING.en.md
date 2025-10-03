# Contributing to StellarX

Thank you for your interest in contributing to StellarX! This document provides guidelines and steps for contributing.

StellarX is a C++ GUI framework built for the **Windows platform**, based on the EasyX graphics library.

## Development Environment Setup

1. Install Visual Studio 2019 or later
2. Install the corresponding version of EasyX graphics library
3. Install CMake 3.12 or later
4. Clone the project repository
5. Use CMake to generate the solution and compile

## How to Contribute

### Reporting Bugs
1.  Check [Issues](../../issues) to see if the bug has already been reported.
2.  If not, create a new Issue.
3.  Use the "Bug Report" template.
4.  Provide a **clear title and description**.
5.  Include relevant code snippets, screenshots, or steps to reproduce the issue.

### Suggesting Enhancements
1.  Check existing Issues to see if your idea has been suggested.
2.  Create a new Issue using the "Feature Request" template.
3.  Clearly describe the new feature and explain why it would be useful.

### Submitting Code Changes (Pull Requests)
1.  **Fork** the repository on GitHub.
2.  **Clone** your forked repository to your local machine.
3.  Create a **new branch** for your feature or bug fix (`git checkout -b my-feature-branch`).
4.  **Make your changes**. Ensure your code follows the project's style (see below).
5.  **Commit your changes** with clear, descriptive commit messages.
6.  **Push** your branch to your forked GitHub repository (`git push origin my-feature-branch`).
7.  Open a **Pull Request** against the original StellarX repository's `main` branch.

## Code Style Guide

*   Follow the existing code formatting and naming conventions in the project.
*   Use meaningful names for variables, functions, and classes.
*   Comment your code when necessary, especially for complex logic.
*   Ensure your code compiles without warnings.
*   Test your changes thoroughly.
*   Use **4 spaces** for indentation (no tabs)
*   Class names use **PascalCase** (e.g., `ClassName`)
*   Functions and variables use **camelCase** (e.g., `functionName`, `variableName`)
*   Constants use **UPPER_CASE** (e.g., `CONSTANT_VALUE`)
*   Member variables use **m_** prefix (e.g., `m_memberVariable`)
*   Use meaningful names for control properties, avoid abbreviations
*   Add detailed comments for all public interfaces
*   Follow RAII principles for resource management

## Example Code Style

```c++
// Good Example
class MyControl : public Control {
public:
    MyControl(int x, int y, int width, int height) 
        : Control(x, y, width, height), m_isActive(false) {}
    
    void draw() override {
        // Drawing logic
    }
    
private:
    bool m_isActive;
};

// Bad Example
class my_control: public Control{
public:
    my_control(int x,int y,int w,int h):Control(x,y,w,h),active(false){}
    void Draw() override{
        // Drawing logic
    }
private:
    bool active;
};
```

## Project Structure

Please follow the project's directory structure:

- Header files go in the `include/StellarX/` directory
- Implementation files go in the `src/` directory
- Example code goes in the `examples/` directory

## Questions?

If you have any questions, feel free to open an Issue or contact the maintainers.

