#pragma once

#include <QSettings>
#include <QStringList>
#include <QObject>
#include <QKeySequence>

class Config : public QObject {
    Q_OBJECT

public:
    static Config& instance();

    void load();
    void save();

    QStringList imagePaths;
    int currentImageIndex = 0;
    double opacity = 0.8;
    double scale = 1.0;
    
    // Virtual Key Codes for switching
    int keyPrev = 0x25; // VK_LEFT
    int keyNext = 0x27; // VK_RIGHT
    int keyModifiers = 0x10; // VK_SHIFT

    // Audio settings
    QString startupSound;
    QString switchSound;

    // Helper to get current image path
    QString currentImage() const;

    // Helper to cycle images
    void nextImage();
    void prevImage();

signals:
    void configChanged();

private:
    Config() {}
};
