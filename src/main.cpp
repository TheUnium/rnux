#include <QApplication>
#include <QDir>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QDebug>
#include <X11/keysym.h>
#include "mainwindow.h"
#include "globalhotkey.h"

int main(int argc, char *argv[]) {
    const QApplication app(argc, argv);

    app.setApplicationName("rnux");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("unium");
    app.setOrganizationDomain("unium.in");

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, "rnux", "system tray is not available on this system. how?");
        return -1;
    }

    app.setQuitOnLastWindowClosed(false);

    MainWindow window;
    QSystemTrayIcon trayIcon;
    trayIcon.setIcon(QIcon(":/icon.png"));
    trayIcon.setToolTip("[rnux] Press Alt+Space to activate");

    QMenu trayMenu;
    const QAction* showAction = trayMenu.addAction("Show rnux");
    const QAction* quitAction = trayMenu.addAction("Quit");

    trayIcon.setContextMenu(&trayMenu);
    trayIcon.show();

    // setup ghk
    GlobalHotkey hotkey;
    if (const bool hotkeyRegistered = hotkey.registerHotkey(XK_space, Qt::AltModifier); !hotkeyRegistered) {
        qWarning() << "ghk ~ failed to register global hotkey alt+space";
        QMessageBox::warning(nullptr, "rnux", "Failed to register global hotkey Alt+Space.\nYou can still use the system tray to access rnux.");
    } else {
        qDebug() << "ghk ~ hotkey alt+apace registered successfully";
    }

    QObject::connect(&hotkey, &GlobalHotkey::hotkeyPressed, [&window]() {
        if (window.isVisible()) {
            window.hide();
        } else {
            window.show();
            window.raise();
            window.activateWindow();
        }
    });

    QObject::connect(showAction, &QAction::triggered, [&window]() {
        window.show();
        window.raise();
        window.activateWindow();
    });

    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);
    QObject::connect(&trayIcon, &QSystemTrayIcon::activated, [&window](const QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            if (window.isVisible()) {
                window.hide();
            } else {
                window.show();
                window.raise();
                window.activateWindow();
            }
        }
    });

    qDebug() << "rnux started, press alt+space to activate or use the system tray";
    return app.exec();
}