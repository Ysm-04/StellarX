#  API 文档

[English API Documentation](API Documentation.en.md)

下面是 **StellarX GUI 框架** 各主要类和函数的 API 文档。每个类的用途、接口、参数和返回值等详细信息如下：

## Control 类 (抽象基类)

**描述：** `Control` 是所有控件的抽象基类，定义了通用的属性和接口，包括位置、尺寸、重绘标记等基础功能。它提供绘图状态的保存与恢复机制，确保控件的绘制操作不影响全局绘图状态。此外还声明了一系列供子类实现的纯虚函数（如 `draw()` 和 `handleEvent()`），并禁止拷贝（支持移动语义）以防止意外的复制开销。一般情况下，`Control` 不直接实例化，而作为其他具体控件的父类存在。

**特性：**

- 定义控件的基本属性（坐标、尺寸、可见性、脏标记等）并提供对应的存取方法。
- 提供绘图状态管理接口（`saveStyle()`/`restoreStyle()` 等）用于在控件绘制前后保存和恢复全局画笔状态。
- 声明纯虚接口，如 `draw()`（绘制控件）和 `handleEvent()`（处理事件），所有具体控件都需实现。
- 提供移动构造和移动赋值支持，防止控件被不小心拷贝。

### 公共成员函数

#### Control::updateBackground()

- **用途：** 释放之前保存的背景快照并重新捕获当前背景，用于在控件尺寸变化或内容变化后更新其背景缓存。调用此函数可确保控件背景的正确性，避免因为尺寸变化导致的背景错位。

- **参数：** 无。

- **返回值：** 无。

- **行为细节：** 默认实现中，`updateBackground()` 会先丢弃旧的背景位图（相当于调用内部的 `discardBackground()`），然后在控件当前位置和新尺寸范围内重新保存一份背景快照。这通常在控件大小改变或重绘需求发生时自动调用，一般不需要用户手动调用。

- **示例：** 假设有自定义控件在大小调整时需要刷新背景，可以在调整尺寸后调用：

  ```c++
  myControl.setWidth(newW);
  myControl.setHeight(newH);
  myControl.updateBackground(); // 更新背景快照以匹配新尺寸
  ```

#### Control::onWindowResize()

- **用途：** 当父窗口尺寸发生变化时，通知控件进行相应处理的事件函数。用于在窗口大小改变时调整控件状态，例如作废过期的背景缓存。

- **参数：** 无。

- **返回值：** 无。

- **行为细节：** 默认实现中，`onWindowResize()` 会调用 `discardBackground()` 丢弃控件保存的背景位图，以防止窗口尺寸变化后背景错位。容器控件会重写该方法（例如 `Canvas` 会在自身处理后继续调用子控件的 `onWindowResize()`）。一般用户不需要主动调用此方法——框架会在窗口或容器尺寸变化时自动触发调用。

- **示例：** 自定义控件如需在窗口大小改变时执行特定逻辑，可以重写此函数：

  ```c++
  void MyControl::onWindowResize()
  {
      Control::onWindowResize(); // 调用基类以丢弃背景等
      // 自定义逻辑，例如根据新窗口尺寸调整位置
      repositionBasedOnNewWindowSize();
  }
  ```

#### Control::getX() / getY() / getWidth() / getHeight()

- **用途：** 获取控件的全局坐标位置（左上角 `x, y`）或当前尺寸（宽度、高度）。

- **参数：** 无。

- **返回值：** 分别返回控件的全局 X 坐标、全局 Y 坐标、当前宽度、高度，类型均为 `int`。

- **行为细节：** 这些方法提供控件在窗口坐标系下的位置和大小，通常用于布局计算或调试。注意，全局坐标是相对于整个应用窗口的原点(0,0)。

- **示例：**

  ```c++
  int x = ctrl.getX();
  int h = ctrl.getHeight();
  std::cout << "Control at X = " << x << ", height = " << h << std::endl;
  ```

#### Control::getRight() / getBottom()

- **用途：** 获取控件的右边缘的 X 坐标 (`getRight()`) 和下边缘的 Y 坐标 (`getBottom()`)，以全局坐标计算。

- **参数：** 无。

- **返回值：** 控件右边缘 X 坐标、底边缘 Y 坐标，类型为 `int`。

- **行为细节：** 这两个方法相当于 `getX() + getWidth()` 和 `getY() + getHeight()`，用于方便地获取控件在窗口坐标系中覆盖的边界位置。

- **示例：**

  ```c++
  if (cursorX < ctrl.getRight() && cursorY < ctrl.getBottom()) 
  {
      // 光标在ctrl控件矩形范围内
  }
  ```

#### Control::getLocalX() / getLocalY() / getLocalWidth() / getLocalHeight()

- **用途：** 获取控件相对于父容器坐标系的位置 (`LocalX, LocalY`) 以及在父容器内的尺寸 (`LocalWidth, LocalHeight`)。

- **参数：** 无。

- **返回值：** 控件相对父容器的 X 坐标、Y 坐标、宽度、高度，类型均为 `int`。

- **行为细节：** 当控件被添加到容器（例如 `Canvas`）时，其本地坐标以容器的左上角为原点。如果控件没有父容器（直接属于窗口），则本地坐标与全局坐标相同。这些方法对于在容器内部布局控件特别有用。

- **示例：** 假设有一个容器 `canvas`，其中包含子控件 `childCtrl`：

  ```c++
  int localX = childCtrl.getLocalX();
  int globalX = childCtrl.getX();
  std::cout << "Child global X: " << globalX 
            << ", local X in canvas: " << localX << std::endl;
  ```

#### Control::getLocalRight() / getLocalBottom()

- **用途：** 获取控件相对于父容器坐标系的右边缘和底边缘坐标。

- **参数：** 无。

- **返回值：** 控件在父容器内的右侧坐标、下侧坐标，类型为 `int`。

- **行为细节：** 分别等价于 `getLocalX() + getLocalWidth()` 和 `getLocalY() + getLocalHeight()`。当需要确定控件在容器内部的边界位置时，这些方法提供了便利。

- **示例：** 可用于判断子控件是否超出了容器范围：

  ```c++
  if (childCtrl.getLocalRight() > canvas.getWidth())
  {
      // 子控件在容器内的右边缘超出了容器宽度
  }
  ```

#### Control::setX(int x) / setY(int y) / setWidth(int w) / setHeight(int h)

- **用途：** 设置控件的全局坐标位置（左上角 X 或 Y）或调整控件的尺寸（宽度或高度）。

- **参数：** `x` 或 `y` 为新的全局坐标；`w` 为新的宽度值；`h` 为新的高度值（类型均为 `int`）。

- **返回值：** 无。

- **行为细节：** 调用这些方法将直接修改控件的位置或大小，并自动将控件标记为“需要重绘”（内部将 `dirty` 标志置为 `true`），使得下一轮刷新时控件会按照新位置或新尺寸绘制。需要注意，如果控件在容器中，设置全局坐标会改变其在窗口中的位置，但相对父容器的位置也会随之更新。

- **示例：** 将控件 `ctrl` 移动到窗口坐标(100,100)，并调整宽度为200：

  ```c++
  ctrl.setX(100);
  ctrl.setY(100);
  ctrl.setWidth(200);
  // 高度保持不变，如果需要也可调用 ctrl.setHeight(newHeight);
  ```

#### Control::draw() (纯虚函数)

- **用途：** 绘制控件的抽象接口，由具体控件类实现实际的绘图逻辑。

- **参数：** 无（绘制所需信息由控件内部状态确定）。

- **返回值：** 无。

- **行为细节：** `draw()` 会在框架重绘时由系统调用，具体控件应在实现中完成自身的绘图工作。通常一个 `draw()` 实现需要先调用 `Control::saveStyle()` 保存当前绘图状态，然后设置绘图颜色/字体等并绘制，再调用 `Control::restoreStyle()` 恢复状态。**用户不应直接调用**此方法；若要强制重绘，可以将控件标记为脏 (`setDirty(true)`) 并等待系统调用。

- **示例：** 以下为自定义控件实现 `draw()` 的示例伪代码：

  ```c++
  void MyControl::draw()
  {
      saveStyle(); 
      setfillcolor(backgroundColor);
      bar(x, y, x+width, y+height); // 绘制背景矩形
      // ... 其他绘制 ...
      restoreStyle();
  }
  ```

#### Control::handleEvent(const ExMessage& msg) (纯虚函数)

- **用途：** 事件处理抽象接口，由具体控件实现，用于处理鼠标、键盘等交互事件。

- **参数：** `msg` 为 EasyX 框架的事件消息 (`ExMessage`)，包含了事件类型、鼠标坐标、按键等信息。

- **返回值：** 布尔值，`true` 表示该事件已被控件处理（“消费”），不需要继续传递；`false` 表示控件未处理该事件，事件可交由其它对象或默认逻辑处理。

- **行为细节：** `handleEvent()` 方法同样由系统在事件循环中自动调用，用于让控件响应用户输入。不同控件根据需要处理不同类型的事件（如按钮处理鼠标点击，文本框处理键盘输入）。如果控件识别并处理了某个事件（例如鼠标点击在按钮范围内），应该返回 `true`，表示事件到此为止，不再向上传播。对于未处理的事件则返回 `false`。

