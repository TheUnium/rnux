#pragma once

#include "feature_base.h"
#include <QRegularExpression>

class Calculator final : public FeatureBase {
public:
    [[nodiscard]] QString getName() const override { return "Calculator"; }
    [[nodiscard]] QString getIcon() const override { return "accessories-calculator"; }
    QList<FeatureItem> search(const QString& query) override;
    void execute(const FeatureItem& item) override;

private:
    static double evaluateExpression(const QString& expr);
    static bool isValidExpression(const QString& expr);
};