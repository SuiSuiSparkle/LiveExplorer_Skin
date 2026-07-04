#include "Config.h"
#include <QCoreApplication>
#include <QDir>

Config& Config::instance() {
    static Config _instance;
    return _instance;
}

void Config::load() {
    QSettings settings("LiveExplorer_Skin", "Config");
    imagePaths = settings.value("imagePaths").toStringList();
    currentImageIndex = settings.value("currentImageIndex", 0).toInt();
    opacity = settings.value("opacity", 0.6).toDouble();
    scale = settings.value("scale", 1.0).toDouble();
    keyPrev = settings.value("keyPrev", 0x25).toInt();
    keyNext = settings.value("keyNext", 0x27).toInt();
    keyModifiers = settings.value("keyModifiers", 0x10).toInt();

    startupSound = settings.value("startupSound", "").toString();
    switchSound = settings.value("switchSound", "").toString();

    // Default image if empty
    if (imagePaths.isEmpty()) {
        QString defaultPath = QCoreApplication::applicationDirPath() + "/image/source.png";
        if (QFile::exists(defaultPath)) {
            imagePaths << defaultPath;
        }
    }
}

void Config::save() {
    QSettings settings("LiveExplorer_Skin", "Config");
    settings.setValue("imagePaths", imagePaths);
    settings.setValue("currentImageIndex", currentImageIndex);
    settings.setValue("opacity", opacity);
    settings.setValue("scale", scale);
    settings.setValue("keyPrev", keyPrev);
    settings.setValue("keyNext", keyNext);
    settings.setValue("keyModifiers", keyModifiers);
    settings.setValue("startupSound", startupSound);
    settings.setValue("switchSound", switchSound);
    emit configChanged();
}

QString Config::currentImage() const {
    if (imagePaths.isEmpty()) return QString();
    if (currentImageIndex < 0 || currentImageIndex >= imagePaths.size()) return imagePaths.first();
    return imagePaths[currentImageIndex];
}

void Config::nextImage() {
    if (imagePaths.isEmpty()) return;
    currentImageIndex++;
    if (currentImageIndex >= imagePaths.size()) currentImageIndex = 0;
    save();
}

void Config::prevImage() {
    if (imagePaths.isEmpty()) return;
    currentImageIndex--;
    if (currentImageIndex < 0) currentImageIndex = imagePaths.size() - 1;
    save();
}