- **示例：** 简化的按钮控件事件处理逻辑：

  ```c++
  bool Button::handleEvent(const ExMessage& msg)
  {
      if(msg.message == WM_LBUTTONDOWN && isPointInButton(msg.x, msg.y)) {
          // 处理按钮点击
          onClickCallback(); // 调用已注册的回调
          return true;
      }
      return false;
  }
  ```

#### Control::setIsVisible(bool show)

- **用途：** 设置控件的可见状态，动态显示或隐藏控件。传入 `false` 将隐藏控件，传入 `true` 则显示控件。

- **参数：** `show` 为布尔值，指定是否让控件可见。`true` 表示可见，`false` 表示隐藏。

- **返回值：** 无。

- **行为细节：** 该方法允许在运行时切换控件的显示状态。调用后控件的 `IsVisible()` 状态将改变，并自动触发重绘逻辑：隐藏时控件内容不再绘制；重新显示时控件会被标记需要重绘。对于容器控件（如 `Canvas`），本方法已被重写以**递归影响其子控件**：隐藏一个容器时，其所有子控件也会被隐藏；再次显示容器时，子控件也随之显示。这简化了整组界面的显隐控制。

- **示例：**

  ```c++
  button.setIsVisible(false); // 隐藏一个按钮
  // ... 某些操作后 ...
  button.setIsVisible(true);  // 再次显示按钮
  ```

#### Control::setParent(Control* parent)

- **用途：** 设置控件的父容器指针。通常由容器在添加控件时调用，手动调用情形较少。

- **参数：** `parent` 指向新的父容器控件 (`Canvas` 等) 的指针。传入 `nullptr` 则表示清除父容器关联。

- **返回值：** 无。

- **行为细节：** 该方法将内部的 `parent` 指针设置为指定容器，并相应调整控件的坐标解释方式（有父容器时本地坐标相对于父容器）。在将控件添加到某个容器时，框架会自动调用此函数，无需用户干预。**注意：** 将控件从一个容器转移到另一个时，除了调用 `setParent()` 外，还需要确保在旧容器中移除、在新容器中添加控件（使用容器的 `addControl()` 等），否则界面状态不一致。

- **示例：**

  ```c++
  canvas1.addControl(std::move(ctrl));    // 内部自动 setParent(&canvas1)
  // 如需从 canvas1 转移到 canvas2:
  canvas1.removeControl(ctrlPtr);
  canvas2.addControl(std::move(ctrlPtr)); // 内部 setParent(&canvas2)
  ```

#### Control::setDirty(bool dirty)

- **用途：** 手动设置控件的“需要重绘”标记。当控件的内容或属性改变且希望立即刷新显示时，可调用此函数。

- **参数：** `dirty` 布尔值，`true` 表示标记控件需要重绘，`false` 表示清除重绘标记。

- **返回值：** 无。

- **行为细节：** 将 `dirty` 设置为 `true` 后，会在下一次屏幕刷新时调用控件的 `draw()` 重新绘制。通常框架内部在控件属性变化时已经设置了该标记，例如调用 `setX()/setWidth()` 等修改位置尺寸的方法后，控件已被标记脏。开发者也可以在自定义场景下调用它，例如控件内部状态变化但没有对应的 setter 方法时。一般不需要将 `dirty` 手动置为 `false`，因为重绘后框架会自动清除该标记。

- **示例：**

  ```c++
  ctrl.setDirty(true); // 请求ctrl控件重绘
  ```

#### Control::IsVisible() const

- **用途：** 检查控件当前是否处于可见状态。

- **参数：** 无。

- **返回值：** `bool`，当前控件是否可见。`true` 表示可见，`false` 表示被隐藏。

- **行为细节：** 该方法返回控件内部 `show` 标志位的状态。初始情况下控件默认为可见 (`true`)，如果通过 `setIsVisible(false)` 或控件所属父容器被隐藏，则此方法返回 `false`。当控件不可见时，其 `draw()` 将不会被调用。

- **示例：**

  ```c++
  if (!ctrl.IsVisible())
  {
      ctrl.setIsVisible(true); // 确保控件可见
  }
  ```

## Window 类 (应用主窗口)

**描述：** `Window` 类代表应用程序的主窗口，它管理窗口的创建、消息循环以及全局的控件容器。`Window` 既负责初始化图形窗口（基于 EasyX），也是所有顶层控件的载体（充当它们的父级容器）。它处理系统事件的分发、定时重绘等，使用户只需关注向窗口添加控件和对事件作出响应。一个应用典型地只创建一个 `Window` 实例。

**特性：**

- 提供多种窗口初始化模式（如是否双缓冲、是否显示控制台等）以适应不同应用需求。
- 支持设置窗口背景颜色或背景图片，以及窗口标题。
- 内置**对话框管理**系统：跟踪已弹出的模态/非模态对话框，避免重复弹出相同对话框。
- 集成完整的消息处理循环，自动分发事件给各控件的 `handleEvent()` 方法，并调度重绘各控件的 `draw()`。
- 管理所有添加到窗口的控件和对话框的生命周期，在窗口销毁时自动清理。

### 公共成员函数

#### Window(int width, int height, int mode, COLORREF bkColor = BLACK, std::string headline = "窗口") (构造函数)

- **用途：** 创建一个指定大小的应用程序窗口，并进行必要的图形初始化。可以指定窗口模式、背景颜色和窗口标题。

- **参数：**

  - `width`：窗口宽度（像素）。
  - `height`：窗口高度（像素）。
  - `mode`：窗口模式标志（int 类型）。不同的值可配置窗口属性，例如是否使用**双缓冲**绘图、是否显示控制台窗口等。典型地，0 表示默认（窗口模式，隐藏控制台），也可以使用 EasyX 提供的常量组合此参数，如 `EW_SHOWCONSOLE` 显示控制台，`EW_NOCLOSE` 禁止关闭按钮等。
  - `bkColor` *(可选)*：窗口背景色，默认为黑色 (`BLACK`)。
  - `headline` *(可选)*：窗口标题文本，默认为“窗口”。

- **返回值：** 无（构造函数）。

- **行为细节：** 构造 `Window` 对象时，会调用 EasyX 的图形初始化函数 `initgraph(width, height, mode)` 创建窗体，并自动应用提供的背景色以及设置窗口标题。如果模式开启了双缓冲，则框架会采用手动刷新机制。创建窗口后，**必须调用** `runEventLoop()` 进入消息循环，窗口才开始响应事件和绘制。

- **示例：**

  ```c++
  // 创建800x600的窗口，双缓冲且显示控制台，背景为白色，标题为“示例”
  int mode = EW_SHOWCONSOLE | EW_NOBORDER; // 仅示例模式组合
  Window mainWin(800, 600, mode, WHITE, "示例");
  mainWin.draw();
  mainWin.runEventLoop();
  ```

#### Window::~Window() (析构函数)

- **用途：** 销毁窗口对象并清理资源。

- **参数：** 无。

- **返回值：** 无。

- **行为细节：** 当 `Window` 对象被销毁时，框架会自动关闭图形窗口、释放 EasyX 图形资源，并清理窗口中托管的控件和对话框对象。通常不需要显式调用，直接让 `Window` 对象离开作用域或程序结束时由系统回收。

- **示例：**

  ```c++
  { 
      Window win(400, 300, 0);
      // ... 使用 win ...
  } // 作用域结束，自动析构，窗口关闭
  ```

#### Window::runEventLoop()

- **用途：** 进入窗口消息处理循环，启动 GUI 主事件循环。调用此函数后，窗口开始响应用户交互并持续运行，直到窗口关闭。

- **参数：** 无。

- **返回值：** `int`，窗口消息循环的退出代码（通常可直接返回给 `main()`）。

- **行为细节：** 该函数内部实现一个典型的事件循环：不断调用 EasyX 的 `peekmessage()` 获取事件消息，并将其分发给窗口内各控件的 `handleEvent()` 方法处理。同时，它负责按需重绘：每帧遍历所有控件调用其 `draw()` 方法。在循环中窗口也会检查模态对话框的存在并阻止底层控件交互等。**必须**在创建窗口并添加控件后调用此方法，否则窗口将一片空白且无法交互。

- **示例：**

  ```c++
  Window win(640, 480, EW_SHOWCONSOLE, LIGHTGRAY, "Demo");
  // ... 添加控件等初始化 ...
  wni.draw();
  return win.runEventLoop(); // 进入事件循环，阻塞直到窗口关闭
  ```

#### Window::addControl(std::unique_ptr<Control> control)

- **用途：** 将一个新控件添加到窗口中进行管理和显示。可以添加诸如按钮、文本框、容器等任何继承自 `Control` 的控件。

- **参数：** `control` 为一个指向待添加控件的 `unique_ptr` 智能指针。函数内部会接管该指针的所有权。

- **返回值：** 无。

- **行为细节：** 添加控件时，窗口会将控件纳入内部维护的控件列表中，不会i调用控件的 `setParent()` 。此后框架每次重绘和事件处理都会考虑该控件。控件在窗口坐标系中的初始位置和尺寸取决于控件自身的属性。如果需要移除控件，目前可以通过 `clearAllControls()`（在 `Canvas` 中）或销毁窗口实现。

- **示例：**

  ```c++
  auto btn = std::make_unique<Button>(50, 50, 80, 30, "OK");
  mainWin.addControl(std::move(btn)); // 将按钮添加到主窗口显示
  ```

#### Window::addDialog(std::unique_ptr<Control> dialog)

