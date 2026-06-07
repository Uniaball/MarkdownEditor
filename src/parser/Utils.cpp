#include "Utils.h"

namespace ParserUtils {

QString escapeHtml(const QString &s) {
    QString result = s;
    result.replace('&', "&amp;");
    result.replace('<', "&lt;");
    result.replace('>', "&gt;");
    return result;
}

bool isInsideTag(const QString &text, int pos) {
    int openCount = 0;
    int i = 0;
    while (i < pos) {
        if (text.mid(i, 5) == "<span") {
            ++openCount;
            i += 5;
        } else if (text.mid(i, 7) == "</span>") {
            if (openCount > 0) --openCount;
            i += 7;
        } else {
            ++i;
        }
    }
    return openCount > 0;
}

QString safeReplaceOutsideSpans(const QString &input, const QRegularExpression &re,
                                std::function<QString(const QRegularExpressionMatch &)> callback) {
    QString result;
    int lastPos = 0;
    auto it = re.globalMatch(input);
    while (it.hasNext()) {
        auto match = it.next();
        int start = match.capturedStart();
        if (!isInsideTag(input, start)) {
            result += input.mid(lastPos, start - lastPos);
            result += callback(match);
            lastPos = match.capturedEnd();
        }
    }
    result += input.mid(lastPos);
    return result;
}

QString safeReplaceOutsideSpans(const QString &input, const QRegularExpression &re,
                                const QString &replacement) {
    return safeReplaceOutsideSpans(input, re, [&](const QRegularExpressionMatch &m) -> QString {
        QString rep = replacement;
        for (int i = 1; i <= 9; ++i) {
            rep.replace(QString("\\%1").arg(i), m.captured(i));
        }
        return rep;
    });
}

} // namespace ParserUtils