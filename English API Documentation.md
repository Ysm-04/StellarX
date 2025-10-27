# English API Documentation

[API文档](API 文档.md)

Below is the API documentation for the **StellarX GUI framework** classes and functions, in English. It covers the usage, parameters, and behavior of each primary component:

### Control Class (Abstract Base Class)

**Description:** `Control` is the abstract base class for all GUI controls. It defines common properties and interfaces such as position, size, and dirty (redraw) flags. It also implements saving and restoring of drawing state to ensure that drawing a control does not affect global graphics state. `Control` itself is not instantiated directly.

- **Key Properties:**
  - `x, y`: Coordinates of the control's top-left corner.
  - `width, height`: Dimensions of the control.
  - `dirty`: A flag indicating whether the control needs to be redrawn.
  - `show`: Visibility flag (whether the control is visible).
  - `rouRectangleSize`: A `StellarX::RouRectangle` struct that stores the ellipse width and height for rounded rectangle corners (used by controls with round-rect shape).
  - **Note:** `Control` internally maintains pointers (`currentFont`, etc.) to backup current drawing state (font, colors, line style) so that it can restore the state after custom drawing.
- **Main Interface Methods:**
  - `virtual void draw() = 0;`
    **Description:** Pure virtual function to draw the control. Each derived control class must implement its own drawing logic.
  - `virtual bool handleEvent(const ExMessage& msg) = 0;`
    **Description:** Pure virtual function to handle an input event message (mouse or keyboard). Returns true if the event is consumed by this control (meaning it should not propagate further).
  - `void saveBackground(int x, int y, int w, int h);`
    **Description:** Saves a background snapshot of the specified area `(x, y, w, h)` (in screen coordinates) where the control is drawn. Used to restore background when needed (e.g., hiding a popup).
  - `void restBackground();`
    **Description:** Restores the last saved background image by putting it back to the saved position. Typically used when a control is hidden or needs to erase its drawing.
  - `void discardBackground();`
    **Description:** Discards the currently saved background snapshot and frees its resources. Should be called when the saved background is no longer valid (e.g., after window resize) to avoid using stale snapshots.
  - *Property Accessors and Mutators:*
    - `int getX() const, getY() const, getWidth() const, getHeight() const`: Get the control's position and size.
    - `int getRight() const, getBottom() const`: Get the coordinate of the control's right boundary (`x + width`) and bottom boundary (`y + height`).
    - `void setX(int nx), setY(int ny), setWidth(int w), setHeight(int h)`: Set the control's position or size. Setting these will mark the control as dirty (need redraw).
    - `void setDirty(bool d)`: Manually mark the control as needing redraw or not.
    - `void setShow(bool visible)`: Set the control's visibility (`show` flag). If set to false, the control will not draw itself.
    - `bool isVisible() const`: Returns the current visibility state of the control (`show` flag). **Note:** If a control is hidden, its `draw()` is typically not called.
  - *Other Methods:*
    - `virtual bool model() const = 0;`
      **Description:** Pure virtual function to check if the control is "modal". Only dialog controls need to implement this (modal dialogs return true), other controls can ignore (return false). The window event loop uses this to prioritize modal dialog events.
    - `void saveStyle(); void restoreStyle();` *(protected)*
      **Description:** Saves the current global drawing style (font, colors, line style, etc.) and restores it. Controls should call `saveStyle()` at the beginning of `draw()` to backup global settings, and `restoreStyle()` at the end of `draw()` to not pollute the global state for other drawings.

### Window Class (Application Main Window)

**Description:** The `Window` class represents the main application window. It manages the window creation, message loop, and acts as the root container for all controls and dialogs. Typically, an application creates one Window instance as the main GUI container.

- **Constructors:**
  - `Window(int width, int height, int mode, COLORREF bkColor = ..., std::string headline = "窗口")`
    **Description:** Creates a Window with specified width and height. `mode` is the graphics mode (such as double-buffering or manual flush flags in EasyX, use NULL for default). `bkColor` is the window background color, and `headline` is the window title text. Construction does not immediately display the window; you need to call `draw()` to open it.
