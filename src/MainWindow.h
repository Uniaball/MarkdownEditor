#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QSet>

class MarkdownEditor;
class PreviewWidget;
class FormatToolBar;
class QResizeEvent;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void newFile();
    void openFile();
    bool saveFile();
    bool saveAsFile();
    void updatePreview();
    void syncFontSize(int size);
    void syncScrollFromEditor(int value);
    void syncScrollFromPreview(int value);

private:
    void setupUi();
    void setupMenuBar();
    void setupConnections();
    void setupScrollSync();

    void loadFile(const QString &path);
    bool saveFileAs(const QString &path);

    MarkdownEditor *m_editor = nullptr;
    PreviewWidget  *m_preview = nullptr;
    FormatToolBar  *m_formatBar = nullptr;
    QSplitter      *m_splitter = nullptr;

    QString m_currentFilePath;
    bool m_modified = false;
    int m_currentFontSize = 14;
};