- **用途：** 弹出一个对话框控件并添加到窗口。当添加对话框时使用。

- **参数：** `dialog` 是指向 `Dialog` 或其他对话框控件对象的 `unique_ptr`。

- **返回值：** 无。

- **行为细节：** 该函数类似于 `addControl()`，但专门用于对话框控件。窗口会将对话框添加到内部对话框列表，并在事件循环中给予特殊处理（例如当存在模态对话框时，阻止其他控件交互）。框架还提供防重复机制：如果即将添加的对话框在当前已经存在（通过对话框的标题和消息匹配），可以避免重复添加。通常不直接使用此函数来添加模态对话框——而是通过 `MessageBox::showModal()` 简化调用。

- **示例：**

  ```c++
  auto dlg = std::make_unique<Dialog>(mainWin, "警告", "出现错误", MessageBoxType::OKCancel);
  mainWin.addDialog(std::move(dlg)); // 非模态对话框，异步显示
  ```

#### Window::hasNonModalDialogWithCaption(const std::string& caption, const std::string& message) const

- **用途：** 检查当前窗口中是否已经存在指定标题和内容的非模态对话框。

- **参数：** `caption` 为对话框标题文本；`message` 为对话框显示的消息内容。

- **返回值：** `bool`，存在相同标题和内容的非模态对话框则返回 `true`，否则返回 `false`。

- **行为细节：** 此方法用于防止重复弹出相同的提示框。当调用 `MessageBox::showAsync()` 显示非模态对话框时，内部会利用该函数判断是否已有相同内容的对话框未关闭，从而避免界面上出现多个重复对话框。一般用户不需要直接调用它，但了解其存在有助于理解框架的防重复机制。

- **示例：** （框架内部用法）

  ```c++
  if (!hasNonModalDialogWithCaption("警告", "出现错误"))
  {
      // 安全弹出新对话框
      addDialog(std::make_unique<Dialog>(...));
  }
  ```

#### Window::draw()

- **用途：** 手动触发窗口内容重绘。

- **参数：** 无。

- **返回值：** 无。

- **行为细节：** 默认情况下，框架每帧都会自动调用内部的绘制逻辑，所以一般无需手动调用此函数。但在某些特殊情况下（如希望强制立即重绘某些内容），可以调用 `mainWin.draw()`。该函数会遍历所有已添加到窗口的控件并调用其 `draw()` 方法进行绘制。如果使用了双缓冲模式，则 draw() 将在后台完成绘制并更新屏幕。

- **示例：**

  ```c++
  // 改变背景后立即刷新窗口显示
  mainWin.setBkcolor(WHITE);
  mainWin.draw();
  ```

#### Window::draw(const std::string& pImgFile)

- **用途：** 将指定的图像文件绘制为窗口背景并刷新显示。

- **参数：** `pImgFile`，图像文件路径（string）。

- **返回值：** 无。

- **行为细节：** 此函数提供简便途径设置窗口背景图。调用时，函数会加载指定路径的图像并铺满窗口背景，然后调用窗口的其他控件的 `draw()` 进行正常绘制。相当于设置背景图片后立即刷新。若指定的文件无法加载或非有效图像，将保持原背景不变。

- **示例：**

  ```c++
  mainWin.draw("background.jpg"); // 以background.jpg作为背景并重绘窗口
  ```

#### Window::setBkImage(const std::string& pImgFile)

- **用途：** 设置窗口背景图片，但不会立即绘制。

- **参数：** `pImgFile`，图像文件路径。

- **返回值：** 无。

- **行为细节：** 该方法更改窗口背景图设置，但不会自动触发重绘（需等待下一次事件循环刷新或调用 `draw()`）。如果需要立即生效，可配合 `draw()` 使用。设置空字符串或无效路径将取消背景图片，仅保留背景色。

- **示例：**

  ```c++
  mainWin.setBkImage("bg.png");
  // 下一帧自动绘制新背景，或手动 mainWin.draw();
  ```

#### Window::setBkcolor(COLORREF c)

- **用途：** 设置窗口背景颜色。

- **参数：** `c`，颜色值（COLORREF，例如 `WHITE`、`RGB(255,255,255)` 等）。

- **返回值：** 无。

- **行为细节：** 修改窗口背景色，新的背景色会在下一次刷新时生效。如果之前设置了背景图片，新颜色会被背景图覆盖（除非移除背景图）。一般在创建窗口时可以通过构造函数参数直接指定背景色，此函数用于运行期间动态更改背景色。

- **示例：**

  ```c++
  mainWin.setBkcolor(LIGHTGRAY);
  ```

#### Window::setHeadline(const std::string& headline)

- **用途：** 设置窗口标题栏文本。

- **参数：** `headline`，字符串，新标题文本。

- **返回值：** 无。

- **行为细节：** 调用此函数将更改窗口的标题栏内容（通过 Win32 API 调用 `SetWindowText` 实现）。该更改是即时的，无需刷新循环。

- **示例：**

  ```c++
  mainWin.setHeadline("处理完成");
  ```

### 获取属性方法（Getters）

#### Window::getWidth() / getHeight()

- **用途：** 获取窗口的当前宽度或高度。

- **参数：** 无。

- **返回值：** 窗口宽度或高度（int）。

- **行为细节：** 该尺寸为窗口绘图区域的大小（与创建时指定的尺寸一致，除非窗口在运行中被拉伸）。如果窗口支持拉伸并被用户调整大小，这两个值会动态更新。

- **示例：**

  ```c++
  int w = mainWin.getWidth();
  int h = mainWin.getHeight();
  ```

#### Window::getHeadline()

- **用途：** 获取窗口当前的标题文本。

- **参数：** 无。

- **返回值：** 标题字符串 (`std::string`)。

- **行为细节：** 如果未设置则返回默认标题（创建时指定或默认的“窗口”）。

- **示例：**

  ```c++
  std::string title = mainWin.getHeadline();
  ```

#### Window::getBkcolor()

- **用途：** 获取窗口当前背景颜色值。

- **参数：** 无。

- **返回值：** 背景色 (`COLORREF`)。

- **行为细节：** 如窗口正使用背景图片，该颜色通常仍然返回最近设置的颜色值，但当前绘制时被背景图覆盖。

- **示例：**

  ```c++
  COLORREF c = mainWin.getBkcolor();
  ```

#### Window::getBkImage()

- **用途：** 获取窗口当前使用的背景图像对象。

- **参数：** 无。

- **返回值：** 指向背景 `IMAGE` 对象的指针，若无背景图片则可能返回 `nullptr`。

- **行为细节：** 返回的 `IMAGE*` 受窗口内部管理，**请勿修改或释放**。仅用于只读场景（如根据背景图大小做布局等）。

- **示例：**

  ```c++
  IMAGE* bg = mainWin.getBkImage();
  if(bg) 
  {
      // 例如：获取背景图宽高
      int bgW = bg->getwidth();
      int bgH = bg->getheight();
  }
  ```

#### Window::getHwnd()

- **用途：** 获取窗口的操作系统句柄 (`HWND`)。

- **参数：** 无。

- **返回值：** Windows 窗口句柄 (`HWND`)。

- **行为细节：** 这个句柄可用于调用底层 Win32 API 以实现框架未提供的窗口级功能。通常不建议直接操作窗口句柄，除非高级需求（如修改窗口样式等）。

- **示例：**

  ```c++
  HWND hWnd = mainWin.getHwnd();
  // 调用 Win32 API，例如改变窗口图标:
  SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
  ```

#### Window::getControls()

- **用途：** 获取对窗口内所有顶级控件容器的引用列表。

- **参数：** 无。

- **返回值：** `std::vector<std::unique_ptr<Control>>&`，窗口管理的控件列表引用。

- **行为细节：** 返回窗口内部维护的控件容器，可以遍历其中的元素指针来检查或调试当前有哪些控件已经添加。请勿尝试修改此列表（例如插入/删除），以免破坏框架内部状态。

- **示例：**

  ```c++
  for(const auto& ctrl : mainWin.getControls())
  {
      std::cout << typeid(*ctrl).name() << std::endl;
  }
  ```

## Canvas 类 (容器控件)

**描述：** `Canvas` 是一个容器类控件，用于分组和管理子控件。它自身也是一个可见控件，可以绘制背景和边框。典型用途是作为**布局面板**：将多个子控件添加到 Canvas 中，由 Canvas 统一处理子控件的事件分发和绘制顺序。Canvas 支持多种布局模式（绝对定位或简单的水平/垂直布局），从而简化界面布局工作。

**特性：**

- 支持四种矩形形状（矩形、圆角矩形，以及各自的无边框版本）作为容器外观，可设置边框有无和圆角程度等。
- 提供自定义的填充方式（纯色填充或背景图片）和边框样式（颜色、线型、线宽）。
- 自动管理子控件：接管子控件的生命周期，在 Canvas 析构时自动销毁其中的控件；同时在事件发生时将事件传递给合适的子控件处理。
- 支持嵌套容器结构：Canvas 本身也可作为另一个 Canvas 的子控件，实现复杂布局。
- 简易布局能力：当前支持**绝对布局**（通过直接设置子控件坐标）。**水平盒子HBox/垂直盒子VBox布局**）、Grid、Flow、Stack 等布局类型目前预留。

### 公共成员函数

#### Canvas() / Canvas(int x, int y, int width, int height) (构造函数)