- **Main Methods:**
  - `void draw();`
    **Description:** Initializes and opens the graphics window. It uses the specified mode to create the drawing window (calls EasyX `initgraph`), sets up the window's title and background color, and then draws all added controls. Usually called once right after creating the Window object.
  - `void draw(std::string pImgFile);`
    **Description:** Similar to the above, but uses an image file as the window background. This loads the image and draws it scaled to the window size, then draws all child controls on top.
  - `void runEventLoop();`
    **Description:** Enters the main event processing loop for the window. This loop continuously retrieves user input events (`peekmessage`) and dispatches them to child controls or dialogs:
    - Dialogs (non-modal ones) get first priority: for each event, it iterates over `dialogs` and if a dialog is visible (`isVisible() == true`) and not modal (`model() == false`), it calls that dialog's `handleEvent`. If any dialog consumes the event (returns true), it stops further propagation.
    - If no dialog consumed the event, it then iterates through `controls` (regular controls) in order and calls each control's `handleEvent` until one returns true (meaning the event was handled).
    - After handling an event (or if none present), it checks if any dialog is open or was just closed (`dialogClose` flag). If so, it forces a redraw of the entire interface to keep things updated:
      - It synthesizes a `WM_MOUSEMOVE` message to update hover states (ensuring controls properly update their highlight states), then calls `draw()` on all controls and dialogs, and flushes the drawing.
      - It resets `dialogClose` to false after the redraw.
    - Sleeps for 10 ms at each loop iteration to prevent high CPU usage.
    - The loop continues until a `WM_CLOSE` message is received (e.g., user closes the window), at which point it sets `running` false and breaks out.
  - `void setBkImage(IMAGE* img);` / `void setBkImage(std::string filePath);`
    **Description:** Changes the window's background image. You can pass an already loaded `IMAGE*`, or specify a file path to load. After setting, it immediately repaints the window (drawing all controls and dialogs) with the new background.
  - `void setBkcolor(COLORREF c);`
    **Description:** Sets the window background color and immediately clears the screen with that color (this does not remove an existing background image if one was set; it just overlays the color).
  - `void setHeadline(std::string title);`
    **Description:** Sets the window title text. If the window is already open, it updates the title bar via `SetWindowText` immediately.
  - `void addControl(std::unique_ptr<Control> control);`
    **Description:** Adds a regular control to the window. The Window maintains a list of such controls; `draw()` and `runEventLoop()` use this list for rendering and event dispatch. Ownership is transferred (control is managed by Window after adding).
  - `void addDialog(std::unique_ptr<Control> dialog);`
    **Description:** Adds a dialog control to the window (usually an instance of `Dialog`). Dialogs are managed separately from normal controls, with their own event and drawing handling.
  - `bool hasNonModalDialogWithCaption(const std::string& caption) const;`
    **Description:** Checks if there is a currently open **non-modal** dialog with the given title caption. Returns true if found. Typically used to avoid opening duplicate dialogs with the same purpose.
    - **Overloaded variant:** `bool hasNonModalDialogWithCaption(const std::string& caption, const std::string& text) const;`
      Additionally compares the dialog's message text. This provides a stricter duplicate check (same caption **and** same content). It is used internally by `MessageBox::showAsync` to prevent popping up the same notification multiple times.
  - *Information Getters:*
    - `HWND getHwnd() const;` Returns the underlying window handle (HWND from EasyX).
    - `int getWidth() const, getHeight() const;` Returns the window's dimensions.
    - `std::string getHeadline() const;` Returns the current window title string.
    - `COLORREF getBkcolor() const;` Returns the background color.
    - `IMAGE* getBkImage() const;` Returns the current background image pointer (if any).
    - `std::vector<std::unique_ptr<Control>>& getControls();` Returns a reference to the list of child controls (for iteration or debugging; generally you don't modify this directly).

### Canvas Class (Container Control)

**Description:** `Canvas` is a container control that can hold child controls, grouping them with a unified background and border style, and enabling composite layouts. Canvas itself is a control (derives from `Control`), and typically used for panels or dialog surfaces.

- **Features:**
  - Supports four rectangular background shapes (normal rectangle or rounded rectangle, each with border or borderless version). Set via `setShape`.
  - Customizable background color (`canvasBkColor`), border color (`canvasBorderColor`), border line style (`canvasLineStyle`), and fill mode (`canvasFillMode`, e.g., solid color, hatched pattern, no fill).
  - Automatically manages the lifecycle of child controls (added via `addControl`, and destroyed when Canvas is destroyed).
  - When drawing, Canvas will draw its background and then iterate through its child controls to draw them. For event handling, it propagates events to children (with last-added getting first chance).
- **Important Members:**
  - `std::vector<std::unique_ptr<Control>> controls;` The list of child controls.
  - `StellarX::ControlShape shape;` Background shape of the container (default is RECTANGLE).
  - `StellarX::FillMode canvasFillMode;` Background fill mode (default Solid color).
  - `StellarX::LineStyle canvasLineStyle;` Border line style (default Solid line).
  - `int canvaslinewidth;` Border line width in pixels.
  - `COLORREF canvasBorderColor, canvasBkColor;` Border and background colors of the container.
  - `StellarX::LayoutKind Kind;` (Reserved) Layout management type (Absolute/HBox/VBox/Grid). Not fully implemented but can be used to indicate layout strategy.
  - **Note:** Canvas overrides `isVisible()` to always return false because a Canvas itself is not considered a standalone visible entity for event loop prioritization (the event loop doesn't handle Canvas directly, only its children). This doesn't affect drawing or child event dispatch.
- **Main Methods:**
  - `Canvas(); Canvas(int x, int y, int width, int height);`
    **Description:** Constructors to create a Canvas container at position `(x,y)` with given size.
  - `void addControl(std::unique_ptr<Control> control);`
    **Description:** Adds a child control to the canvas. The child control's coordinates are relative to the canvas (Canvas does not reposition children automatically; you should set the child's position accordingly before adding). After adding, the Canvas is marked dirty so it will redraw with the new content.
  - `void setShape(StellarX::ControlShape shape);`
    **Description:** Sets the canvas background shape. Supports `RECTANGLE`, `B_RECTANGLE` (borderless rectangle), `ROUND_RECTANGLE` (rounded corners with border), `B_ROUND_RECTANGLE` (rounded without border). If a circular/ellipse shape is passed, Canvas does not support it (it will treat it as RECTANGLE internally).
  - `void setCanvasFillMode(StellarX::FillMode mode);`
    **Description:** Sets the background fill mode, e.g., Solid, Null (no fill), Hatched pattern, or pattern/bitmap fill. This influences how the background is drawn (using EasyX `setfillstyle`). Default is solid fill.
  - `void setBorderColor(COLORREF color);` / `void setCanvasBkColor(COLORREF color);` / `void setCanvasLineStyle(StellarX::LineStyle style);` / `void setLinewidth(int width);`
    **Description:** Set the container's border color, background color, border line style, and line width, respectively. Changing any of these will mark the Canvas dirty (to be redrawn).
  - `void draw() override;`
    **Description:** Draws the canvas background and all its child controls. Steps:
    1. If not dirty, returns immediately (no redraw needed).
    2. Saves current style and sets line color, fill color, fill style, and line style based on canvas properties.
    3. Draws the background shape: e.g., calls `fillrectangle` or `fillroundrect` depending on `shape`. If an unsupported shape (Circle/Ellipse) was set, it defaults to rectangle.
    4. Iterates over `controls` and calls each child's `setDirty(true)` then `draw()`, ensuring each child is redrawn on this canvas.
    5. Restores the style and marks itself clean (`dirty = false`).
       *Note:* Canvas uses `saveStyle()`/`restoreStyle()` to preserve global drawing state, and uses EasyX drawing functions to fill the background according to its shape.
  - `bool handleEvent(const ExMessage& msg) override;`
    **Description:** Propagates the event to its children in reverse order (so the last added child gets the event first). It iterates `controls` from end to start and calls each child's `handleEvent(msg)`. If any child returns true (meaning it consumed the event), Canvas stops and returns true. If none consumed it, returns false.
    *Use case:* This ensures that in overlapping or layered controls, the topmost (last added) gets first crack at the event, implementing a basic Z-order.
  - `void clearAllControls();` *(protected)*
    **Description:** Clears all child controls from the canvas, deleting them. This releases all smart pointers in the `controls` list. Typically used in destructor or when resetting the UI. In normal use, you don't call this directly (the Canvas destructor will automatically free children).

### Label Class (Static Text Label)

**Description:** `Label` displays a static text string on the UI. It supports transparent background and custom text style, but does not handle user input (no interactive events). It is lightweight and intended for captions, status messages, etc.

- **Constructors:**

  - `Label(); Label(int x, int y, std::string text = "标签", COLORREF textColor = BLACK, COLORREF bkColor = WHITE);`
    **Description:** Creates a Label control at `(x,y)`. You can specify initial text, text color, and background color. The default text is "标签" (label in Chinese), default text color is black, background is white.

- **Key Properties:**

  - `std::string text;` The text content displayed.
  - `COLORREF textColor;` The text color.
  - `COLORREF textBkColor;` The background color behind the text when not transparent.
  - `bool textBkDisap;` Flag indicating if the background is transparent. If true, the label is drawn with a transparent background (so whatever is behind it shows through); if false, it draws an opaque rectangle behind the text using `textBkColor`.
  - `StellarX::ControlText textStyle;` The text style struct (includes font face, size, weight, etc. as well as text color).
  - **Note:** Label's width and height are initially 0; usually a Label's size is determined by its text content automatically when drawn. You normally don't need to set width/height for labels.

- **Main Methods:**

  - `void setTextdisap(bool transparent);`
    **Description:** Sets whether the label's background is transparent. If true, when drawing, the text is rendered with `TRANSPARENT` background mode (not overwriting the background behind it). If false, the text is drawn on an opaque background colored `textBkColor`.
  - `void setTextColor(COLORREF color);` / `void setTextBkColor(COLORREF color);`
    **Description:** Sets the text color and background color, respectively. After setting, the label is marked dirty for redraw.
  - `void setText(std::string text);`
    **Description:** Changes the label's displayed text content. Marks the label dirty (so it will redraw with the new text).
  - `void draw() override;`
    **Description:** Draws the label's text (and background if not transparent). It:
    - Calls `saveStyle()`.
    - If `textBkDisap` is true, calls `setbkmode(TRANSPARENT)`; otherwise `setbkmode(OPAQUE)` and sets `bkcolor` to `textBkColor`.
    - Sets the text color via `settextcolor(textColor)` and font via `settextstyle` based on `textStyle`.
    - Saves the area behind where the text will be drawn using `saveBackground` (so it can restore it later if needed).
    - Writes the text at `(x,y)` using `outtextxy`.
    - Restores the drawing style and marks `dirty = false`.
      This approach ensures that if the label text is redrawn, the background behind it is handled properly.
  - `bool handleEvent(...) override;`
    **Description:** Label does not handle any events; it always returns false (meaning it never consumes events).
  - `void hide();`
    **Description:** Hides the label. Specifically, it uses `restBackground()` to put back the saved background (erasing the text from the screen) and then `discardBackground()` to free the snapshot, and sets `dirty = false`. This is typically used when a Label is serving as a transient tooltip or overlay and needs to be removed without causing a full screen redraw.

- **Usage Scenarios:**
  Label is used for static text like descriptions, titles, or status information. You can adjust `textStyle` to change the font and size (by default, the font might be "微软雅黑" with height 0 meaning default height). For example:

  ```
  Label *status = new Label(10, 10, "Ready", RGB(0,128,0));
  status->textStyle.nHeight = 20; // set font size if needed
  ```

  If you want the label to blend into a custom background, you can do:

  ```
  status->setTextdisap(true);
  ```

  to make the background transparent.

### Button Class (Button Control)

**Description:** `Button` provides a clickable button control, supporting both standard push-button behavior and toggle (on/off) behavior. It handles click and hover events, and allows setting various styles (shape, colors). Buttons are one of the primary interactive controls.

- **Operating Modes:** Determined by `StellarX::ButtonMode`:

  - `NORMAL` – Standard push-button. Each click triggers an action but does not maintain a pressed state.
  - `TOGGLE` – Toggle button. Each click changes the button's state (pressed vs not pressed) and triggers different callbacks for each state.
  - `DISABLED` – Disabled button. It does not respond to user clicks and typically displays in a grayed-out style with strikeout text.
  - The mode can be changed via `setButtonMode(ButtonMode mode)`.

- **Appearance Shape:** Determined by `StellarX::ControlShape`:

  - Supports rectangle (`RECTANGLE`/`B_RECTANGLE`), rounded rectangle (`ROUND_RECTANGLE`/`B_ROUND_RECTANGLE`), circle (`CIRCLE`/`B_CIRCLE`), and ellipse (`ELLIPSE`/`B_ELLIPSE`) – eight shape options in total (`B_` prefix indicates borderless). Use `setButtonShape(ControlShape shape)` to set.
  - Note: When switching shapes, ensure the button's width/height are appropriate (for circle/ellipse shapes, the drawing will use width and height differently; circle uses min(width,height)/2 as radius, ellipse uses width,height as bounding box).
  - The button's border will be drawn for shapes without the `B_` prefix; borderless shapes omit the border.

- **Key Configurable Properties:**

  - **Colors:**
    - `buttonTrueColor` – color when button is in pressed state (for toggle or momentary press in normal mode).
    - `buttonFalseColor` – color when button is not pressed (normal default state).
    - `buttonHoverColor` – color when mouse is hovering over the button.
    - `buttonBorderColor` – border outline color.
  - **Fill:**
    - `buttonFillMode` – fill mode for the button background (solid, hatched pattern, custom pattern, custom image).
    - `buttonFillIma` – pattern style for hatched fills (if `FillMode == Hatched`).
    - `buttonFileIMAGE` – pointer to an `IMAGE` for custom image fill (if `FillMode == DibPattern`).
  - **Text:**
    - `text` – the text label displayed on the button.
    - `textStyle` – text style (font face, size, weight, etc., including text color).
    - Additionally, `cutText` is used internally if the text is too long to fit; the button can automatically truncate and add "..." or a Chinese ellipsis to fit.
  - **Tooltip (hover hint):**
    - `tipEnabled` – whether a tooltip is enabled on hover.
    - `tipTextClick` – tooltip text for the NORMAL mode (single-state).
    - `tipTextOn` / `tipTextOff` – tooltip texts for toggle mode when the button is ON or OFF.
    - `tipFollowCursor` – if true, tooltip appears near the cursor; if false, tooltip appears at a fixed offset below the button.
    - `tipDelayMs` – delay in milliseconds before tooltip appears when hovering.
    - `tipOffsetX, tipOffsetY` – offset of tooltip position relative to cursor or button (depending on `tipFollowCursor`).
    - `tipLabel` – an internal `Label` object used to display the tooltip text.
  - **Rounded Corner Size:** In round-rectangle shapes, the corner radii come from the inherited `rouRectangleSize` (`ROUND_RECTANGLEwidth` and `height`). Set via `setRoundRectangleWidth(int)` and `setRoundRectangleHeight(int)`.

- **Main Methods:**

  - `void setOnClickListener(const std::function<void()>&& callback);`
    **Description:** Sets the callback function to be invoked when the button is clicked in NORMAL mode. For a toggle button, this callback is called every time the button is clicked (but typically you might use toggle-specific callbacks instead). The callback is executed when the user releases the mouse button over the button.
  - `void setOnToggleOnListener(const std::function<void()>&& callback);`
    **Description:** Sets the callback for when a toggle button is switched to the "on/pressed" state.
  - `void setOnToggleOffListener(const std::function<void()>&& callback);`
    **Description:** Sets the callback for when a toggle button is switched to the "off/released" state.
  - `void setButtonText(const char* text)` / `void setButtonText(std::string text);`
    **Description:** Changes the button's label text. Accepts a C-string or an std::string. After changing, it recalculates the text width/height (for centering and truncation logic) and marks the button dirty to redraw.
  - `void setButtonBorder(COLORREF border);` / `void setButtonFalseColor(COLORREF color);`
    **Description:** Sets the border color and the default (false) state fill color, respectively.
  - `void setFillMode(FillMode mode);` / `void setFillPattern(FillStyle pattern);` / `void setFillImage(const std::string& path);`
    **Description:** Configures the background fill of the button:
    - `setFillMode` changes the fill mode (solid, hatched pattern, custom bitmap, etc).
    - If setting to a hatched pattern fill, call `setFillPattern` to choose the hatch style (`StellarX::FillStyle`).
    - If setting to a custom image fill, call `setFillImage` with the image file path; it will load the image (resizing to button size) and store it in `buttonFileIMAGE`.
  - `void setButtonClick(bool click);`
    **Description:** Programmatically sets the button's "clicked" state.
    - For NORMAL mode: setting true will simulate a click – it triggers the onClick callback (if any) and then immediately resets to false (not pressed).
    - For TOGGLE mode: setting true or false will force the button into that state and trigger the corresponding onToggle callback if provided.
    - In any case, it marks the button dirty and redraws it (ensuring visual state update).
    - **Note:** If the button is disabled, this method has no effect.
  - `bool isClicked() const;`
    **Description:** Returns whether the button is currently in the pressed state. This is primarily meaningful for toggle buttons (true if toggled on, false if off). For a normal button, this is typically false except during the brief moment of click handling.
  - Other getters such as `getButtonText()`, `getButtonMode()`, `getButtonShape()`, `getFillMode()`, `getFillPattern()`, `getFillImage()`, `getButtonBorder()`, `getButtonTextColor()`, `getButtonTextStyle()`, `getButtonWidth()`, `getButtonHeight()` provide read access to the button's corresponding properties.
  - `void draw() override;`
    **Description:** Draws the button's background and text according to its current state and properties:
    - Chooses the fill color based on state: if disabled, uses `DISABLEDCOLOUR` (gray) and applies a strike-out to the text; otherwise, if `click` is true (pressed or toggled on) uses `buttonTrueColor`, else if `hover` is true uses `buttonHoverColor`, otherwise `buttonFalseColor`.
    - Sets transparent background mode for text, sets border color and text style (color, font).
    - If `needCutText` is true, calls `cutButtonText()` to possibly truncate the text with ellipsis if it doesn't fit in the button width.
    - Recalculates `text_width` and `text_height` if the text content or style changed (to ensure text is centered correctly).
    - Sets the fill style for background (using `buttonFillMode`, pattern or image as needed).
    - Draws the shape:
      - For rectangle shapes: uses `fillrectangle` or `solidrectangle`.
      - For round-rectangle: uses `fillroundrect` or `solidroundrect` with `rouRectangleSize` radii.
      - For circle: uses `fillcircle` or `solidcircle` with radius = min(width,height)/2.
      - For ellipse: uses `fillellipse` or `solidellipse` with bounding box corners.
    - Draws the text:
      - It calculates the position to center the text/cutText horizontally and vertically within the button.
      - If `isUseCutText` is true (meaning the original text was truncated), it draws `cutText` (with "..." or Chinese ellipsis).
      - Otherwise, draws the full `text`.
    - Restores style and marks dirty false after drawing.
  - `bool handleEvent(const ExMessage& msg) override;`
    **Description:** Handles mouse events for the button:
    - If the button is hidden (`show == false`), returns false immediately.
    - Tracks the previous `hover` and `click` states to detect changes.
    - If the message is `WM_MOUSEMOVE`, updates `lastMouseX/Y` for tooltip positioning.
    - Determines `hover` state by checking if the mouse `(msg.x, msg.y)` is within the button's shape (calls `isMouseInCircle` or `isMouseInEllipse` for those shapes, otherwise simple rectangle bounds check).
    - Handling `WM_LBUTTONDOWN`: if button is enabled and mouse is over it:
      - In NORMAL mode: set `click = true` (button appears pressed), mark dirty, and mark event consumed.
      - In TOGGLE mode: do nothing on down (toggle action is deferred to release).
    - Handling `WM_LBUTTONUP`: if button is enabled and mouse is currently over it:
      - In NORMAL mode: if it was pressed (`click` was true), trigger the onClick callback (if set), then set `click = false` (release it). Mark dirty, consume event, and hide tooltip (if any). Also flush the message queue of any pending mouse/keyboard events using `flushmessage` to prevent processing a duplicate click message.
      - In TOGGLE mode: flip the `click` state (`click = !click`). If it becomes true, trigger onToggleOn callback; if false, trigger onToggleOff callback. Mark dirty, consume event, update tooltip text via `refreshTooltipTextForState()`, hide tooltip, and flush message queue similarly.
    - Handling `WM_MOUSEMOVE`:
      - If moving outside (not hover anymore) while in NORMAL mode and the button was pressed (`click == true`), then user is dragging out: set `click = false` (cancel the press) and mark dirty (so it returns to unpressed visual).
      - If hover state changed (entered or exited), mark dirty.
    - Tooltip management:
      - If tooltip enabled (`tipEnabled`):
        - If just hovered (hover true and oldHover false): record the timestamp via `tipHoverTick` and set `tipVisible = false` (starting hover timer).
        - If just exited (hover false and oldHover true): immediately call `hideTooltip()` (which hides the tooltip label if it was visible).
        - If still hovering and tooltip not yet visible:
          - Check if current time minus `tipHoverTick` >= `tipDelayMs`; if so:
            - Set `tipVisible = true`.
            - Determine tooltip position: if `tipFollowCursor` is true, `tipX = lastMouseX + tipOffsetX`, `tipY = lastMouseY + tipOffsetY`. If false, perhaps `tipX = lastMouseX` (or button center) and `tipY = y + height` (so it appears below the button).
            - Set the tooltip text: if `tipUserOverride` is true (meaning user explicitly set tooltip text via `setTooltipText` or `setTooltipTextsForToggle`), then:
              - For NORMAL mode: always use `tipTextClick` (explicitly set text or maybe button text by default).
              - For TOGGLE mode: use `click ? tipTextOn : tipTextOff`.
            - If `tipUserOverride` is false (no explicit text provided):
              - If mode is TOGGLE: default behavior might be to use `tipTextOn/Off` (which could be set to default values like "On"/"Off" or left empty).
              - If mode is NORMAL: possibly do nothing (the code currently only sets text in else for toggle, leaving normal with no dynamic text unless overridden).
            - Position the internal `tipLabel` at (tipX, tipY), mark it dirty.
      - After updating tooltip state,
    - If `hover` or `click` state changed compared to old states, mark the button dirty so it will redraw to reflect highlight or press/unpress.
    - If the button is dirty, call `draw()` immediately for real-time feedback.
    - If tooltip is enabled and now visible (`tipVisible`), call `tipLabel.draw()` to draw the tooltip label.
    - Return true if the event was handled (for example, a click or certain modal conditions), else false.
    - (In summary, the event is considered consumed if a click occurred or if the modal logic swallowed it. Hover alone doesn’t consume the event.)

- **Usage Notes:**
  Typically, you create a Button and set its callback like:

  ```
  auto btn = std::make_unique<Button>(50, 50, 80, 30, "OK");
  btn->setOnClickListener([](){ /* handle click */ });
  ```

  For toggle functionality:

  ```
  btn->setButtonMode(StellarX::ButtonMode::TOGGLE);
  btn->setOnToggleOnListener([](){ /* handle toggle on */ });
  btn->setOnToggleOffListener([](){ /* handle toggle off */ });
  ```

  Adjusting appearance:

  ```
  btn->textStyle.color = RGB(255,255,255);              // white text
  btn->setButtonFalseColor(RGB(100,150,200));           // default background
  btn->setButtonBorder(RGB(80,80,80));                  // dark gray border
  btn->setButtonShape(StellarX::ControlShape::RECTANGLE);
  ```

  To provide a tooltip on hover:

  ```
  btn->tipEnabled = true;
  btn->tipTextClick = "Click to confirm";               // for normal mode
  // for toggle mode:
  // btn->setTooltipTextsForToggle("On state hint", "Off state hint");
  ```

  The Button control will handle highlighting itself when hovered, pressing down on click, toggling, etc. Ensure to keep the `runEventLoop` running so these UI feedbacks happen in real time.

### TextBox Class (Single-line Text Box)

**Description:** `TextBox` is a single-line text input box, which can either allow user input or be read-only. It uses an EasyX input box for user text entry (which is a modal popup), then displays the entered text in the control. It's a simple text field primarily for small inputs like numbers or short strings.

- **Modes:** Controlled by `StellarX::TextBoxmode`:

  - `INPUT_MODE` – the user can click and enter text. On click, a modal input dialog appears where the user can type.
  - `READONLY_MODE` – the text box is for display only. Clicking it will not change the text; it might just show an alert that input is not allowed (the current implementation pops an InputBox with a message).
  - Set via `setMode(TextBoxmode mode)`.

- **Appearance:**

  - Supports rectangular shapes (normal or rounded, with or without border). Set with `setTextBoxShape(ControlShape)`. Circular/ellipse shapes are not logically typical for text entry and are treated as rectangle in the implementation.
  - By default, a TextBox has a black border and white background. Use `setTextBoxBorder` and `setTextBoxBk` to change those.
  - Text style is adjustable via `textStyle` (including font face, size, color).

- **Key Properties:**

  - `std::string text;` The content string displayed in the text box.
  - `TextBoxmode mode;` Current mode (input or read-only).
  - `ControlShape shape;` Current border shape (default RECTANGLE or B_RECTANGLE).
  - `bool click;` (Internal flag) Indicates if the text box was clicked (used to trigger the InputBox; it's set true on LButtonUp and then immediately reset after processing).
  - `size_t maxCharLen;` Maximum number of characters allowed. Default is 255.
  - `COLORREF textBoxBorderColor, textBoxBkColor;` Border color and background color of the text box.
  - `StellarX::ControlText textStyle;` Text style for the content (e.g., can use monospace font for numeric input if desired, or change the color).

- **Main Methods:**

  - `void setMode(StellarX::TextBoxmode mode);`
    **Description:** Switches the text box between input mode and read-only mode. Changing mode does not clear the current text content.
  - `void setMaxCharLen(size_t len);`
    **Description:** Sets the maximum number of characters that can be input. If len > 0 (non-zero positive), it will enforce that limit. Input beyond this length will be truncated.
  - `void setTextBoxShape(StellarX::ControlShape shape);`
    **Description:** Sets the shape of the text box. Supports rectangle or rounded rectangle (with or without border). If a shape like circle/ellipse is passed, it's internally treated as rectangle (the implementation falls back to rectangle for unsupported shapes). Changing shape marks the text box dirty for redraw.
  - `void setTextBoxBorder(COLORREF color);` / `void setTextBoxBk(COLORREF color);`
    **Description:** Sets the border color and background color of the text box.
  - `void setText(std::string text);`
    **Description:** Updates the displayed text content of the text box. If the new text exceeds `maxCharLen`, it is automatically truncated to that length. Marks the text box dirty and immediately calls `draw()` to update the displayed text.
  - `std::string getText() const;`
    **Description:** Returns the current text content of the text box.
  - `void draw() override;`
    **Description:** Draws the text box background and the text:
    - Saves style and sets `fillcolor` to `textBoxBkColor`, `linecolor` to `textBoxBorderColor`.
    - Ensures the font height/width do not exceed the control's dimensions (if `textStyle.nHeight` is larger than `height`, it sets it equal to height; similarly for width).
    - Sets the font and text color via `settextstyle` and `settextcolor` according to `textStyle`.
    - Transparent background mode (`setbkmode(TRANSPARENT)` is used to avoid drawing a separate background behind text, since the background is already filled by fillrectangle).
    - Calculates the pixel width and height of the text (`textwidth` and `textheight`).
    - Draws the background shape:
      - If shape is RECTANGLE (with border): calls `fillrectangle` (the border is then drawn by outline because linecolor is set).
      - If shape is B_RECTANGLE: calls `solidrectangle` (no border outline).
      - If shape is ROUND_RECTANGLE: calls `fillroundrect` using `rouRectangleSize` for corners.
      - If shape is B_ROUND_RECTANGLE: calls `solidroundrect`.
      - (Other shapes default to rectangle).
    - Draws the text: It positions the text 10 pixels from the left inside the box (`x + 10`) and vertically centers it (`y + (height - text_h) / 2`).
    - Restores the style and sets dirty false.
    - **Note:** If text content is larger than the control width, it will overflow/clipped; there's no automatic horizontal scrolling or ellipsis for text box implemented. The developer should ensure the width is sufficient or the `maxCharLen` is set appropriately.
  - `bool handleEvent(const ExMessage& msg) override;`
    **Description:** Handles the mouse events for the text box:
    - Determines if the mouse is over the text box (`hover`) using the shape logic (similar to button, but generally rectangular).
    - For `WM_LBUTTONUP` when the cursor is over the text box:
      - If in `INPUT_MODE`: It sets `click = true` and calls `InputBox` (EasyX's modal input dialog). The `InputBox` parameters include:
        - The output buffer (here passing `LPTSTR(text.c_str())` to supply initial text).
        - Max characters (`maxCharLen`).
        - A title like "输入框" (Input Box).
        - It passes `text.c_str()` as default content, etc.
        - The returned value (the function returns nonzero if text changed, 0 if canceled, but here they capture it as `dirty`).
        - After `InputBox` returns, if any input was done, the `text` variable will contain the new text (because the first parameter was the buffer referencing `text` content).
        - They mark `dirty` based on InputBox result (the code sets `dirty` to whatever InputBox returned, which in EasyX indicates whether text was changed).
        - They mark `consume = true` to indicate the click was handled.
      - If in `READONLY_MODE`: They do not change the text. Instead, they possibly call `InputBox(NULL, maxCharLen, "输出框（输入无效！）", ... text ...)` which essentially shows the text in an output box stating input is invalid (effectively a message box).
        - They keep `dirty = false` (since no visual change), set `consume = true`.
      - In both cases, after handling, they flush the input messages with `flushmessage(EX_MOUSE | EX_KEY)` to clear any leftover events (similar reason to button: to avoid re-triggering).
    - For other messages (like `WM_LBUTTONDOWN` or `WM_MOUSEMOVE`):
      - `WM_LBUTTONDOWN` is not explicitly handled (the code simply sets `hover` and sets `consume = false` in the switch).
      - `WM_MOUSEMOVE`: The code sets `hover` accordingly (and uses `consume = false`).
    - After event processing:
      - If `dirty` is true (text changed), it calls `draw()` to refresh the display with new text.
      - If `click` was set (meaning an input happened), it resets `click = false` after handling to ensure state is consistent.
      - Returns `consume` (which would be true if a click caused an InputBox or a read-only alert, false otherwise).
    - Essentially, clicking the text box in input mode opens an input dialog for user to type, and clicking it in read-only mode just shows a dummy output box (so user knows it's not editable).

- **Usage Considerations:**

  - Because `InputBox` is modal, it will pause the entire program's event loop until the user closes the input dialog. This simplifies input handling but means the UI doesn't update during text entry (the `runEventLoop` is blocked). For small apps and short inputs, this is acceptable, but for more advanced usage you might want to implement a custom non-blocking input field.

  - Always set an appropriate `maxCharLen` if expecting certain input sizes (like numeric values of certain length) to avoid overflow of the display region.

  - Use `textStyle` to set a suitable font for the input. For instance, for numeric input, you might choose a monospace font for alignment.

  - Example:

    ```
    auto field = std::make_unique<TextBox>(100, 100, 200, 30, "0");
    field->setMaxCharLen(10);
    field->textStyle.color = RGB(255,69,0); // orange text
    field->textStyle.nHeight = 18;
    // field is input mode by default. Optionally:
    // field->setMode(StellarX::TextBoxmode::READONLY_MODE);
    ```

  - After user input (in input mode), you can retrieve the text via `field->getText()`. In read-only mode, you may set the text programmatically via `setText` and it will just display.

### Dialog Class (Dialog Control)

**Description:** `Dialog` implements modal and modeless dialog windows with rich content and multiple buttons (like message boxes). It inherits from `Canvas` (so it is a container), and includes a caption, message text, and standard buttons combination. It integrates with `Window` to manage blocking input for modal dialogs.

- **Creation & Modes:**

  - Constructor `Dialog(Window& parent, std::string title, std::string message = "对话框", MessageBoxType type = OK, bool modal = true)`
    **Description:** Creates a dialog associated with the `parent` window. `title` is the dialog title text (often shown at top of dialog), `message` is the body text content. `type` specifies which buttons are included (see `MessageBoxType` enum for combinations, e.g., OK, YesNo, etc.). `modal` determines if this is a modal dialog (true) or modeless (false). The dialog is initially not visible (you must call `show()` to display it).
  - `MessageBoxType` enumeration defines common button sets:
    - `OK` – only an "OK" button.
    - `OKCancel` – "OK" and "Cancel" buttons.
    - `YesNo` – "Yes" and "No" buttons.
    - `YesNoCancel` – "Yes", "No", and "Cancel".
    - `RetryCancel` – "Retry" and "Cancel".
    - `AbortRetryIgnore` – "Abort", "Retry", "Ignore".
  - Modal dialogs (`modal == true`): When `show()` is called, it will block the current thread until the dialog is closed (the internal `show()` implementation runs its own loop). This is useful for confirmations or critical inputs that need immediate resolution. After `show()` returns, you can get the result via `getResult()`.
  - Modeless dialogs (`modal == false`): When `show()` is called, it returns immediately (the dialog remains open, but the program flow continues and the main event loop will handle the dialog’s interactions concurrently). To get the result, you typically provide a callback or check `getResult()` after the dialog closes (e.g., via a callback or a flag).

- **Component Elements of Dialog:**

  - **Title label:** A `Label` (stored in `title`) showing the dialog title at top.
  - **Message text:** The main content text, which can be multi-line. It's internally split into `lines` and drawn in the dialog client area.
  - **Buttons:** One to three `Button` controls at the bottom of the dialog, depending on `MessageBoxType`. They are usually centered or evenly spaced.
  - **Close (X) button:** A small `Button` in the top-right corner to close the dialog (especially for modeless dialogs, acts like a Cancel).
  - **Background style:** Typically a rounded rectangle background with a slight border. There is no actual transparency or blur (the background might just capture what's behind for restoration when closed).

- **Key Properties:**

  - `bool modal;` – Whether the dialog is modal.
  - `titleText` – The dialog's title text string.
  - `message` – The dialog's message content string.
  - `MessageBoxType type;` – The type of dialog (determines buttons).
  - `std::vector<std::string> lines;` – The message content split into lines (populated internally).
  - `std::unique_ptr<Label> title;` – Label control for title.
  - `Button* closeButton;` – Pointer to the "X" close button.
  - `Window& hWnd;` – Reference to the parent window (for sending refresh signals, etc.).
  - `StellarX::MessageBoxResult result;` – The result of the dialog (which button was clicked by the user). Common values: OK=1, Cancel=2, Yes=6, No=7, etc. (Matches typical message box results).
  - `std::function<void(MessageBoxResult)> resultCallback;` – If set (for modeless use), a callback function to be called when the dialog is closed with the final result.
  - (Other internal flags track if initialization is needed, if a cleanup is pending, etc.)

- **Main Methods:**

  - `void show();`
    **Description:** Displays the dialog.
    - If `modal == true`, this function enters a loop that blocks the rest of the application until the dialog is closed. During this period, it intercepts events in its own loop (preventing main window events).
    - If `modal == false`, this function simply makes the dialog visible and returns immediately; the dialog will be handled by the main window's event loop.
    - Note: The implementation sets `dirty` and possibly runs its own flush for modals, and uses `shouldClose` to break out of the modal loop if needed.
  - `void closeDialog();`
    **Description:** Closes the dialog.
    - It hides the dialog (`show = false`), marks it for cleanup, and signals the parent window to refresh other controls (by marking them dirty).
    - If a `resultCallback` is set and the dialog is modeless, it invokes the callback with the result.
    - This is typically called internally when a button is clicked (like Cancel or the X button).
  - `StellarX::MessageBoxResult getResult() const;`
    **Description:** Returns the `MessageBoxResult` code indicating which button the user pressed. For modal dialogs, you call this after `show()` returns to get the user's choice. For modeless, you'll typically get the result from the callback rather than polling this function.
  - `void setTitle(const std::string& title);` / `void setMessage(const std::string& message);`
    **Description:** Changes the dialog's title or message text. Changing the message triggers re-splitting into lines and recalculating sizes (the code marks dirty and reinitializes layout).
  - `void setType(StellarX::MessageBoxType newType);`
    **Description:** Changes the dialog's button configuration. This will recreate the buttons layout (calls `initButtons()` internally) and mark the dialog dirty.
  - `void setModal(bool modal);`
    **Description:** Sets the modal property (should be done before showing). Typically you decide at creation whether a dialog is modal or not; toggling at runtime is rare.
  - `void setResult(StellarX::MessageBoxResult res);`
    **Description:** Sets the dialog's result code. This is called internally when a button is clicked (for example, clicking "Yes" calls `setResult(MessageBoxResult::Yes)`).
  - `void setInitialization(bool init);`
    **Description:** If `init` is true, it performs an initialization of dialog size and captures the background. This method is used before showing the dialog (particularly by `MessageBox::showModal` and `showAsync`) to prepare the dialog geometry off-screen. Essentially, it calls `initDialogSize()` and saves the background behind where the dialog will appear (so that it can restore it on close).
  - `void setResultCallback(std::function<void(MessageBoxResult)> cb);`
    **Description:** Sets a callback function to be called with the dialog result when a modeless dialog is closed. Use this for asynchronous dialog handling.
  - *Private internal methods (for implementation):*
    - `initButtons()` – Creates the appropriate `Button` objects for the dialog's `type`, positions them, and sets their onClick listeners to call `setResult` and `closeDialog`. For example, in a YesNo dialog, it creates "Yes" and "No" buttons and sets their listeners to `SetResult(Yes)` / `...No` and `closeDialog()` accordingly.
    - `initCloseButton()` – Creates the "X" close button at top-right of dialog (small size). Its onClick calls `SetResult(Cancel)` (or similarly appropriate default) and triggers `hWnd.dialogClose = true` and `closeDialog()`.
    - `initTitle()` – Creates the title Label at the top inside the dialog.
    - `splitMessageLines()` – Splits the `message` string by newline characters into the `lines` vector for drawing.
    - `getTextSize()` – Calculates the maximum text width and single line height among the `lines`. It does this by setting the text style and measuring each line (`textwidth` & `textheight` calls). It stores results in `textWidth` and `textHeight`.
    - `initDialogSize()` – Computes the dialog's required width and height based on:
      - The greater of text area width (textWidth + margins) and button area width (depending on number of buttons).
      - For height: sums title bar height, spacing to text, total text block height (line count * textHeight + line spacing), and button area height.
      - Enforces a minimum width (in code, 200 px).
      - Centers the dialog on the parent window by setting `x,y` such that the dialog is at window center.
      - Also sets a default text style height (e.g., 20 px).
      - Then calls `initButtons()`, `initTitle()`, and `initCloseButton()` to create all child controls.
    - `performDelayedCleanup()` – Handles the delayed destruction of the dialog's content after it’s closed. It:
      - Marks `isCleaning = true` to avoid reentry.
      - Clears all child controls (`controls.clear()`).
      - Resets pointers (closeButton, title).
      - If a background image was saved and still present (`hasSnap` is true), it restores the screen background (`restBackground()`) and discards the snapshot (`discardBackground()`) to remove any ghost image of dialog.
      - Resets flags (needsInitialization, pendingCleanup, isCleaning, shouldClose).
      - This is called by the event loop after the dialog is hidden, to safely remove the dialog from UI and restore background.
  - `void draw() override;`
    **Description:** Draws the dialog. It combines container drawing with additional text drawing:
    - If dialog is not visible (`show == false`), it does nothing (except if `pendingCleanup` and not cleaning, it calls `performDelayedCleanup()`, but that's in handleEvent rather than draw).
    - If `needsInitialization` is true and dialog is now visible, it calls `initDialogSize()` to compute layout and then sets `needsInitialization = false`. This ensures the dialog's size and children are all properly set up on first draw.
    - It then checks `dirty` and `show`, and if both are true:
      - Saves drawing style.
      - If it's the first draw and `saveBkImage` is null, it calls `saveBackground(...)` to capture what's behind the dialog.
      - Sets the border color, border width, background color, and shape on the base Canvas part (calls `Canvas::setBorderColor`, etc.) to round rectangle and appropriate style.
      - Marks all child controls dirty (so they will be redrawn).
      - Calls `Canvas::draw()` to draw the rounded rectangle background and all the child controls (like buttons, title).
      - Then sets text color and style (for the message text) and calculates a starting y coordinate `ty` just below the title area (using `closeButtonHeight` and a margin).
      - For each line in `lines`, it computes an x coordinate `tx` to center that line in the dialog (x + (width - textwidth(line))/2), and then outputs the text with `outtextxy(tx, ty, line)`.
      - Moves `ty` down by textheight + 5 for the next line (5 px line spacing).
      - Restores style, sets `dirty = false`.
    - The base Canvas::draw already drew the background and the title label and buttons. So these text drawing steps are to overlay the message text content (which is not a separate control, but drawn directly).
  - `bool handleEvent(const ExMessage& msg) override;`
    **Description:** Handles events for the dialog:
    - If the dialog is not visible (`!show`):
      - If there's a `pendingCleanup` and not already cleaning, it calls `performDelayedCleanup()`.
      - Returns false (no event consumed).
    - If a cleanup is pending or in progress (`pendingCleanup || isCleaning`), it immediately returns false (dialog isn't interactive at this moment).
    - If modal:
      - If the user clicked outside the dialog (LButtonUp at a point outside `[x, y, x+width, y+height]`):
        - It prints a bell (`std::cout << "\a"`) to alert (like system beep).
        - Returns true (consumes the event) to prevent underlying controls from receiving the click.
    - Then it attempts to dispatch the event to dialog's child controls:
      - `consume = Canvas::handleEvent(msg)` calls the base container logic to let any button inside the dialog handle the event (e.g., if user clicked "OK", the button's handleEvent will consume it).
      - If a child consumed it, `consume` becomes true.
    - After dispatching, if `pendingCleanup && !isCleaning`, it calls `performDelayedCleanup()` (maybe if a button triggered immediate closure).
    - Returns `consume` (so if a dialog button or modal logic handled the event, it returns true, otherwise false).
    - The effect is that normal controls behind won't receive events while a modal dialog is open (due to the modal check swallowing outside clicks and because the Window event loop prioritizes dialogs).
    - For modeless dialogs, events outside are not swallowed by Dialog (so user can interact with background controls as well, which is typical for modeless).

- **Usage Examples:**
  It's more common to use `MessageBox` static methods to show dialogs (documented next), but you can use `Dialog` directly:

  - **Modal example:**

    ```
    Dialog confirm(window, "Confirm Delete", "Are you sure you want to delete?", StellarX::MessageBoxType::YesNo, true);
    confirm.show();
    if (confirm.getResult() == StellarX::MessageBoxResult::Yes) {
        // proceed with delete
    }
    ```

  - **Modeless example:**

    ```
    Dialog *notify = new Dialog(window, "Notice", "Background task started.", StellarX::MessageBoxType::OK, false);
    notify->setResultCallback([](StellarX::MessageBoxResult res) {
        // maybe log that user acknowledged
    });
    notify->show();
    ```

    Here we allocate on heap (since for modeless we need it to persist beyond function scope), set a callback to handle result asynchronously, and show it. The main loop will handle its events. The callback will be invoked on close (OK click).

  - Typically, prefer using `MessageBox::showModal` and `showAsync` which handle these patterns for you, including cleanup of the `Dialog` object when done.

### MessageBox Utility Class

**Description:** `StellarX::MessageBox` provides convenient static methods to display standard dialogs (message boxes) without manually managing `Dialog` objects. It mimics the idea of a system message box where you specify a message and get a user response.

- **Static Functions:**

  - `static MessageBoxResult showModal(Window& wnd, const std::string& text, const std::string& caption = "提示", MessageBoxType type = MessageBoxType::OK);`
    **Description:** Displays a modal message box on window `wnd` with given content. `caption` is the title of the dialog (e.g., "Error", "Confirmation"), `text` is the message to display, and `type` is the combination of buttons to include. This function blocks until the user closes the dialog. It returns a `MessageBoxResult` indicating which button the user clicked (e.g., `MessageBoxResult::OK` or `Cancel`, etc.). The caller can then branch logic based on this result.
  - `static void showAsync(Window& wnd, const std::string& text, const std::string& caption = "提示", MessageBoxType type = MessageBoxType::OK, std::function<void(MessageBoxResult)> onResult = nullptr);`
    **Description:** Displays a modeless (asynchronous) message box. It returns immediately, leaving the dialog on screen. If `onResult` callback is provided, it will be called once the user closes the dialog, with the `MessageBoxResult` of what they clicked. If a dialog with the same caption **and text** is already open on `wnd`, this function will emit a beep (`\a`) and not open another (to prevent duplicates).
    - This duplicate check uses `wnd.hasNonModalDialogWithCaption(caption, text)` internally.
    - If no callback is provided, the function simply opens the dialog and returns; you'll have to find another way to know when it's closed (perhaps by checking the `Dialog` via some global or so, but ideally you provide a callback).

- **Implementation Details:**

  - `showModal`:
    - Creates a `Dialog` (with modal = true) on the stack.
    - Calls `dlg.setInitialization(true)` to prepare its layout and capture background before showing (to minimize flicker).
    - Calls `dlg.show()`, which will block until the dialog is closed.
    - When `show()` returns, it retrieves the result with `dlg.getResult()` and returns that.
    - All memory is cleaned up on function exit since `dlg` was on stack.
  - `showAsync`:
    - First checks for duplicates; if found, prints a bell and returns without doing anything.
    - Otherwise, creates a `Dialog` on the heap (with modal = false) using a unique_ptr. It then:
      - Calls `dlgPtr->setInitialization(true)` to layout and capture background.
      - If an `onResult` callback is provided, calls `dlgPtr->setResultCallback(std::move(onResult))` to register it.
      - Uses `wnd.addDialog(std::move(dlg))` to add the dialog to the window's management list (transferring ownership to the window).
      - Calls `dlgPtr->show()` to display the dialog modelessly.
    - Then returns immediately. The dialog will operate asynchronously; when closed, it triggers `resultCallback` if set.

- **Typical Uses:**

  - To display an information message and wait for user acknowledgment:

    ```
    StellarX::MessageBox::showModal(mainWindow, "Operation completed successfully.", "Info", MessageBoxType::OK);
    ```

    This will show an OK dialog and block until user presses OK.

  - To ask a yes/no question:

    ```
    auto res = StellarX::MessageBox::showModal(mainWindow, "Delete this file?", "Confirm", MessageBoxType::YesNo);
    if (res == MessageBoxResult::Yes) {
        // delete the file
    }
    ```

  - To show a non-blocking notification:

    ```
    StellarX::MessageBox::showAsync(mainWindow, "The update is downloading in the background.", "Update", MessageBoxType::OK);
    // user can continue using app; the dialog will close when they press OK.
    ```

  - To ask something in the background and get result via callback:

    ```
    StellarX::MessageBox::showAsync(mainWindow, "New version available. Download now?", "Update Available", MessageBoxType::YesNo, 
        [](StellarX::MessageBoxResult res) {
            if (res == StellarX::MessageBoxResult::Yes) {
                startDownload();
            }
        });
    ```

    Here, the lambda will execute once the user clicks Yes or No (Yes triggers `startDownload()` in this example).

Using these static methods is recommended because they handle the creation and cleanup of `Dialog` internally, and provide a simple synchronous interface for modal dialogs and an event-driven interface for modeless dialogs.

## Table Class 

### Class Overview and Purpose

The **Table** class is an advanced table UI control that supports pagination and displaying large datasets[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/include/StellarX/table.h#L2-L10). It provides a comprehensive data grid functionality, including column headers, data rows, and pagination navigation, similar to a spreadsheet. The Table control automatically computes appropriate column widths and row heights, and it allows customization of border style, fill mode, and text font style to fit various UI design requirements. With its built-in paging mechanism, Table can efficiently handle and present large amounts of data without overwhelming the interface, utilizing background buffering to optimize rendering and prevent flicker during redraws.

**Key Features:**

- **Pagination Support:** Automatically handles pagination by computing total pages based on data size and configurable rows-per-page, and provides **Previous/Next** navigation buttons with a page indicator[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/README.md#L274-L282). This enables users to browse through data that spans multiple pages easily.
- **Customization:** Developers can configure the number of rows per page, toggle the visibility of page navigation buttons, and adjust visual aspects such as border color, background color, fill mode (solid fill or transparent), and line style to match the application's theme.
- **Efficient Rendering:** The Table control uses double-buffering techniques by capturing the background under the table and only redrawing changed areas[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L210-L219)[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L241-L249). This minimizes redraw artifacts and improves performance when updating the table content or appearance.
- **Typical Use Cases:** Ideal for displaying tabular data like lists, reports, and records in a GUI application[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/include/StellarX/table.h#L14-L17). For example, it can be used to show database query results, log entries, or statistical reports in a scrollable, paginated view, allowing users to easily read and navigate the information.

### Public Member Functions

#### Table(int x, int y)

**Prototype:** `Table(int x, int y)`
 **Parameters:**

- `x` – The X coordinate for the table’s top-left corner position.
- `y` – The Y coordinate for the table’s top-left corner position.
  **Return Value:** None (constructor).
  **Description:** Constructs a new Table control at the specified position on the screen[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L164-L171). Initially, the table has no headers or data; its width and height are set to 0 and will be calculated once data is provided and headers are set. The `x` and `y` coordinates position the table within the parent window or container. Internally, Table inherits from the base `Control` class, so it is created with an initial area that will expand based on content.
  **Usage Scenario:** Use this constructor when you need to create a table in your application’s UI. For example, after creating a main window, you might call `auto table = std::make_unique<Table>(50, 50);` to place a Table at coordinates (50,50). You would then set up its headers and data before rendering it. The Table can be added to a Window or Canvas as a sub-control for display.

#### ~Table()

**Prototype:** `~Table()`
 **Parameters:** None.
 **Return Value:** None.
 **Description:** Destructor for the Table class. It is called automatically when a Table object is destroyed, and it ensures that all internally allocated resources are freed. This includes deleting any internal controls such as the pagination buttons, page number label, and the background image buffer that were created during the Table’s operation[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L169-L178). By cleaning up these resources, the destructor prevents memory leaks.
 **Usage Scenario:** Generally, you do not need to call this explicitly; it will be invoked when the Table goes out of scope or the program terminates. Understanding the destructor’s behavior is useful for developers to ensure that if a Table is dynamically allocated (e.g., via `new` or `make_unique`), it should be deleted or allowed to go out of scope to trigger cleanup of internal components.

#### draw()

**Prototype:** `void draw() override`
 **Parameters:** None.
 **Return Value:** None.
 **Description:** Overrides the `Control::draw()` method to render the table onto the screen[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L185-L194)[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L252-L260). When `draw()` is called, the Table will draw its column headers, the data rows for the current page, the page number label, and the navigation buttons (if they are enabled). The drawing routine uses the current text style, border color, background color, fill mode, and line style settings that have been configured. The Table employs a “dirty flag” mechanism to optimize rendering: it will only redraw its content if something has changed (such as data or style) that marks it as needing an update[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L187-L196)[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L258-L266). If `dirty` is false, calling `draw()` has no effect. Internally, the function also takes care of restoring any saved background and drawing with minimal flicker.
 **Usage Scenario:** In most cases, you do not need to call `table.draw()` manually, as the framework’s window or event loop will handle drawing. Typically, after adding the Table to a Window and calling the window’s `draw()`, the Table will be drawn. However, if you update the Table’s content or appearance at runtime (for example, in response to an event) and want to refresh immediately, you may call `table.draw()` to force a redraw. This will ensure the table’s display is updated with any new data or style changes.

#### handleEvent(const ExMessage& msg)

**Prototype:** `bool handleEvent(const ExMessage& msg) override`
 **Parameters:**

- `msg` – An event message (`ExMessage` from EasyX library) containing details of a user input event (mouse click, key press, etc.).
  **Return Value:** Boolean, indicating whether the event was handled by the Table. Returns `true` if the event was consumed (handled) by this Table control, or `false` if the Table did not handle the event.
  **Description:** Overrides the `Control::handleEvent()` method to process user input events relevant to the Table. For the Table control, `handleEvent` primarily deals with events on the pagination buttons. If page navigation buttons are enabled (`isShowPageButton` is true), this function will forward the incoming event to the internal "Previous Page" and "Next Page" Button controls for handling[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L274-L283). For example, if the user clicks one of these buttons, the respective internal Button will capture the event, update the Table’s `currentPage` (decrement or increment it), and trigger a redraw of the table to show the new page. In such a case, `handleEvent` returns `true` to indicate the event was handled (and no further processing is needed). If the page buttons are hidden (`isShowPageButton == false`), the Table will ignore all events and always return `false`[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L276-L283), as there is no interactive element for the user to manipulate directly in the Table.
  **Usage Scenario:** In normal usage, you do not call this method directly. The framework’s main loop will call `handleEvent` on each control (including Table) to propagate user inputs. However, understanding this function is useful if you plan to extend the Table or integrate it into a custom event handling flow. For instance, when integrating Table into a Window, the Window’s `runEventLoop()` will pass events to `table.handleEvent(msg)` automatically. If you have disabled the default page buttons, you might implement a custom paging control elsewhere; in that case, Table’s `handleEvent` would not consume events, and you could handle them in your custom controls.

#### setHeaders(std::initializer_liststd::string headers)

**Prototype:** `void setHeaders(std::initializer_list<std::string> headers)`
 **Parameters:**

- `headers` – An initializer list of strings, each representing a column header title.
  **Return Value:** None.
  **Description:** Defines the column headers of the table. This function clears any existing headers and sets up new headers as provided by the list[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L288-L296). Each string in the list becomes the title of a column in the table header. After calling this, the Table marks itself as needing to recalculate cell dimensions and to redraw the headers[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L291-L295) on the next draw. The number of headers determines the number of columns in the table; any subsequent data rows added should match this column count.
  **Usage Scenario:** Call this after creating a Table and before adding data. For example: `table.setHeaders({"Name", "Age", "Occupation"});` will configure the table to have three columns titled "Name", "Age", and "Occupation". Defining headers is typically the first step in preparing the Table for use. Once headers are set, you can proceed to populate data rows with `setData`. Changing headers after data has been added is possible but not recommended, as the existing data might not align with the new columns (if the counts differ); if you must do so, you should reset or adjust the data accordingly.

#### setData(std::vectorstd::string data) / setData(std::initializer_list<std::vectorstd::string> data)

**Prototype 1:** `void setData(std::vector<std::string> data)`
 **Parameters:**

- `data` – A vector of strings representing a single row of data, where each element corresponds to a cell under the respective column.
  **Return Value:** None.
  **Description:** Adds one row of data to the table. If the number of elements in the provided vector is less than the number of headers (columns), the function will append empty strings to the vector until it matches the header count[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L300-L308). This ensures that every row has a value (or placeholder) for each column. The row is then appended to the Table’s internal data storage, and the total page count (`totalPages`) is recalculated based on the new total number of rows and the current `rowsPerPage` setting[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L302-L308). The calculation rounds up to ensure at least one page[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L303-L308). After adding the row, the Table is marked as needing a layout recompute and redraw.
  **Usage Scenario:** Use this function to build the table row by row. For example, in a loop reading records from a file or database, you might call `table.setData({val1, val2, val3});` for each record to append it as a new row. This is especially useful for dynamically updating the table with new entries (e.g., real-time log updates). Each call adds the row at the end of the data and the Table’s display will include it on the appropriate page (immediately if it falls on the current page and you redraw, or after navigation).

**Prototype 2:** `void setData(std::initializer_list<std::vector<std::string>> data)`
 **Parameters:**

- `data` – An initializer list of rows, where each row is represented by a `std::vector<std::string>` (as described above).
  **Return Value:** None.
  **Description:** Adds multiple rows of data to the table in one call. For each row provided in the initializer list, the function checks if its number of elements matches the number of table headers; if a row is shorter, empty strings will be appended until its length equals the header count[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L313-L321). Each row (adjusted as needed) is then appended to the table’s data. After inserting all the rows, the total page count is recalculated accordingly, ensuring it is at least 1[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L322-L328). Like the single-row version, this overload marks the table as needing re-layout and redraw. Note that this function does **not** clear existing data; new rows are appended to any data already present.
  **Usage Scenario:** This is convenient for initializing the table with a batch of data. For example, if you already have a collection of rows ready (perhaps from a data structure or file), you can call `table.setData({row1, row2, row3});` to add them all at once instead of calling the single-row version repeatedly. It simplifies populating the table with initial data. If you intend to replace all existing data, you may want to clear the old data (by resetting the Table or using the returned data vector) before calling this overload. Use this when you have a static set of data to display, especially during initialization or when loading a new dataset into the table.

#### setRowsPerPage(int rows)

**Prototype:** `void setRowsPerPage(int rows)`
 **Parameters:**

- `rows` – The number of data rows to display per page.
  **Return Value:** None.
  **Description:** Sets the pagination size of the table by specifying how many rows are shown on each page. This function updates the internal `rowsPerPage` setting and recalculates the total number of pages (`totalPages`) based on the current total number of data rows[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L332-L338). If the recalculation results in less than 1 page, it defaults to 1 to maintain a valid state. Changing this value will affect the layout (particularly the vertical size of the table and the distribution of data across pages), so the Table marks itself for resizing of cells and redraw. The current page (`currentPage`) is not explicitly adjusted in this function, so if the new `rowsPerPage` significantly reduces the total pages, the table may internally handle out-of-range page indices on the next draw by capping the current page to the last page.
  **Usage Scenario:** Use this to adjust how dense the table appears in terms of data per page. For instance, if a table initially shows 5 rows per page by default but you want to show more data on a larger screen, calling `table.setRowsPerPage(10);` will double the number of rows shown at once, reducing the total page count roughly by half. This is useful for user preferences (show more or fewer items per page) or responsive design considerations. It can be called before or after adding data. If called after data is loaded and the current page is beyond the new total pages, the next redraw will naturally show the last page of data.

#### showPageButton(bool isShow)

**Prototype:** `void showPageButton(bool isShow)`
 **Parameters:**

- `isShow` – A boolean flag indicating whether the page navigation buttons (Previous/Next) should be displayed. `true` to show the buttons; `false` to hide them.
  **Return Value:** None.
  **Description:** Toggles the visibility of the pagination control buttons of the table. When set to `false`, the “Previous Page” and “Next Page” buttons will not be drawn, and the user will not have a UI control to navigate between pages (the table will effectively remain on whatever the current page is, unless changed programmatically). When set to `true`, the buttons are visible, allowing the user to click them to change pages. Hiding the buttons does not alter the data or current page internally; it purely affects the UI and event handling (when hidden, `handleEvent` will ignore navigation clicks as described above). Any call to this function will mark the table as dirty so that on the next draw the buttons will be shown/hidden accordingly.
  **Usage Scenario:** Useful when the dataset fits on one page or when you want to control pagination through a different means. For example, if your table only has a few rows that all fit on a single page, you might call `table.showPageButton(false);` to simplify the interface by removing unnecessary controls. Conversely, if you later load more data that requires paging, you can call `showPageButton(true)` to reveal the navigation. It can also be toggled in response to user actions (like a "show all data" toggle that disables manual paging).

#### setTableBorder(COLORREF color)

**Prototype:** `void setTableBorder(COLORREF color)`
 **Parameters:**

- `color` – A COLORREF value specifying the new color for the table’s border lines (grid lines). This is typically created by the `RGB(r,g,b)` macro on Windows (e.g., `RGB(0,0,0)` for black).
  **Return Value:** None.
  **Description:** Sets the color used to draw the table’s cell borders and outline. After calling this, the Table updates its internal border color and marks itself for redraw. The next time the table is drawn, all grid lines and borders will be rendered in the specified color[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L192-L199). By default, the table’s border color is black. Changing the border color can be useful to match the overall UI theme or to highlight the table. This affects the lines around each cell and the table perimeter.
  **Usage Scenario:** Use this to customize the appearance of the table’s grid. For example, in a dark-themed application you might use a lighter border color for contrast, or in a light theme, a subtle gray to avoid a heavy look: `table.setTableBorder(RGB(200, 200, 200));`. This is purely a cosmetic setting and can be changed anytime (even at runtime) to alter the table’s look. Remember to refresh the display (via `draw()` or window redraw) to see the effect immediately.

#### setTableBk(COLORREF color)

**Prototype:** `void setTableBk(COLORREF color)`
 **Parameters:**

- `color` – A COLORREF specifying the background fill color for the table’s cells.
  **Return Value:** None.
  **Description:** Sets the background color used to fill each cell of the table. This updates the internal background color property and marks the Table as needing redraw. During the next `draw()`, each cell’s background will be filled with the given color (provided the fill mode is solid). By default, table cell backgrounds are white. Changing this can provide alternating row colors or highlight the table area against the window background. If the fill mode is set to Null (transparent), this color will not be visible since cells won’t be filled with a solid color.
  **Usage Scenario:** To change the background color of the table for styling or readability. For instance, `table.setTableBk(RGB(240, 240, 240));` would set a light gray background for all cells, which can reduce glare compared to bright white. This can be paired with custom text colors for better contrast. If you want to implement alternating row colors (like “zebra striping”), the current Table API does not support per-row color via this function directly – you would need to draw custom backgrounds or modify the data drawing logic.

#### setTableFillMode(StellarX::FillMode mode)

**Prototype:** `void setTableFillMode(StellarX::FillMode mode)`
 **Parameters:**

- `mode` – A `StellarX::FillMode` value indicating the fill pattern mode for the cell backgrounds. Common values are `FillMode::Solid` (solid color fill) or `FillMode::Null` (no fill/transparent background). Other modes like `Hatched`, `Pattern`, etc., are also defined but may fall back to Solid if not supported.
  **Return Value:** None.
  **Description:** Configures how the table’s cell background is filled when drawn. In practice, this toggles between solid fill and no fill for the Table (the current implementation treats any mode other than Solid or Null as Solid)[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L360-L369). Setting `FillMode::Solid` means cells will be drawn with the current background color (`tableBk`), whereas `FillMode::Null` means cell backgrounds will not be drawn (making them transparent, showing whatever was behind the table). This function also propagates the fill mode and text style to the internal navigation buttons and page number label to ensure visual consistency[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L365-L373). For example, if you choose Null mode (transparent), the page number label’s background will be made transparent as well. Any change here marks the Table (and its sub-controls) for redraw.
  **Usage Scenario:** Use this when you need to adjust whether the table’s background is visible. A typical use is to set `table.setTableFillMode(StellarX::FillMode::Null);` if you want the table to overlay on a custom background (so only text and borders render, letting the underlying image or color show through). Otherwise, keep it as Solid to use a uniform background color for cells. The default is Solid fill. Be aware that patterns or hatched fills are not fully supported in this version of the framework; even if set, they will revert to a solid fill for simplicity[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L360-L368).

#### setTableLineStyle(StellarX::LineStyle style)

**Prototype:** `void setTableLineStyle(StellarX::LineStyle style)`
 **Parameters:**

- `style` – A `StellarX::LineStyle` value indicating the style of the grid lines (e.g., `LineStyle::Solid` for solid lines, `LineStyle::Dash` for dashed lines, `LineStyle::Dot` for dotted lines, etc.).
  **Return Value:** None.
  **Description:** Sets the style used for drawing the table’s border and grid lines. Changing this will cause the table to use the specified line style (if supported by the underlying graphics library) for all cell borders and the outline on the next redraw. Examples include dashed or dotted lines for a lighter look compared to solid lines. After calling this, the line style property is updated and the table is marked dirty for redrawing. Note that the actual rendering of different line styles depends on the graphics library capabilities.
  **Usage Scenario:** This is a visual customization for the table’s grid. For instance, calling `table.setTableLineStyle(StellarX::LineStyle::Dash);` will make the table draw its cell borders with dashed lines, which might be desirable in a print preview or a specialized UI theme. Use it to differentiate sections or to achieve a stylistic effect. Combine it with appropriate border color and width for best results. If a particular line style is not obvious on screen, ensure that line width is 1 and the color contrasts with the background.

#### setTableBorderWidth(int width)

**Prototype:** `void setTableBorderWidth(int width)`
 **Parameters:**

- `width` – The thickness of the border lines in pixels. Must be a positive integer.
  **Return Value:** None.
  **Description:** Sets the width (thickness) of the lines used for the table’s cell borders and outline. By default, grid lines are 1 pixel thick. Increasing this value will make the table’s grid lines thicker/bolder, while a value of 1 keeps them thin. When this property is changed, the table marks itself for redraw and will use the new line width for drawing borders on the next `draw()`. If a thicker border is set (greater than 1), the Table’s drawing logic takes into account the larger stroke when restoring the background to avoid artifacts (adjusting the area it refreshes)[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L220-L228)[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L240-L248).
  **Usage Scenario:** Use a larger border width when you want the table grid to stand out more, such as in a presentation or when displaying on a high-DPI screen where a 1-pixel line may appear too thin. For example, `table.setTableBorderWidth(2);` will draw grid lines at 2 pixels thickness, making the table look more pronounced. Be cautious with very thick lines as they can clutter the appearance and might overlap cell content if too large relative to cell size. Always test the appearance after changing this setting.

#### Accessor Methods (Getters)

The following methods allow you to query the Table’s current state or configuration:

- **int getCurrentPage() const** – Returns the current page index (1-based) that the table is displaying[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L390-L398). For example, if the table is showing the first page of data, this returns 1; if the user has navigated to the second page, it returns 2, and so on. This can be used to display or log the current page number.
- **int getTotalPages() const** – Returns the total number of pages of data in the table given the current dataset and rows-per-page setting[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L394-L402). This is calculated as ceil(total_rows / rowsPerPage). It’s useful for informing the user (e.g., “Page 2 of 5”) or for logic that might depend on whether more pages are available.
- **int getRowsPerPage() const** – Returns the number of data rows the table is set to display on each page[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L400-L408). This reflects the value last set by `setRowsPerPage`. It can be used to verify the current pagination setting or to adjust external controls accordingly.
- **bool getShowPageButton() const** – Returns the current setting of whether the pagination buttons are visible[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L404-L412) (`true` if they are shown, `false` if hidden). This could be checked to decide if manual navigation UI should be enabled or if an alternate navigation method is needed.
- **COLORREF getTableBorder() const** – Returns the current color (COLORREF) used for the table’s border/grid lines[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L410-L418). For instance, `RGB(0,0,0)` for black. This allows you to retrieve the color if you need to apply the same color elsewhere or for saving the configuration.
- **COLORREF getTableBk() const** – Returns the current background fill color (COLORREF) of the table’s cells[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L414-L422).
- **StellarX::FillMode getTableFillMode() const** – Returns the current fill mode for the table’s cells[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L420-L428), as a `StellarX::FillMode` enum (e.g., Solid or Null). This lets you check whether the table is drawing solid backgrounds or is in transparent mode.
- **StellarX::LineStyle getTableLineStyle() const** – Returns the current line style used for the table’s grid lines[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L424-L432) (as a `StellarX::LineStyle` enum). You can use this to inspect the style (solid, dashed, etc.) programmatically.
- **std::vectorstd::string getHeaders() const** – Returns a copy of the current list of header strings[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L430-L438). The returned vector contains each column’s header text in order. This can be used to retrieve the headers for display elsewhere or for serialization.
- **std::vector<std::vectorstd::string> getData() const** – Returns a copy of all the table’s data[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L434-L442). The returned value is a 2D vector of strings, where each inner vector represents one row of the table (with elements corresponding to each column’s value in that row). This is useful for exporting the table data or performing computations on the data outside of the table (like searching or sorting, which would typically be done on the data and then reapplied to the table).
- **int getTableBorderWidth() const** – Returns the current width (in pixels) of the table’s border lines[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/src/table.cpp#L440-L446).

All these getter functions are `const` and do not modify the Table’s state. They are intended for retrieving the table’s configuration and content at any moment. For example, after populating the table, you might call `getTotalPages()` to update a UI element that shows the total page count, or use `getData()` to verify the data loaded into the table.

### Public Member Variables

- **StellarX::ControlText textStyle** – A struct that defines the font and style used for text rendered in the table (including header text, cell content, and the page number label). By modifying this public member, you can customize the appearance of the table’s text. For instance, you can set `textStyle.nHeight` to change the font height (size), `textStyle.lpszFace` to change the font family name, `textStyle.color` to change the text color, and other attributes such as `nWeight` for boldness, `bItalic` for italics, `bUnderline` for underline, etc.[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/include/StellarX/CoreTypes.h#L134-L142). The Table uses these settings when drawing text via EasyX functions. The default `ControlText` values are initialized to a standard font (e.g., a default Chinese font "微软雅黑" as in the code, or "SimSun") at a default size and black color. Developers can adjust `textStyle` any time before calling `draw()` to apply new text styles. Notably, the Table’s internal prev/next Buttons and page number Label also use the Table’s `textStyle` to ensure consistency in typography across the entire control. For example, to make the table text larger and blue:

  ```
  table.textStyle.nHeight = 20;              // set font height
  table.textStyle.color = RGB(0, 0, 128);    // set text color to navy blue
  ```

  After adjusting, calling `table.draw()` (or a window redraw) will render the table with the new font settings.

### Example

Below is an example demonstrating how to create and use a Table control in a StellarX application[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/README.en.md#L275-L283)[GitHub](https://github.com/Ysm-04/StellarX/blob/95149238e221b19a15bfa750bcf937620e23fd47/README.en.md#L284-L290):

```c++
// Create a table control at position (50, 50)
auto myTable = std::make_unique<Table>(50, 50);

// Set column headers
myTable->setHeaders({ "ID", "Name", "Age", "Occupation" });

// Add data rows
myTable->setData({ "1", "Zhang San", "25", "Engineer" });
myTable->setData({ "2", "Li Si", "30", "Designer" });
myTable->setData({ "3", "Wang Wu", "28", "Product Manager" });

// Set to display 2 rows per page
myTable->setRowsPerPage(2);

// Customize text style and table appearance
myTable->textStyle.nHeight = 16;                       // set font height
myTable->setTableBorder(RGB(50, 50, 50));              // set border color (dark gray)
myTable->setTableBk(RGB(240, 240, 240));               // set background color (light gray)

// Add the table to the main window (assume mainWindow is a Window instance)
mainWindow.addControl(std::move(myTable));
```

In this code, we create a `Table` at coordinates (50, 50) on the window. We then define four column headers: "ID", "Name", "Age", "Occupation". Next, three rows of sample data are added to the table. The `setRowsPerPage(2)` call configures the table to show only 2 rows per page; since we added 3 rows of data, the table will have 2 pages (the first page showing the first 2 rows, and the second page showing the remaining 1 row). We then adjust the table’s appearance by setting the font size (making text slightly larger), changing the border color to a dark gray, and the background color of cells to a light gray for better contrast. Finally, we add the table to a `mainWindow` (which is an instance of `Window`) so that it becomes part of the GUI. Once added, the window’s drawing routine will render the table, and the event loop will ensure the table’s pagination buttons respond to user clicks. Typically, after adding controls to a window, you would call `mainWindow.draw()` to render the initial UI, and then `mainWindow.runEventLoop()` to start processing events, allowing interactions like clicking the "Next" button to flip pages. This example illustrates the basic steps to integrate the Table control into an application and configure its data and appearance.



The above documentation covers all key classes in the StellarX framework. With these, developers can construct fully-featured GUI applications with windows, interactive buttons, text fields, and pop-up dialogs. Keep in mind StellarX is built on EasyX and thus inherits its platform limitation (Windows only) and certain modal behaviors (like `InputBox` use). It is ideal for educational purposes and lightweight tools requiring a simple GUI.