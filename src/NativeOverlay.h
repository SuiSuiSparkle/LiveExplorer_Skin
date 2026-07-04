#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <QImage>
#include <QSize>
#include <QString>

using namespace Gdiplus;

class NativeOverlay {
public:
    NativeOverlay();
    ~NativeOverlay();

    bool init(HINSTANCE hInstance);
    
    // Loads image if changed, returns size. If same path, returns cached size.
    QSize loadImage(const QString& imagePath);
    
    // Updates the window content and position
    void update(int w, int h, double opacity);
    
    // Updates only position/z-order (faster)
    void setPosition(int x, int y, int w, int h, HWND hInsertAfter);

    void show();
    void hide();
    bool isVisible() const;
    HWND getHandle() const { return m_hWnd; }

private:
    HWND m_hWnd;
    ULONG_PTR m_gdiplusToken;
    Image* m_currentImage;
    QString m_lastPath;
    
    int m_lastDrawW, m_lastDrawH;
    double m_lastDrawOpacity;
    
    void updateLayeredWindow(int w, int h, double opacity);
};
