# LiveExplorer_Skin 项目技术文档

## 项目简介

**LiveExplorer_Skin** 是一个面向 Windows 资源管理器（File Explorer）的桌面皮肤覆盖工具，当前版本基于 Qt 6 / Qt 5 与 Win32 API 混合实现，支持托盘控制、透明度与缩放调节、图片切换和音效反馈。

程序通过创建一个不可点击、完全透明的覆盖窗口（Overlay Window），实时追踪资源管理器的位置和大小，并将用户指定的 PNG 图片绘制在资源管理器右下角，实现“伪背景”效果。

## 核心技术实现

### 1. 透明窗口与点击穿透 (Layered Window)

为了让贴图看起来像是资源管理器的一部分，并且不阻挡用户对资源管理器的操作（如点击文件、滚动列表），项目主要利用了 Windows 的 **扩展窗口样式 (Extended Window Styles)**：

*   **`WS_EX_LAYERED`**: 启用分层窗口。这是实现半透明和异形窗口的基础。
*   **`WS_EX_TRANSPARENT`**: 这是一个关键属性。它告诉 Windows 系统，该窗口在命中测试（Hit Testing）时由于是“透明”的，不应接收鼠标输入。因此，鼠标点击事件会直接“穿透”该窗口，传递给下方的资源管理器窗口。
*   **`WS_EX_TOOLWINDOW`**: 防止该窗口在任务栏中显示图标。

```cpp
hOverlayWnd = CreateWindowExW(
    WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
    CLASS_NAME, L"Overlay", WS_POPUP, ...
);
```

### 2. GDI+ 高质量绘图与 Alpha 通道处理

为了支持 PNG 图片的 Alpha 透明通道（半透明渐变效果），程序使用了 **GDI+** 库配合 `UpdateLayeredWindow` API。

*   **Alpha 预乘 (Premultiplied Alpha)**: Windows 的 `UpdateLayeredWindow` 要求位图数据必须是预乘 Alpha 格式。项目创建了一个 32位 的 DIBSection（设备无关位图），并使用 GDI+ 的 `PixelFormat32bppPARGB` 格式将图片绘制到内存中，自动处理了预乘。
*   **一次性合成**: 不同于传统的 `WM_PAINT` 绘制，分层窗口使用 `UpdateLayeredWindow` 将整个内存位图一次性提交给桌面窗口管理器 (DWM)，效率更高且无闪烁。

### 3. 智能窗口追踪与 Z 序管理

为了保持贴图始终附着在资源管理器上，程序在一个循环中执行以下逻辑：

#### 位置与大小计算
*   使用 `FindWindowW(L"CabinetWClass", ...)` 查找资源管理器窗口。
*   根据资源管理器的 Client Area（客户区）或 Window Rect 计算贴图的目标坐标（右下角对齐）。
*   **动态缩放**: 实时计算资源管理器的面积，将贴图面积调整为资源管理器的 **1/5**，保持视觉比例协调。

#### 智能 Z-Order (覆盖层级)
为了实现“仅在资源管理器上方显示，但会被其他窗口（如浏览器）遮挡”的效果，程序通过分析窗口链表动态调整 Z 序：
*   使用 `GetWindow(hExplorer, GW_HWNDPREV)` 获取资源管理器**上层**的一个窗口句柄。
*   将覆盖窗口插入到该上层窗口之后 (`SetWindowPos(..., hInsertAfter, ...)` )。
*   这样，覆盖窗口就成了资源管理器的“贴身保镖”，夹在资源管理器和它上面的窗口之间。

### 4. 性能优化策略

由于涉及死循环检测，性能控制至关重要：
*   **按需更新 (Dirty Check)**: 引入状态缓存 (`lastX`, `lastY`)，仅当目标位置、大小或 Z 序实际发生改变时，才调用昂贵的 `SetWindowPos`。静止状态下几乎零 CPU 占用。
*   **高精度定时器**: 使用 `timeBeginPeriod(1)` 将系统定时器精度提升至 1ms，配合 `Sleep(8)` 实现约 120Hz 的高刷新率，大幅减少拖动时的滞后感。

### 5. 其他特性

*   **资源加载**: 支持从多个路径（当前目录、image子目录、上级目录等）自动搜索图片资源。
*   **启动进度条**: 在初始化阶段使用 GDI 绘制自定义的 Splash Screen 进度条，提升用户体验。
*   **快捷键退出**: 全局监控 `Alt + Q + T` 组合键，实现无界面程序的安全退出。

## 构建说明

本项目使用 **CMake** 构建系统，支持 MSVC (Visual Studio) 和 MinGW (GCC) 编译器。

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

*   构建系统会自动处理 `.rc` 资源文件（图标）的编译。
*   构建完成后，会自动将 `image` 文件夹和生成的 `.exe` 文件复制到项目根目录。

