#pragma once
#include <QToolBar>

class MarkdownEditor;

class FormatToolBar : public QToolBar
{
    Q_OBJECT
public:
    explicit FormatToolBar(MarkdownEditor *editor, QWidget *parent = nullptr);

signals:
    void boldTriggered();
    void italicTriggered();
    void inlineCodeTriggered();
    void codeBlockTriggered();
    void headerTriggered(int level);
    void unorderedListTriggered();
    void orderedListTriggered();
    void linkTriggered();
    void imageTriggered();
    void detailsTriggered();
    void tableTriggered();      // 新增

private slots:
    void onEditorUnorderedChanged(bool active);
    void onEditorOrderedChanged(bool active);

private:
    void createActions(MarkdownEditor *editor);
    QIcon loadIcon(const QString &resourcePath, const QString &fallbackText);

    QAction *m_unorderedAction = nullptr;
    QAction *m_orderedAction = nullptr;
};