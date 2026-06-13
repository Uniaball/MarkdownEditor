#pragma once
#include <QPlainTextEdit>

class MarkdownHighlighter;

class MarkdownEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit MarkdownEditor(QWidget *parent = nullptr);

    bool isUnorderedListActive() const { return m_unorderedActive; }
    bool isOrderedListActive() const { return m_orderedActive; }

signals:
    void contentChanged();
    void unorderedListModeChanged(bool active);
    void orderedListModeChanged(bool active);
    void fontSizeChanged(int size);
    void scrollPixelChanged(int pixelValue);

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

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void wrapSelection(const QString &prefix, const QString &suffix, const QString &placeholder = {});
    void prependToLine(const QString &prefix);
    void applyListNewline();

    MarkdownHighlighter *m_highlighter = nullptr;
    bool m_unorderedActive = false;
    bool m_orderedActive = false;

    bool m_dragging = false;
    bool m_hasDragConsent = false;
    QPoint m_dragStartPos;
};
