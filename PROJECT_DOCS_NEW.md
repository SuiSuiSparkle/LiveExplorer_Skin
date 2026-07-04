# LiveExplorer_Skin 项目技术文档 (2.0)

## 项目简介

**LiveExplorer_Skin** 是一个用于 Windows 资源管理器（File Explorer）的增强工具，通过在资源管理器窗口上方绘制自定义的透明背景图片，实现个性化的视觉效果。当前代码已更新为以 Qt 作为主界面框架，并通过 Win32 / GDI+ 实现覆盖层渲染。

本项目采用 **Qt 6 (C++)** 与 **Win32 API** 混合架构：
- **Qt 6**: 负责应用程序的生命周期管理、系统托盘图标、设置界面以及配置管理。
- **Win32 API / GDI+**: 负责核心的“透明覆盖窗口”实现，确保高性能、低资源占用且不影响资源管理器的正常交互（点击穿透）。

## 核心架构与技术实现

### 1. 混合架构设计

项目将现代 GUI 框架与底层系统调用相结合，主要包含以下模块：

*   **Main / Application**: `main.cpp`
    *   初始化 `QApplication`。
    *   创建并管理 **系统托盘图标 (QSystemTrayIcon)**。
    *   提供右键菜单（设置、退出、关于）。
    *   加载应用图标（支持 `.ico`）。

*   **控制器 (Controller)**: `Controller.cpp`
    *   作为后台核心逻辑，驱动一个高频定时器（约 60 FPS）。
    *   **窗口追踪**: 实时查找 Windows 资源管理器窗口 (`FindWindowW`)。
    *   **状态监控**: 检测全局快捷键状态 (`GetAsyncKeyState`) 以响应图片切换。
    *   **音频反馈**: 调用 `PlaySound` API 播放启动与切换音效。

*   **原生覆盖层 (NativeOverlay)**: `NativeOverlay.cpp`
    *   封装了底层的 **Win32 窗口创建**与 **GDI+ 绘图**。
    *   维护一个纯 Win32 分层窗口（Layered Window）。

*   **配置管理 (Config)**: `Config.cpp`
    *   单例模式。
    *   使用 `QSettings` 读写 `Config` (注册表或 INI)，持久化用户设置。

*   **设置界面 (SettingsDialog)**: `SettingsDialog.cpp`
    *   基于 Qt Widgets 的图形化配置界面。
    *   支持图片列表管理、透明度/缩放滑块调节、快捷键自定义、音频文件选择。

### 2. 关键技术细节

#### A. 透明窗口与点击穿透 (Win32 Layered Window)
为了实现“看得见但摸不着”的效果，覆盖窗口使用了以下扩展样式：
```cpp
WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW
```
*   **WS_EX_TRANSPARENT**: 确保鼠标事件穿透该窗口，直接传递给下方的资源管理器。
*   **UpdateLayeredWindow**: 不同于普通的 `WM_PAINT`，使用此 API 直接提交带有 Alpha 通道的位图数据，实现高质量半透明合成。

#### B. 智能追踪与 Z 序 (Z-Order)
程序不将窗口设置为“总在最前”，而是动态调整 Z 序以确保留在资源管理器上方但被其他窗口遮挡：
*   获取资源管理器窗口位置 (`GetWindowRect`)。
*   获取资源管理器的上一个兄弟窗口句柄 (`GetWindow(hExplorer, GW_HWNDPREV)`)。
*   使用 `SetWindowPos` 将覆盖窗口插入到该兄弟窗口之后，从而紧贴资源管理器上层。

#### C. GDI+ 图片处理
*   使用 GDI+ `Image::FromFile` 加载 PNG/JPG 图片。
*   创建 32 位 `DIBSection` 内存位图，支持 Alpha 通道。
*   支持根据设置动态调整图片的 **全局透明度** 和 **缩放比例**。

#### D. 音频播放
引入 `winmm.lib`，使用 Windows 多媒体 API 播放 WAV 文件：
```cpp
PlaySound(path, NULL, SND_FILENAME | SND_ASYNC);
```
支持配置 **启动音效** 和 **切换图片音效**。

## 构建与部署

### 1. 构建系统 (CMake + PowerShell)
项目使用 CMake 管理构建过程，支持 MinGW 编译器。为了简化环境配置和依赖部署，提供了一键构建脚本 `build_and_run.ps1`。

**脚本功能：**
1.  **环境配置**: 自动检测并将 Qt 和 MinGW 的 `bin` 目录添加到 PATH。
2.  **清理与构建**: 清除旧构建文件，运行 `cmake` 和 `mingw32-make`。
3.  **资源编译**: 为了解决特殊路径字符（如 `&`）导致的 `windres` 错误，脚本手动处理 `.rc` 资源文件的编译。
4.  **资源部署**: 自动将 `image` 文件夹和 `icon.ico` 复制到构建目录。

### 2. 依赖部署 (windeployqt)
由于使用了 Qt 框架，生成的 EXE 依赖多个 Qt 动态库（DLL）。
构建脚本集成了 `windeployqt` 工具：
*   自动扫描 EXE 依赖。
*   复制必要的 Qt 模块：`Qt6Core`, `Qt6Gui`, `Qt6Widgets`, `Qt6Network`。
*   复制平台插件：`platforms/qwindows.dll`。
*   脚本还会额外检查并复制 MinGW 运行时库：`libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`。

## 使用说明

1.  **启动**: 运行 `LiveExplorer_Skin.exe`。
2.  **托盘控制**: 程序启动后最小化至系统托盘。
    *   **右键菜单**: Settings (设置), About (关于), Exit (退出)。
3.  **设置**:
    *   **Images**: 添加/删除背景图片。
    *   **Opacity**: 调整图片透明度。
    *   **Scale**: 调整图片大小（相对于资源管理器窗口的比例）。
    *   **Keys**: 自定义切换图片的快捷键组合（支持 Ctrl/Alt/Shift + 方向键/字母）。
    *   **Sounds**: 设置启动和切换图片时的音效 (WAV 格式)。

## 项目文件结构

```
FileMngr_BGImage/
├── CMakeLists.txt          # CMake 构建定义
├── filemngr_bgimage.rc     # Windows 资源文件 (图标)
├── build_and_run.ps1       # 自动化构建与运行脚本
├── icon.ico                # 应用程序图标
├── src/
│   ├── main.cpp            # 程序入口
│   ├── Config.h/.cpp       # 配置管理单例
│   ├── Controller.h/.cpp   # 业务逻辑控制
│   ├── NativeOverlay.h/.cpp # Win32 窗口实现
│   └── SettingsDialog.h/.cpp # Qt 设置界面
└── image/                  # 默认图片资源
    ├── source.png
    └── ...
```
