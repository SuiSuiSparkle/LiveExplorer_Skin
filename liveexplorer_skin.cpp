/**,,
 * @file liveexplorer_skin.cpp
 * @brief LiveExplorer_Skin - Windows 资源管理器透明背景贴图工具
 * @details 
 * 本程序创建一个分层透明窗口(Layered Window)，使用 GDI+ 绘制带 Alpha 通道的 PNG 图片，
 * 并实时追踪 Windows 资源管理器(Explorer)的位置和层级，将图片“附着”在其右下角。
 * 
 * 核心功能：
 * 1. 使用 WS_EX_LAYERED | WS_EX_TRANSPARENT 实现点击穿透。
 * 2. 动态计算 Z-Order，确保贴图仅覆盖在资源管理器上方，不遮挡其他窗口。
 * 3. 性能优化：按需重绘与位置更新，结合高精度定时器(TimeBeginPeriod)降低 CPU 占用并减少视觉延迟。
 * 4. 启动时显示加载进度条，自动搜索图片资源。
 * 
 * @author Guancheng Wang (dsagypy3@163.com)
 * @copyright Copyright (c) 2021-2026 Guancheng Wang. All rights reserved.
 * @license Proprietary (Closed Source) - Modification Prohibited
 * @date 2026-01-29
 * @version 1.2 (Release Candidate)
 * 
 * @warning Unauthorized copying, modification, or distribution of this software is strictly prohibited.
 * @note 编译指令参考 CMakeLists.txt 或使用 VS Code Task "CMake Build"
 * @note 依赖库: GDI+, User32, Gdi32, Winmm
 */
#include <windows.h>
#include <gdiplus.h>
#include <iostream>
#include <string>
#include <cmath>
#include <mmsystem.h> // 多媒体定时器

// 链接库
#pragma comment (lib,"Gdiplus.lib")
#pragma comment (lib, "User32.lib")
#pragma comment (lib, "Gdi32.lib")
#pragma comment (lib, "Winmm.lib") // 链接 Winmm 以使用 timeBeginPeriod

using namespace Gdiplus;

#define is_pressing 0x8000
// 配置
const wchar_t* IMAGE_PATH = L"image/source.png"; // 请将您的 PNG 图片放置在同一目录下
const int OFFSET_X = -20; // 距离右边缘的偏移量
const int OFFSET_Y = -20; // 距离底边的偏移量

// 全局变量
ULONG_PTR gdiplusToken;
HWND hOverlayWnd = NULL;
Image* pImage = NULL;
int imgWidth = 0;
int imgHeight = 0;
bool load_pass = true;

