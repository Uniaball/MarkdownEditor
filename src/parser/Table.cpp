#include "Table.h"
#include "InlineProc.h"
#include <QRegularExpression>

namespace TableParser {

QString convertTable(const QStringList &lines) {
    if (lines.size() < 2) return {};

    QRegularExpression splitRe(R"(\s*\|\s*)");
    QRegularExpression alignRe(R"(^:?-+:?$)");
    QVector<Qt::Alignment> alignments;

    // 第一行：表头
    QStringList headers = lines[0].trimmed().split(splitRe, Qt::SkipEmptyParts);
    for (QString &h : headers) h = InlineProc::processInline(h.trimmed());

    // 第二行：对齐分隔行
    QStringList sepCells = lines[1].trimmed().split(splitRe, Qt::SkipEmptyParts);
    if (sepCells.size() != headers.size()) return {};
    for (const QString &cell : sepCells) {
        QString c = cell.trimmed();
        if (!alignRe.match(c).hasMatch()) return {};
        if (c.startsWith(':') && c.endsWith(':')) alignments.append(Qt::AlignCenter);
        else if (c.endsWith(':')) alignments.append(Qt::AlignRight);
        else alignments.append(Qt::AlignLeft);
    }

    QString html;
    html += "<table>\n<thead>\n<tr>\n";
    for (int i = 0; i < headers.size(); ++i) {
        QString style;
        if (i < alignments.size()) {
            if (alignments[i] == Qt::AlignCenter) style = " style=\"text-align:center\"";
            else if (alignments[i] == Qt::AlignRight) style = " style=\"text-align:right\"";
        }
        html += QStringLiteral("<th%1>%2</th>\n").arg(style, headers[i]);
    }
    html += "</tr>\n</thead>\n<tbody>\n";

    // 数据行（从第三行开始）
    for (int li = 2; li < lines.size(); ++li) {
        QStringList cells = lines[li].trimmed().split(splitRe, Qt::SkipEmptyParts);
        html += "<tr>\n";
        for (int i = 0; i < qMin(cells.size(), headers.size()); ++i) {
            QString cellHtml = InlineProc::processInline(cells[i].trimmed());
            QString style;
            if (i < alignments.size()) {
                if (alignments[i] == Qt::AlignCenter) style = " style=\"text-align:center\"";
                else if (alignments[i] == Qt::AlignRight) style = " style=\"text-align:right\"";
            }
            html += QStringLiteral("<td%1>%2</td>\n").arg(style, cellHtml);
        }
        html += "</tr>\n";
    }
    html += "</tbody>\n</table>\n";
    return html;
}

} // namespace TableParser