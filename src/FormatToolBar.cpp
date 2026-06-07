#include "FormatToolBar.h"
#include "MarkdownEditor.h"
#include <QAction>
#include <QPainter>
#include <QPixmap>
#include <QFont>

FormatToolBar::FormatToolBar(MarkdownEditor *editor, QWidget *parent)
    : QToolBar("格式", parent)
{
    createActions(editor);

    connect(editor, &MarkdownEditor::unorderedListModeChanged,
            this, &FormatToolBar::onEditorUnorderedChanged);
    connect(editor, &MarkdownEditor::orderedListModeChanged,
            this, &FormatToolBar::onEditorOrderedChanged);
}

void FormatToolBar::createActions(MarkdownEditor *editor)
{
    QAction *boldAct = addAction(loadIcon(":/icons/bold.svg", "B"), "");
    boldAct->setToolTip("粗体 (Ctrl+B)");
    boldAct->setShortcut(QKeySequence("Ctrl+B"));
    connect(boldAct, &QAction::triggered, this, &FormatToolBar::boldTriggered);
    connect(boldAct, &QAction::triggered, editor, &MarkdownEditor::boldText);

    QAction *italicAct = addAction(loadIcon(":/icons/italic.svg", "I"), "");
    italicAct->setToolTip("斜体 (Ctrl+I)");
    italicAct->setShortcut(QKeySequence("Ctrl+I"));
    connect(italicAct, &QAction::triggered, this, &FormatToolBar::italicTriggered);
    connect(italicAct, &QAction::triggered, editor, &MarkdownEditor::italicText);

    QAction *codeAct = addAction(loadIcon(":/icons/code.svg", "<>"), "");
    codeAct->setToolTip("行内代码 (Ctrl+`)");
    codeAct->setShortcut(QKeySequence("Ctrl+`"));
    connect(codeAct, &QAction::triggered, this, &FormatToolBar::inlineCodeTriggered);
    connect(codeAct, &QAction::triggered, editor, &MarkdownEditor::inlineCode);

    QAction *blockAct = addAction(loadIcon(":/icons/code_blocks.svg", "{ }"), "");
    blockAct->setToolTip("代码块 (Ctrl+Shift+C)");
    blockAct->setShortcut(QKeySequence("Ctrl+Shift+C"));
    connect(blockAct, &QAction::triggered, this, &FormatToolBar::codeBlockTriggered);
    connect(blockAct, &QAction::triggered, editor, &MarkdownEditor::codeBlock);

    addSeparator();

    const QString hIcons[] = {":/icons/format_h1.svg", ":/icons/format_h2.svg", ":/icons/format_h3.svg"};
    const QString hLetters[] = {"H1", "H2", "H3"};
    for (int i = 0; i < 3; ++i) {
        QIcon icon = loadIcon(hIcons[i], hLetters[i]);
        QAction *hAct = addAction(icon, "");
        hAct->setToolTip(QString("标题 %1 (Ctrl+%2)").arg(i+1).arg(i+1));
        hAct->setShortcut(QKeySequence(QString("Ctrl+%1").arg(i+1)));
        connect(hAct, &QAction::triggered, this, [this, i]() { emit headerTriggered(i+1); });
        connect(hAct, &QAction::triggered, editor, [editor, i]() { editor->header(i+1); });
    }

    addSeparator();

    m_unorderedAction = addAction(loadIcon(":/icons/list.svg", "•"), "");
    m_unorderedAction->setToolTip("无序列表 (Ctrl+U)");
    m_unorderedAction->setShortcut(QKeySequence("Ctrl+U"));
    m_unorderedAction->setCheckable(true);
    connect(m_unorderedAction, &QAction::triggered, this, &FormatToolBar::unorderedListTriggered);
    connect(m_unorderedAction, &QAction::triggered, editor, &MarkdownEditor::toggleUnorderedList);

    m_orderedAction = addAction(loadIcon(":/icons/numbered_list.svg", "1."), "");
    m_orderedAction->setToolTip("有序列表 (Ctrl+Shift+O)");
    m_orderedAction->setShortcut(QKeySequence("Ctrl+Shift+O"));
    m_orderedAction->setCheckable(true);
    connect(m_orderedAction, &QAction::triggered, this, &FormatToolBar::orderedListTriggered);
    connect(m_orderedAction, &QAction::triggered, editor, &MarkdownEditor::toggleOrderedList);

    addSeparator();

    QAction *detailsAct = addAction(loadIcon(":/icons/unfold_more.svg", "▶"), "");
    detailsAct->setToolTip("折叠块 (Ctrl+Shift+D)");
    detailsAct->setShortcut(QKeySequence("Ctrl+Shift+D"));
    connect(detailsAct, &QAction::triggered, this, &FormatToolBar::detailsTriggered);
    connect(detailsAct, &QAction::triggered, editor, &MarkdownEditor::detailsBlock);

    QAction *tableAct = addAction(loadIcon(":/icons/table.svg", "▦"), "");
    tableAct->setToolTip("表格 (Ctrl+Shift+T)");
    tableAct->setShortcut(QKeySequence("Ctrl+Shift+T"));
    connect(tableAct, &QAction::triggered, this, &FormatToolBar::tableTriggered);
    connect(tableAct, &QAction::triggered, editor, &MarkdownEditor::insertTable);

    QAction *linkAct = addAction(loadIcon(":/icons/link.svg", "🔗"), "");
    linkAct->setToolTip("插入链接 (Ctrl+L)");
    linkAct->setShortcut(QKeySequence("Ctrl+L"));
    connect(linkAct, &QAction::triggered, this, &FormatToolBar::linkTriggered);
    connect(linkAct, &QAction::triggered, editor, &MarkdownEditor::insertLink);

    QAction *imageAct = addAction(loadIcon(":/icons/image.svg", "🖼"), "");
    imageAct->setToolTip("插入图片 (Ctrl+Shift+I)");
    imageAct->setShortcut(QKeySequence("Ctrl+Shift+I"));
    connect(imageAct, &QAction::triggered, this, &FormatToolBar::imageTriggered);
    connect(imageAct, &QAction::triggered, editor, &MarkdownEditor::insertImage);
}

void FormatToolBar::onEditorUnorderedChanged(bool active)
{
    m_unorderedAction->setChecked(active);
}

void FormatToolBar::onEditorOrderedChanged(bool active)
{
    m_orderedAction->setChecked(active);
}

QIcon FormatToolBar::loadIcon(const QString &resourcePath, const QString &fallbackText)
{
    QIcon icon(resourcePath);
    if (!icon.isNull()) return icon;

    QPixmap pix(24, 24);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setFont(QFont("Segoe UI", 14));
    painter.drawText(pix.rect(), Qt::AlignCenter, fallbackText);
    painter.end();
    return QIcon(pix);
}