- **用途：** 创建一个容器控件。可以使用默认构造函数创建一个不可见的 Canvas，然后通过属性进行设置；也可以直接指定位置和尺寸进行构造。

- **参数：**

  - `x, y`：Canvas 左上角在父容器中的坐标（int）。
  - `width, height`：Canvas 初始宽度和高度（int）。

- **返回值：** 无（构造函数）。

- **行为细节：** 默认构造的 Canvas 初始化为0大小，需要手动设置尺寸；指定坐标/尺寸的构造函数则创建一个相应大小的可见容器。Canvas 默认布局模式为 `LayoutKind::Absolute`（绝对定位），可通过其 `layoutKind` 属性改变（当前仅支持 HBox/VBox）。构造完成后，可使用 `addControl()` 添加子控件。

- **示例：**

  ```c++
  Canvas panel(50, 50, 300, 200); // 在父容器(或窗口)坐标(50,50)处建立300x200的面板
  panel.setCanvasBkColor(LIGHTGRAY);
  ```

#### Canvas::addControl(std::unique_ptr<Control> control)

- **用途：** 将一个控件添加到当前 Canvas 容器中。

- **参数：** `control`，要添加的控件的 `unique_ptr`。

- **返回值：** 无。

- **行为细节：** 调用此函数会将子控件纳入 Canvas 的管理：Canvas 会将子控件指针存入内部列表，并自动调用 `control->setParent(this)` 设定父子关系。添加后，子控件的位置以 Canvas 的左上角为参考原点（即其 `getLocalX()/getLocalY()` 会以Canvas为基准）。Canvas 不会自动布局绝对定位的子控件——需要手动设置其坐标；但如果Canvas使用 HBox/VBox布局模式，则添加控件后会按布局规则重新排列子控件位置。

- **示例：**

  ```c++
  auto txt = std::make_unique<Label>(0, 0, "Hello");
  canvas.addControl(std::move(txt)); // Label 将添加到 canvas 容器内
  ```

#### Canvas::clearAllControls()

- **用途：** 移除并销毁 Canvas 容器内的所有子控件。

- **参数：** 无。

- **返回值：** 无。

- **行为细节：** 该方法会遍历并删除内部保存的每个子控件指针（智能指针会自动析构对象），然后清空列表。在需要动态刷新界面或更换整个面板内容时很有用。**注意：** 移除控件后，相应控件不再显示，且其内存会被释放。

- **示例：**

  ```c++
  canvas.clearAllControls(); // 清空所有已添加的子控件
  ```

#### Canvas::setShape(StellarX::ControlShape shape)

- **用途：** 设置 Canvas 容器的外形（形状和边框类型）。

- **参数：** `shape`，`ControlShape` 枚举值，指定容器的形状和边框样式。

- **返回值：** 无。

- **行为细节：** `ControlShape` 定义了多种形状，如有边框矩形、无边框矩形、有边框圆角矩形、无边框圆角矩形、圆形/椭圆等。调用此方法会改变 Canvas 在绘制时的外轮廓形状（例如如果设为圆角矩形，则绘制圆角边框等）。默认形状为 `ControlShape::RECTANGLE`（有边框矩形）。设置后应将 Canvas 标记 `dirty` 以便下次重绘呈现新形状。

- **示例：**

  ```c++
  canvas.setShape(StellarX::ControlShape::B_ROUND_RECTANGLE); // 设置为无边框圆角矩形
  ```

#### Canvas::setCanvasFillMode(StellarX::FillMode mode)

- **用途：** 设置 Canvas 背景的填充模式。

- **参数：** `mode`，`FillMode` 枚举值。该枚举定义了背景填充方式，如纯色填充 (`Solid`)、填充当前背景图 (`Image`)、使用网格/线条图案等。

- **返回值：** 无。

- **行为细节：** 缺省为纯色填充。如果设置为 `Image`，则 Canvas 会使用其当前设置的背景图像绘制自身背景（可通过 Canvas 继承的接口设置背景图）。修改填充模式后需要重绘以生效。

- **示例：**

  ```c++
  canvas.setCanvasFillMode(StellarX::FillMode::Solid); // 背景纯色填充
  ```

#### Canvas::setBorderColor(COLORREF color)

- **用途：** 设置 Canvas 边框颜色。

- **参数：** `color`，颜色值 (`COLORREF`)。

- **返回值：** 无。

- **行为细节：** 仅当 Canvas 当前形状含有边框时（如 `RECTANGLE` 或 `ROUND_RECTANGLE` 等），此颜色才会在绘制时用于边框。默认边框颜色通常为黑色。

- **示例：**

  ```c++
  canvas.setBorderColor(RGB(0, 128, 255)); // 边框设置为蓝色
  ```

#### Canvas::setCanvasBkColor(COLORREF color)

- **用途：** 设置 Canvas 背景色。

- **参数：** `color`，颜色值 (`COLORREF`)，将用作容器区域的背景填充颜色。

- **返回值：** 无。

- **行为细节：** 若当前填充模式为纯色，则此颜色将用于填充整个 Canvas 区域背景；如果填充模式为图像且无背景图可用，也会退化使用此颜色填充。更改背景色后，需要重绘 Canvas 才能看到效果（框架会自动标记重绘）。

- **示例：**

  ```c++
  canvas.setCanvasBkColor(DARKGRAY);
  ```

#### Canvas::setCanvasLineStyle(StellarX::LineStyle style)

- **用途：** 设置 Canvas 边框线条的样式。

- **参数：** `style`，`LineStyle` 枚举值（实线、虚线、点线等）。

- **返回值：** 无。

- **行为细节：** 当 Canvas 绘制边框时，将使用指定的线型来绘制。该属性可影响边框的视觉效果，比如 `LineStyle::Solid` 是实线（默认），`LineStyle::Dash` 是虚线等。

- **示例：**

  ```c++
  canvas.setCanvasLineStyle(StellarX::LineStyle::Dash);
  ```

#### Canvas::setLinewidth(int width)

- **用途：** 设置 Canvas 边框线条的宽度（粗细）。

- **参数：** `width`，线宽，单位像素。

- **返回值：** 无。

- **行为细节：** 边框宽度默认为1像素。设置较大的值可以绘制厚边框。若 Canvas 当前为无边框形状，则此设置暂不生效，直到形状切换为有边框类型。

- **示例：**

  ```c++
  canvas.setLinewidth(3); // 边框加粗为3像素
  ```

#### Canvas::setIsVisible(bool visible) (override)

- **用途：** 重载自 `Control::setIsVisible`，用于显示或隐藏整个容器以及其内的所有子控件。

- **参数：** `visible` 布尔值，`true` 显示容器，`false` 隐藏容器。

- **返回值：** 无。

- **行为细节：** 除了改变自身的可见状态外，`Canvas::setIsVisible` 会对容器内的每个子控件调用相同的 `setIsVisible(visible)`，确保子控件的可见性与容器保持一致。也就是说，隐藏一个 Canvas 容器将递归地隐藏其所有内容，重新显示则会显示全部内容。

- **示例：**

  ```c++
  canvas.setIsVisible(false); // 隐藏整个容器和子控件
  ```

#### Canvas::setDirty(bool dirty) (override)

- **用途：** 重载自 `Control::setDirty`，将容器及其子控件标记为需要重绘。

- **参数：** `dirty` 布尔值，`true` 标记需要重绘。

- **返回值：** 无。

- **行为细节：** 当对 Canvas 容器调用 `setDirty(true)` 时，Canvas 自身和它包含的所有子控件都会在下一帧进行重绘。这在容器整体移动或背景改变时非常有用。通常框架会自动处理大部分情况，开发者很少需要手动调用。

- **示例：**

  ```c++
  canvas.setDirty(true); // 整个容器及子元素将在下一次刷新时重绘
  ```

#### Canvas::onWindowResize() (override)

- **用途：** 重载 `Control::onWindowResize`，在父窗口大小改变时进行处理，并将通知传递给所有子控件。
- **参数：** 无。
- **返回值：** 无。
- **行为细节：** `Canvas` 覆盖该方法以在窗口尺寸变化时：首先调用自身基类逻辑处理（例如丢弃背景快照），然后遍历调用所有子控件的 `onWindowResize()`，使子控件也能感知到窗口的变化。这可确保在窗口尺寸变化后，容器内部布局能够更新（例如 HBox/VBox 布局会在下次绘制时自动重新计算排列）。
- **示例：** *该方法由框架自动调用，无需用户直接使用。*

#### Canvas::model() const (override)

- **用途：** 判断 Canvas 是否为模态对话框容器。此方法是 `Dialog` 类特有的接口，在 Canvas 中不适用，恒定返回 false。
- **参数：** 无。
- **返回值：** `false`。
- **行为细节：** 普通 Canvas 不是对话框，所以这个方法始终返回 false。可以忽略。

### 示例

```c++
// 创建一个 Canvas 容器并添加子控件示例
Canvas panel(10, 10, 400, 300);
panel.setCanvasBkColor(LIGHTGRAY);
panel.setShape(StellarX::ControlShape::ROUND_RECTANGLE);
panel.setLinewidth(2);
panel.setBorderColor(BLACK);

// 向 Canvas 添加几个按钮，假设已创建 mainWin Window
auto btn1 = std::make_unique<Button>(20, 20, 80, 30, "Btn1");
auto btn2 = std::make_unique<Button>(20, 60, 80, 30, "Btn2");
panel.addControl(std::move(btn1));
panel.addControl(std::move(btn2));

// 将 Canvas 添加到主窗口
mainWin.addControl(std::make_unique<Canvas>(panel));
```

