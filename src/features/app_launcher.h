#pragma once

#include "feature_base.h"
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QSet>
#include <QRegularExpression>

class AppLauncher final : public FeatureBase {
public:
    AppLauncher();
    [[nodiscard]] QString getName() const override { return "Applications"; }
    [[nodiscard]] QString getIcon() const override { return "applications-system"; }
    QList<FeatureItem> search(const QString& query) override;
    void execute(const FeatureItem& item) override;

private:
    void loadApplications();
    void parseDesktopFile(const QString& filePath);

    static int fuzzyMatch(const QString& query, const QString& text);

    QList<FeatureItem> m_applications;
    QSet<QString> m_seenApps; // prevent duplicates
};