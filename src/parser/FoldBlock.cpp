#include "FoldBlock.h"
#include "InlineProc.h"
#include "CodeHighlight.h"
#include <QRegularExpression>

namespace FoldBlock {

static int foldIdCounter = 0;

void resetCounter() { foldIdCounter = 0; }

QString convertDetailsBlock(const QStringList &lines, const QSet<QString> &expandedIds) {
    int currentId = foldIdCounter++;
    QString id = QString::number(currentId);
    bool isExpanded = expandedIds.contains(id);
    bool inSummary = false;
    QString summaryText = "详情";
    QStringList contentLines;

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed == "<details>" || trimmed.startsWith("<details ") || trimmed == "</details>")
            continue;

        QRegularExpression openRe("<summary[^>]*>");
        QRegularExpression closeRe("</summary>");
        bool hasOpen = openRe.match(trimmed).hasMatch();
        bool hasClose = closeRe.match(trimmed).hasMatch();

        if (hasOpen && !hasClose) {
            inSummary = true;
            QString t = trimmed;
            t.remove(openRe);
            summaryText = t.trimmed();
            continue;
        } else if (hasOpen && hasClose) {
            QString t = trimmed;
            t.remove(openRe);
            t.remove(closeRe);
            summaryText = t.trimmed();
            inSummary = false;
            continue;
        } else if (!hasOpen && hasClose) {
            inSummary = false;
            continue;
        }

        if (inSummary) {
            summaryText += " " + trimmed;
        } else {
            contentLines.append(line);
        }
    }

    QString processedContent;
    bool inCodeBlock = false;
    QString codeLang;
    QString codeBuffer;

    for (const QString &line : contentLines) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith("```")) {
            if (!inCodeBlock) {
                inCodeBlock = true;
                codeLang = trimmed.mid(3).trimmed();
                codeBuffer.clear();
            } else {
                processedContent += "<pre><code>";
                processedContent += CodeHighlight::highlightCode(codeLang, codeBuffer.trimmed());
                processedContent += "</code></pre>\n";
                inCodeBlock = false;
                codeBuffer.clear();
            }
            continue;
        }

        if (inCodeBlock) {
            if (!codeBuffer.isEmpty())
                codeBuffer += '\n';
            codeBuffer += line;
            continue;
        }

        QString processed = InlineProc::processInline(trimmed);
        if (!processed.isEmpty())
            processedContent += processed + "<br>\n";
    }

    if (inCodeBlock) {
        processedContent += "<pre><code>";
        processedContent += CodeHighlight::highlightCode(codeLang, codeBuffer.trimmed());
        processedContent += "</code></pre>\n";
    }

    QString arrow = isExpanded ? "▾" : "▸";
    QString link = QString("fold:%1").arg(id);

    QString html;
    html += QStringLiteral("<div class=\"fold\">\n");
    html += QStringLiteral("<a href=\"%1\" style=\"text-decoration:none; color:#2c3e50; font-weight:bold;\">%2 %3</a>\n")
                .arg(link, arrow, summaryText);
    if (isExpanded) {
        html += QStringLiteral("<div class=\"fold-content\">\n%1\n</div>\n").arg(processedContent.trimmed());
    }
    html += QStringLiteral("</div>\n");
    return html;
}

} // namespace FoldBlock