// 使用 PNG 图片更新分层窗口
// 使用健壮的方法确保 Alpha 通道被正确保留
void UpdateLayeredWindowImage(HWND hWnd, Image* img, int w, int h) {
    if (!img) return;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    // 1. 严格为表面创建一个 32 位 DIBSection
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // 负高度 = 自上而下的 DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pvBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);

    if (!hBitmap) {
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return;
    }

    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    // 2. 创建一个 GDI+ Bitmap 包装这个 DIBSection 内存
    // 我们使用 PixelFormat32bppPARGB，因为 UpdateLayeredWindow 需要预乘 Alpha
    Bitmap* targetBitmap = new Bitmap(w, h, w * 4, PixelFormat32bppPARGB, (BYTE*)pvBits);

    // 3. 将原始图像绘制到此 DIB 支持的 Bitmap 上
    Graphics* g = Graphics::FromImage(targetBitmap);
    
    // 重要：首先用透明色清除
    g->Clear(Color(0, 0, 0, 0));
    
    // 绘制图像
    g->DrawImage(img, 0, 0, w, h);

    // 4. 更新分层窗口
    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 0xA0; // 全局透明度设为 :
    blend.AlphaFormat = AC_SRC_ALPHA; // 使用逐像素 Alpha

    SIZE sizeWnd = { w, h };
    POINT pptSrc = { 0, 0 };
    POINT pptDst = { 0, 0 }; // 如果我们不传递位置更新就不需要它，但这是标准做法
    
    UpdateLayeredWindow(hWnd, hdcScreen, NULL, &sizeWnd, hdcMem, &pptSrc, 0, &blend, ULW_ALPHA);

    // 清理
    delete g;
    delete targetBitmap;
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 绘制加载界面
void ShowLoadingUI(HWND hWnd, const std::wstring& text, float progress) {
    int w = 400;
    int h = 100;
    
    // 获取屏幕尺寸以居中
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);
    int x = (scrW - w) / 2;
    int y = (scrH - h) / 2;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pvBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    Bitmap* targetBitmap = new Bitmap(w, h, w * 4, PixelFormat32bppPARGB, (BYTE*)pvBits);
    Graphics* g = Graphics::FromImage(targetBitmap);
    g->SetSmoothingMode(SmoothingModeAntiAlias);
    g->SetTextRenderingHint(TextRenderingHintAntiAlias);

    // 1. 绘制背景 (圆角矩形)
    g->Clear(Color(0, 0, 0, 0));
    SolidBrush brushBg(Color(200, 30, 30, 30)); // 半透明深色背景
    g->FillRectangle(&brushBg, 0, 0, w, h);

    // 2. 绘制文字
    FontFamily fontFamily(L"Microsoft YaHei");
    Font font(&fontFamily, 14, FontStyleRegular, UnitPixel);
    SolidBrush brushText(Color(255, 255, 255, 255));
    g->DrawString(text.c_str(), -1, &font, PointF(20, 20), &brushText);

    // 3. 绘制进度条
    int barX = 20;
    int barY = 60;
    int barW = w - 40;
    int barH = 6;
    
    SolidBrush brushBarBg(Color(255, 80, 80, 80));
    g->FillRectangle(&brushBarBg, barX, barY, barW, barH);
    
    if (progress > 0 && load_pass) {
        SolidBrush brushBarFill(Color(255, 0, 120, 215)); // 蓝色进度
        g->FillRectangle(&brushBarFill, barX, barY, (int)(barW * progress), barH);
    }else if (progress > 0 && !load_pass){
        SolidBrush brushBarFill(Color(255, 215, 0, 0)); // 红色进度
        g->FillRectangle(&brushBarFill, barX, barY, (int)(barW * progress), barH);
    }

    // 4. 更新窗口
    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255; // 加载界面不使用全局半透明，清晰可见
    blend.AlphaFormat = AC_SRC_ALPHA;

    SIZE sizeWnd = { w, h };
    POINT pptSrc = { 0, 0 };
    POINT pptDst = { x, y }; // 居中位置
    
    UpdateLayeredWindow(hWnd, hdcScreen, &pptDst, &sizeWnd, hdcMem, &pptSrc, 0, &blend, ULW_ALPHA);

    delete g;
    delete targetBitmap;
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

// 尝试从多个路径加载图片
Image* TryLoadImage(HWND hWnd) {
    const wchar_t* paths[] = {              
        
        L"source.png",

        L"image\\source.png",       
  
        L"../image/source.png",

        
   
    };

    int total = sizeof(paths) / sizeof(paths[0]);
    int current = 0;

    // 先显示窗口
    ShowWindow(hWnd, SW_SHOW);

    for (const wchar_t* path : paths) {
        current++;
        
        // 稍微延时以便看到加载效果
        
        Image* img = Image::FromFile(path);
        if (img && img->GetLastStatus() == Ok) {
            // 找到图片了
            load_pass = true;
            std::wstring msg = L"正在搜索图片资源: " + std::wstring(path);
            ShowLoadingUI(hWnd, msg, (float)current / total);
            Sleep(500); 
            return img;
        }
        else if (img){
            delete img;
            load_pass = false;
            std::wstring msg = L"ERROR: 无法加载图片资源: " + std::wstring(path) + L" 按下enter继续搜索下一个路径。";
            ShowLoadingUI(hWnd, msg, (float)current / total);
            while(!(GetAsyncKeyState(VK_RETURN) & is_pressing)) Sleep(10);
        }
    }
    return NULL;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 1. 提高进程优先级 (已注释，为了降低资源占用，除非有严重延迟否则不建议开启)
    // SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    
    // 2. 设置系统定时器精度为 1ms
    timeBeginPeriod(1);

    // 初始化 GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // 注册窗口类
    const wchar_t CLASS_NAME[] = L"OverlayWindowClass";
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    // 创建窗口
    // WS_EX_LAYERED 用于透明
    // WS_EX_TRANSPARENT 允许点击穿透
    // WS_EX_TOOLWINDOW 在任务栏中隐藏
    // 去除 WS_EX_TOPMOST，因为我们需要动态调整层级
    hOverlayWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        L"Overlay",
        WS_POPUP,
        0, 0, 0, 0,
        NULL, NULL, hInstance, NULL
    );

    if (hOverlayWnd == NULL) {
        return 0;
    }

    // 加载图片 (带进度条)
    pImage = TryLoadImage(hOverlayWnd);
    
    if (pImage == NULL) {
        wchar_t cwd[MAX_PATH] = {0};
        GetCurrentDirectoryW(MAX_PATH, cwd);
        std::wstring msg = L"无法找到图片文件 (image/source.png 等)。\n\n当前工作目录:\n" + std::wstring(cwd);
        MessageBoxW(NULL, msg.c_str(), L"错误", MB_ICONERROR);
        
        GdiplusShutdown(gdiplusToken);
        return 0;
    }
    imgWidth = pImage->GetWidth();
    imgHeight = pImage->GetHeight();
    int currentW = 0;
    int currentH = 0;
    
    // 状态缓存，用于减少不必要的 API 调用
    int lastX = -9999;
    int lastY = -9999;

    // 初始绘制（稍后在循环中会更新大小，这里可以先不画，或者画个默认）
    // UpdateLayeredWindowImage(hOverlayWnd, pImage, imgWidth, imgHeight); 
    // ShowWindow(hOverlayWnd, SW_SHOWNA);

    // 循环
    MSG msg = { 0 };
    while (true) {
        // 使用具体的 PeekMessage 而不是 GetMessage，以防止阻塞
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                goto Cleanup;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // 监听键盘退出组合键: Alt + Q + T
        if ((GetAsyncKeyState(VK_MENU) & is_pressing) && 
            (GetAsyncKeyState('Q') & is_pressing) && 
            (GetAsyncKeyState('T') & is_pressing)) {
            goto Cleanup;
        }

        // 查找文件资源管理器
        HWND hExplorer = FindWindowW(L"CabinetWClass", NULL);
        // 注意：有时资源管理器在旧版本或特定模式下使用 ExploreWClass，但在 Win10/11 上主要是 CabinetWClass
        if (!hExplorer) hExplorer = FindWindowW(L"ExploreWClass", NULL);

        if (hExplorer && IsWindowVisible(hExplorer) && !IsIconic(hExplorer)) {
            RECT rect;
            if (GetWindowRect(hExplorer, &rect)) {
                long explorerW = rect.right - rect.left;
                long explorerH = rect.bottom - rect.top;
                long explorerArea = explorerW * explorerH;

                // 计算目标面积 (1/5.5)
                double targetArea = (double)explorerArea / 5.5;
                long origArea = pImage->GetWidth() * pImage->GetHeight();
                
                // 计算缩放比例
                double scale = sqrt(targetArea / (double)origArea);
                int newW = (int)(pImage->GetWidth() * scale);
                int newH = (int)(pImage->GetHeight() * scale);

                // 如果尺寸变化明显，重新绘制
                if (abs(newW - currentW) > 2 || abs(newH - currentH) > 2) {
                    UpdateLayeredWindowImage(hOverlayWnd, pImage, newW, newH);
                    currentW = newW;
                    currentH = newH;
                }

                 // 计算位置（右下角）
                int x = rect.right - currentW + OFFSET_X;
                int y = rect.bottom - currentH + OFFSET_Y;

                // 计算 Z-Order (仅大于资源管理器)
                HWND hPrev = GetWindow(hExplorer, GW_HWNDPREV);
                
                // 优化：仅当位置或 Z-Order 发生变化时才调用 SetWindowPos
                // 这是一个重要的性能优化，避免了每一帧都进行繁重的窗口重绘/IPC调用
                bool posChanged = (x != lastX || y != lastY || abs(currentW - imgWidth) > 0 /* 大小变化已触发重绘，这里仅检查位置 */ );
                // 注意：currentW 已经在上面更新了，但我们需要确保如果大小变了，SetWindowPos 也要调用
                
                bool zOrderChanged = (hPrev != hOverlayWnd);

                if (posChanged || zOrderChanged) {
                    HWND hInsertAfter = HWND_TOP;
                    UINT flags = SWP_NOACTIVATE;

                    if (!zOrderChanged) {
                        // Z 序正确（我们就在资源管理器上面），不改变 Z 序，仅移动
                        flags |= SWP_NOZORDER;
                    } else if (hPrev != NULL) {
                        // 有窗体插在中间，我们需要把自己插到它下面（即它和 Explorer 之间）
                        // 逻辑：hPrev 是 Explorer 上面的窗口。我们要插在 hPrev 后面？
                        // 不，SelectWindowPos(hwnd, hInsertAfter, ...) 把 hwnd 放在 hInsertAfter 后面。
                        hInsertAfter = hPrev;
                    }

                    // 移动覆盖窗口
                    SetWindowPos(hOverlayWnd, hInsertAfter, x, y, currentW, currentH, flags);
                    
                    lastX = x;
                    lastY = y;
                }
                
                if (!IsWindowVisible(hOverlayWnd)) {
                    ShowWindow(hOverlayWnd, SW_SHOWNA);
                }
            }
        } else {
             // 如果资源管理器不可见/最小化/关闭，则隐藏
             if (IsWindowVisible(hOverlayWnd)) {
                 ShowWindow(hOverlayWnd, SW_HIDE);
             }
        }

        // 3. 减少睡眠时间。15ms 约为 60fps，8ms 约为 120fps。
        // 由于调整了 timeBeginPeriod(1)，这个 Sleep 会非常精准。
        Sleep(8); 
        
    }

Cleanup:
    timeEndPeriod(1); // 恢复定时器精度
    delete pImage;
    GdiplusShutdown(gdiplusToken);
    return 0;
}
