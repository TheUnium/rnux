#pragma once

#include <QAbstractNativeEventFilter>
#include <QApplication>

class GlobalHotkey final : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

public:
    explicit GlobalHotkey(QObject* parent = nullptr);
    ~GlobalHotkey() override;
    bool registerHotkey(int key, int modifiers);
    void unregisterHotkey();

protected:
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;
    signals:
        void hotkeyPressed();

private:
    bool m_registered;
    int m_key;
    int m_modifiers;
    void* m_display;
    unsigned long m_window;
};