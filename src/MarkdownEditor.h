#pragma once
#include <QPlainTextEdit>
#include <QSet>

class MarkdownHighlighter;

class MarkdownEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit MarkdownEditor(QWidget *parent = nullptr);

    bool isUnorderedListActive() const { return m_unorderedActive; }
    bool isOrderedListActive() const { return m_orderedActive; }

public slots:
    void boldText();
    void italicText();
    void inlineCode();
    void codeBlock();
    void header(int level);
    void toggleUnorderedList();
    void toggleOrderedList();
    void insertLink();
    void insertImage();
    void detailsBlock();
    void insertTable();

signals:
    void contentChanged();
    void unorderedListModeChanged(bool active);
    void orderedListModeChanged(bool active);
    void fontSizeChanged(int size);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool viewportEvent(QEvent *event) override;

private:
    void wrapSelection(const QString &prefix, const QString &suffix, const QString &placeholder = "");
    void prependToLine(const QString &prefix);
    void applyListNewline();

    MarkdownHighlighter *m_highlighter;
    bool m_unorderedActive = false;
    bool m_orderedActive = false;
};