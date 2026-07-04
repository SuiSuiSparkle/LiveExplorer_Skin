#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QStyle>
#include <QMessageBox>
#include <QFile>
#include "Controller.h"
#include "SettingsDialog.h"
#include "Config.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    // Set Application Icon
    QString iconPath = QCoreApplication::applicationDirPath() + "/icon.ico";
    QIcon appIcon;
    if (QFile::exists(iconPath)) {
        appIcon = QIcon(iconPath);
    } else {
        appIcon = a.style()->standardIcon(QStyle::SP_DesktopIcon);
    }
    a.setWindowIcon(appIcon);

    // Load configuration
    Config::instance().load();

    // Start controller
    Controller controller;
    controller.start();

    // Setup Tray Icon
    QSystemTrayIcon trayIcon;
    trayIcon.setIcon(appIcon);
    trayIcon.setToolTip("LiveExplorer Skin");

    QMenu* trayMenu = new QMenu();
    QAction* settingsAction = trayMenu->addAction("Settings");
    QAction* aboutAction = trayMenu->addAction("About");
    trayMenu->addSeparator();
    QAction* quitAction = trayMenu->addAction("Exit");

    trayIcon.setContextMenu(trayMenu);
    trayIcon.show();

    // Connect actions
    QObject::connect(quitAction, &QAction::triggered, &a, &QApplication::quit);

    QObject::connect(settingsAction, &QAction::triggered, [&]() {
        SettingsDialog dlg;
        if (dlg.exec() == QDialog::Accepted) {
            // Settings saved automatically by dlg
        }
    });

    QObject::connect(aboutAction, &QAction::triggered, [&]() {
        QMessageBox::about(nullptr, "About", 
            "LiveExplorer Skin\n\n"
            "An overlay tool for Windows File Explorer.\n"
            "Supports multiple images, transparency, and scaling.\n\n"
            "Use the tray icon to configure it.");
    });
    
    // Allow double click to open settings
    QObject::connect(&trayIcon, &QSystemTrayIcon::activated, [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            settingsAction->trigger();
        }
    });

    return a.exec();
}
