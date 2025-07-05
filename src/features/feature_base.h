#pragma once

#include <QIcon>
#include <QList>
#include <utility>

struct FeatureItem {
    QString title;
    QString subtitle;
    QString icon;
    QString data;
    QString type;

    FeatureItem() = default;
    FeatureItem(QString  t, QString  s, QString  i, QString  d, QString  type)
        : title(std::move(t)), subtitle(std::move(s)), icon(std::move(i)), data(std::move(d)), type(std::move(type)) {}

    bool operator==(const FeatureItem& other) const {
        return title == other.title &&
               subtitle == other.subtitle &&
               icon == other.icon &&
               data == other.data &&
               type == other.type;
    }
};

class FeatureBase {
public:
    virtual ~FeatureBase() = default;
    [[nodiscard]] virtual QString getName() const = 0;
    [[nodiscard]] virtual QString getIcon() const = 0;
    virtual QList<FeatureItem> search(const QString& query) = 0;
    virtual void execute(const FeatureItem& item) = 0;
    [[nodiscard]] virtual bool isEnabled() const { return true; }
};