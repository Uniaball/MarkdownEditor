#include "MarkdownParser.h"
#include "Core.h"

std::expected<QString, QString> MarkdownParser::toHtml(const QString &markdown,
                                                       const QSet<QString> &expandedIds) {
    return ParserCore::parseMarkdown(markdown, expandedIds);
}