#include "globalhotkey.h"
#include <QDebug>
#include <QGuiApplication>

#include <X11/Xlib.h>
#include <xcb/xcb.h>

GlobalHotkey::GlobalHotkey(QObject* parent)
    : QObject(parent)
    , m_registered(false)
    , m_key(0)
    , m_modifiers(0)
    , m_display(nullptr)
    , m_window(0)
{
    if (const auto *x11App = qApp->nativeInterface<QNativeInterface::QX11Application>()) {
        m_display = x11App->display();
        if (m_display) {
            auto* display = static_cast<Display*>(m_display);
            m_window = DefaultRootWindow(display);
            QApplication::instance()->installNativeEventFilter(this);
            qDebug() << "ghk ~ x11 display initd";
        } else {
            qWarning() << "ghk ~ failed to get x11 display :(";
        }
    } else {
        qWarning() << "ghk ~ x11 native interface not available";
    }
}

GlobalHotkey::~GlobalHotkey() {
    unregisterHotkey();
}

bool GlobalHotkey::registerHotkey(const int key, const int modifiers) {
    if (!m_display) {
        qWarning() << "ghk ~ x11 display not available";
        return false;
    }

    unregisterHotkey();
    m_key = key;
    m_modifiers = modifiers;

    const auto display = static_cast<Display*>(m_display);

    const KeyCode keycode = XKeysymToKeycode(display, key);
    if (keycode == 0) {
        qWarning() << "ghk ~ invalid keycode for key :" << key;
        return false;
    }

    unsigned int x11_modifiers = 0;
    if (modifiers & Qt::AltModifier) x11_modifiers |= Mod1Mask;
    if (modifiers & Qt::ControlModifier) x11_modifiers |= ControlMask;
    if (modifiers & Qt::ShiftModifier) x11_modifiers |= ShiftMask;
    if (modifiers & Qt::MetaModifier) x11_modifiers |= Mod4Mask;

    const XErrorHandler oldHandler = XSetErrorHandler([](Display*, XErrorEvent* error) -> int {
        qWarning() << "ghk ~ x11 error when registering hotkey :" << error->error_code;
        return 0;
    });

    XGrabKey(display, keycode, x11_modifiers, m_window, False, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keycode, x11_modifiers | Mod2Mask, m_window, False, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keycode, x11_modifiers | LockMask, m_window, False, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, keycode, x11_modifiers | Mod2Mask | LockMask, m_window, False, GrabModeAsync, GrabModeAsync);

    XSync(display, False);
    XSetErrorHandler(oldHandler);

    m_registered = true;
    qDebug() << "ghk ~ registered hotkey : keycode" << keycode << "modifiers" << x11_modifiers;
    return true;
}

void GlobalHotkey::unregisterHotkey() {
    if (!m_registered || !m_display) return;

    const auto display = static_cast<Display*>(m_display);
    const KeyCode keycode = XKeysymToKeycode(display, m_key);

    unsigned int x11_modifiers = 0;
    if (m_modifiers & Qt::AltModifier) x11_modifiers |= Mod1Mask;
    if (m_modifiers & Qt::ControlModifier) x11_modifiers |= ControlMask;
    if (m_modifiers & Qt::ShiftModifier) x11_modifiers |= ShiftMask;
    if (m_modifiers & Qt::MetaModifier) x11_modifiers |= Mod4Mask;

    XUngrabKey(display, keycode, x11_modifiers, m_window);
    XUngrabKey(display, keycode, x11_modifiers | Mod2Mask, m_window);
    XUngrabKey(display, keycode, x11_modifiers | LockMask, m_window);
    XUngrabKey(display, keycode, x11_modifiers | Mod2Mask | LockMask, m_window);

    XSync(display, False);
    m_registered = false;
    qDebug() << "ghk ~ unregistered hotkey";
}

bool GlobalHotkey::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) {
    Q_UNUSED(result)

    if (eventType == "xcb_generic_event_t") {
        const auto event = static_cast<xcb_generic_event_t*>(message);

        if (const uint8_t response_type = event->response_type & ~0x80; response_type == XCB_KEY_PRESS) {
            const auto keyEvent = reinterpret_cast<xcb_key_press_event_t*>(event);

            if (m_registered && m_display) {
                const auto display = static_cast<Display*>(m_display);
                const KeyCode expectedKeycode = XKeysymToKeycode(display, m_key);

                unsigned int x11_modifiers = 0;
                if (m_modifiers & Qt::AltModifier) x11_modifiers |= Mod1Mask;
                if (m_modifiers & Qt::ControlModifier) x11_modifiers |= ControlMask;
                if (m_modifiers & Qt::ShiftModifier) x11_modifiers |= ShiftMask;
                if (m_modifiers & Qt::MetaModifier) x11_modifiers |= Mod4Mask;

                if (const uint16_t relevantModifiers = keyEvent->state & ~(Mod2Mask | LockMask);
                    keyEvent->detail == expectedKeycode && relevantModifiers == x11_modifiers) {
                    qDebug() << "ghk ~ hotkey pressed";
                    emit hotkeyPressed();
                    return true;
                }
            }
        }
    }
    
    return false;
}