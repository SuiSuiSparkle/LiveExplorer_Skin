#include "NativeOverlay.h"
#include <QDebug>
#include <QFileInfo>

#pragma comment (lib,"Gdiplus.lib")
#pragma comment (lib, "User32.lib")
#pragma comment (lib, "Gdi32.lib")

LRESULT CALLBACK OverlayWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

NativeOverlay::NativeOverlay() : m_hWnd(NULL), m_currentImage(NULL), m_lastDrawW(0), m_lastDrawH(0), m_lastDrawOpacity(-1) {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
}

NativeOverlay::~NativeOverlay() {
    if (m_currentImage) delete m_currentImage;
    if (m_hWnd) DestroyWindow(m_hWnd);
    GdiplusShutdown(m_gdiplusToken);
}

bool NativeOverlay::init(HINSTANCE hInstance) {
    const wchar_t CLASS_NAME[] = L"QtOverlayWindowClass";
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = OverlayWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    m_hWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        L"Overlay",
        WS_POPUP,
        0, 0, 0, 0,
        NULL, NULL, hInstance, NULL
    );

    return (m_hWnd != NULL);
}

QSize NativeOverlay::loadImage(const QString& imagePath) {
    if (imagePath != m_lastPath) {
        if (m_currentImage) delete m_currentImage;
        m_currentImage = Image::FromFile(imagePath.toStdWString().c_str());
        m_lastPath = imagePath;
        m_lastDrawOpacity = -1; // Force redraw
    }

    if (m_currentImage && m_currentImage->GetLastStatus() == Ok) {
        return QSize(m_currentImage->GetWidth(), m_currentImage->GetHeight());
    }
    return QSize(0, 0);
}

void NativeOverlay::update(int w, int h, double opacity) {
    if (!m_hWnd || !m_currentImage) return;

    // Check if we need to redraw
    bool needRedraw = (opacity != m_lastDrawOpacity) || (w != m_lastDrawW) || (h != m_lastDrawH);

    if (needRedraw) {
         updateLayeredWindow(w, h, opacity);
         m_lastDrawW = w;
         m_lastDrawH = h;
         m_lastDrawOpacity = opacity;
    }
    
    if (!IsWindowVisible(m_hWnd)) {
        ShowWindow(m_hWnd, SW_SHOWNA);
    }
}

void NativeOverlay::updateLayeredWindow(int w, int h, double opacity) {
    if (!m_hWnd || !m_currentImage) return;
    
    if (w <= 0 || h <= 0) return;

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
    if (!hBitmap) {
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return;
    }

    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    Bitmap* targetBitmap = new Bitmap(w, h, w * 4, PixelFormat32bppPARGB, (BYTE*)pvBits);
    Graphics* g = Graphics::FromImage(targetBitmap);
    
    g->Clear(Color(0, 0, 0, 0));
    g->SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g->DrawImage(m_currentImage, 0, 0, w, h);

    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = (BYTE)(opacity * 255);
    blend.AlphaFormat = AC_SRC_ALPHA;

    POINT pptSrc = { 0, 0 };
    SIZE sizeWnd = { w, h };

    UpdateLayeredWindow(m_hWnd, hdcScreen, NULL, &sizeWnd, hdcMem, &pptSrc, 0, &blend, ULW_ALPHA);

    delete g;
    delete targetBitmap;
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

void NativeOverlay::setPosition(int x, int y, int w, int h, HWND hInsertAfter) {
    if (!m_hWnd) return;
    UINT flags = SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSENDCHANGING; 
    
    if (hInsertAfter == NULL) {
        flags |= SWP_NOZORDER;
    }
    
    SetWindowPos(m_hWnd, hInsertAfter, x, y, w, h, flags);
}

void NativeOverlay::show() {
    ShowWindow(m_hWnd, SW_SHOWNA);
}

void NativeOverlay::hide() {
    ShowWindow(m_hWnd, SW_HIDE);
}

bool NativeOverlay::isVisible() const {
    return IsWindowVisible(m_hWnd);
}
