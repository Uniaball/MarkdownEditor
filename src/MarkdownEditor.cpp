#include "MarkdownEditor.h"
#include "MarkdownHighlighter.h"
#include <QTextCursor>
#include <QRegularExpression>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QApplication>
#include <QTouchEvent>
#include <QScrollBar>

MarkdownEditor::MarkdownEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    m_highlighter = new MarkdownHighlighter(document());
    connect(this, &QPlainTextEdit::textChanged, this, &MarkdownEditor::contentChanged);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setFocusPolicy(Qt::ClickFocus);
    setAttribute(Qt::WA_InputMethodEnabled, true);
    viewport()->installEventFilter(this);
    viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
}

bool MarkdownEditor::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != viewport())
        return QPlainTextEdit::eventFilter(watched, event);

    switch (event->type()) {
    case QEvent::TouchBegin: {
        auto *te = static_cast<QTouchEvent *>(event);
        if (te->points().size() == 1) {
            m_touchDragging = true;
            m_touchLastPos = te->points().first().position();
            QPoint pos = m_touchLastPos.toPoint();
            QTextCursor cur = cursorForPosition(pos);
            setTextCursor(cur);
            return true;
        }
        break;
    }
    case QEvent::TouchUpdate: {
        auto *te = static_cast<QTouchEvent *>(event);
        if (!m_touchDragging || te->points().size() != 1)
            break;
        QPointF now = te->points().first().position();
        qreal dy = m_touchLastPos.y() - now.y();
        m_touchLastPos = now;
        QScrollBar *v = verticalScrollBar();
        if (v && v->maximum() > v->minimum()) {
            int newVal = v->value() + int(qRound(dy));
            newVal = qBound(v->minimum(), newVal, v->maximum());
            v->setValue(newVal);
            emit scrollPixelChanged(newVal);
        }
        return true;
    }
    case QEvent::TouchEnd:
    case QEvent::TouchCancel: {
        m_touchDragging = false;
        return true;
    }
    default:
        break;
    }
    return QPlainTextEdit::eventFilter(watched, event);
}

void MarkdownEditor::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (m_unorderedActive || m_orderedActive) {
            applyListNewline();
            return;
        }
    }
    QPlainTextEdit::keyPressEvent(event);
}

void MarkdownEditor::applyListNewline()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
    QString lineText = cursor.selectedText();

    QRegularExpression ulRe("^\\s*(-|\\*)\\s+(.*)$");
    QRegularExpression olRe("^\\s*(\\d+)\\.\\s+(.*)$");
    auto ulMatch = ulRe.match(lineText);
    auto olMatch = olRe.match(lineText);

    bool isUlLine = ulMatch.hasMatch();
    bool isOlLine = olMatch.hasMatch();

    if (m_unorderedActive && isUlLine && ulMatch.captured(2).isEmpty()) {
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        cursor.insertText("");
        m_unorderedActive = false;
        emit unorderedListModeChanged(false);
        return;
    }
    if (m_orderedActive && isOlLine && olMatch.captured(2).isEmpty()) {
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        cursor.insertText("");
        m_orderedActive = false;
        emit orderedListModeChanged(false);
        return;
    }

    QString newMarker;
    if (m_unorderedActive) {
        if (!isUlLine) {
            QPlainTextEdit::keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier));
            return;
        }
        newMarker = "- ";
    } else if (m_orderedActive) {
        if (!isOlLine) {
            QPlainTextEdit::keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier));
            return;
        }
        int num = olMatch.captured(1).toInt() + 1;
        newMarker = QString::number(num) + ". ";
    }

    cursor.movePosition(QTextCursor::EndOfLine);
    cursor.insertText("\n" + newMarker);
    setTextCursor(cursor);
}

void MarkdownEditor::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        const int delta = event->angleDelta().y();
        if (delta != 0) {
            QFont currentFont = font();
            int currentSize = currentFont.pointSize();
            if (currentSize == -1) currentSize = QApplication::font().pointSize();
            int newSize = currentSize + (delta > 0 ? 1 : -1);
            newSize = qBound(6, newSize, 48);
            currentFont.setPointSize(newSize);
            setFont(currentFont);
            emit fontSizeChanged(newSize);
            event->accept();
            return;
        }
    }
    QPlainTextEdit::wheelEvent(event);
}

