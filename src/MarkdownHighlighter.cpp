#include "MarkdownHighlighter.h"

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    headerFormat.setForeground(Qt::darkBlue);
    headerFormat.setFontWeight(QFont::Bold);

    boldFormat.setFontWeight(QFont::Bold);
    boldFormat.setForeground(Qt::darkGreen);

    italicFormat.setFontItalic(true);
    italicFormat.setForeground(Qt::darkGreen);

    boldItalicFormat.setFontWeight(QFont::Bold);
    boldItalicFormat.setFontItalic(true);
    boldItalicFormat.setForeground(Qt::darkGreen);

    codeFormat.setForeground(Qt::darkRed);
    codeFormat.setBackground(QColor(240, 240, 240));

    codeBlockFormat.setForeground(Qt::black);
    codeBlockFormat.setBackground(QColor(230, 230, 230));

    listFormat.setForeground(Qt::darkMagenta);

    linkFormat.setForeground(Qt::blue);
    linkFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);

    htmlTagFormat.setForeground(QColor(180, 100, 0));
    htmlTagFormat.setFontWeight(QFont::Bold);

    rules.append({QRegularExpression("^#{1,6}\\s+.*$"), headerFormat});
    rules.append({QRegularExpression("\\*\\*\\*.+?\\*\\*\\*"), boldItalicFormat});
    rules.append({QRegularExpression("(?<!\\*)\\*\\*(?!\\*).+?(?<!\\*)\\*\\*(?!\\*)"), boldFormat});
    rules.append({QRegularExpression("(?<!\\*)\\*(?!\\*)([^\\*]+)\\*(?!\\*)"), italicFormat});
    rules.append({QRegularExpression("`[^`]+`"), codeFormat});
    rules.append({QRegularExpression("^[\\s]*[-\\*]\\s"), listFormat});
    rules.append({QRegularExpression("\\[([^\\]]+)\\]\\([^)]+\\)"), linkFormat});
    rules.append({QRegularExpression("</?[a-zA-Z][\\w-]*(?:\\s[^>]*)?>"), htmlTagFormat});
}

void MarkdownHighlighter::highlightBlock(const QString &text)
{
    if (previousBlockState() == 1) {
        setFormat(0, text.length(), codeBlockFormat);
        setCurrentBlockState(1);
        if (text.trimmed().startsWith("```")) {
            setCurrentBlockState(0);
        }
        return;
    }

    if (text.trimmed().startsWith("```")) {
        setFormat(0, text.length(), codeBlockFormat);
        setCurrentBlockState(1);
        return;
    }

    setCurrentBlockState(0);

    for (const auto &rule : std::as_const(rules)) {
        auto matchIt = rule.pattern.globalMatch(text);
        while (matchIt.hasNext()) {
            auto match = matchIt.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}