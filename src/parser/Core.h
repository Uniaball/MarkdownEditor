#pragma once
#include <QString>
#include <expected>
#include <QSet>

namespace ParserCore {
    std::expected<QString, QString> parseMarkdown(const QString &markdown, const QSet<QString> &expandedIds);
}