## Label 类 (静态文本标签)

**描述：** `Label` 是用于显示静态文本的控件，不可交互。它可以在界面上显示一段文字，支持设置文字颜色、背景颜色以及背景透明与否等样式属性。Label 适合作为界面上的说明文字、标题文本或提示信息等。其特点是重量轻、无额外事件处理开销。

**特性：**

- 支持背景透明或不透明两种显示模式，可根据需要在文字后显示控件下层背景。
- 提供完整的文本样式控制：包括字体、字号、颜色、粗体/斜体等效果（通过公开的 `textStyle` 成员结构进行设置）。
- Label 会根据其文本内容自动调整显示（例如计算文本所占区域大小，用于布局）。
- Label 本身不处理用户输入事件，因此不会干扰其他控件的交互。绘制简单高效。

### 公共成员函数

#### Label() / Label(int x, int y, std::string text = "标签", COLORREF textColor = BLACK, COLORREF bkColor = RGB(255,255,255)) (构造函数)

- **用途：** 创建一个文本标签控件。可以使用默认构造然后设置属性，或直接指定位置、文本和颜色进行构造。

- **参数：**

  - `x, y`：Label 左上角位置坐标。
  - `text`：初始显示的文本内容（默认为“标签”）。
  - `textColor`：文本颜色（可选，默认黑色）。
  - `bkColor`：背景颜色（可选，默认白色）。

- **返回值：** 无。

- **行为细节：** 构造函数根据提供的信息初始化 Label。若使用默认构造，则须调用 `setText()` 等设置内容。Label 初始背景不透明，如果需要透明背景需调用 `setTextdisap(true)` 开启。位置坐标相对于父容器或窗口。

- **示例：**

  ```c++
  Label title(100, 20, "Hello, World", BLACK, WHITE);
  title.setTextdisap(true); // 背景透明
  ```

#### Label::setText(const std::string& text)

- **用途：** 设置 Label 显示的文本内容。

- **参数：** `text`，新的文本字符串。

- **返回值：** 无。

- **行为细节：** 更新 Label 的内部文本，并将控件标记为需要重绘以显示新文本。如果文本长度发生变化，Label 本身尺寸不会自动改变——Label 始终按最初设定的大小显示文本，超出范围的文本将被裁剪或换行（Label 本身不自动换行，需显式增加 `'\n'`）。

- **示例：**

  ```c++
  label.setText("欢迎使用 StellarX!");
  ```

#### Label::setTextBkColor(COLORREF color)

- **用途：** 设置 Label 文本背景矩形区域的填充颜色。

- **参数：** `color`，颜色值。

- **返回值：** 无。

- **行为细节：** 当 Label 背景不透明时，会使用该颜色填充文字背景区域。默认为白色。若 Label 背景被设置为透明（见 `setTextdisap`），则此颜色不会被绘制。更改背景色后需要刷新控件以生效（框架会自动重绘下一帧）。

- **示例：**

  ```c++
  label.setTextBkColor(LIGHTBLUE);
  ```

#### Label::setTextdisap(bool transparent)

- **用途：** 切换 Label 背景的透明/不透明模式。

- **参数：** `transparent`，布尔值。`true` 表示背景透明，`false` 表示不透明（使用背景色填充）。

- **返回值：** 无。

- **行为细节：** 背景透明模式下，Label 绘制文字时不会覆盖底下的内容，文字周围区域完全透明；非透明模式下，则用当前背景色填充 Label 区域再绘制文字。默认情况下 Label 背景为不透明。设置透明后可再次设置回不透明以恢复背景色填充。

- **示例：**

  ```c++
  label.setTextdisap(true); // 启用背景透明
  ```

#### Label::hide()

- **用途：** 隐藏该 Label 控件。

- **参数：** 无。

- **返回值：** 无。

- **行为细节：** 调用后，Label 将不再可见，相当于对该控件调用 `setIsVisible(false)`。框架将在下一次刷新时不绘制该 Label。可以通过 `setIsVisible(true)` 或将 Label 再次添加到父容器等方式使其重新可见。

- **示例：**

  ```c++
  label.hide();
  ```

### 公共成员变量

- **textStyle** (`StellarX::ControlText`)：Label 文本样式结构体，公有成员。包含字体名称、高度、宽度、颜色、斜体/下划线等属性。可以直接修改其中的字段自定义文本外观，例如 `label.textStyle.color = RED;` 改变文字颜色。修改后应将 Label 标记为需要重绘。

### 示例

```c++
Label nameLabel(20, 20, "Name:", BLACK, WHITE);
nameLabel.setTextdisap(false);                // 背景不透明
nameLabel.setTextBkColor(RGB(240,240,240));   // 浅灰背景
nameLabel.textStyle.lpszFace = "Consolas";    // 改变字体
nameLabel.textStyle.nHeight = 20;             // 字号20
nameLabel.textStyle.color = RGB(80,80,80);    // 深灰文字
// 此时Name标签将以Consolas字体深灰色显示在浅灰底上
```

## Button 类 (按钮控件)

**描述：** `Button` 是一个功能丰富的按钮控件，支持多种状态（普通、悬停、按下、禁用）和模式（普通按钮或切换开关）。它可以响应用户的鼠标点击等交互并触发回调。Button 的外观高度可定制，提供不同几何形状（矩形、圆角、圆形、椭圆）及多种视觉样式（颜色填充、图像背景等），适用于执行命令、切换某种状态等场景。

**特性：**

- **三种工作模式：** 支持**普通按钮**（每次点击触发一次动作）、**切换按钮**（两态开关，点击改变自身状态）以及**禁用**（不可交互）。
- **八种外观形状：** 提供矩形、无边框矩形、圆角矩形、无边框圆角矩形、圆形、无边框圆形、椭圆、无边框椭圆八种形状。可通过参数选择是否绘制边框。
- **多状态颜色：** 允许为默认状态、悬停状态、按下状态分别设定不同的前景/背景颜色，提供视觉反馈。
- **多种填充模式：** 背景既可使用纯色，也可使用预设的填充图案或指定图像，实现丰富的按钮视觉效果。
- **完整鼠标事件处理：** 能检测鼠标悬停、移出、按下、释放等事件并改变外观或触发回调。

### 公共成员函数

#### Button(int x, int y, int width, int height, const std::string& text = "", StellarX::ButtonMode mode = StellarX::ButtonMode::NORMAL, StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE) (构造函数 1)

- **用途：** 创建一个按钮控件，指定位置、尺寸、显示文本，可选按钮模式和形状。

- **参数：**

  - `x, y`：按钮左上角坐标。
  - `width, height`：按钮宽度和高度。
  - `text`：按钮上显示的文字（默认为空字符串，即无文字）。
  - `mode`：按钮工作模式，`ButtonMode` 枚举，默认普通按钮 (`NORMAL`)，可设为 `TOGGLE`（切换按钮）。
  - `shape`：按钮外形，`ControlShape` 枚举，默认有边框矩形。

- **返回值：** 无。

- **行为细节：** 此构造函数创建一个具有默认颜色方案的按钮。普通模式下按钮每次点击触发一次点击事件；切换模式下按钮具有“按下/释放”两种状态，每次点击切换状态。按钮形状参数决定按钮是圆形、矩形还是其他形状，并结合边框有无。

- **示例：**

  ```c++
  Button okBtn(50, 100, 80, 30, "OK", ButtonMode::NORMAL, ControlShape::RECTANGLE);
  ```

#### Button(int x, int y, int width, int height, const std::string& text, COLORREF colorDefault, COLORREF colorPressed, StellarX::ButtonMode mode = StellarX::ButtonMode::NORMAL, StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE) (构造函数 2)

- **用途：** 创建一个按钮，并自定义其默认和按下状态的底色。

- **参数：**

  - 前四项 `x, y, width, height, text` 同上。
  - `colorDefault`：按钮未被按下时的背景填充颜色。
  - `colorPressed`：按钮被按下时的背景填充颜色。
  - `mode`：按钮模式（默认普通）。
  - `shape`：按钮形状（默认有边框矩形）。

- **返回值：** 无。

- **行为细节：** 按钮在不同状态下会采用不同颜色，此构造允许直接设置普通和按下两种状态的颜色。悬停状态默认会使用介于两者之间的一个中间色（或通过内部算法计算），可通过进一步函数调用调整。文本颜色默认黑色，可另行设置。

- **示例：**

  ```c++
  Button toggleBtn(10, 10, 60, 25, "On/Off", GREEN, RED, ButtonMode::TOGGLE);
  // 未按下显示绿色，按下显示红色
  ```

#### Button(int x, int y, int width, int height, const std::string& text, COLORREF colorDefault, COLORREF colorPressed, COLORREF colorHover, StellarX::ButtonMode mode = StellarX::ButtonMode::NORMAL, StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE) (构造函数 3)

- **用途：** 创建一个按钮，手动指定普通、按下、悬停三种状态下的背景颜色。

- **参数：**

  - 前五项同上（含 `text`）。
  - `colorDefault`：默认背景色。
  - `colorPressed`：按下时背景色。
  - `colorHover`：鼠标悬停时背景色。
  - `mode`：模式（默认普通）。
  - `shape`：形状（默认矩形）。

- **返回值：** 无。

