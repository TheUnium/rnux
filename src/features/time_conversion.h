#pragma once

#include "feature_base.h"

namespace timelib {
    class TimeConverter;
}

class Time final : public FeatureBase {
public:
    Time();
    ~Time() override;

    [[nodiscard]] QString getName() const override { return "Time"; }
    [[nodiscard]] QString getIcon() const override { return "accessories-clock"; }
    QList<FeatureItem> search(const QString& query) override;
    void execute(const FeatureItem& item) override;

private:
    timelib::TimeConverter* m_converter;
};