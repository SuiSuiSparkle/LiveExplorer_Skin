#include "Controller.h"
#include "Config.h"
#include <QDebug>
#include <cmath>
#include <mmsystem.h> // Sound

#pragma comment(lib, "Winmm.lib")

#define IS_PRESSING(key) (GetAsyncKeyState(key) & 0x8000)

Controller::Controller(QObject *parent) : QObject(parent), m_timer(new QTimer(this)), m_overlay(new NativeOverlay()), m_lastX(-9999), m_lastY(-9999), m_lastW(0), m_lastH(0) {
    if (!m_overlay->init(GetModuleHandle(NULL))) {
        qWarning() << "Failed to initialize overlay window";
    }
    
    connect(&Config::instance(), &Config::configChanged, this, &Controller::onConfigChanged);
    connect(m_timer, &QTimer::timeout, this, &Controller::updateLoop);
}

Controller::~Controller() {
    delete m_overlay;
}

void Controller::start() {
    m_timer->start(15); // ~60 FPS
    onConfigChanged(); // Initial update
    
    // Play startup sound
    Config& cfg = Config::instance();
    if (!cfg.startupSound.isEmpty()) {
        PlaySound(cfg.startupSound.toStdWString().c_str(), NULL, SND_FILENAME | SND_ASYNC);
    }
}

void Controller::stop() {
    m_timer->stop();
    m_overlay->hide();
}

void Controller::onConfigChanged() {
    // Reset state to force update
    m_lastW = 0; 
    m_lastH = 0;
    
    // Pre-load image
    Config& cfg = Config::instance();
    QString newImg = cfg.currentImage();
    
    // Play switch sound if image changed and it's not the first load
    if (!m_lastImagePath.isEmpty() && newImg != m_lastImagePath && !cfg.switchSound.isEmpty()) {
        PlaySound(cfg.switchSound.toStdWString().c_str(), NULL, SND_FILENAME | SND_ASYNC);
    }
    m_lastImagePath = newImg;

    m_overlay->loadImage(newImg);
}

void Controller::updateLoop() {
    checkKeys();
    updatePosition();
}

void Controller::checkKeys() {
    static bool shiftPressed = false;
    static bool leftPressed = false;
    static bool rightPressed = false;

    // Check modifiers (VK_SHIFT is 0x10)
    int modifier = Config::instance().keyModifiers; 
    bool modActive = IS_PRESSING(modifier);

    bool left = IS_PRESSING(Config::instance().keyPrev);
    bool right = IS_PRESSING(Config::instance().keyNext);

    if (modActive) {
        if (left && !leftPressed) {
            Config::instance().prevImage();
        }
        if (right && !rightPressed) {
            Config::instance().nextImage();
        }
    }

    leftPressed = left;
    rightPressed = right;
    shiftPressed = modActive;
}

void Controller::updatePosition() {
    Config& cfg = Config::instance();
    QSize imgSize = m_overlay->loadImage(cfg.currentImage());
    
    // If no image, hide and return
    if (imgSize.isEmpty()) {
        if (m_overlay->isVisible()) m_overlay->hide();
        return;
    }

    HWND hExplorer = FindWindowW(L"CabinetWClass", NULL);
    if (!hExplorer) hExplorer = FindWindowW(L"ExploreWClass", NULL);

    if (hExplorer && IsWindowVisible(hExplorer) && !IsIconic(hExplorer)) {
        RECT rect;
        if (GetWindowRect(hExplorer, &rect)) {
            long explorerW = rect.right - rect.left;
            long explorerH = rect.bottom - rect.top;
            long explorerArea = explorerW * explorerH;

            // Target area calculation
            double targetArea = (double)explorerArea / 5.5;
            long origArea = imgSize.width() * imgSize.height();
            
            // Apply scale factor from config (linear multiplier)
            double autoScale = sqrt(targetArea / (double)origArea);
            double finalScale = autoScale * cfg.scale;

            int newW = (int)(imgSize.width() * finalScale);
            int newH = (int)(imgSize.height() * finalScale);
            
            // Constrain minimum size
            if (newW < 1) newW = 1;
            if (newH < 1) newH = 1;

            // Update content if size changed significantly
            if (abs(newW - m_lastW) > 2 || abs(newH - m_lastH) > 2) {
                m_overlay->update(newW, newH, cfg.opacity);
                m_lastW = newW;
                m_lastH = newH;
            }

            // Position calculation
            int offsetX = -20;
            int offsetY = -20;
            int x = rect.right - newW + offsetX;
            int y = rect.bottom - newH + offsetY;

            // Z-Order logic
            HWND hPrev = GetWindow(hExplorer, GW_HWNDPREV);
            bool zOrderChanged = (hPrev != m_overlay->getHandle());
            bool posChanged = (x != m_lastX || y != m_lastY);
            
            if (posChanged || zOrderChanged) {
                 HWND hInsertAfter = hPrev;
                 if (hPrev == m_overlay->getHandle()) {
                     // We are already in place relative to what was above explorer?
                     // Actually GetWindow(hExplorer, GW_HWNDPREV) returns the window ABOVE explorer.
                     // If that is US, then we are good.
                     hInsertAfter = NULL;
                 }
                 // If hPrev is NULL (Explorer is top most?), we should be top most?
                 // Be careful not to fight for top most.
                 // Ideally: Place m_overlay just above hExplorer.
                 // SetWindowPos(m_overlay, hExplorer, ...) places it BEHIND hExplorer? No.
                 // SetWindowPos(hWnd, hWndInsertAfter, ...) places hWnd AFTER hWndInsertAfter in Z order (below it).
                 // So we need to find the window ABOVE explorer, and place ourselves AFTER it (below it, but above explorer).
                 
                 // If hPrev is NULL, Explorer is at the top. We want to be at top.
                 // So hInsertAfter = HWND_TOP.
                 if (hPrev == NULL) hInsertAfter = HWND_TOP;
                 
                 // If hPrev is US, search for the one above us?
                 // No, just don't move Z order.
                 
                 m_overlay->setPosition(x, y, newW, newH, hInsertAfter);
                 m_lastX = x;
                 m_lastY = y;
            }
            
            if (!m_overlay->isVisible()) {
                m_overlay->show();
            }
        }
    } else {
        if (m_overlay->isVisible()) m_overlay->hide();
    }
}
