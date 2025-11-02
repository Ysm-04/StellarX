# API Documentation

[中文 API ](API 文档.md)

Below is the API documentation for the main classes and functions of the **StellarX GUI Framework**. Detailed information such as the purpose, interface, parameters, and return values for each class are as follows:

## Control Class (Abstract Base Class)

**Description:** `Control` is the abstract base class for all controls, defining common properties and interfaces, including basic functionalities like position, size, and dirty flag. It provides drawing state save and restore mechanisms to ensure that a control's drawing operations do not affect the global drawing state. It also declares a series of pure virtual functions (such as `draw()` and `handleEvent()`) to be implemented by subclasses and prohibits copying (supports move semantics) to prevent accidental copying overhead. Generally, `Control` is not instantiated directly but serves as the parent class for other specific controls.

**Features:**

- Defines the basic properties of a control (coordinates, size, visibility, dirty flag, etc.) and provides corresponding accessor methods.
- Provides drawing state management interfaces (`saveStyle()`/`restoreStyle()`, etc.) for saving and restoring the global pen state before and after control drawing.
- Declares pure virtual interfaces, such as `draw()` (draw the control) and `handleEvent()` (handle events), which all concrete controls must implement.
- Supports move construction and move assignment to prevent controls from being accidentally copied.

### Public Member Functions

#### Control::updateBackground()

- **Purpose:** Releases the previously saved background snapshot and recaptures the current background. Used to update the control's background cache after its size changes or content changes. Calling this function ensures the correctness of the control's background, avoiding misalignment due to size changes.

- **Parameters:** None.

- **Return Value:** None.

- **Behavior Details:** In the default implementation, `updateBackground()` first discards the old background bitmap (equivalent to calling the internal `discardBackground()`), then resaves a background snapshot within the control's current position and new size range. This is usually called automatically when the control size changes or a redraw is needed, and generally does not require manual user calls.

- **Example:** Assuming a custom control needs to refresh its background upon resizing, it can be called after adjusting the size:

  c++

  ```
  myControl.setWidth(newW);
  myControl.setHeight(newH);
  myControl.updateBackground(); // Update the background snapshot to match the new size
  ```

  

#### Control::onWindowResize()

- **Purpose:** An event function called to notify the control to handle corresponding actions when the parent window's size changes. Used to adjust the control's state when the window size changes, for example, invalidating outdated background caches.

- **Parameters:** None.

- **Return Value:** None.