## 主要库函数详解

### Win32 API（windows.h）
- **CreateWindowExW / RegisterClassW / WNDCLASSW**：注册和创建窗口，支持扩展样式（如透明、穿透）。
- **SetWindowPos**：移动/缩放窗口，调整窗口层级（Z序）。
- **FindWindowW / GetWindow**：查找窗口句柄、获取窗口链表中的前后窗口。
- **GetWindowRect / IsWindowVisible / IsIconic**：获取窗口矩形、判断可见/最小化。
- **ShowWindow / IsWindowVisible**：显示/隐藏窗口。
- **GetAsyncKeyState**：检测全局按键状态，实现快捷键退出。
- **Sleep / timeBeginPeriod / timeEndPeriod**：线程休眠与高精度定时器。
- **GetSystemMetrics**：获取屏幕参数。
- **MessageBoxW**：弹窗提示。

### GDI+（gdiplus.h）
- **GdiplusStartup / GdiplusShutdown**：初始化/关闭GDI+。
- **Image / Bitmap / Graphics**：加载图片、位图操作、2D绘图。
- **PixelFormat32bppPARGB / PixelFormat32bppARGB**：32位带Alpha像素格式。
- **SolidBrush / Font / FontFamily**：画刷、字体。
- **DrawImage / DrawString / FillRectangle / Clear**：绘制图片、文字、矩形、清空画布。
- **SetSmoothingMode / SetTextRenderingHint**：抗锯齿与文本渲染质量。

### 设备上下文与位图（HDC/HBITMAP）
- **CreateCompatibleDC / CreateDIBSection / SelectObject / DeleteDC / DeleteObject**：内存DC、DIB位图创建与资源释放。
- **UpdateLayeredWindow**：分层窗口高效更新，支持Alpha通道。

### C++标准库
- **std::wstring / std::string**：Unicode/ANSI字符串处理。
- **std::abs / std::sqrt**：数学运算。
- **for (const wchar_t* path : paths)**：范围for循环，遍历路径。

### 其他
- **#pragma comment (lib, ...)**：链接指定库。
- **HWND / HDC / HBITMAP / BLENDFUNCTION / SIZE / POINT**：Win32核心结构体。

### 主要库函数参数与实参举例

#### Win32 API
```cpp
CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)
```
  - 形参：扩展样式、窗口类名、窗口名、窗口样式、位置、大小、父窗口、菜单、实例句柄、附加参数
  - 实参：`WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW, CLASS_NAME, L"Overlay", WS_POPUP, 0, 0, 0, 0, NULL, NULL, hInstance, NULL`
  - 返回值：成功时返回新创建窗口的句柄（HWND），失败返回NULL。返回的句柄可用于后续窗口操作。

```cpp
SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags)
```
  - 形参：目标窗口、插入到谁后面、X坐标、Y坐标、宽、高、标志
  - 实参：`hOverlayWnd, hInsertAfter, x, y, currentW, currentH, flags`
  - 返回值：返回TRUE表示窗口位置、大小或Z序调整成功，返回FALSE表示操作失败（如窗口句柄无效或参数错误）。

```cpp
FindWindowW(lpClassName, lpWindowName)
```
  - 形参：窗口类名、窗口名
  - 实参：`L"CabinetWClass", NULL`
  - 返回值：返回找到的窗口句柄（HWND），如果未找到指定窗口则返回NULL。

```cpp
GetWindowRect(hWnd, lpRect)
```
  - 形参：窗口句柄、RECT结构体指针
  - 实参：`hExplorer, &rect`
  - 返回值：返回TRUE表示成功获取窗口矩形，FALSE表示失败（如窗口句柄无效）。
  - 被修改变量：`lpRect`（如本程序中的`rect`），调用成功后其内容会被目标窗口的屏幕坐标填充。

```cpp
IsWindowVisible(hWnd)
```
  - 形参：窗口句柄
  - 实参：`hExplorer`
  - 返回值：返回TRUE表示窗口当前可见，FALSE表示窗口被隐藏。

```cpp
IsIconic(hWnd)
```
  - 形参：窗口句柄
  - 实参：`hExplorer`
  - 返回值：返回TRUE表示窗口处于最小化状态，FALSE表示窗口未最小化。

```cpp
ShowWindow(hWnd, nCmdShow)
```
  - 形参：窗口句柄、显示命令
  - 实参：`hOverlayWnd, SW_SHOWNA`
  - 返回值：返回值为BOOL，TRUE表示窗口之前是可见的，FALSE表示窗口之前是隐藏的。

```cpp
GetAsyncKeyState(vKey)
```
  - 形参：虚拟键码
  - 实参：`VK_MENU`、`'Q'`、`'T'`
  - 返回值：返回值的高位为1表示按键当前被按下，低位为1表示按键自上次调用后被按下过。