- **行为细节：** 提供了对三态颜色的完全控制。若未单独指定悬停色（使用前一个构造），按钮内部会自动生成一个悬停颜色；使用此构造则以提供的 `colorHover` 为准。其它属性同上。

- **示例：**

  ```c++
  Button customBtn(100,100,100,40,"Custom", RGB(0,200,0), RGB(0,150,0), RGB(0,220,0));
  ```

#### Button::setOnClickListener(const std::function<void()>&& callback)

- **用途：** 为按钮设置点击事件的回调函数。当按钮在普通模式下被点击（或在切换模式下从未按下状态点击）时，将调用此回调。

- **参数：** `callback`，一个无参数、无返回值的函数对象（`std::function<void()>`），可以是lambda或函数指针等。

- **返回值：** 无。

- **行为细节：** 一旦设置回调，当用户用鼠标左键点击按钮且该按钮事件被识别时，框架将在处理事件时自动执行此回调函数。请确保回调中执行的操作不会阻塞过久（以免影响UI响应）。可多次调用此方法更改回调，以最后一次设置为准。

- **示例：**

  ```c++
  okBtn.setOnClickListener([](){
      std::cout << "OK button clicked!" << std::endl;
  });
  ```

#### Button::setOnToggleOnListener(const std::function<void()>&& callback)

- **用途：** 为**切换模式**按钮设置“开启”状态的回调。当按钮由非按下状态转变为按下状态（切换打开）时调用。

- **参数：** `callback`，`std::function<void()>` 类型的回调。

- **返回值：** 无。

- **行为细节：** 仅当按钮模式为 `TOGGLE` 时有效。当用户点击按钮使其进入“按下”（ON）状态时执行。典型用途如切换某功能开启：可以在回调中实现“打开”动作逻辑。

- **示例：**

  ```c++
  toggleBtn.setOnToggleOnListener([](){
      turnFeatureOn();
  });
  ```

#### Button::setOnToggleOffListener(const std::function<void()>&& callback)

- **用途：** 为**切换模式**按钮设置“关闭”状态的回调。当按钮由按下状态变为弹起状态（切换关闭）时调用。

- **参数：** `callback`，`std::function<void()>` 类型的回调。

- **返回值：** 无。

- **行为细节：** 仅用于 `TOGGLE` 模式按钮。当用户再次点击按钮使其从ON状态切换回OFF状态时执行。例如关闭某项功能的逻辑应放在此回调中。

- **示例：**

  ```c++
  toggleBtn.setOnToggleOffListener([](){
      turnFeatureOff();
  });
  ```

#### Button::setTooltipOffset(int dx, int dy)

- **用途：** 设置按钮悬停提示框（Tooltip）相对于鼠标位置的偏移量。

- **参数：** `dx, dy`，分别为提示框在X轴和Y轴方向的偏移像素。

- **返回值：** 无。

- **行为细节：** 当按钮启用了悬停提示（Tooltip）时，默认的提示框会在鼠标指针右下偏移一段距离显示。通过此函数可自定义偏移，`dx, dy` 为正表示向右下偏移。

- **示例：**

  ```c++
  btn.setTooltipOffset(10, 15); // 提示框相对鼠标再向右10、下15像素
  ```

#### Button::setTooltipStyle(COLORREF textColor, COLORREF bkColor, bool transparent)

- **用途：** 设置按钮悬停提示框的文本颜色、背景颜色和背景透明属性。

- **参数：**

  - `textColor`：提示文本颜色。
  - `bkColor`：提示框背景填充颜色。
  - `transparent`：是否背景透明，`true` 则提示框背景透明只显示文字。

- **返回值：** 无。

- **行为细节：** 可以定制提示框的外观，比如设置文字白色、背景黑色等。如果设为背景透明，则 `bkColor` 不起作用。调用此函数会立即影响后续出现的提示框样式。默认提示框样式通常为浅黄色背景、黑字、不透明。

- **示例：**

  ```c++
  btn.setTooltipStyle(WHITE, BLUE, false); // 提示框白字蓝底
  ```

#### Button::setTooltipText(const std::string& text)

- **用途：** 设置按钮的提示文本内容。

- **参数：** `text`，字符串，新提示内容。

- **返回值：** 无。

- **行为细节：** 当鼠标悬停在按钮上一定时间，会出现一个小提示框显示此文本。默认情况下，若未设置提示文本，则会使用按钮自身的 `text` 作为提示。调用此函数可指定不同的提示内容。设为空字符串则表示不显示提示。

- **示例：**

  ```c++
  btn.setTooltipText("点击此按钮提交表单");
  ```

#### Button::setTooltipTextsForToggle(const std::string& onText, const std::string& offText)

- **用途：** 为切换模式按钮分别设置“开启状态”和“关闭状态”下悬停提示的文本。

- **参数：**

  - `onText`：按钮被按下（ON）时的提示文字。
  - `offText`：按钮弹起（OFF）时的提示文字。

- **返回值：** 无。

- **行为细节：** 切换按钮可能在两种状态下需要不同提示信息，例如ON状态提示“点击关闭XXX”，OFF状态提示“点击开启XXX”。调用此函数分别提供两种状态的提示内容。调用后会覆盖先前通过 `setTooltipText` 设置的统一提示。

- **示例：**

  ```c++
  toggleBtn.setTooltipTextsForToggle("当前已打开，点击关闭", "当前已关闭，点击开启");
  ```

### 示例

```c++
// 创建并设置一个按钮
Button sendBtn(50, 50, 100, 40, "Send", BLUE, LIGHTBLUE, ButtonMode::NORMAL);
sendBtn.setOnClickListener([](){
    sendMessage();
});
sendBtn.setTooltipText("发送消息"); 
sendBtn.setTooltipStyle(WHITE, BLACK, false);
```

## TextBox 类 (文本框控件)

**描述：** `TextBox` 是一个单行文本输入框控件，支持用户输入和编辑文本，也可配置为只读模式用于显示不可编辑的文本内容。内部集成 EasyX 的输入框功能来处理文本输入，同时提供一定程度的外观定制。典型用途包括获取用户名、搜索框等。

**特性：**

- **输入/只读模式：** 提供两种模式，输入模式允许用户点击后弹出系统输入框录入文字，只读模式则显示文本但禁止用户修改。
- **最大长度限制：** 可以设置输入文本的最大字符数，避免溢出。
- **系统输入框集成：** 使用 Windows 输入法保证输入兼容性，通过 EasyX 的 `InputBox` 来获取用户输入。
- **多种形状：** 支持矩形、圆角矩形等变体作为文本框边框形状，满足UI风格需求。

### 公共成员函数

#### TextBox(int x, int y, int width, int height, const std::string& text = "", StellarX::TextBoxMode mode = StellarX::TextBoxMode::INPUT) (构造函数)

- **用途：** 创建一个文本框控件，指定位置、尺寸、初始文字和模式（输入或只读）。

- **参数：**

  - `x, y`：文本框左上角坐标。
  - `width, height`：文本框宽度和高度。
  - `text`：初始文本内容，默认空。
  - `mode`：文本框模式，`TextBoxMode` 枚举，默认为可输入模式 (`INPUT`)，可选 `READONLY`。

- **返回值：** 无。

- **行为细节：** 构造时可提供一个初始显示文本（比如提示“请输入...”，或已有内容），以及模式。如果是 READONLY，则文本框不会响应点击输入，仅用于显示。默认背景为白色、有边框矩形。用户点击输入模式的文本框时，框架会自动调用 EasyX 的 `InputBox` 弹出输入对话框获取文本。

- **示例：**

  ```c++
  TextBox nameField(100, 80, 150, 25, "", TextBoxMode::INPUT);
  ```

#### TextBox::setText(const std::string& text)

- **用途：** 设置文本框当前显示的文本内容。

- **参数：** `text`，字符串，新文本。

- **返回值：** 无。

- **行为细节：** 直接更改文本框内容，不经过用户输入。常用于初始化文本框内容或在程序运行过程中更新显示。若文本框当前是只读模式，则仅仅改变显示文字；如果是可输入模式，用户稍后仍可编辑/覆盖该文本。

- **示例：**

  ```c++
  nameField.setText("Guest");
  ```

#### TextBox::getText() const

- **用途：** 获取文本框当前包含的文本内容。

- **参数：** 无。

- **返回值：** `std::string`，文本框内容。

- **行为细节：** 当用户通过输入框修改了文本，这里可以获取最新结果。即使在只读模式下，该方法也可以用于取得当前显示文本。

- **示例：**

  ```c++
  std::string input = nameField.getText();
  ```

#### TextBox::setMaxCharLen(size_t len)

- **用途：** 设置文本框可输入的最大字符数限制。

- **参数：** `len`，最大字符长度。

- **返回值：** 无。

- **行为细节：** 通过此方法可以限制用户输入过长的文本。当调用 EasyX 输入框时，会将该值作为参数，确保用户不能输入超过指定长度的字符串（超过部分会被截断）。默认为某个内部预设值（例如 64）。

- **示例：**

  ```c++
  nameField.setMaxCharLen(20); // 限制最多输入20字符
  ```

#### TextBox::setMode(StellarX::TextBoxMode mode)

- **用途：** 切换文本框的工作模式：输入 (INPUT) 或只读 (READONLY)。

- **参数：** `mode`，`TextBoxMode` 枚举值。

- **返回值：** 无。

