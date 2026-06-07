#pragma once
#include <QString>
#include <QRegularExpression>
#include <functional>

namespace ParserUtils {
    QString escapeHtml(const QString &s);
    bool isInsideTag(const QString &text, int pos);
    QString safeReplaceOutsideSpans(const QString &input, const QRegularExpression &re,
                                    std::function<QString(const QRegularExpressionMatch &)> callback);
    QString safeReplaceOutsideSpans(const QString &input, const QRegularExpression &re,
                                    const QString &replacement);
}