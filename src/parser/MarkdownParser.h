#pragma once
#include <QString>
#include <expected>
#include <QSet>

class MarkdownParser {
public:
    static std::expected<QString, QString> toHtml(const QString &markdown,
                                                  const QSet<QString> &expandedIds = {});
};