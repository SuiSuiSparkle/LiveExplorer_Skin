# LiveExplorer_Skin 代码深度解析文档

本文档对项目的关键代码模块进行解析，重点阐述实现原理与技术决策。

## 1. 核心逻辑层 (Core Logic)

### 1.1 NativeOverlay (Win32/GDI+ 渲染后端)
**文件**: `src/NativeOverlay.h`, `src/NativeOverlay.cpp`

此模块封装了底层的 Windows 窗口管理与绘图逻辑，是实现“透明覆盖”的核心。

*   **窗口创建 (`init`)**:
    *   注册窗口类 `QtOverlayWindowClass`。
    *   使用 `CreateWindowExW` 创建窗口，关键样式：
        *   `WS_EX_LAYERED`: 启用分层窗口，支持 Alpha 通道。
        *   `WS_EX_TRANSPARENT`: **关键属性**，使窗口在命中测试中被忽略，从而实现“点击穿透”，让鼠标事件直接传递给下方的资源管理器。
        *   `WS_EX_TOOLWINDOW`: 隐藏任务栏图标。

*   **图像加载 (`loadImage`)**:
    *   使用 GDI+ `Image::FromFile` 加载图片。
    *   实现了简单的缓存机制：如果路径未变且对象有效，则复用现有 `Image` 对象，避免重复 I/O。

*   **渲染管线 (`updateLayeredWindow`)**:
    *   **内存绘图**: 创建一个 32位 的内存 DC (`CreateCompatibleDC`) 和 DIBSection (`CreateDIBSection`)，作为离屏缓冲区。
    *   **GDI+ 绘制**: 使用 `Graphics` 对象将图片绘制到内存位图中，开启 `InterpolationModeHighQualityBicubic` 插值以确保缩放质量。
    *   **一帧提交**: 使用 `UpdateLayeredWindow` API 将内存位图一次性更新到屏幕。这是分层窗口的标准更新方式，比 `WM_PAINT` 更高效且无闪烁，完美支持 Alpha 混合 (`AC_SRC_ALPHA`)。

*   **位置同步 (`setPosition`)**:
    *   使用 `SetWindowPos` 更新窗口位置与 Z 序。
    *   `SWP_NOACTIVATE`: 更新位置时不抢占焦点，确保资源管理器保持活动状态。
    *   `SWP_NOSENDCHANGING`: 减少消息发送，微优化性能。

### 1.2 Controller (业务控制器)
**文件**: `src/Controller.h`, `src/Controller.cpp`

此模块是程序的“大脑”，负责协调 UI、配置与底层窗口。

*   **主循环 (`updateLoop`)**:
    *   由 `QTimer` 驱动 (约 60FPS)。
    *   分离了 `checkKeys` (输入检测) 和 `updatePosition` (窗口追踪) 两个任务。

*   **资源管理器追踪 (`updatePosition`)**:
    *   **窗口查找**: 优先查找 `CabinetWClass` (Win10/11 资源管理器)，后备查找 `ExploreWClass`。
    *   **几何计算**:
        *   获取资源管理器窗口矩形 (`GetWindowRect`)。
        *   计算目标区域面积（约为资源管理器面积的 1/5）。
        *   根据配置的 `scale` 动态计算图片的最终尺寸。
    *   **静默更新**: 通过比较 `m_lastW`, `m_lastH` 等状态，仅在尺寸或位置发生显著变化时调用 `m_overlay->update`，大幅降低 CPU 占用。
    *   **Z-Order 同步**:
        *   获取资源管理器的上层兄弟窗口 (`GetWindow(..., GW_HWNDPREV)`)。
        *   将覆盖窗口插入到该兄弟窗口之后，确保其紧贴在资源管理器上方，但位于浏览器等其他窗口下方。

*   **输入与音频 (`checkKeys`)**:
    *   **输入**: 使用 `GetAsyncKeyState` 轮询全局按键状态（因为覆盖窗口不接收焦点，无法使用 Qt 事件）。支持修饰键 (Ctrl/Alt/Shift) 组合检测。
    *   **音频**: 引入 `<mmsystem.h>`，使用 `PlaySound` (`SND_ASYNC`) 异步播放 WAV 音效，避免阻塞主线程。

