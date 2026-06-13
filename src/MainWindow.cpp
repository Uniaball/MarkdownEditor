#include "MainWindow.h"
#include "MarkdownEditor.h"
#include "PreviewWidget.h"
#include "FormatToolBar.h"
#include "parser/MarkdownParser.h"
#include <QMenuBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QScrollBar>
#include <QScroller>
#include <QScreen>
#include <QGuiApplication>
#include <QResizeEvent>
#include <format>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setupMenuBar();
    setupConnections();
    setupScrollSync();
    setWindowTitle("Markdown 编辑器 - 未命名");
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    Qt::Orientation initOrient = Qt::Vertical;
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QSize sz = screen->availableSize();
        if (sz.width() > sz.height()) initOrient = Qt::Horizontal;
    }

    m_splitter = new QSplitter(initOrient, this);
    m_editor = new MarkdownEditor;
    m_preview = new PreviewWidget;

    m_editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_preview->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    QWidget *previewViewport = m_preview->viewport();
    if (!QScroller::hasScroller(previewViewport)) {
        QScroller::grabGesture(previewViewport, QScroller::TouchGesture);
    }
    QScroller *previewScroller = QScroller::scroller(previewViewport);
    QScrollerProperties props = previewScroller->scrollerProperties();
    props.setScrollMetric(QScrollerProperties::DragVelocitySmoothingFactor, 0.2);
    props.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.1);
    props.setScrollMetric(QScrollerProperties::AcceleratingFlickMaximumTime, 0.1);
    props.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.6);
    previewScroller->setScrollerProperties(props);

    m_splitter->addWidget(m_editor);
    m_splitter->addWidget(m_preview);
    setCentralWidget(m_splitter);

    m_formatBar = new FormatToolBar(m_editor);
    addToolBar(m_formatBar);
    m_formatBar->setMovable(false);
    m_formatBar->setFloatable(false);
    m_formatBar->toggleViewAction()->setEnabled(false);
}

void MainWindow::setupMenuBar()
{
    QMenuBar *mb = menuBar();
    mb->setNativeMenuBar(false);
    QMenu *fileMenu = mb->addMenu("文件(&F)");
    fileMenu->addAction("新建(&N)", QKeySequence::New, this, &MainWindow::newFile);
    fileMenu->addAction("打开(&O)...", QKeySequence::Open, this, &MainWindow::openFile);
    fileMenu->addAction("保存(&S)", QKeySequence::Save, this, &MainWindow::saveFile);
    fileMenu->addAction("另存为(&A)...", QKeySequence::SaveAs, this, &MainWindow::saveAsFile);
    fileMenu->addSeparator();
    fileMenu->addAction("退出(&X)", QKeySequence::Quit, this, &QWidget::close);
}

void MainWindow::setupConnections()
{
    connect(m_editor, &MarkdownEditor::contentChanged, this, &MainWindow::updatePreview);
    connect(m_preview, &PreviewWidget::foldToggled, this, &MainWindow::updatePreview);
    connect(m_editor, &MarkdownEditor::fontSizeChanged, this, &MainWindow::syncFontSize);
}

void MainWindow::setupScrollSync()
{
    connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &MainWindow::syncScrollFromEditor);
    connect(m_preview->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &MainWindow::syncScrollFromPreview);
}

void MainWindow::updatePreview()
{
    int savedPos = m_preview->verticalScrollBar()->value();

    auto result = MarkdownParser::toHtml(m_editor->toPlainText(), m_preview->expandedIds());
    if (result) {
        m_preview->verticalScrollBar()->blockSignals(true);
        m_preview->setContent(*result);
        m_preview->verticalScrollBar()->setValue(savedPos);
        m_preview->verticalScrollBar()->blockSignals(false);

        m_modified = true;
        std::string title = std::format("Markdown 编辑器 - {}[*]",
            m_currentFilePath.isEmpty() ? "未命名" : m_currentFilePath.toStdString());
        setWindowTitle(QString::fromStdString(title));
    } else {
        m_preview->setPlainText(QString("解析错误: %1").arg(result.error()));
    }
}

void MainWindow::syncFontSize(int size)
{
    m_preview->setFontSize(size);
    m_currentFontSize = size;
}

void MainWindow::syncScrollFromEditor(int value)
{
    QScrollBar *editorBar = m_editor->verticalScrollBar();
    QScrollBar *previewBar = m_preview->verticalScrollBar();

    double editorRange = editorBar->maximum() - editorBar->minimum();
    double previewRange = previewBar->maximum() - previewBar->minimum();
    if (editorRange <= 0 || previewRange <= 0) return;

    double ratio = (value - editorBar->minimum()) / editorRange;
    int previewValue = previewBar->minimum() + qRound(ratio * previewRange);

    previewBar->blockSignals(true);
    previewBar->setValue(previewValue);
    previewBar->blockSignals(false);
    m_preview->viewport()->update();
}

void MainWindow::syncScrollFromPreview(int value)
{
    QScrollBar *editorBar = m_editor->verticalScrollBar();
    QScrollBar *previewBar = m_preview->verticalScrollBar();

    double previewRange = previewBar->maximum() - previewBar->minimum();
    double editorRange = editorBar->maximum() - editorBar->minimum();
    if (previewRange <= 0 || editorRange <= 0) return;

    double ratio = (value - previewBar->minimum()) / previewRange;
    int editorValue = editorBar->minimum() + qRound(ratio * editorRange);

    editorBar->blockSignals(true);
    editorBar->setValue(editorValue);
    editorBar->blockSignals(false);
    m_editor->viewport()->update();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (!m_splitter) return;

    QSize newSize = event->size();
    Qt::Orientation newOrient = (newSize.width() > newSize.height()) ? Qt::Horizontal : Qt::Vertical;
    if (m_splitter->orientation() != newOrient) {
        m_splitter->setOrientation(newOrient);
    }
}

void MainWindow::newFile()
{
    m_editor->clear();
    m_currentFilePath.clear();
    m_modified = false;
    updatePreview();
}

void MainWindow::openFile()
{
    QString path = QFileDialog::getOpenFileName(this, "打开 Markdown 文件", "",
        "Markdown 文件 (*.md *.markdown);;所有文件 (*)");
    if (!path.isEmpty())
        loadFile(path);
}

void MainWindow::loadFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法打开文件。");
        return;
    }
    QTextStream in(&file);
    m_editor->setPlainText(in.readAll());
    m_currentFilePath = path;
    m_modified = false;
    updatePreview();
}

bool MainWindow::saveFile()
{
    if (m_currentFilePath.isEmpty())
        return saveAsFile();
    return saveFileAs(m_currentFilePath);
}

bool MainWindow::saveAsFile()
{
    QString path = QFileDialog::getSaveFileName(this, "保存 Markdown 文件", "",
        "Markdown 文件 (*.md *.markdown);;所有文件 (*)");
    if (path.isEmpty())
        return false;
    return saveFileAs(path);
}

bool MainWindow::saveFileAs(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法保存文件。");
        return false;
    }
    QTextStream out(&file);
    out << m_editor->toPlainText();
    m_currentFilePath = path;
    m_modified = false;
    updatePreview();
    return true;
}