```cpp
Sleep(dwMilliseconds)
```
  - 形参：毫秒数
  - 实参：`8`
  - 返回值：无返回值，调用后线程挂起指定毫秒数。

```cpp
timeBeginPeriod(uPeriod)
```
  - 形参：定时器分辨率（毫秒）
  - 实参：`1`
  - 返回值：返回0表示设置定时器精度成功，非0表示失败。

```cpp
MessageBoxW(hWnd, lpText, lpCaption, uType)
```
  - 形参：父窗口、内容、标题、类型
  - 实参：`NULL, msg.c_str(), L"错误", MB_ICONERROR`
  - 返回值：返回用户点击的按钮ID（如IDOK、IDCANCEL等），可用于判断用户选择。

#### GDI+
```cpp
GdiplusStartup(token, input, output)
```
  - 形参：GDI+令牌、启动参数、输出参数
  - 实参：`&gdiplusToken, &gdiplusStartupInput, NULL`
  - 返回值：返回0表示GDI+初始化成功，其他值为错误码，表示初始化失败。

```cpp
Image::FromFile(filename)
```
  - 形参：文件路径
  - 实参：`path`
  - 返回值：返回指向加载图片对象的指针，若加载失败则返回NULL。

```cpp
Bitmap(w, h, stride, format, scan0)
```
  - 形参：宽、高、步长、像素格式、像素数据指针
  - 实参：`w, h, w * 4, PixelFormat32bppPARGB, (BYTE*)pvBits`
  - 返回值：返回Bitmap对象实例，若参数无效则构造失败。

```cpp
Graphics::FromImage(bitmap)
```
  - 形参：Bitmap对象指针
  - 实参：`targetBitmap`
  - 返回值：返回Graphics对象指针，若参数无效则返回NULL。

```cpp
DrawImage(image, x, y, w, h)
```
  - 形参：图片对象、X、Y、宽、高
  - 实参：`img, 0, 0, w, h`
  - 返回值：无返回值，操作失败时一般不会抛出异常，但绘制结果可能无效。

```cpp
DrawString(str, len, font, point, brush)
```
  - 形参：字符串、长度、字体、起点、画刷
  - 实参：`text.c_str(), -1, &font, PointF(20, 20), &brushText`
  - 返回值：无返回值，操作失败时一般不会抛出异常，但绘制结果可能无效。

```cpp
FillRectangle(brush, x, y, w, h)
```
  - 形参：画刷、X、Y、宽、高
  - 实参：`&brushBarFill, barX, barY, (int)(barW * progress), barH`
  - 返回值：无返回值，操作失败时一般不会抛出异常，但绘制结果可能无效。

```cpp
Clear(color)
```
  - 形参：颜色
  - 实参：`Color(0, 0, 0, 0)`
  - 返回值：无返回值，操作失败时一般不会抛出异常，但绘制结果可能无效。

#### 设备上下文与位图
```cpp
CreateCompatibleDC(hdc)
```
  - 形参：参考DC
  - 实参：`hdcScreen`
  - 返回值：返回新建的内存设备上下文句柄，失败返回NULL。

```cpp
CreateDIBSection(hdc, pbmi, usage, bits, section, offset)
```
  - 形参：DC、位图信息、用法、像素指针、文件映射、偏移
  - 实参：`hdcScreen, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0`
  - 返回值：返回新建的位图句柄，失败返回NULL。
  - 被修改变量：`bits`（如本程序中的`pvBits`），调用成功后会被赋值为新位图的像素数据指针。

```cpp
SelectObject(hdc, hObject)
```
  - 形参：DC、GDI对象
  - 实参：`hdcMem, hBitmap`
  - 返回值：返回先前选入DC的GDI对象句柄，失败返回NULL。
  - 被修改变量：`hdc`的当前选中对象被替换为`hObject`，原先的对象句柄作为返回值。

```cpp
DeleteDC(hdc)
```
  - 形参：DC
  - 实参：`hdcMem`
  - 返回值：返回TRUE表示资源释放成功，FALSE表示失败（如句柄无效）。

```cpp
DeleteObject(hObject)
```
  - 形参：GDI对象
  - 实参：`hBitmap`
  - 返回值：返回TRUE表示资源释放成功，FALSE表示失败（如句柄无效）。

```cpp
UpdateLayeredWindow(hwnd, hdcDst, pptDst, psize, hdcSrc, pptSrc, crKey, pblend, dwFlags)
```
  - 形参：窗口、目标DC、目标点、尺寸、源DC、源点、透明色、混合、标志
  - 实参：`hWnd, hdcScreen, NULL, &sizeWnd, hdcMem, &pptSrc, 0, &blend, ULW_ALPHA`
  - 返回值：返回TRUE表示窗口内容成功更新，FALSE表示失败（如参数错误或系统不支持）。
