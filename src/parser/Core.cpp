#include "Core.h"
#include "InlineProc.h"
#include "CodeHighlight.h"
#include "FoldBlock.h"
#include "HtmlGen.h"
#include "Table.h"
#include <ranges>
#include <vector>
#include <format>
#include <QRegularExpression>

namespace ParserCore {

static QStringList collectDetailsLines(const std::vector<std::string> &linesVec, size_t &i) {
    QStringList result;
    int depth = 0;
    while (i < linesVec.size()) {
        QString line = QString::fromStdString(linesVec[i]);
        result.append(line);
        QString trimmed = line.trimmed();
        if (trimmed.startsWith("<details>") || trimmed.startsWith("<details ")) depth++;
        if (trimmed == "</details>") {
            depth--;
            if (depth == 0) break;
        }
        i++;
    }
    return result;
}

std::expected<QString, QString> parseMarkdown(const QString &markdown, const QSet<QString> &expandedIds) {
    FoldBlock::resetCounter();

    std::vector<std::string> linesVec;
    auto linesView = markdown.toStdString()
                     | std::views::split('\n')
                     | std::views::transform([](auto &&rng) {
                           return std::string(rng.begin(), rng.end());
                       });
    for (auto &&line : linesView)
        linesVec.push_back(std::move(line));

    QString html;
    bool inCodeBlock = false;
    bool inList = false;
    QString codeLang;
    QString codeBuffer;

    auto closeList = [&]() {
        if (inList) { html += "</ul>\n"; inList = false; }
    };

    auto flushCodeBlock = [&]() {
        if (!codeBuffer.isEmpty()) {
            html += "<pre><code>";
            html += CodeHighlight::highlightCode(codeLang, codeBuffer);
            html += "</code></pre>\n";
            codeBuffer.clear();
            codeLang.clear();
        }
    };

    for (size_t i = 0; i < linesVec.size(); ++i) {
        QString line = QString::fromStdString(linesVec[i]);
        QString trimmed = line.trimmed();

        // details 块
        if (trimmed.startsWith("<details>") || trimmed.startsWith("<details ")) {
            closeList();
            QStringList detailsLines = collectDetailsLines(linesVec, i);
            html += FoldBlock::convertDetailsBlock(detailsLines, expandedIds);
            continue;
        }

        // 表格检测（至少包含两个竖线且非代码块）
        if (trimmed.contains('|') && !inCodeBlock) {
            QStringList tableLines;
            size_t j = i;
            bool hasHeaderSep = false;
            while (j < linesVec.size()) {
                QString l = QString::fromStdString(linesVec[j]).trimmed();
                if (l.startsWith('|') && l.endsWith('|')) {
                    tableLines.append(l);
                    if (l.contains("---")) hasHeaderSep = true;
                    j++;
                } else {
                    break;
                }
            }
            if (hasHeaderSep && tableLines.size() >= 2) {
                closeList();
                QString tableHtml = TableParser::convertTable(tableLines);
                if (!tableHtml.isEmpty()) {
                    html += tableHtml;
                    i = j - 1;
                    continue;
                }
            }
        }

        // 代码块
        if (trimmed.startsWith("```")) {
            if (!inCodeBlock) {
                closeList();
                QString lang = trimmed.mid(3).trimmed();
                codeLang = lang.isEmpty() ? QString() : lang;
                inCodeBlock = true;
            } else {
                flushCodeBlock();
                inCodeBlock = false;
            }
            continue;
        }

        if (inCodeBlock) {
            if (!codeBuffer.isEmpty()) codeBuffer += '\n';
            codeBuffer += line;
            continue;
        }

        if (trimmed.isEmpty()) { closeList(); continue; }

        // 标题
        QRegularExpression headerRe("^(#{1,6})\\s+(.*)$");
        auto headerMatch = headerRe.match(trimmed);
        if (headerMatch.hasMatch()) {
            closeList();
            int level = headerMatch.captured(1).length();
            QString content = InlineProc::processInline(headerMatch.captured(2));
            html += std::format("<h{0}>{1}</h{0}>\n", level, content.toStdString()).c_str();
            continue;
        }

        // 无序列表
        if (trimmed.startsWith("- ") || trimmed.startsWith("* ")) {
            if (!inList) { html += "<ul>\n"; inList = true; }
            QString content = InlineProc::processInline(trimmed.mid(2));
            html += std::format("<li>{}</li>\n", content.toStdString()).c_str();
            continue;
        } else { closeList(); }

        // 段落
        html += std::format("<p>{}</p>\n", InlineProc::processInline(trimmed).toStdString()).c_str();
    }

    closeList();
    if (inCodeBlock) {
        flushCodeBlock();
        html += "<p><em>(未闭合的代码块)</em></p>\n";
        return std::unexpected(QStringLiteral("Unclosed code block found"));
    }

    return HtmlGen::wrapDocument(html);
}

} // namespace ParserCore