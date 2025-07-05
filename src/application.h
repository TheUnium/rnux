#pragma once

#include <QString>
#include <QIcon>

struct Application {
    QString name;
    QString exec;
    QString icon;
    QString comment;

    Application() = default;
    Application(const QString& n, const QString& e, const QString& i = "", const QString& c = "")
        : name(n), exec(e), icon(i), comment(c) {}
};