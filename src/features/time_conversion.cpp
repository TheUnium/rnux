#include "time_conversion.h"
#include <QApplication>
#include <QClipboard>
#include "time.hpp"

Time::Time() {
    m_converter = new timelib::TimeConverter();
}

Time::~Time() {
    delete m_converter;
}

QList<FeatureItem> Time::search(const QString& query) {
    QList<FeatureItem> results;
    if (query.trimmed().length() < 5) {
        return results;
    }

    const auto parsed_query = m_converter->parseInput(query.toStdString());

    if (const auto res = timelib::TimeConverter::processQuery(parsed_query); res.code == timelib::ErrorCode::Success) {
        const QString resultString = QString::fromStdString(res.result);
        results.append(FeatureItem(
            resultString,
            "Press Enter to copy",
            "accessories-clock",
            resultString,
            "time"
        ));
    }

    return results;
}

void Time::execute(const FeatureItem& item) {
    if (QClipboard* clipboard = QApplication::clipboard()) {
        clipboard->setText(item.data);
    }
}