- **行为细节：** 运行时调用此函数可以将文本框由可编辑切换为只读，或反之。切换为只读模式将禁止用户点击激活输入；切换为输入模式则恢复可交互。改变模式不会影响当前文本内容。

- **示例：**

  ```c++
  nameField.setMode(TextBoxMode::READONLY); // 禁止编辑
  ```

### 示例

```c++
TextBox inputField(50, 50, 200, 25, "请输入名字");
inputField.setMaxCharLen(30);
inputField.setOnClickListener([](){
    // TextBox 本身不支持直接 setOnClickListener，但可通过 handleEvent 处理
});
std::string text = inputField.getText(); // 读取用户输入结果
```

## Dialog 类 (对话框控件)

**描述：** `Dialog` 是一个对话框控件，可以包含一段提示文本和若干按钮选项，支持模态和非模态两种显示方式。Dialog 通常通过 `MessageBox` 类的静态方法来创建并显示。它封装了对话框窗口的布局、按钮响应以及返回结果等逻辑。对话框可用于显示消息、警告、错误提示等，并收集用户的确认/取消等操作。

**特性：**

- 提供六种标准对话框类型（OK、YesNo、YesNoCancel 等），方便快速生成常用的确认提示框。
- 支持**模态**（阻塞调用者直到对话框关闭）和**非模态**（对话框显示的同时程序继续运行，并通过回调获取结果）两种模式。
- 自动根据文本长度和按钮数量计算窗口大小和布局，不需手工调整。
- 内置背景快照保存与恢复机制，使对话框弹出时界面不闪烁，在关闭时恢复原界面。
- 在使用工厂方法 (`MessageBox`) 创建时，框架会自动防止重复弹出相同内容的对话框。

### 公共成员函数

#### Dialog(Window& parent, const std::string& caption, const std::string& message = "对话框", StellarX::MessageBoxType type = StellarX::MessageBoxType::OK) (构造函数)

- **用途：** 创建一个对话框控件，指定所属父窗口、标题、消息内容和对话框类型（按钮组合）。
- **参数：**
  - `parent`：所属的主窗口引用。对话框将以此窗口为依附。
  - `caption`：对话框标题文本。
  - `message`：对话框主体消息文本。默认显示“对话框”。
  - `type`：对话框类型，`MessageBoxType` 枚举值，决定包含哪些按钮选项（如 `OK`、`YesNo` 等）。默认是 OK 型只有一个确定按钮。
- **返回值：** 无。
- **行为细节：** 构造函数根据 `type` 创建相应数量和文本的按钮，并布局在对话框底部；将 `message` 文本放置在对话框中部区域显示。对话框本身是一个特殊的窗口内控件，位置通常在窗口中央。一般不直接调用构造函数生成对话框，而通过 `MessageBox::showModal` 或 `showAsync` 来创建并显示。
- **示例：** *通常由 MessageBox 调用，无需直接使用。*

#### Dialog::draw() (override)

- **用途：** 绘制对话框，包括背景、文本和按钮。
- **参数：** 无。
- **返回值：** 无。
- **行为细节：** `Dialog::draw()` 会绘制对话框自身的半透明背景遮罩（模态情况下）或边框，以及内部的消息文本和按钮控件。模态对话框绘制时会首先绘制窗口的背景快照，然后叠加对话框内容，以突出对话框。用户无需直接调用，由框架自动处理。

#### Dialog::handleEvent(const ExMessage& msg) (override)

- **用途：** 处理对话框内部的按钮点击等事件。
- **参数：** `msg`，事件消息。
- **返回值：** `bool`，指示事件是否被对话框消费。
- **行为细节：** Dialog 实现的 `handleEvent` 会将点击事件分派给其包含的按钮，并根据按钮（如“确定”或“取消”）点击来设置对话框结果。如果对话框在模态显示，则点击后会结束模态循环返回结果；如果是非模态，则会通过事先注册的回调函数传递结果。

#### Dialog::model() const (override)

- **用途：** 指示对话框是否为模态。
- **参数：** 无。
- **返回值：** `bool`，模态对话框返回 true，非模态返回 false。
- **行为细节：** 框架根据 Dialog 的创建方式设置这个标志。模态对话框在 `showModal` 中创建，非模态在 `showAsync` 创建。这个标志用于在事件循环中判断是否需要拦截底层窗口的其他控件事件（当存在模态对话框时，底层控件事件被暂停）。

#### Dialog::getResult()

- **用途：** 获取对话框的返回结果。
- **参数：** 无。
- **返回值：** `MessageBoxResult` 枚举值，表示用户在对话框上最后点击的按钮结果。如 OK、Cancel、Yes、No 等。
- **行为细节：** 仅对模态对话框有意义。在模态 `MessageBox::showModal` 返回前，会将结果保存在 Dialog 对象中并返回给调用者。在非模态模式下结果通过回调获得，这个函数不被直接使用。

### 示例

```c++
// （一般不直接使用Dialog类，而是通过MessageBox）
// 这里演示非模态对话框的手动创建和添加：
Dialog* dlg = new Dialog(mainWin, "标题", "内容", MessageBoxType::YesNo);
mainWin.addDialog(std::unique_ptr<Dialog>(dlg));
// Dialog 将显示，用户点击按钮后自动销毁。结果可通过 dlg->getResult() 查询或回调获取。
```

## MessageBox 工具类

**描述：** `MessageBox` 提供简化的对话框创建接口，以**静态方法**的形式让用户快速弹出消息框，而不必直接操纵 `Dialog` 类。它支持模态对话框（阻塞式）和非模态对话框（异步通过回调）。典型调用非常简洁，例如一行代码即可弹出一个“确认”对话框等待用户点确定。

**特性：**

- 静态方法调用，无需实例化对象即可使用，方便全局调用。
- 自动处理模态与非模态的区别逻辑；模态对话框会阻塞等待结果，非模态则立即返回并通过回调在用户选择后得到结果。
- 与 `Window` 紧密集成：MessageBox 内部会使用主窗口的对话框管理机制，确保一个窗口内不会重复弹出相同内容的对话框。
- 提供防重复弹出的机制：对于内容相同的非模态提示，第二次调用时可以选择不再弹出新的。

### 公共静态函数

#### MessageBox::showModal(Window& wnd, const std::string& text, const std::string& caption = "提示", MessageBoxType type = MessageBoxType::OK)

- **用途：** 弹出一个**模态**对话框，阻塞当前线程直到用户关闭对话框，并返回用户选择的结果。

- **参数：**

  - `wnd`：所属的 `Window` 引用，指定在哪个主窗口中弹出对话框。
  - `text`：对话框显示的主体消息文本。
  - `caption`：对话框标题（可选，默认为“提示”）。
  - `type`：对话框类型（可选，`MessageBoxType` 枚举，默认为 `OK` 类型）。

- **返回值：** `MessageBoxResult` 枚举，表示用户点击的按钮结果。如 `MessageBoxResult::OK`、`MessageBoxResult::Cancel` 等。

- **行为细节：** 该函数在内部创建一个 `Dialog` 对象，配置好内容和按钮后，以模态形式显示。调用线程将被阻塞，进入一个内部消息循环，只响应对话框相关事件，直到用户点击按钮或关闭对话框。然后函数返回相应结果。模态对话框期间，父窗口的其他控件将暂时失去响应（被遮罩）。这是一个同步调用，简单易用。

- **示例：**

  ```c++
  MessageBoxResult res = MessageBox::showModal(mainWin, "确定要退出吗？", "退出确认", MessageBoxType::YesNo);
  if(res == MessageBoxResult::Yes) {
      // 用户选择是，执行退出逻辑
  }
  ```

#### MessageBox::showAsync(Window& wnd, const std::string& text, const std::string& caption = "提示", MessageBoxType type = MessageBoxType::OK, std::function<void(MessageBoxResult)> onResult = nullptr)

- **用途：** 以**非模态**方式弹出一个对话框，函数会立即返回，同时通过回调函数异步获取用户选择结果。

- **参数：**

  - `wnd`：所属的 `Window` 引用。
  - `text`：消息文本。
  - `caption`：标题文本（默认为“提示”）。
  - `type`：对话框类型（默认为 OK）。
  - `onResult`：结果回调，可选。类型为 `std::function<void(MessageBoxResult)>`。当用户点击按钮关闭对话框时，框架将调用此回调并传入结果。如果传入 `nullptr` 则不使用回调。

- **返回值：** 无。

- **行为细节：** 函数立即返回，调用线程不会阻塞。对话框以非模态形式显示，父窗口上的其它控件仍可交互（除非对话框独占区域）。当用户做出选择（点击按钮）或关闭对话框时，如果提供了回调，则调用之并传递结果枚举。同时，对话框对象自行销毁。内部使用 `Window::addDialog()` 添加对话框到窗口管理。框架具有**去重**机制：如果当前已有一个内容完全相同的非模态对话框未关闭，再次调用 `showAsync` 时将避免弹出新的（即函数会直接返回且不再创建新对话框）。

- **示例：**

  ```c++
  MessageBox::showAsync(mainWin, "文件保存成功！", "通知", MessageBoxType::OK, 
      [](MessageBoxResult res){
          // 非模态对话框关闭时的回调逻辑
          if(res == MessageBoxResult::OK) {
              // 用户点了OK
          }
      });
  ```

## TabControl 类 (选项卡容器控件)

