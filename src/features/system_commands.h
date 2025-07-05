#pragma once

#include "feature_base.h"
#include <QProcess>

class SystemCommands final : public FeatureBase {
public:
    SystemCommands();
    [[nodiscard]] QString getName() const override { return "System"; }
    [[nodiscard]] QString getIcon() const override { return "system-shutdown"; }
    QList<FeatureItem> search(const QString& query) override;
    void execute(const FeatureItem& item) override;

private:
    QList<FeatureItem> m_commands;
    static int fuzzyMatch(const QString& query, const QString& text);
};