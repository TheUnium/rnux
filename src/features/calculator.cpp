#include "calculator.h"
#include <cmath>
#include <QClipboard>
#include <QApplication>
#include "../third_party/exprtk.hpp"

QList<FeatureItem> Calculator::search(const QString& query) {
    QList<FeatureItem> results;
    if (query.isEmpty() || !isValidExpression(query)) return results;

    if (const double result = evaluateExpression(query); !std::isnan(result) && !std::isinf(result)) {
        const QString resultStr = QString::number(result, 'g', 10);
        results.append(FeatureItem(
            resultStr,
            "Press Enter to copy to clipboard",
            "accessories-calculator",
            resultStr,
            "calculator"
        ));
    }
    return results;
}

void Calculator::execute(const FeatureItem& item) {
    if (QClipboard* clipboard = QApplication::clipboard()) {
        clipboard->setText(item.data);
    }
}

bool Calculator::isValidExpression(const QString& expr) {
    if (expr.trimmed().isEmpty()) return false;

    const QRegularExpression regex("^[0-9+\\-*/().\\s]+$");
    return regex.match(expr).hasMatch() &&
           expr.contains(QRegularExpression("[0-9+\\-*/]"));
}

double Calculator::evaluateExpression(const QString& expr) {
    typedef double T;
    exprtk::symbol_table<T> symbol_table;
    exprtk::expression<T> expression;
    exprtk::parser<T> parser;

    symbol_table.add_constants();
    expression.register_symbol_table(symbol_table);

    if (const std::string e = expr.toStdString(); !parser.compile(e, expression)) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    try {
        return expression.value();
    } catch (...) {
        return std::numeric_limits<double>::quiet_NaN();
    }
}