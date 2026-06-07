#pragma once
#include <QTextBrowser>
#include <QSet>

class PreviewWidget : public QTextBrowser
{
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget *parent = nullptr);

    QSet<QString> expandedIds() const { return m_expandedIds; }
    int fontSize() const { return m_fontSize; }

public slots:
    void setFontSize(int size);
    void setContent(const QString &html);

signals:
    void foldToggled();

protected:
    void doSetSource(const QUrl &url, QTextDocument::ResourceType type) override;

private slots:
    void onAnchorClicked(const QUrl &url);

private:
    QSet<QString> m_expandedIds;
    int m_fontSize = 14;
    QString m_pendingHtml;
};