void MarkdownEditor::wrapSelection(const QString &prefix, const QString &suffix, const QString &placeholder)
{
    QTextCursor cursor = textCursor();
    QString selected = cursor.selectedText();

    if (selected.isEmpty()) {
        QString insertText = prefix + placeholder + suffix;
        cursor.insertText(insertText);
        if (!placeholder.isEmpty()) {
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, suffix.length() + placeholder.length());
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, placeholder.length());
            setTextCursor(cursor);
        }
    } else {
        cursor.insertText(prefix + selected + suffix);
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, suffix.length());
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, selected.length() + prefix.length());
        setTextCursor(cursor);
    }
}

void MarkdownEditor::prependToLine(const QString &prefix)
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.insertText(prefix);
}

void MarkdownEditor::boldText() { wrapSelection("**", "**", "粗体文本"); }
void MarkdownEditor::italicText() { wrapSelection("*", "*", "斜体文本"); }
void MarkdownEditor::inlineCode() { wrapSelection("`", "`", "代码"); }

void MarkdownEditor::codeBlock()
{
    QTextCursor cursor = textCursor();
    if (!cursor.selectedText().isEmpty()) {
        cursor.insertText(QString("```\n%1\n```").arg(cursor.selectedText()));
    } else {
        cursor.insertText("```\n\n```");
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 4);
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
        setTextCursor(cursor);
    }
}

void MarkdownEditor::header(int level)
{
    QString prefix = QString("#").repeated(level) + " ";
    QTextCursor cursor = textCursor();
    if (cursor.selectedText().isEmpty()) {
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.insertText(prefix);
    } else {
        cursor.insertText(prefix + cursor.selectedText());
    }
}

void MarkdownEditor::toggleUnorderedList()
{
    m_unorderedActive = !m_unorderedActive;
    if (m_unorderedActive) {
        m_orderedActive = false;
        emit orderedListModeChanged(false);
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfLine);
        QString line = cursor.block().text();
        if (!line.trimmed().startsWith("- ") && !line.trimmed().startsWith("* ")) {
            cursor.insertText("- ");
            setTextCursor(cursor);
        }
    }
    emit unorderedListModeChanged(m_unorderedActive);
}

void MarkdownEditor::toggleOrderedList()
{
    m_orderedActive = !m_orderedActive;
    if (m_orderedActive) {
        m_unorderedActive = false;
        emit unorderedListModeChanged(false);
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfLine);
        QString line = cursor.block().text();
        QRegularExpression re("^\\s*(\\d+)\\.\\s");
        if (!re.match(line).hasMatch()) {
            cursor.insertText("1. ");
            setTextCursor(cursor);
        }
    }
    emit orderedListModeChanged(m_orderedActive);
}

void MarkdownEditor::insertLink()
{
    QTextCursor cursor = textCursor();
    if (cursor.selectedText().isEmpty())
        cursor.insertText("[链接文字](url)");
    else
        cursor.insertText(QString("[%1](url)").arg(cursor.selectedText()));
}

void MarkdownEditor::insertImage()
{
    QTextCursor cursor = textCursor();
    if (cursor.selectedText().isEmpty())
        cursor.insertText("![替代文字](图片链接)");
    else
        cursor.insertText(QString("![%1](图片链接)").arg(cursor.selectedText()));
}

void MarkdownEditor::detailsBlock()
{
    QTextCursor cursor = textCursor();
    QString templateText = "<details>\n<summary>标题</summary>\n\n内容\n\n</details>";
    cursor.insertText(templateText);
    cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, templateText.length());
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 9);
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
    setTextCursor(cursor);
}

void MarkdownEditor::insertTable()
{
    QTextCursor cursor = textCursor();
    cursor.insertText(
        "| 标题1 | 标题2 | 标题3 |\n"
        "|-------|-------|-------|\n"
        "| 内容1 | 内容2 | 内容3 |\n"
    );
}