## 2. 应用程序框架 (Application Framework)

### 2.1 Main Entry
**文件**: `src/main.cpp`

*   **生命周期**: 初始化 `QApplication` 并设置 `setQuitOnLastWindowClosed(false)`，确保关闭设置窗口后程序仍在后台运行。
*   **系统集成**:
    *   创建 `QSystemTrayIcon`，实现从系统托盘管理应用。
    *   加载 `icon.ico` 并同时应用为窗口图标和托盘图标。
*   **交互逻辑**: 实现了双击托盘打开设置的逻辑。

### 2.2 Configuration Management
**文件**: `src/Config.h`, `src/Config.cpp`

*   **设计模式**: 单例模式 (`instance()`)，确保全局配置唯一性。
*   **持久化**: 使用 `QSettings` 将配置保存到注册表 (Windows) 或 INI 文件。
*   **数据模型**: 维护图片列表、当前索引、透明度、缩放比、快捷键码值及音效路径。
*   **逻辑封装**: 提供了 `nextImage()` / `prevImage()` 方法，内部处理索引循环逻辑。

### 2.3 Settings Interface
**文件**: `src/SettingsDialog.h`, `src/SettingsDialog.cpp`

*   **UI 实现**: 纯代码布局 (QVBoxLayout/QHBoxLayout)，未依赖 `.ui` 文件，便于编译控制。
*   **功能映射**:
    *   `QListWidget` 管理图片路径。
    *   `QComboBox` 映射 Win32 虚拟键码 (VK_CODE)。
    *   `QFileDialog` 选择图片和音频文件。
*   **逻辑**: `saveSettings()` 方法负责将 UI 控件的状态回写到 `Config` 单例并触发保存。

## 3. 构建与部署系统 (Build & Deployment)

### 3.1 CMake 构建定义
**文件**: `CMakeLists.txt`

*   **依赖管理**: 优先查找 Qt6，降级兼容 Qt5。
*   **链接库**:
    *   Qt 模块: `Core`, `Gui`, `Widgets`, `Network`.
    *   Windows SDK: `dwmapi`, `user32`, `gdi32`, `gdiplus` (图形), `winmm` (音频)。
*   **特殊处理**: 由于路径中可能包含 `&` 等特殊字符，常规的 `.rc` 编译在 CMake 中可能失败。因此脚本采用了**手动编译资源对象文件** (`filemngr_bgimage_res.o`) 并直接链接到 executable 的策略。

### 3.2 自动化脚本
**文件**: `build_and_run.ps1`

这是一个功能完备的 DevOps 脚本：

1.  **环境自检**: 自动探测 Qt 和 MinGW 路径并注入当前 Session 的 `PATH`。
2.  **资源编译**:
    *   **Hack**: 在 `CMake` 运行前，手动调用 `windres.exe` 编译 `filemngr_bgimage.rc`。
    *   如果编译失败（作为最后手段），生成一个空的 C 文件编译为对象文件，防止链接器报错。
3.  **构建流程**: 标准的 `cmake` -> `mingw32-make` 流程。
4.  **智能部署**:
    *   解析 EXE 位置（Debug/Release）。
    *   **资源同步**: 复制 `image` 文件夹和 `icon.ico`。
    *   **依赖打包**: 调用 `windeployqt` 自动分析并复制 Qt 依赖 DLL。
    *   **Runtime 补充**: 手动复制 `windeployqt` 可能遗漏的 MinGW 运行时库 (`libstdc++`, `libgcc`, `libwinpthread`)。
5.  **进程管理**: 构建前自动终止旧进程，构建后自动启动新实例。

### 3.3 资源文件
**文件**: `filemngr_bgimage.rc`

*   定义了程序的图标资源 ID (`IDI_ICON1`)，使编译出的 EXE 能够显示自定义图标。
