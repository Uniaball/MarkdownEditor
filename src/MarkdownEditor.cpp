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

    viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
    viewport()->installEventFilter(this);
}

bool MarkdownEditor::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != viewport())
        return QPlainTextEdit::eventFilter(watched, event);

    switch (event->type()) {

    case QEvent::TouchBegin: {
        auto *te = static_cast<QTouchEvent *>(event);
        if (te->points().size() == 1) {
            m_touchStartPos  = te->points().first().position();
            m_touchLastPos   = m_touchStartPos;
            m_touchActive    = true;
            m_touchDidDrag   = false;
        } else {
            m_touchActive = false;
        }
        return false;
    }

    case QEvent::TouchUpdate: {
        if (!m_touchActive)
            return false;

        auto *te = static_cast<QTouchEvent *>(event);
        if (te->points().size() != 1) {
            m_touchActive = false;
            return false;
        }

        QPointF now = te->points().first().position();

        if (!m_touchDidDrag) {
            int th = QApplication::startDragDistance() * 2;
            qreal dx = qAbs(now.x() - m_touchStartPos.x());
            qreal dy = qAbs(now.y() - m_touchStartPos.y());
            if (dx < th && dy < th) {
                return false;
            }

            m_touchDidDrag = true;
            m_touchLastPos = now;

            QTextCursor cur = textCursor();
            if (cur.hasSelection())
                cur.clearSelection();
            setTextCursor(cur);

            QScrollBar *v = verticalScrollBar();
            v->setValue(v->value() + qRound(m_touchLastPos.y() - now.y()));
            emit scrollPixelChanged(v->value());
            return true;
        }

        qreal dy = m_touchLastPos.y() - now.y();
        m_touchLastPos = now;

        QScrollBar *v = verticalScrollBar();
        if (v && v->maximum() > v->minimum()) {
            int val = qBound(v->minimum(),
                             v->value() + qRound(dy),
                             v->maximum());
            v->setValue(val);
            emit scrollPixelChanged(val);
        }
        return true;
    }

    case QEvent::TouchEnd:
    case QEvent::TouchCancel: {
        bool ate = m_touchDidDrag;
        m_touchActive  = false;
        m_touchDidDrag = false;
        return ate;
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
            QFont f = font();
            int s = f.pointSize();
            if (s == -1) s = QApplication::font().pointSize();
            s = qBound(6, s + (delta > 0 ? 1 : -1), 48);
            f.setPointSize(s);
            setFont(f);
            emit fontSizeChanged(s);
            event->accept();
            return;
        }
    }
    QPlainTextEdit::wheelEvent(event);
}

void MarkdownEditor::wrapSelection(const QString &prefix, const QString &suffix, const QString &placeholder)
{
    QTextCursor cursor = textCursor();
    QString sel = cursor.selectedText();
    if (sel.isEmpty()) {
        QString t = prefix + placeholder + suffix;
        cursor.insertText(t);
        if (!placeholder.isEmpty()) {
            cursor.movePosition(QTextCursor::Left,  QTextCursor::MoveAnchor, suffix.length() + placeholder.length());
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, placeholder.length());
            setTextCursor(cursor);
        }
    } else {
        cursor.insertText(prefix + sel + suffix);
        cursor.movePosition(QTextCursor::Left,  QTextCursor::MoveAnchor, suffix.length());
        cursor.movePosition(QTextCursor::Left,  QTextCursor::KeepAnchor, sel.length() + prefix.length());
        setTextCursor(cursor);
    }
}

void MarkdownEditor::prependToLine(const QString &prefix)
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.insertText(prefix);
}

void MarkdownEditor::boldText()      { wrapSelection("**","**","粗体文本"); }
void MarkdownEditor::italicText()     { wrapSelection("*","*","斜体文本"); }
void MarkdownEditor::inlineCode()     { wrapSelection("`","`","代码"); }

void MarkdownEditor::codeBlock()
{
    QTextCursor cursor = textCursor();
    if (!cursor.selectedText().isEmpty())
        cursor.insertText(QString("```\n%1\n```").arg(cursor.selectedText()));
    else {
        cursor.insertText("```\n\n```");
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 4);
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
        setTextCursor(cursor);
    }
}

void MarkdownEditor::header(int level)
{
    QString p = QString("#").repeated(level) + " ";
    QTextCursor c = textCursor();
    if (c.selectedText().isEmpty()) {
        c.movePosition(QTextCursor::StartOfLine);
        c.insertText(p);
    } else {
        c.insertText(p + c.selectedText());
    }
}

void MarkdownEditor::toggleUnorderedList()
{
    m_unorderedActive = !m_unorderedActive;
    if (m_unorderedActive) {
        m_orderedActive = false;
        emit orderedListModeChanged(false);
        QTextCursor c = textCursor();
        c.movePosition(QTextCursor::StartOfLine);
        QString ln = c.block().text();
        if (!ln.trimmed().startsWith("- ") && !ln.trimmed().startsWith("* "))
            c.insertText("- ");
        setTextCursor(c);
    }
    emit unorderedListModeChanged(m_unorderedActive);
}

void MarkdownEditor::toggleOrderedList()
{
    m_orderedActive = !m_orderedActive;
    if (m_orderedActive) {
        m_unorderedActive = false;
        emit unorderedListModeChanged(false);
        QTextCursor c = textCursor();
        c.movePosition(QTextCursor::StartOfLine);
        QString ln = c.block().text();
        if (!QRegularExpression("^\\s*(\\d+)\\.\\s").match(ln).hasMatch())
            c.insertText("1. ");
        setTextCursor(c);
    }
    emit orderedListModeChanged(m_orderedActive);
}

void MarkdownEditor::insertLink()
{
    QTextCursor c = textCursor();
    if (c.selectedText().isEmpty()) c.insertText("[链接文字](url)");
    else c.insertText(QString("[%1](url)").arg(c.selectedText()));
}

void MarkdownEditor::insertImage()
{
    QTextCursor c = textCursor();
    if (c.selectedText().isEmpty()) c.insertText("![替代文字](图片链接)");
    else c.insertText(QString("![%1](图片链接)").arg(c.selectedText()));
}

void MarkdownEditor::detailsBlock()
{
    QTextCursor c = textCursor();
    QString tpl = "<details>\n<summary>标题</summary>\n\n内容\n\n</details>";
    c.insertText(tpl);
    c.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, tpl.length());
    c.movePosition(QTextCursor::Right,QTextCursor::MoveAnchor, 9);
    c.movePosition(QTextCursor::Right,QTextCursor::KeepAnchor, 2);
    setTextCursor(c);
}

void MarkdownEditor::insertTable()
{
    QTextCursor c = textCursor();
    c.insertText(
        "| 标题1 | 标题2 | 标题3 |\n"
        "|-------|-------|-------|\n"
        "| 内容1 | 内容2 | 内容3 |\n"
    );
}