**描述：** `TabControl` 是一个选项卡容器控件，管理"页签按钮 + 对应页面(Canvas)"的配对组合。它提供页签栏在四个方向的布局（上/下/左/右）、选中切换功能以及页内容区域的精确定位。该控件支持窗口大小变化时的自适应、可见性联动与脏区重绘，是在同一区域内承载多张页面的理想解决方案。

**特性：**

- **四向页签布局：** 支持页签栏在顶部、底部、左侧、右侧四种排列方式。
- **页签-页面配对管理：** 每个选项卡由按钮页签和对应的Canvas页面组成，支持一键添加。
- **智能切换机制：** 与Button的TOGGLE模式深度集成，点击页签自动显示/隐藏对应页面。
- **自适应布局：** 自动计算页签和页面区域，响应窗口大小变化。
- **子控件管理：** 支持为特定页签添加子控件，自动处理坐标转换。
- **状态管理：** 提供当前激活页签的索引获取和设置功能。

### 公共成员函数

#### TabControl() (构造函数 1)

- **用途：** 创建一个默认的TabControl控件，使用默认位置和尺寸。

- **参数：** 无

- **返回值：** 无

- **行为细节：** 此构造函数创建一个位置和尺寸为默认值的TabControl。控件ID设置为"TabControl"，页签默认排列在顶部，页签栏高度使用默认值。

- **示例：**

  c++

  ```c++
  TabControl tabCtrl;
  ```

  

#### TabControl(int x, int y, int width, int height) (构造函数 2)

- **用途：** 创建一个指定位置和尺寸的TabControl控件。

- **参数：**

  - `x, y`：控件左上角坐标
  - `width, height`：控件的宽度和高度

- **返回值：** 无

- **行为细节：** 使用指定位置和尺寸创建TabControl，控件ID设置为"TabControl"，页签默认排列在顶部，页签栏高度使用默认值。

- **示例：**

  c++

  ```c++
  TabControl tabCtrl(10, 10, 400, 300);
  ```

  

#### ~TabControl() (析构函数)

- **用途：** 销毁TabControl实例并释放相关资源。
- **参数：** 无
- **返回值：** 无
- **行为细节：** 自动清理所有页签按钮和页面Canvas对象，确保无内存泄漏。

#### void draw() override

- **用途：** 绘制TabControl及其所有子控件。

- **参数：** 无

- **返回值：** 无

- **行为细节：** 重写Canvas的draw方法。首先检查脏标志和可见性，如果需要重绘，则先保存/恢复背景，然后调用基类绘制方法，最后绘制所有页签按钮和当前可见的页面。绘制完成后重置脏标志。

- **示例：**

  c++

  ```c++
  tabCtrl.draw();
  ```

  

#### bool handleEvent(const ExMessage& msg) override

- **用途：** 处理输入事件并分发给子控件。

- **参数：**

  - `msg`：输入事件消息

- **返回值：** `bool` - 如果事件被消费返回true，否则返回false

- **行为细节：** 将事件依次传递给所有页签按钮和当前可见的页面进行处理。如果有任何子控件消费了事件，则立即返回true。如果控件状态变脏，会请求重绘。

- **示例：**

  c++

  ```c++
  ExMessage msg;
  if (tabCtrl.handleEvent(msg))
  {
      // 事件已被处理
  }
  ```

  

#### void add(std::pair<std::unique_ptr<Button>, std::unique_ptr<Canvas>>&& control)

- **用途：** 添加一个页签和对应的页面到TabControl。

- **参数：**

  - `control`：包含按钮页签和Canvas页面的pair对象，使用移动语义传递

- **返回值：** 无

- **行为细节：** 将新的页签-页面对添加到控件列表中，初始化页签栏和页面布局。为新页签设置父控件、启用工具提示、设置为切换模式，并注册切换状态的监听器。新添加的页面默认不可见。

- **示例：**

  c++

  ```c++
  auto tabPair = std::make_pair(std::make_unique<Button>(), std::make_unique<Canvas>());
  tabPair.first->setButtonText("Tab 1");
  tabCtrl.add(std::move(tabPair));
  ```

  

#### void add(std::string tabText, std::unique_ptr<Control> control)

- **用途：** 为指定页签添加子控件。

- **参数：**

  - `tabText`：目标页签的文本标识
  - `control`：要添加的子控件

- **返回值：** 无

- **行为细节：** 根据页签文本查找对应的页面Canvas，将子控件添加到该页面中。子控件的可见性会与目标页面同步。

- **示例：**

  c++

  ```c++
  auto button = std::make_unique<Button>(20, 20, 80, 30, "Click me");
  tabCtrl.add("Tab 1", std::move(button));
  ```

  

#### void setTabPlacement(StellarX::TabPlacement placement)

- **用途：** 设置页签的排列位置。

- **参数：**

  - `placement`：页签排列方式，为TabPlacement枚举值

- **返回值：** 无

- **行为细节：** 设置页签在顶部、底部、左侧或右侧排列，会触发页签栏和页面的重新布局，并标记控件为脏状态需要重绘。

- **示例：**

  c++

  ```c++
  tabCtrl.setTabPlacement(StellarX::TabPlacement::Left);
  ```

  

#### void setTabBarHeight(int height)

- **用途：** 设置页签栏的高度（对于左右排列时为宽度）。

- **参数：**

  - `height`：页签栏的新高度值

- **返回值：** 无

- **行为细节：** 修改页签栏的尺寸，触发页签栏和页面的重新布局，并标记控件为脏状态需要重绘。

- **示例：**

  c++

  ```c++
  tabCtrl.setTabBarHeight(30);
  ```

  

#### void setIsVisible(bool visible) override

- **用途：** 设置控件的可见性，并同步所有子控件的可见状态。

- **参数：**

  - `visible`：是否可见

- **返回值：** 无

- **行为细节：** 重写基类方法，首先调用Canvas的setIsVisible处理背景快照逻辑，然后设置自身可见性，最后同步所有页签按钮和页面的可见状态。页面设为不可见时会清除其背景快照。

- **示例：**

  c++

  ```c++
  tabCtrl.setIsVisible(false); // 隐藏整个TabControl及其所有子控件
  ```

  

#### void onWindowResize() override

- **用途：** 响应窗口大小变化事件。

- **参数：** 无

- **返回值：** 无

- **行为细节：** 重写基类方法，调用父类的窗口大小变化处理，然后通知所有页签按钮和页面进行自适应调整。

- **示例：**

  c++

  ```c++
  tabCtrl.onWindowResize();
  ```

  

#### int getActiveIndex() const

- **用途：** 获取当前激活页签的索引。

- **参数：** 无

- **返回值：** `int` - 当前激活页签的索引，如果没有激活的页签返回-1

- **行为细节：** 遍历所有页签按钮，返回第一个处于点击状态（激活状态）的页签索引。

- **示例：**

  c++

  ```c++
  int activeIndex = tabCtrl.getActiveIndex();
  ```

  

#### void setActiveIndex(int idx)

- **用途：** 设置指定索引的页签为激活状态。

- **参数：**

  - `idx`：要激活的页签索引

- **返回值：** 无

- **行为细节：** 如果索引有效且对应页签未被禁用，则将其设置为激活状态。如果页签已经是激活状态但页面不可见，会显示页面；如果不是激活状态，则模拟点击激活它。

- **示例：**

  c++

  ```c++
  tabCtrl.setActiveIndex(2); // 激活第三个页签
  ```

  

#### int count() const

- **用途：** 获取TabControl中的页签数量。

- **参数：** 无

- **返回值：** `int` - 页签的总数

- **行为细节：** 返回控件中页签-页面对的数量。

- **示例：**

  c++

  ```c++
  int tabCount = tabCtrl.count();
  ```

  

#### int indexOf(const std::string& tabText) const

- **用途：** 通过页签文本查找对应的索引。

- **参数：**

  - `tabText`：要查找的页签文本

- **返回值：** `int` - 匹配页签的索引，如果未找到返回-1

- **行为细节：** 遍历所有页签按钮，比较按钮文本与目标文本，返回第一个匹配的页签索引。

- **示例：**

  c++

  ```c++
  int index = tabCtrl.indexOf("Settings");
  ```

  

#### 使用示例

c++

```c++
//创建选项卡控件
auto ta = std::make_unique<TabControl>(10, 10, 950, 240);
//创建页签按钮
auto but = std::make_unique<Button>(0, 0, 100, 15, "按钮1");
//创建一个pair用来包页签和页整体添加到选项卡
std::pair<std::unique_ptr<Button>,std::unique_ptr<Canvas>> p;
p.first =std::move( but);
p.second = std::make_unique<Canvas>(200, 200, 100, 100);
p.second->setCanvasBkColor(RGB(0,255,0));
p.second->addControl(std::move(table));

std::pair<std::unique_ptr<Button>, std::unique_ptr<Canvas>> p1;
p1.first = std::make_unique<Button>(0, 0, 100, 15, "按钮2");
p1.second = std::make_unique<Canvas>(200, 200, 100, 100);
p1.second->setCanvasBkColor(RGB(255,200,0));
//设置页签栏位置
ta->setTabPlacement(StellarX::TabPlacement::Bottom);
//为选项卡添加控件
ta->add(std::move(p))；
ta->add(std::move(p1));
ta->setCanvasfillMode(StellarX::FillMode::Null);
//将选项卡添加到窗口控件列表
mainWindow.addControl(std::move(ta));
```