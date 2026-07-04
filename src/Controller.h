#pragma once

#include <QObject>
#include <QTimer>
#include <windows.h>
#include "NativeOverlay.h"

class Controller : public QObject {
    Q_OBJECT

public:
    explicit Controller(QObject *parent = nullptr);
    ~Controller();

    void start();
    void stop();

public slots:
    void onConfigChanged();
    void updateLoop();

private:
    QTimer* m_timer;
    NativeOverlay* m_overlay;
    
    // State for optimization
    int m_lastX, m_lastY, m_lastW, m_lastH;
    QString m_lastImagePath;
    HWND m_hExplorer;
    
    void checkKeys();
    void updatePosition();
};