- **Behavior Details:** In the default implementation, `onWindowResize()` calls `discardBackground()` to discard the background bitmap saved by the control, preventing background misalignment after the window size changes. Container controls override this method (e.g., `Canvas` will call the child controls' `onWindowResize()` after handling its own). Generally, users do not need to call this method actively—the framework automatically triggers the call when the window or container size changes.

- **Example:** If a custom control needs to execute specific logic when the window size changes, it can override this function:

  c++

  ```
  void MyControl::onWindowResize()
  {
      Control::onWindowResize(); // Call base class to discard background, etc.
      // Custom logic, e.g., adjust position based on new window size
      repositionBasedOnNewWindowSize();
  }
  ```

  

#### Control::getX() / getY() / getWidth() / getHeight()

- **Purpose:** Gets the control's global coordinate position (top-left corner `x, y`) or current size (width, height).

- **Parameters:** None.

- **Return Value:** Returns the control's global X coordinate, global Y coordinate, current width, and height respectively, all of type `int`.

- **Behavior Details:** These methods provide the control's position and size in the window coordinate system, typically used for layout calculations or debugging. Note that global coordinates are relative to the origin (0,0) of the entire application window.

- **Example:**

  c++

  ```
  int x = ctrl.getX();
  int h = ctrl.getHeight();
  std::cout << "Control at X = " << x << ", height = " << h << std::endl;
  ```

  

#### Control::getRight() / getBottom()

- **Purpose:** Gets the X coordinate of the control's right edge (`getRight()`) and the Y coordinate of the control's bottom edge (`getBottom()`), calculated in global coordinates.

- **Parameters:** None.

- **Return Value:** The X coordinate of the control's right edge and the Y coordinate of the bottom edge, type `int`.

- **Behavior Details:** These two methods are equivalent to `getX() + getWidth()` and `getY() + getHeight()`, used to conveniently obtain the boundary positions covered by the control in the window coordinate system.

- **Example:**

  c++

  ```
  if (cursorX < ctrl.getRight() && cursorY < ctrl.getBottom())
  {
      // Cursor is within the control's rectangular area
  }
  ```

  

#### Control::getLocalX() / getLocalY() / getLocalWidth() / getLocalHeight()

- **Purpose:** Gets the control's position relative to the parent container's coordinate system (`LocalX, LocalY`) and its size within the parent container (`LocalWidth, LocalHeight`).

- **Parameters:** None.

- **Return Value:** The control's X coordinate, Y coordinate, width, and height relative to the parent container, all of type `int`.

- **Behavior Details:** When a control is added to a container (e.g., `Canvas`), its local coordinates use the container's top-left corner as the origin. If the control has no parent container (directly belongs to the window), the local coordinates are the same as the global coordinates. These methods are particularly useful for laying out controls inside a container.

- **Example:** Assuming a container `canvas` contains a child control `childCtrl`:

  c++

  ```
  int localX = childCtrl.getLocalX();
  int globalX = childCtrl.getX();
  std::cout << "Child global X: " << globalX
            << ", local X in canvas: " << localX << std::endl;
  ```

  

#### Control::getLocalRight() / getLocalBottom()

- **Purpose:** Gets the coordinates of the control's right and bottom edges relative to the parent container's coordinate system.

- **Parameters:** None.

- **Return Value:** The control's right-side coordinate and bottom coordinate within the parent container, type `int`.

- **Behavior Details:** Equivalent to `getLocalX() + getLocalWidth()` and `getLocalY() + getLocalHeight()` respectively. These methods provide convenience when needing to determine the control's boundary positions inside the container.

- **Example:** Can be used to check if a child control exceeds the container's bounds:

  c++

  ```
  if (childCtrl.getLocalRight() > canvas.getWidth())
  {
      // The child control's right edge within the container exceeds the container's width
  }
  ```

  

#### Control::setX(int x) / setY(int y) / setWidth(int w) / setHeight(int h)

- **Purpose:** Sets the control's global coordinate position (top-left X or Y) or adjusts the control's size (width or height).

- **Parameters:** `x` or `y` is the new global coordinate; `w` is the new width value; `h` is the new height value (all of type `int`).

- **Return Value:** None.

- **Behavior Details:** Calling these methods directly modifies the control's position or size and automatically marks the control as "needing redraw" (internally sets the `dirty` flag to `true`), causing the control to be drawn at the new position or with the new size in the next refresh cycle. Note that if the control is in a container, setting the global coordinates will change its position in the window, but its position relative to the parent container will also be updated accordingly.

- **Example:** Move control `ctrl` to window coordinates (100,100) and adjust the width to 200:

  c++

  ```
  ctrl.setX(100);
  ctrl.setY(100);
  ctrl.setWidth(200);
  // Height remains unchanged; if needed, call ctrl.setHeight(newHeight);
  ```

  

#### Control::draw() (Pure Virtual Function)

- **Purpose:** Abstract interface for drawing the control, implemented by concrete control classes with actual drawing logic.

- **Parameters:** None (drawing information is determined by the control's internal state).

- **Return Value:** None.

- **Behavior Details:** `draw()` is called by the system during framework redraws; specific controls should complete their own drawing work in the implementation. Typically, a `draw()` implementation needs to first call `Control::saveStyle()` to save the current drawing state, then set drawing colors/fonts/etc. and draw, followed by calling `Control::restoreStyle()` to restore the state. **Users should not call this method directly**; to force a redraw, mark the control as dirty (`setDirty(true)`) and wait for the system call.

- **Example:** The following is example pseudocode for a custom control implementing `draw()`:

  c++

  ```
  void MyControl::draw()
  {
      saveStyle();
      setfillcolor(backgroundColor);
      bar(x, y, x+width, y+height); // Draw background rectangle
      // ... Other drawing ...
      restoreStyle();
  }
  ```

  

#### Control::handleEvent(const ExMessage& msg) (Pure Virtual Function)

- **Purpose:** Abstract interface for event handling, implemented by concrete controls, used to handle interactive events like mouse and keyboard.

- **Parameters:** `msg` is the EasyX framework's event message (`ExMessage`), containing information such as event type, mouse coordinates, and key presses.

- **Return Value:** Boolean value, `true` indicates the event has been handled ("consumed") by the control and does not need further propagation; `false` indicates the control did not handle the event, and the event can be passed to other objects or default logic.

- **Behavior Details:** The `handleEvent()` method is also automatically called by the system in the event loop, allowing controls to respond to user input. Different controls handle different types of events as needed (e.g., buttons handle mouse clicks, text boxes handle keyboard input). If a control recognizes and handles an event (e.g., a mouse click within the button area), it should return `true`, indicating the event stops here and does not propagate further. For unhandled events, it returns `false`.

- **Example:** Simplified event handling logic for a button control:

  c++

  ```
  bool Button::handleEvent(const ExMessage& msg)
  {
      if(msg.message == WM_LBUTTONDOWN && isPointInButton(msg.x, msg.y)) {
          // Handle button click
          onClickCallback(); // Call the registered callback
          return true;
      }
      return false;
  }
  ```

  

#### Control::setIsVisible(bool show)

- **Purpose:** Sets the control's visible state, dynamically showing or hiding the control. Passing `false` hides the control; passing `true` shows it.

- **Parameters:** `show` is a boolean value specifying whether the control should be visible. `true` means visible, `false` means hidden.

- **Return Value:** None.

- **Behavior Details:** This method allows switching the control's display state at runtime. After calling, the control's `IsVisible()` state changes and the redraw logic is automatically triggered: when hidden, the control's content is no longer drawn; when shown again, the control is marked as needing redraw. For container controls (like `Canvas`), this method is overridden to **recursively affect its child controls**: hiding a container also hides all its child controls; showing the container again also shows the child controls. This simplifies the visibility control of entire interface groups.

- **Example:**

  c++

  ```
  button.setIsVisible(false); // Hide a button
  // ... After some operations ...
  button.setIsVisible(true);  // Show the button again
  ```

  

#### Control::setParent(Control* parent)

- **Purpose:** Sets the control's parent container pointer. Usually called by the container when adding the control; manual calls are less common.

- **Parameters:** `parent` is a pointer to the new parent container control (`Canvas`, etc.). Passing `nullptr` clears the parent container association.

- **Return Value:** None.

- **Behavior Details:** This method sets the internal `parent` pointer to the specified container and adjusts the control's coordinate interpretation accordingly (local coordinates are relative to the parent container when one exists). The framework automatically calls this function when adding a control to a container; user intervention is not required. **Note:** When moving a control from one container to another, besides calling `setParent()`, you must also ensure the control is removed from the old container and added to the new one (using the container's `addControl()`, etc.), otherwise the interface state will be inconsistent.

- **Example:**

  c++

  ```
  canvas1.addControl(std::move(ctrl));    // Internally automatically calls setParent(&canvas1)
  // To transfer from canvas1 to canvas2:
  canvas1.removeControl(ctrlPtr);
  canvas2.addControl(std::move(ctrlPtr)); // Internally calls setParent(&canvas2)
  ```

  

#### Control::setDirty(bool dirty)

- **Purpose:** Manually sets the control's "needs redraw" flag. Call this function when the control's content or properties change and you want to refresh the display immediately.

- **Parameters:** `dirty` boolean value, `true` marks the control as needing redraw, `false` clears the redraw flag.

- **Return Value:** None.

- **Behavior Details:** After setting `dirty` to `true`, the control's `draw()` will be called to redraw during the next screen refresh. Usually, the framework internally sets this flag when control properties change, e.g., after calling methods like `setX()/setWidth()` that modify position or size, the control is already marked dirty. Developers can also call it in custom scenarios, for example, when the control's internal state changes but there is no corresponding setter method. Generally, there is no need to manually set `dirty` to `false` because the framework automatically clears the flag after redrawing.

- **Example:**

  c++

  ```
  ctrl.setDirty(true); // Request the ctrl control to redraw
  ```

  

#### Control::IsVisible() const

- **Purpose:** Checks if the control is currently in a visible state.

- **Parameters:** None.

- **Return Value:** `bool`, whether the control is currently visible. `true` means visible, `false` means hidden.

- **Behavior Details:** This method returns the state of the control's internal `show` flag. Initially, controls are visible by default (`true`). If `setIsVisible(false)` is called or the control's parent container is hidden, this method returns `false`. When the control is not visible, its `draw()` will not be called.

- **Example:**

  c++

  ```
  if (!ctrl.IsVisible())
  {
      ctrl.setIsVisible(true); // Ensure the control is visible
  }
  ```

  

## Window Class (Application Main Window)

**Description:** The `Window` class represents the application's main window. It manages window creation, the message loop, and the global control container. `Window` is responsible for initializing the graphics window (based on EasyX) and also serves as the carrier for all top-level controls (acting as their parent container). It handles system event distribution, timed redraws, etc., allowing users to focus on adding controls to the window and responding to events. An application typically creates only one `Window` instance.

**Features:**

- Provides multiple window initialization modes (e.g., with or without double buffering, whether to show the console, etc.) to suit different application needs.
- Supports setting window background color or background image, as well as window title.
- Built-in **dialog management** system: tracks popped-up modal/non-modal dialogs to avoid repeating the same dialog.
- Integrates a complete message processing loop, automatically dispatching events to each control's `handleEvent()` method and scheduling redraws of each control's `draw()`.
- Manages the lifecycle of all controls and dialogs added to the window, automatically cleaning up when the window is destroyed.

### Public Member Functions

#### Window(int width, int height, int mode, COLORREF bkColor = BLACK, std::string headline = "Window") (Constructor)

- **Purpose:** Creates an application window of the specified size and performs necessary graphics initialization. You can specify the window mode, background color, and window title.

- **Parameters:**

  - `width`: Window width (pixels).
  - `height`: Window height (pixels).
  - `mode`: Window mode flags (int type). Different values configure window properties, such as whether to use **double buffering** for drawing, whether to show the console window, etc. Typically, 0 represents the default (window mode, hide console), or you can use constants provided by EasyX to combine this parameter, e.g., `EW_SHOWCONSOLE` to show the console, `EW_NOCLOSE` to disable the close button, etc.
  - `bkColor` *(optional)*: Window background color, defaults to black (`BLACK`).
  - `headline` *(optional)*: Window title text, defaults to "Window".

- **Return Value:** None (constructor).

- **Behavior Details:** When constructing a `Window` object, it calls EasyX's graphics initialization function `initgraph(width, height, mode)` to create the form, automatically applies the provided background color, and sets the window title. If the mode enables double buffering, the framework will use a manual refresh mechanism. After creating the window, **you must call** `runEventLoop()` to enter the message loop before the window starts responding to events and drawing.

- **Example:**

  c++

  ```
  // Create an 800x600 window, double-buffered and showing the console, background white, titled "Example"
  int mode = EW_SHOWCONSOLE | EW_NOBORDER; // Example mode combination only
  Window mainWin(800, 600, mode, WHITE, "Example");
  mainWin.draw();
  mainWin.runEventLoop();
  ```

  

#### Window::~Window() (Destructor)

- **Purpose:** Destroys the window object and cleans up resources.

- **Parameters:** None.

- **Return Value:** None.

- **Behavior Details:** When the `Window` object is destroyed, the framework automatically closes the graphics window, releases EasyX graphics resources, and cleans up the controls and dialog objects hosted in the window. Usually does not require explicit calls; simply let the `Window` object go out of scope or be reclaimed by the system when the program ends.

- **Example:**

  c++

  ```
  {
      Window win(400, 300, 0);
      // ... Use win ...
  } // Scope ends, automatically destructs, window closes
  ```

  

#### Window::runEventLoop()

- **Purpose:** Enters the window message processing loop, starting the GUI main event loop. After calling this function, the window begins responding to user interactions and continues running until the window is closed.

- **Parameters:** None.

- **Return Value:** `int`, the exit code of the window message loop (can typically be returned directly to `main()`).

- **Behavior Details:** This function internally implements a typical event loop: continuously calls EasyX's `peekmessage()` to get event messages and dispatches them to the `handleEvent()` methods of the controls within the window. Simultaneously, it handles redraws as needed: each frame traverses all controls and calls their `draw()` methods. In the loop, the window also checks for the existence of modal dialogs and prevents interaction with underlying controls, etc. **Must** be called after creating the window and adding controls; otherwise, the window will be blank and unresponsive.

- **Example:**

  c++

  ```
  Window win(640, 480, EW_SHOWCONSOLE, LIGHTGRAY, "Demo");
  // ... Add controls and other initialization ...
  wni.draw();
  return win.runEventLoop(); // Enter event loop, blocks until window closes
  ```

  

#### Window::addControl(std::unique_ptr<Control> control)

- **Purpose:** Adds a new control to the window for management and display. Can add any control inheriting from `Control`, such as buttons, text boxes, containers, etc.

- **Parameters:** `control` is a `unique_ptr` smart pointer pointing to the control to be added. The function internally takes ownership of this pointer.

- **Return Value:** None.

- **Behavior Details:** When adding a control, the window includes the control in its internally maintained control list and calls the control's `setParent()`. Thereafter, the framework considers this control during each redraw and event handling. The control's initial position and size in the window coordinate system depend on the control's own properties. If you need to remove a control, currently it can be done via `clearAllControls()` (in `Canvas`) or by destroying the window.

- **Example:**

  c++

  ```
  auto btn = std::make_unique<Button>(50, 50, 80, 30, "OK");
  mainWin.addControl(std::move(btn)); // Add the button to the main window for display
  ```

  

#### Window::addDialog(std::unique_ptr<Control> dialog)

- **Purpose:** Pops up a dialog control and adds it to the window. Used when adding dialogs.

- **Parameters:** `dialog` is a `unique_ptr` to a `Dialog` or other dialog control object.

- **Return Value:** None.

- **Behavior Details:** This function is similar to `addControl()` but specifically for dialog controls. The window adds the dialog to its internal dialog list and gives it special handling in the event loop (e.g., when a modal dialog exists, it prevents interaction with other controls). The framework also provides a duplicate prevention mechanism: if the dialog to be added already exists (matched by the dialog's title and message), it can avoid duplicate addition. Usually, this function is not used directly to add modal dialogs—instead, use the simplified call via `MessageBox::showModal()`.

- **Example:**

  c++

  ```
  auto dlg = std::make_unique<Dialog>(mainWin, "Warning", "An error occurred", MessageBoxType::OKCancel);
  mainWin.addDialog(std::move(dlg)); // Non-modal dialog, displays asynchronously
  ```

  

#### Window::hasNonModalDialogWithCaption(const std::string& caption, const std::string& message) const

- **Purpose:** Checks if a non-modal dialog with the specified title and content already exists in the current window.

- **Parameters:** `caption` is the dialog title text; `message` is the message content displayed by the dialog.

- **Return Value:** `bool`, returns `true` if a non-modal dialog with the same title and content exists, otherwise `false`.

- **Behavior Details:** This method is used to prevent popping up duplicate prompt boxes. When calling `MessageBox::showAsync()` to display a non-modal dialog, it internally uses this function to check if a dialog with the same content is already open and unclosed, thus avoiding multiple duplicate dialogs on the interface. Generally, users do not need to call it directly, but knowing its existence helps understand the framework's duplicate prevention mechanism.

- **Example:** (Internal framework usage)

  c++

  ```
  if (!hasNonModalDialogWithCaption("Warning", "An error occurred"))
  {
      // Safely pop up a new dialog
      addDialog(std::make_unique<Dialog>(...));
  }
  ```

  

#### Window::draw()

- **Purpose:** Manually triggers a redraw of the window content.

- **Parameters:** None.

- **Return Value:** None.

- **Behavior Details:** By default, the framework automatically calls the internal drawing logic every frame, so manual calls to this function are generally unnecessary. But in some special cases (e.g., wanting to force an immediate redraw of certain content), you can call `mainWin.draw()`. This function traverses all controls added to the window and calls their `draw()` method for drawing. If double buffering mode is used, draw() will complete the drawing in the background and update the screen.

- **Example:**

  c++

  ```
  // Immediately refresh the window display after changing the background
  mainWin.setBkcolor(WHITE);
  mainWin.draw();
  ```

  

#### Window::draw(const std::string& pImgFile)

- **Purpose:** Draws the specified image file as the window background and refreshes the display.

- **Parameters:** `pImgFile`, the image file path (string).

- **Return Value:** None.

- **Behavior Details:** This function provides a convenient way to set the window background image. When called, it loads the image from the specified path and tiles it as the window background, then calls the `draw()` of the window's other controls for normal drawing. Equivalent to setting the background image and immediately refreshing. If the specified file cannot be loaded or is not a valid image, the original background remains unchanged.

- **Example:**

  c++

  ```
  mainWin.draw("background.jpg"); // Use background.jpg as the background and redraw the window
  ```

  

#### Window::setBkImage(const std::string& pImgFile)

- **Purpose:** Sets the window background image but does not draw it immediately.

- **Parameters:** `pImgFile`, the image file path.

- **Return Value:** None.

- **Behavior Details:** This method changes the window background image setting but does not automatically trigger a redraw (waits for the next event loop refresh or a call to `draw()`). If immediate effect is needed, use it with `draw()`. Setting an empty string or invalid path will remove the background image, leaving only the background color.

- **Example:**

  c++

  ```
  mainWin.setBkImage("bg.png");
  // The new background will be drawn automatically next frame, or manually call mainWin.draw();
  ```

  

#### Window::setBkcolor(COLORREF c)

- **Purpose:** Sets the window background color.

- **Parameters:** `c`, the color value (COLORREF, e.g., `WHITE`, `RGB(255,255,255)`, etc.).

- **Return Value:** None.

- **Behavior Details:** Modifies the window background color; the new background color takes effect on the next refresh. If a background image was set previously, the new color will be covered by the background image (unless the background image is removed). Usually, the background color can be specified directly via the constructor parameter when creating the window; this function is for dynamically changing the background color during runtime.

- **Example:**

  c++

  ```
  mainWin.setBkcolor(LIGHTGRAY);
  ```

  

#### Window::setHeadline(const std::string& headline)

- **Purpose:** Sets the window title bar text.

- **Parameters:** `headline`, a string, the new title text.

- **Return Value:** None.

- **Behavior Details:** Calling this function changes the window's title bar content (implemented via Win32 API call `SetWindowText`). The change is immediate and does not require a refresh cycle.

- **Example:**

  c++

  ```
  mainWin.setHeadline("Processing Complete");
  ```

  

### Getter Methods (Getters)

#### Window::getWidth() / getHeight()

- **Purpose:** Gets the current width or height of the window.

- **Parameters:** None.

- **Return Value:** Window width or height (int).

- **Behavior Details:** This size is the size of the window's drawing area (consistent with the size specified at creation, unless the window was stretched during runtime). If the window supports stretching and was resized by the user, these two values update dynamically.

- **Example:**

  c++

  ```
  int w = mainWin.getWidth();
  int h = mainWin.getHeight();
  ```

  

#### Window::getHeadline()

- **Purpose:** Gets the current title text of the window.

- **Parameters:** None.

- **Return Value:** Title string (`std::string`).

- **Behavior Details:** If not set, returns the default title (specified at creation or the default "Window").

- **Example:**

  c++

  ```
  std::string title = mainWin.getHeadline();
  ```

  

#### Window::getBkcolor()

- **Purpose:** Gets the current window background color value.

- **Parameters:** None.

- **Return Value:** Background color (`COLORREF`).

- **Behavior Details:** If the window is using a background image, this color usually still returns the most recently set color value, but it is currently covered by the background image during drawing.

- **Example:**

  c++

  ```
  COLORREF c = mainWin.getBkcolor();
  ```

  

#### Window::getBkImage()

- **Purpose:** Gets the background image object currently used by the window.

- **Parameters:** None.

- **Return Value:** Pointer to the background `IMAGE` object, may return `nullptr` if there is no background image.

- **Behavior Details:** The returned `IMAGE*` is managed internally by the window; **do not modify or release it**. Only for read-only scenarios (e.g., layout based on background image size, etc.).

- **Example:**

  c++

  ```
  IMAGE* bg = mainWin.getBkImage();
  if(bg)
  {
      // Example: Get background image width and height
      int bgW = bg->getwidth();
      int bgH = bg->getheight();
  }
  ```

  

#### Window::getHwnd()

- **Purpose:** Gets the operating system handle (`HWND`) of the window.

- **Parameters:** None.

- **Return Value:** Windows window handle (`HWND`).

- **Behavior Details:** This handle can be used to call underlying Win32 APIs to achieve window-level functionalities not provided by the framework. Generally not recommended to manipulate the window handle directly unless for advanced needs (like modifying window styles, etc.).

- **Example:**

  c++

  ```
  HWND hWnd = mainWin.getHwnd();
  // Call Win32 API, e.g., change window icon:
  SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
  ```

  

#### Window::getControls()

- **Purpose:** Gets a reference to the list of all top-level control containers within the window.

- **Parameters:** None.

- **Return Value:** `std::vector<std::unique_ptr<Control>>&`, a reference to the control list managed by the window.

- **Behavior Details:** Returns the control container maintained internally by the window. You can traverse the element pointers to inspect or debug which controls have been added. Do not attempt to modify this list (e.g., insert/delete) to avoid corrupting the framework's internal state.

- **Example:**

  c++

  ```
  for(const auto& ctrl : mainWin.getControls())
  {
      std::cout << typeid(*ctrl).name() << std::endl;
  }
  ```

  

## Canvas Class (Container Control)

**Description:** `Canvas` is a container class control used to group and manage child controls. It is itself a visible control that can draw a background and border. A typical use is as a **layout panel**: add multiple child controls to a Canvas, and let the Canvas handle event distribution and drawing order uniformly. Canvas supports multiple layout modes (absolute positioning or simple horizontal/vertical layouts), simplifying interface layout work.

**Features:**

- Supports four rectangular shapes (rectangle, rounded rectangle, and their borderless versions) as container appearances, with options to set the presence of borders and corner roundness, etc.
- Provides custom fill modes (solid color or background image) and border styles (color, line style, line width).
- Automatically manages child controls: takes over the lifecycle of child controls, automatically destroying them when the Canvas is destructed; also passes events to appropriate child controls when events occur.
- Supports nested container structures: Canvas itself can be a child control of another Canvas, enabling complex layouts.
- Simple layout capabilities: Currently supports **absolute layout** (by directly setting child control coordinates). **Horizontal Box HBox/Vertical Box VBox layout**), Grid, Flow, Stack and other layout types are currently reserved.

### Public Member Functions

#### Canvas() / Canvas(int x, int y, int width, int height) (Constructors)

- **Purpose:** Creates a container control. You can use the default constructor to create an invisible Canvas and then set properties, or directly specify position and size for construction.

- **Parameters:**

  - `x, y`: The coordinates of the Canvas's top-left corner in the parent container (int).
  - `width, height`: The initial width and height of the Canvas (int).

- **Return Value:** None (constructor).

- **Behavior Details:** A Canvas constructed by default is initialized to size 0 and requires manual size setting; the constructor with specified coordinates/size creates a visible container of the corresponding size. The default layout mode for Canvas is `LayoutKind::Absolute` (absolute positioning), which can be changed via its `layoutKind` property (currently only HBox/VBox are supported). After construction is complete, use `addControl()` to add child controls.

- **Example:**

  c++

  ```
  Canvas panel(50, 50, 300, 200); // Create a 300x200 panel at coordinates (50,50) in the parent container (or window)
  panel.setCanvasBkColor(LIGHTGRAY);
  ```

  

#### Canvas::addControl(std::unique_ptr<Control> control)

- **Purpose:** Adds a control to the current Canvas container.

- **Parameters:** `control`, a `unique_ptr` to the control to be added.

- **Return Value:** None.

- **Behavior Details:** Calling this function incorporates the child control into the Canvas's management: the Canvas stores the child control pointer in its internal list and automatically calls `control->setParent(this)` to establish the parent-child relationship. After addition, the child control's position uses the Canvas's top-left corner as the reference origin (i.e., its `getLocalX()/getLocalY()` will be relative to the Canvas). The Canvas does not automatically layout absolutely positioned child controls—their coordinates need to be set manually; but if the Canvas uses HBox/VBox layout mode, adding a control will rearrange the child control positions according to the layout rules.

- **Example:**

  c++

  ```
  auto txt = std::make_unique<Label>(0, 0, "Hello");
  canvas.addControl(std::move(txt)); // Label will be added inside the canvas container
  ```

  

#### Canvas::clearAllControls()

- **Purpose:** Removes and destroys all child controls within the Canvas container.

- **Parameters:** None.

- **Return Value:** None.

- **Behavior Details:** This method traverses and deletes each child control pointer saved internally (smart pointers automatically destruct the objects), then clears the list. Useful when needing to dynamically refresh the interface or replace the entire panel content. **Note:** After removing controls, the corresponding controls are no longer displayed, and their memory is freed.

- **Example:**

  c++

  ```
  canvas.clearAllControls(); // Clear all added child controls
  ```

  

#### Canvas::setShape(StellarX::ControlShape shape)

- **Purpose:** Sets the Canvas container's shape (form and border type).

- **Parameters:** `shape`, a `ControlShape` enumeration value specifying the container's shape and border style.

- **Return Value:** None.

- **Behavior Details:** `ControlShape` defines various shapes, such as bordered rectangle, borderless rectangle, bordered rounded rectangle, borderless rounded rectangle, circle/ellipse, etc. Calling this method changes the Canvas's outer contour shape when drawn (e.g., if set to rounded rectangle, it draws rounded borders, etc.). The default shape is `ControlShape::RECTANGLE` (bordered rectangle). After setting, the Canvas should be marked `dirty` so the new shape appears on the next redraw.

- **Example:**

  c++

  ```
  canvas.setShape(StellarX::ControlShape::B_ROUND_RECTANGLE); // Set to borderless rounded rectangle
  ```

  

#### Canvas::setCanvasFillMode(StellarX::FillMode mode)

- **Purpose:** Sets the Canvas background's fill mode.

- **Parameters:** `mode`, a `FillMode` enumeration value. This enumeration defines background fill methods, such as solid color fill (`Solid`), fill with current background image (`Image`), use grid/line patterns, etc.

- **Return Value:** None.

- **Behavior Details:** Default is solid color fill. If set to `Image`, the Canvas will use its currently set background image to draw its own background (can set the background image via the interface inherited by Canvas). Changing the fill mode requires a redraw to take effect.

- **Example:**

  c++

  ```
  canvas.setCanvasFillMode(StellarX::FillMode::Solid); // Background solid color fill
  ```

  

#### Canvas::setBorderColor(COLORREF color)

- **Purpose:** Sets the Canvas border color.

- **Parameters:** `color`, the color value (`COLORREF`).

- **Return Value:** None.

- **Behavior Details:** This color is only used for the border during drawing if the Canvas's current shape has a border (like `RECTANGLE` or `ROUND_RECTANGLE`, etc.). The default border color is usually black.

- **Example:**

  c++

  ```
  canvas.setBorderColor(RGB(0, 128, 255)); // Set border to blue
  ```

  

#### Canvas::setCanvasBkColor(COLORREF color)

- **Purpose:** Sets the Canvas background color.

- **Parameters:** `color`, the color value (`COLORREF`), will be used as the background fill color for the container area.

- **Return Value:** None.

- **Behavior Details:** If the current fill mode is solid color, this color will be used to fill the entire Canvas background area; if the fill mode is image and no background image is available, it will also degrade to using this color for filling. After changing the background color, the Canvas needs to be redrawn to see the effect (the framework automatically marks it for redraw).

- **Example:**

  c++

  ```
  canvas.setCanvasBkColor(DARKGRAY);
  ```

  

#### Canvas::setCanvasLineStyle(StellarX::LineStyle style)

- **Purpose:** Sets the Canvas border line's style.

- **Parameters:** `style`, a `LineStyle` enumeration value (solid line, dashed line, dotted line, etc.).

- **Return Value:** None.

- **Behavior Details:** When the Canvas draws its border, it will use the specified line style. This property can affect the border's visual effect, e.g., `LineStyle::Solid` is a solid line (default), `LineStyle::Dash` is a dashed line, etc.

- **Example:**

  c++

  ```
  canvas.setCanvasLineStyle(StellarX::LineStyle::Dash);
  ```

  

#### Canvas::setLinewidth(int width)

- **Purpose:** Sets the Canvas border line's width (thickness).

- **Parameters:** `width`, the line width, in pixels.

- **Return Value:** None.

- **Behavior Details:** The default border width is 1 pixel. Setting a larger value can draw a thick border. If the Canvas's current shape is borderless, this setting has no effect until the shape is switched to a bordered type.

- **Example:**

  c++

  ```
  canvas.setLinewidth(3); // Thicken border to 3 pixels
  ```

  

#### Canvas::setIsVisible(bool visible) (override)

- **Purpose:** Overridden from `Control::setIsVisible`, used to show or hide the entire container and all its child controls.

- **Parameters:** `visible` boolean value, `true` shows the container, `false` hides it.

- **Return Value:** None.

- **Behavior Details:** Besides changing its own visible state, `Canvas::setIsVisible` calls the same `setIsVisible(visible)` for each child control within the container, ensuring the child controls' visibility matches the container's. That is, hiding a Canvas container recursively hides all its contents, and showing it again shows all contents.

- **Example:**

  c++

  ```
  canvas.setIsVisible(false); // Hide the entire container and child controls
  ```

  

#### Canvas::setDirty(bool dirty) (override)

- **Purpose:** Overridden from `Control::setDirty`, marks the container and its child controls as needing redraw.

- **Parameters:** `dirty` boolean value, `true` marks as needing redraw.

- **Return Value:** None.

- **Behavior Details:** When `setDirty(true)` is called on a Canvas container, both the Canvas itself and all the child controls it contains will be redrawn in the next frame. This is useful when the container moves entirely or the background changes. Usually, the framework handles most cases automatically, and developers rarely need to call this manually.

- **Example:**

  c++

  ```
  canvas.setDirty(true); // The entire container and child elements will redraw on the next refresh
  ```

  

#### Canvas::onWindowResize() (override)

- **Purpose:** Overrides `Control::onWindowResize`, handles actions when the parent window size changes, and passes the notification to all child controls.
- **Parameters:** None.
- **Return Value:** None.
- **Behavior Details:** `Canvas` overrides this method so that when the window size changes: it first calls its own base class logic (e.g., discarding background snapshots), then traverses and calls all child controls' `onWindowResize()`, allowing child controls to also perceive the window change. This ensures that after the window size changes, the container's internal layout can update (e.g., HBox/VBox layouts will automatically recalculate and rearrange on the next draw).
- **Example:** *This method is called automatically by the framework; users do not need to use it directly.*

#### Canvas::model() const (override)

- **Purpose:** Determines if the Canvas is a modal dialog container. This method is specific to the `Dialog` class and is not applicable in Canvas; it consistently returns false.
- **Parameters:** None.
- **Return Value:** `false`.
- **Behavior Details:** A regular Canvas is not a dialog, so this method always returns false. Can be ignored.

### Example

c++

```
// Example of creating a Canvas container and adding child controls
Canvas panel(10, 10, 400, 300);
panel.setCanvasBkColor(LIGHTGRAY);
panel.setShape(StellarX::ControlShape::ROUND_RECTANGLE);
panel.setLinewidth(2);
panel.setBorderColor(BLACK);

// Add a few buttons to the Canvas, assuming mainWin Window is already created
auto btn1 = std::make_unique<Button>(20, 20, 80, 30, "Btn1");
auto btn2 = std::make_unique<Button>(20, 60, 80, 30, "Btn2");
panel.addControl(std::move(btn1));
panel.addControl(std::move(btn2));

// Add the Canvas to the main window
mainWin.addControl(std::make_unique<Canvas>(panel));
```



## Label Class (Static Text Label)

**Description:** `Label` is a control for displaying static text and is not interactive. It can display a piece of text on the interface, supporting style attributes like text color, background color, and whether the background is transparent or not. Label is suitable for explanatory text, title text, or prompt messages on the interface. Its characteristics are lightweight and no additional event handling overhead.

**Features:**

- Supports two display modes: transparent or opaque background, allowing the underlying background to show through behind the text if needed.
- Provides complete text style control: including font, size, color, bold/italic effects, etc. (set via the public `textStyle` member structure).
- Label automatically adjusts display based on its text content (e.g., calculating the area occupied by the text for layout).
- Label itself does not handle user input events, so it does not interfere with other controls' interactions. Drawing is simple and efficient.

### Public Member Functions

#### Label() / Label(int x, int y, std::string text = "Label", COLORREF textColor = BLACK, COLORREF bkColor = RGB(255,255,255)) (Constructors)

- **Purpose:** Creates a text label control. Can use default construction and then set properties, or directly specify position, text, and colors for construction.

- **Parameters:**

  - `x, y`: The Label's top-left corner position coordinates.
  - `text`: The initially displayed text content (defaults to "Label").
  - `textColor`: Text color (optional, default black).
  - `bkColor`: Background color (optional, default white).

- **Return Value:** None.

- **Behavior Details:** The constructor initializes the Label based on the provided information. If using default construction, you must call `setText()` etc., to set the content. The Label's initial background is opaque; if a transparent background is needed, call `setTextdisap(true)` to enable it. Position coordinates are relative to the parent container or window.

- **Example:**

  c++

  ```
  Label title(100, 20, "Hello, World", BLACK, WHITE);
  title.setTextdisap(true); // Background transparent
  ```

  

#### Label::setText(const std::string& text)

- **Purpose:** Sets the text content displayed by the Label.

- **Parameters:** `text`, the new text string.

- **Return Value:** None.

- **Behavior Details:** Updates the Label's internal text and marks the control as needing redraw to display the new text. If the text length changes, the Label's own size does not automatically change—the Label always displays text according to its initially set size; text beyond the range will be clipped or wrap (Label itself does not auto-wrap; you need to explicitly add `'\n'`).

- **Example:**

  c++

  ```
  label.setText("Welcome to StellarX!");
  ```

  

#### Label::setTextBkColor(COLORREF color)

- **Purpose:** Sets the fill color for the Label's text background rectangular area.

- **Parameters:** `color`, the color value.

- **Return Value:** None.

- **Behavior Details:** When the Label background is opaque, this color is used to fill the text background area. Default is white. If the Label background is set to transparent (see `setTextdisap`), this color is not drawn. Changing the background color requires refreshing the control to take effect (the framework automatically redraws the next frame).

- **Example:**

  c++

  ```
  label.setTextBkColor(LIGHTBLUE);
  ```

  

#### Label::setTextdisap(bool transparent)

- **Purpose:** Toggles the Label background's transparent/opaque mode.

- **Parameters:** `transparent`, boolean value. `true` means background transparent, `false` means opaque (uses background color fill).

- **Return Value:** None.

- **Behavior Details:** In transparent background mode, when drawing text, the Label does not cover the content underneath; the area around the text is completely transparent. In non-transparent mode, it fills the Label area with the current background color before drawing the text. By default, the Label background is opaque. Setting transparent allows you to set it back to opaque later to restore background color filling.

- **Example:**

  c++

  ```
  label.setTextdisap(true); // Enable background transparency
  ```

  

#### Label::hide()

- **Purpose:** Hides this Label control.

- **Parameters:** None.

- **Return Value:** None.

- **Behavior Details:** After calling, the Label will no longer be visible, equivalent to calling `setIsVisible(false)` on this control. The framework will not draw this Label on the next refresh. It can be made visible again by calling `setIsVisible(true)` or re-adding the Label to the parent container, etc.

- **Example:**

  c++

  ```
  label.hide();
  ```

  

### Public Member Variable

- **textStyle** (`StellarX::ControlText`): Label text style structure, public member. Contains attributes like font name, height, width, color, italic/underline, etc. You can directly modify its fields to customize the text appearance, e.g., `label.textStyle.color = RED;` changes the text color. After modification, the Label should be marked as needing redraw.

### Example

c++

```
Label nameLabel(20, 20, "Name:", BLACK, WHITE);
nameLabel.setTextdisap(false);                // Background opaque
nameLabel.setTextBkColor(RGB(240,240,240));   // Light gray background
nameLabel.textStyle.lpszFace = "Consolas";    // Change font
nameLabel.textStyle.nHeight = 20;             // Font size 20
nameLabel.textStyle.color = RGB(80,80,80);    // Dark gray text
// The Name label will now display in Consolas font, dark gray text on a light gray background
```



## Button Class (Button Control)

**Description:** `Button` is a feature-rich button control supporting multiple states (normal, hover, pressed, disabled) and modes (normal button or toggle switch). It can respond to user interactions like mouse clicks and trigger callbacks. The Button's appearance is highly customizable, offering different geometric shapes (rectangle, rounded, circle, ellipse) and various visual styles (color fill, image background, etc.), suitable for executing commands, toggling states, etc.

**Features:**

- **Three Operating Modes:** Supports **normal button** (each click triggers an action once), **toggle button** (two-state switch, click changes its own state), and **disabled** (non-interactive).
- **Eight Appearance Shapes:** Provides rectangle, borderless rectangle, rounded rectangle, borderless rounded rectangle, circle, borderless circle, ellipse, borderless ellipse eight shapes. Can choose whether to draw a border via parameters.
- **Multi-State Colors:** Allows setting different foreground/background colors for default, hover, and pressed states, providing visual feedback.
- **Multiple Fill Modes:** Background can use solid colors, preset fill patterns, or specified images, achieving rich button visual effects.
- **Complete Mouse Event Handling:** Can detect mouse hover, leave, press, release, etc., and change appearance or trigger callbacks.

### Public Member Functions

#### Button(int x, int y, int width, int height, const std::string& text = "", StellarX::ButtonMode mode = StellarX::ButtonMode::NORMAL, StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE) (Constructor 1)

- **Purpose:** Creates a button control, specifying position, size, display text, optional button mode, and shape.

- **Parameters:**

  - `x, y`: Button top-left corner coordinates.
  - `width, height`: Button width and height.
  - `text`: Text displayed on the button (defaults to empty string, i.e., no text).
  - `mode`: Button operating mode, `ButtonMode` enumeration, default normal button (`NORMAL`), can be set to `TOGGLE` (toggle button).
  - `shape`: Button shape, `ControlShape` enumeration, default bordered rectangle.

- **Return Value:** None.

- **Behavior Details:** This constructor creates a button with a default color scheme. In normal mode, the button triggers a click event once per click; in toggle mode, the button has "pressed/released" two states, toggling state with each click. The button shape parameter determines whether the button is circular, rectangular, or another shape, combined with border presence.

- **Example:**

  c++

  ```
  Button okBtn(50, 100, 80, 30, "OK", ButtonMode::NORMAL, ControlShape::RECTANGLE);
  ```

  

#### Button(int x, int y, int width, int height, const std::string& text, COLORREF colorDefault, COLORREF colorPressed, StellarX::ButtonMode mode = StellarX::ButtonMode::NORMAL, StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE) (Constructor 2)

- **Purpose:** Creates a button and customizes its default and pressed state background colors.

- **Parameters:**

  - First four `x, y, width, height, text` same as above.
  - `colorDefault`: Background fill color when the button is not pressed.
  - `colorPressed`: Background fill color when the button is pressed.
  - `mode`: Button mode (default normal).
  - `shape`: Button shape (default bordered rectangle).

- **Return Value:** None.

- **Behavior Details:** The button uses different colors in different states; this constructor allows directly setting the colors for normal and pressed states. The hover state defaults to an intermediate color between the two (or calculated via an internal algorithm) and can be adjusted via further function calls. Text color defaults to black and can be set separately.

- **Example:**

  c++

  ```
  Button toggleBtn(10, 10, 60, 25, "On/Off", GREEN, RED, ButtonMode::TOGGLE);
  // Unpressed shows green, pressed shows red
  ```

  

#### Button(int x, int y, int width, int height, const std::string& text, COLORREF colorDefault, COLORREF colorPressed, COLORREF colorHover, StellarX::ButtonMode mode = StellarX::ButtonMode::NORMAL, StellarX::ControlShape shape = StellarX::ControlShape::RECTANGLE) (Constructor 3)

- **Purpose:** Creates a button, manually specifying the background colors for normal, pressed, and hover states.

- **Parameters:**

  - First five same as above (including `text`).
  - `colorDefault`: Default background color.
  - `colorPressed`: Pressed background color.
  - `colorHover`: Mouse hover background color.
  - `mode`: Mode (default normal).
  - `shape`: Shape (default rectangle).

- **Return Value:** None.

- **Behavior Details:** Provides full control over the three-state colors. If the hover color is not specified separately (using the previous constructor), the button internally automatically generates a hover color; using this constructor uses the provided `colorHover`. Other properties are the same as above.

- **Example:**

  c++

  ```
  Button customBtn(100,100,100,40,"Custom", RGB(0,200,0), RGB(0,150,0), RGB(0,220,0));
  ```

  

#### Button::setOnClickListener(const std::function<void()>&& callback)

- **Purpose:** Sets the click event callback function for the button. Called when the button is clicked in normal mode (or clicked from the unpressed state in toggle mode).

- **Parameters:** `callback`, a function object (`std::function<void()>`) with no parameters and no return value, can be a lambda or function pointer, etc.

- **Return Value:** None.

- **Behavior Details:** Once the callback is set, when the user left-clicks the button and the button event is recognized, the framework will automatically execute this callback function during event processing. Ensure the operations performed in the callback do not block for too long (to avoid affecting UI responsiveness). This method can be called multiple times to change the callback, with the last setting taking effect.

- **Example:**

  c++

  ```
  okBtn.setOnClickListener([](){
      std::cout << "OK button clicked!" << std::endl;
  });
  ```

  

#### Button::setOnToggleOnListener(const std::function<void()>&& callback)

- **Purpose:** Sets the "on" state callback for a **toggle mode** button. Called when the button transitions from the non-pressed state to the pressed state (toggled on).

- **Parameters:** `callback`, a callback of type `std::function<void()>`.

- **Return Value:** None.

- **Behavior Details:** Only effective when the button mode is `TOGGLE`. Executed when the user clicks the button causing it to enter the "pressed" (ON) state. Typical use is like toggling a feature on: you can implement the "on" action logic in the callback.

- **Example:**

  c++

  ```
  toggleBtn.setOnToggleOnListener([](){
      turnFeatureOn();
  });
  ```

  

#### Button::setOnToggleOffListener(const std::function<void()>&& callback)

- **Purpose:** Sets the "off" state callback for a **toggle mode** button. Called when the button transitions from the pressed state to the released state (toggled off).

- **Parameters:** `callback`, a callback of type `std::function<void()>`.

- **Return Value:** None.

- **Behavior Details:** Only used for `TOGGLE` mode buttons. Executed when the user clicks the button again, causing it to switch from the ON state back to the OFF state. For example, the logic to turn off a feature should be placed in this callback.

- **Example:**

  c++

  ```
  toggleBtn.setOnToggleOffListener([](){
      turnFeatureOff();
  });
  ```

  

#### Button::setTooltipOffset(int dx, int dy)

- **Purpose:** Sets the offset of the button's hover tooltip relative to the mouse position.

- **Parameters:** `dx, dy`, the offset pixels in the X-axis and Y-axis directions respectively.

- **Return Value:** None.

- **Behavior Details:** When the button has a hover tooltip enabled, the default tooltip displays at an offset down and right from the mouse pointer. This function allows customizing the offset; positive `dx, dy` means offset down and right.

- **Example:**

  c++

  ```
  btn.setTooltipOffset(10, 15); // Tooltip offset 10 right, 15 down relative to mouse
  ```

  

#### Button::setTooltipStyle(COLORREF textColor, COLORREF bkColor, bool transparent)

- **Purpose:** Sets the button's hover tooltip text color, background color, and background transparency property.

- **Parameters:**

  - `textColor`: Tooltip text color.
  - `bkColor`: Tooltip box background fill color.
  - `transparent`: Whether the background is transparent, `true` means the tooltip background is transparent, only showing text.

- **Return Value:** None.

- **Behavior Details:** Allows customizing the tooltip's appearance, e.g., setting white text, black background, etc. If set to background transparent, `bkColor` has no effect. Calling this function immediately affects the style of subsequent tooltips. The default tooltip style is usually light yellow background, black text, opaque.

- **Example:**

  c++

  ```
  btn.setTooltipStyle(WHITE, BLUE, false); // Tooltip white text, blue background
  ```

  

#### Button::setTooltipText(const std::string& text)

- **Purpose:** Sets the button's tooltip text content.

- **Parameters:** `text`, string, new tooltip content.

- **Return Value:** None.

- **Behavior Details:** When the mouse hovers over the button for a certain time, a small tooltip appears displaying this text. By default, if no tooltip text is set, the button's own `text` is used as the tooltip. Calling this function allows specifying different tooltip content. Setting it to an empty string means no tooltip is displayed.

- **Example:**

  c++

  ```
  btn.setTooltipText("Click this button to submit the form");
  ```

  

#### Button::setTooltipTextsForToggle(const std::string& onText, const std::string& offText)

- **Purpose:** For a toggle mode button, sets the hover tooltip text for the "on state" and "off state" respectively.

- **Parameters:**

  - `onText`: Tooltip text when the button is pressed (ON).
  - `offText`: Tooltip text when the button is released (OFF).

- **Return Value:** None.

- **Behavior Details:** A toggle button might need different prompt information in its two states, e.g., ON state prompt "Click to close XXX", OFF state prompt "Click to open XXX". Call this function to provide the tooltip content for both states. Calling this will override any unified tooltip set previously via `setTooltipText`.

- **Example:**

  c++

  ```
  toggleBtn.setTooltipTextsForToggle("Currently ON, click to turn off", "Currently OFF, click to turn on");
  ```

  

### Example

c++

```
// Create and set up a button
Button sendBtn(50, 50, 100, 40, "Send", BLUE, LIGHTBLUE, ButtonMode::NORMAL);
sendBtn.setOnClickListener([](){
    sendMessage();
});
sendBtn.setTooltipText("Send message");
sendBtn.setTooltipStyle(WHITE, BLACK, false);
```



## TextBox Class (Text Box Control)

**Description:** `TextBox` is a single-line text input box control supporting user text input and editing, and can also be configured as read-only mode for displaying non-editable text content. It integrates EasyX's input box functionality internally to handle text input while providing a degree of appearance customization. Typical uses include getting usernames, search boxes, etc.

**Features:**

- **Input/Read-Only Modes:** Provides two modes; input mode allows users to click and pop up a system input box to enter text, read-only mode displays text but prohibits user modification.
- **Maximum Length Limit:** Can set the maximum number of characters for input text to avoid overflow.
- **System Input Box Integration:** Uses Windows input method to ensure input compatibility, using EasyX's `InputBox` to get user input.
- **Multiple Shapes:** Supports rectangle, rounded rectangle variants, etc., as text box border shapes to meet UI style needs.

### Public Member Functions

#### TextBox(int x, int y, int width, int height, const std::string& text = "", StellarX::TextBoxMode mode = StellarX::TextBoxMode::INPUT) (Constructor)

- **Purpose:** Creates a text box control, specifying position, size, initial text, and mode (input or read-only).

- **Parameters:**

  - `x, y`: Text box top-left corner coordinates.
  - `width, height`: Text box width and height.
  - `text`: Initial text content, default empty.
  - `mode`: Text box mode, `TextBoxMode` enumeration, defaults to input mode (`INPUT`), optional `READONLY`.

- **Return Value:** None.

- **Behavior Details:** Can provide an initial display text at construction (like a hint "Please enter...", or existing content), and the mode. If READONLY, the text box won't respond to click input, only for display. Default background is white, bordered rectangle. When the user clicks an input mode text box, the framework automatically calls EasyX's `InputBox` to pop up an input dialog to get text.

- **Example:**

  c++

  ```
  TextBox nameField(100, 80, 150, 25, "", TextBoxMode::INPUT);
  ```

  

#### TextBox::setText(const std::string& text)

- **Purpose:** Sets the text content currently displayed in the text box.

- **Parameters:** `text`, string, new text.

- **Return Value:** None.

- **Behavior Details:** Directly changes the text box content without user input. Often used to initialize the text box content or update the display during program operation. If the text box is currently in read-only mode, it only changes the displayed text; if in input mode, the user can still edit/overwrite this text later.

- **Example:**

  c++

  ```
  nameField.setText("Guest");
  ```

  

#### TextBox::getText() const

- **Purpose:** Gets the text content currently contained in the text box.

- **Parameters:** None.

- **Return Value:** `std::string`, the text box content.

- **Behavior Details:** When the user modifies the text via the input box, this can get the latest result. Even in read-only mode, this method can be used to get the currently displayed text.

- **Example:**

  c++

  ```
  std::string input = nameField.getText();
  ```

  

#### TextBox::setMaxCharLen(size_t len)

- **Purpose:** Sets the maximum character number limit for text box input.

- **Parameters:** `len`, the maximum character length.

- **Return Value:** None.

- **Behavior Details:** This method can restrict users from inputting overly long text. When calling the EasyX input box, this value is passed as a parameter, ensuring the user cannot input a string longer than the specified length (excess parts are truncated). Defaults to some internal preset value (e.g., 64).

- **Example:**

  c++

  ```
  nameField.setMaxCharLen(20); // Limit input to 20 characters max
  ```

  

#### TextBox::setMode(StellarX::TextBoxMode mode)

- **Purpose:** Switches the text box's working mode: input (INPUT) or read-only (READONLY).

- **Parameters:** `mode`, a `TextBoxMode` enumeration value.

- **Return Value:** None.

- **Behavior Details:** Calling this function at runtime can switch the text box from editable to read-only, or vice versa. Switching to read-only mode prohibits users from clicking to activate input; switching to input mode restores interactivity. Changing the mode does not affect the current text content.

- **Example:**

  c++

  ```
  nameField.setMode(TextBoxMode::READONLY); // Disable editing
  ```

  

### Example

c++

```
TextBox inputField(50, 50, 200, 25, "Please enter name");
inputField.setMaxCharLen(30);
inputField.setOnClickListener([](){
    // TextBox itself does not directly support setOnClickListener, but can handle via handleEvent
});
std::string text = inputField.getText(); // Read user input result
```



## Dialog Class (Dialog Control)

**Description:** `Dialog` is a dialog control that can contain a prompt text and several button options, supporting both modal and non-modal display methods. Dialog is typically created and displayed via static methods of the `MessageBox` class. It encapsulates the dialog window's layout, button responses, and return results logic. Dialogs can be used to display messages, warnings, error prompts, etc., and collect user confirm/cancel operations.

**Features:**

- Provides six standard dialog types (OK, YesNo, YesNoCancel, etc.), facilitating quick generation of common confirmation prompts.
- Supports both **modal** (blocks the caller until the dialog closes) and **non-modal** (dialog displays while the program continues running, and results are obtained via callback) modes.
- Automatically calculates window size and layout based on text length and button count, no manual adjustment needed.
- Built-in background snapshot save and restore mechanism, preventing interface flicker when the dialog pops up and restoring the original interface when closed.
- When created using factory methods (`MessageBox`), the framework automatically prevents popping up duplicate dialogs with the same content.

### Public Member Functions

#### Dialog(Window& parent, const std::string& caption, const std::string& message = "Dialog", StellarX::MessageBoxType type = StellarX::MessageBoxType::OK) (Constructor)

- **Purpose:** Creates a dialog control, specifying the parent window, title, message content, and dialog type (button combination).
- **Parameters:**
  - `parent`: The main window reference to which the dialog belongs. The dialog will attach to this window.
  - `caption`: Dialog title text.
  - `message`: Dialog body message text. Default displays "Dialog".
  - `type`: Dialog type, `MessageBoxType` enumeration value, determines which button options are included (e.g., `OK`, `YesNo`, etc.). Default is OK type with only an OK button.
- **Return Value:** None.
- **Behavior Details:** The constructor creates corresponding buttons in number and text based on `type` and lays them out at the dialog bottom; places the `message` text in the central area of the dialog for display. The dialog itself is a special control within the window, usually positioned centrally. Generally, the constructor is not called directly to generate dialogs; instead, use `MessageBox::showModal` or `showAsync` to create and display.
- **Example:** *Usually called by MessageBox; no need to use directly.*

#### Dialog::draw() (override)

- **Purpose:** Draws the dialog, including background, text, and buttons.
- **Parameters:** None.
- **Return Value:** None.
- **Behavior Details:** `Dialog::draw()` draws the dialog's own semi-transparent background overlay (in modal cases) or border, as well as the internal message text and button controls. Modal dialogs first draw the window's background snapshot when drawing, then overlay the dialog content to highlight the dialog. Users do not need to call this directly; handled automatically by the framework.

#### Dialog::handleEvent(const ExMessage& msg) (override)

- **Purpose:** Handles button clicks and other events within the dialog.
- **Parameters:** `msg`, the event message.
- **Return Value:** `bool`, indicates whether the event was consumed by the dialog.
- **Behavior Details:** The `handleEvent` implemented by Dialog dispatches click events to its contained buttons and sets the dialog result based on button clicks (like "OK" or "Cancel"). If the dialog is displayed modally, clicking will end the modal loop and return the result; if non-modal, it passes the result through a pre-registered callback function.

#### Dialog::model() const (override)

- **Purpose:** Indicates whether the dialog is modal.
- **Parameters:** None.
- **Return Value:** `bool`, returns true for modal dialogs, false for non-modal.
- **Behavior Details:** The framework sets this flag based on how the Dialog was created. Modal dialogs are created in `showModal`, non-modal in `showAsync`. This flag is used in the event loop to determine whether to intercept events for other underlying window controls (when a modal dialog exists, underlying control events are paused).

#### Dialog::getResult()

- **Purpose:** Gets the dialog's return result.
- **Parameters:** None.
- **Return Value:** `MessageBoxResult` enumeration value, representing the button the user finally clicked on the dialog, e.g., OK, Cancel, Yes, No, etc.
- **Behavior Details:** Only meaningful for modal dialogs. Before the modal `MessageBox::showModal` returns, it saves the result in the Dialog object and returns it to the caller. In non-modal mode, the result is obtained via callback; this function is not used directly.

### Example

c++

```
// (Generally not using the Dialog class directly, but through MessageBox)
// Here demonstrates manual creation and addition of a non-modal dialog:
Dialog* dlg = new Dialog(mainWin, "Title", "Content", MessageBoxType::YesNo);
mainWin.addDialog(std::unique_ptr<Dialog>(dlg));
// Dialog will display, automatically destroys after user clicks a button. Result can be queried via dlg->getResult() or obtained via callback.
```



## MessageBox Utility Class

**Description:** `MessageBox` provides a simplified dialog creation interface, allowing users to quickly pop up message boxes in the form of **static methods** without directly manipulating the `Dialog` class. It supports both modal dialogs (blocking) and non-modal dialogs (asynchronous via callback). Typical calls are very concise, e.g., one line of code can pop up a "confirmation" dialog waiting for the user to click OK.

**Features:**

- Static method calls, can be used without instantiating an object, convenient for global calls.
- Automatically handles the difference between modal and non-modal logic; modal dialogs block and wait for results, non-modal return immediately and get results via callback after user selection.
- Tightly integrated with `Window`: MessageBox internally uses the main window's dialog management mechanism, ensuring the same dialog content isn't popped up repeatedly within a window.
- Provides a mechanism to prevent duplicate pops: for non-modal prompts with the same content, the second call can choose not to pop up a new one.

### Public Static Functions

#### MessageBox::showModal(Window& wnd, const std::string& text, const std::string& caption = "Prompt", MessageBoxType type = MessageBoxType::OK)

- **Purpose:** Pops up a **modal** dialog, blocking the current thread until the user closes the dialog, and returns the user's selection result.

- **Parameters:**

  - `wnd`: The belonging `Window` reference, specifying in which main window to pop up the dialog.
  - `text`: The main message text displayed by the dialog.
  - `caption`: Dialog title (optional, defaults to "Prompt").
  - `type`: Dialog type (optional, `MessageBoxType` enumeration, defaults to `OK` type).

- **Return Value:** `MessageBoxResult` enumeration, representing the button the user clicked, e.g., `MessageBoxResult::OK`, `MessageBoxResult::Cancel`, etc.

- **Behavior Details:** This function internally creates a `Dialog` object, configures the content and buttons, and displays it modally. The calling thread is blocked, entering an internal message loop that only responds to dialog-related events until the user clicks a button or closes the dialog. Then the function returns the corresponding result. During the modal dialog, other controls in the parent window temporarily lose responsiveness (are masked). This is a synchronous call, simple and easy to use.

- **Example:**

  c++

  ```
  MessageBoxResult res = MessageBox::showModal(mainWin, "Are you sure you want to exit?", "Exit Confirmation", MessageBoxType::YesNo);
  if(res == MessageBoxResult::Yes) {
      // User selected Yes, execute exit logic
  }
  ```

  

#### MessageBox::showAsync(Window& wnd, const std::string& text, const std::string& caption = "Prompt", MessageBoxType type = MessageBoxType::OK, std::function<void(MessageBoxResult)> onResult = nullptr)

- **Purpose:** Pops up a dialog in **non-modal** manner, the function returns immediately, and the user's selection result is obtained asynchronously via a callback function.

- **Parameters:**

  - `wnd`: The belonging `Window` reference.
  - `text`: Message text.
  - `caption`: Title text (defaults to "Prompt").
  - `type`: Dialog type (defaults to OK).
  - `onResult`: Result callback, optional. Type is `std::function<void(MessageBoxResult)>`. When the user clicks a button to close the dialog, the framework calls this callback and passes the result. If `nullptr` is passed, no callback is used.

- **Return Value:** None.

- **Behavior Details:** The function returns immediately; the calling thread is not blocked. The dialog displays non-modally, and other controls on the parent window can still interact (unless the dialog monopolizes the area). When the user makes a selection (clicks a button) or closes the dialog, if a callback was provided, it is called and passed the result enumeration. Simultaneously, the dialog object self-destructs. Internally uses `Window::addDialog()` to add the dialog to window management. The framework has a **deduplication** mechanism: if a non-modal dialog with exactly the same content is currently open and unclosed, calling `showAsync` again will avoid popping up a new one (i.e., the function returns directly and no new dialog is created).

- **Example:**

  c++

  ```
  MessageBox::showAsync(mainWin, "File saved successfully!", "Notification", MessageBoxType::OK,
      [](MessageBoxResult res){
          // Callback logic when non-modal dialog closes
          if(res == MessageBoxResult::OK) {
              // User clicked OK
          }
      });
  ```

  

## TabControl Class (Tab Container Control)

**Description:** `TabControl` is a tab container control managing "tab button + corresponding page (Canvas)" pair combinations. It provides tab bar layout in four directions (top/bottom/left/right), selection switching functionality, and precise positioning of the page content area. This control supports adaptive layout upon window size changes, visibility linkage, and dirty region redraws, making it an ideal solution for hosting multiple pages within the same area.

**Features:**

- **Four-Direction Tab Layout:** Supports tab bar arrangement at the top, bottom, left, and right.
- **Tab-Page Pair Management:** Each tab consists of a button tab and its corresponding Canvas page, supporting one-click addition.
- **Intelligent Switching Mechanism:** Deeply integrated with Button's TOGGLE mode; clicking a tab automatically shows/hides the corresponding page.
- **Adaptive Layout:** Automatically calculates tab and page areas, responds to window size changes.
- **Child Control Management:** Supports adding child controls to specific tabs, automatically handling coordinate conversion.
- **State Management:** Provides functionality to get and set the index of the currently active tab.

### Public Member Functions

#### TabControl() (Constructor 1)

- **Purpose:** Creates a default TabControl control using default position and size.

- **Parameters:** None

- **Return Value:** None

- **Behavior Details:** This constructor creates a TabControl with default position and size values. The control ID is set to "TabControl", tabs are arranged at the top by default, and the tab bar height uses the default value.

- **Example:**

  c++

  ```
  TabControl tabCtrl;
  ```

  

#### TabControl(int x, int y, int width, int height) (Constructor 2)

- **Purpose:** Creates a TabControl control with specified position and size.

- **Parameters:**

  - `x, y`: Control top-left corner coordinates
  - `width, height`: Control's width and height

- **Return Value:** None

- **Behavior Details:** Creates a TabControl with the specified position and size. The control ID is set to "TabControl", tabs are arranged at the top by default, and the tab bar height uses the default value.

- **Example:**

  c++

  ```
  TabControl tabCtrl(10, 10, 400, 300);
  ```

  

#### ~TabControl() (Destructor)

- **Purpose:** Destroys the TabControl instance and releases related resources.
- **Parameters:** None
- **Return Value:** None
- **Behavior Details:** Automatically cleans up all tab buttons and page Canvas objects, ensuring no memory leaks.

#### void draw() override

- **Purpose:** Draws the TabControl and all its child controls.

- **Parameters:** None

- **Return Value:** None

- **Behavior Details:** Overrides Canvas's draw method. First checks the dirty flag and visibility; if redraw is needed, saves/restores the background first, then calls the base class draw method, and finally draws all tab buttons and the currently visible page. Resets the dirty flag after drawing.

- **Example:**

  c++

  ```
  tabCtrl.draw();
  ```

  

#### bool handleEvent(const ExMessage& msg) override

- **Purpose:** Handles input events and dispatches them to child controls.

- **Parameters:**

  - `msg`: Input event message

- **Return Value:** `bool` - Returns true if the event was consumed, otherwise false

- **Behavior Details:** Passes events sequentially to all tab buttons and the currently visible page for processing. If any child control consumes the event, returns true immediately. If the control state becomes dirty, requests a redraw.

- **Example:**

  c++

  ```
  ExMessage msg;
  if (tabCtrl.handleEvent(msg))
  {
      // Event has been handled
  }
  ```

  

#### void add(std::pair<std::unique_ptr<Button>, std::unique_ptr<Canvas>>&& control)

- **Purpose:** Adds a tab and its corresponding page to the TabControl.

- **Parameters:**

  - `control`: A pair object containing the button tab and Canvas page, passed using move semantics

- **Return Value:** None

- **Behavior Details:** Adds the new tab-page pair to the control list, initializes the tab bar and page layout. Sets the parent control for the new tab, enables tooltips, sets it to toggle mode, and registers a listener for toggle state changes. Newly added pages are invisible by default.

- **Example:**

  c++

  ```
  auto tabPair = std::make_pair(std::make_unique<Button>(), std::make_unique<Canvas>());
  tabPair.first->setButtonText("Tab 1");
  tabCtrl.add(std::move(tabPair));
  ```

  

#### void add(std::string tabText, std::unique_ptr<Control> control)

- **Purpose:** Adds a child control to the specified tab.

- **Parameters:**

  - `tabText`: The target tab's text identifier
  - `control`: The child control to add

- **Return Value:** None

- **Behavior Details:** Finds the corresponding page Canvas based on the tab text and adds the child control to that page. The child control's visibility will be synchronized with the target page.

- **Example:**

  c++

  ```
  auto button = std::make_unique<Button>(20, 20, 80, 30, "Click me");
  tabCtrl.add("Tab 1", std::move(button));
  ```

  

#### void setTabPlacement(StellarX::TabPlacement placement)

- **Purpose:** Sets the arrangement position of the tabs.

- **Parameters:**

  - `placement`: Tab arrangement method, a TabPlacement enumeration value

- **Return Value:** None

- **Behavior Details:** Sets the tabs to be arranged at the top, bottom, left, or right. Triggers re-layout of the tab bar and pages, and marks the control as dirty needing redraw.

- **Example:**

  c++

  ```
  tabCtrl.setTabPlacement(StellarX::TabPlacement::Left);
  ```

  

#### void setTabBarHeight(int height)

- **Purpose:** Sets the height of the tab bar (or width for left/right arrangement).

- **Parameters:**

  - `height`: The new height value for the tab bar

- **Return Value:** None

- **Behavior Details:** Modifies the tab bar's dimensions, triggers re-layout of the tab bar and pages, and marks the control as dirty needing redraw.

- **Example:**

  c++

  ```
  tabCtrl.setTabBarHeight(30);
  ```

  

#### void setIsVisible(bool visible) override

- **Purpose:** Sets the control's visibility and synchronizes the visible state of all child controls.

- **Parameters:**

  - `visible`: Whether it is visible

- **Return Value:** None

- **Behavior Details:** Overrides the base class method. First calls Canvas's setIsVisible to handle background snapshot logic, then sets its own visibility, and finally synchronizes the visible state of all tab buttons and pages. When a page is set to invisible, its background snapshot is cleared.

- **Example:**

  c++

  ```
  tabCtrl.setIsVisible(false); // Hide the entire TabControl and all its child controls
  ```

  

#### void onWindowResize() override

- **Purpose:** Responds to the window resize event.

- **Parameters:** None

- **Return Value:** None

- **Behavior Details:** Overrides the base class method, calls the parent class's window resize handling, and then notifies all tab buttons and pages to perform adaptive adjustments.

- **Example:**

  c++

  ```
  tabCtrl.onWindowResize();
  ```

  

#### int getActiveIndex() const

- **Purpose:** Gets the index of the currently active tab.

- **Parameters:** None

- **Return Value:** `int` - The index of the currently active tab, returns -1 if no tab is active

- **Behavior Details:** Traverses all tab buttons and returns the index of the first one in the clicked (active) state.

- **Example:**

  c++

  ```
  int activeIndex = tabCtrl.getActiveIndex();
  ```

  

#### void setActiveIndex(int idx)

- **Purpose:** Sets the tab at the specified index to the active state.

- **Parameters:**

  - `idx`: The index of the tab to activate

- **Return Value:** None

- **Behavior Details:** If the index is valid and the corresponding tab is not disabled, sets it to the active state. If the tab is already active but the page is not visible, shows the page; if it's not active, simulates a click to activate it.

- **Example:**

  c++

  ```
  tabCtrl.setActiveIndex(2); // Activate the third tab
  ```

  

#### int count() const

- **Purpose:** Gets the number of tabs in the TabControl.

- **Parameters:** None

- **Return Value:** `int` - The total number of tabs

- **Behavior Details:** Returns the number of tab-page pairs in the control.

- **Example:**

  c++

  ```
  int tabCount = tabCtrl.count();
  ```

  

#### int indexOf(const std::string& tabText) const

- **Purpose:** Finds the corresponding index by tab text.

- **Parameters:**

  - `tabText`: The tab text to find

- **Return Value:** `int` - The index of the matching tab, returns -1 if not found

- **Behavior Details:** Traverses all tab buttons, compares the button text with the target text, and returns the index of the first matching tab.

- **Example:**

  c++

  ```
  int index = tabCtrl.indexOf("Settings");
  ```

  

#### Usage Example

c++

```
// Create a tab control
auto ta = std::make_unique<TabControl>(10, 10, 950, 240);
// Create tab buttons
auto but = std::make_unique<Button>(0, 0, 100, 15, "Button1");
// Create a pair to wrap the tab and page, then add it to the tab control
std::pair<std::unique_ptr<Button>,std::unique_ptr<Canvas>> p;
p.first =std::move( but);
p.second = std::make_unique<Canvas>(200, 200, 100, 100);
p.second->setCanvasBkColor(RGB(0,255,0));
p.second->addControl(std::move(table));

std::pair<std::unique_ptr<Button>, std::unique_ptr<Canvas>> p1;
p1.first = std::make_unique<Button>(0, 0, 100, 15, "Button2");
p1.second = std::make_unique<Canvas>(200, 200, 100, 100);
p1.second->setCanvasBkColor(RGB(255,200,0));
// Set the tab bar position
ta->setTabPlacement(StellarX::TabPlacement::Bottom);
// Add controls to the tab control
ta->add(std::move(p));
ta->add(std::move(p1));
ta->setCanvasfillMode(StellarX::FillMode::Null);
// Add the tab control to the window's control list
mainWindow.addControl(std::move(ta));
```