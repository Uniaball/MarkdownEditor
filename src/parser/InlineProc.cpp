#include "InlineProc.h"
#include <QRegularExpression>

namespace InlineProc {

QString processInline(const QString &text) {
    QString html = text;
    QRegularExpression imgRe("!\\[([^\\]]*)\\]\\(([^)]+)\\)");
    html.replace(imgRe, R"(<img src="\2" alt="\1">)");
    QRegularExpression linkRe("\\[([^\\]]*)\\]\\(([^)]+)\\)");
    html.replace(linkRe, R"(<a href="\2">\1</a>)");
    QRegularExpression codeRe("`([^`]+)`");
    html.replace(codeRe, R"(<code>\1</code>)");
    QRegularExpression boldRe("\\*\\*(.+?)\\*\\*");
    html.replace(boldRe, R"(<strong>\1</strong>)");
    QRegularExpression italicRe("\\*(.+?)\\*");
    html.replace(italicRe, R"(<em>\1</em>)");
    return html;
}

} // namespace InlineProc