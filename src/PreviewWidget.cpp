#include "PreviewWidget.h"
#include <QDesktopServices>
#include <QFont>
#include <QScrollBar>

PreviewWidget::PreviewWidget(QWidget *parent)
    : QTextBrowser(parent), m_fontSize(14)
{
    setOpenLinks(false);
    connect(this, &QTextBrowser::anchorClicked, this, &PreviewWidget::onAnchorClicked);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

void PreviewWidget::setFontSize(int size)
{
    if (m_fontSize != size) {
        m_fontSize = size;
        if (!m_pendingHtml.isEmpty()) {
            setContent(m_pendingHtml);
        }
    }
}

void PreviewWidget::setContent(const QString &html)
{
    m_pendingHtml = html;
    QFont f = font();
    f.setPointSize(m_fontSize);
    setFont(f);

    // 保存当前滚动位置
    int savedPos = verticalScrollBar()->value();

    QTextBrowser::setHtml(html);

    // 恢复滚动位置（若超出范围则自动调整）
    verticalScrollBar()->setValue(savedPos);
}

void PreviewWidget::doSetSource(const QUrl &url, QTextDocument::ResourceType type)
{
    if (url.scheme() == "fold") {
        QTextBrowser::doSetSource(url, type);
    } else {
        QDesktopServices::openUrl(url);
    }
}

void PreviewWidget::onAnchorClicked(const QUrl &url)
{
    if (url.scheme() == "fold") {
        QString id = url.path();
        if (m_expandedIds.contains(id))
            m_expandedIds.remove(id);
        else
            m_expandedIds.insert(id);
        emit foldToggled();
    }
}