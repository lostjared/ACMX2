#include "editor.hpp"
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QColor>
#include <QFont>
#include <QMenuBar>
#include <QAction>
#include <QFile>

TextEditor::TextEditor(QWidget *parent)
    : QDialog(parent)
{
    init();
}

void TextEditor::setText(const QString &text) {
    m_textEdit->setPlainText(text);
}

void TextEditor::setFileName(const QString &filen) {
    filename = filen;
    setWindowTitle("ACMX2 - " + filen);
}

void TextEditor::init() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    m_textEdit = new CustomTextEdit(this);
    m_textEdit->setTabStopDistance(4 * m_textEdit->fontMetrics().horizontalAdvance(' '));
    m_textEdit->setStyleSheet("QPlainTextEdit { color: white;  font-size: 24px; font-family: 'Courier New', Courier, monospace; background-color: black; }");
    layout->addWidget(m_textEdit);
    m_highlighter = new GlslSyntaxHighlighter(m_textEdit->document());
    setLayout(layout);
    setGeometry(300,300, 1024, 768);
    QMenuBar *menuBar = new QMenuBar(this);
    QMenu *fileMenu = menuBar->addMenu("File");
    QAction *saveAction = fileMenu->addAction("Save");
    saveAction->setShortcut(QKeySequence("Ctrl+S"));
    QAction *exitAction = fileMenu->addAction("Close");
    connect(saveAction, &QAction::triggered, this, [=]() {
            saveContents();
    });
    connect(exitAction, &QAction::triggered, this, &TextEditor::close);
    layout->setMenuBar(menuBar);

    connect(this, &QDialog::finished, this, [this]() {
        if (vec && index >= 0 && index < vec->size()) {
            vec->removeAt(index);
        }
    });

    setAttribute(Qt::WA_DeleteOnClose);

    m_textEdit->setTabStopDistance(5 * QFontMetrics(m_textEdit->font()).horizontalAdvance(' '));
}

void TextEditor::saveContents() {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    QString content = m_textEdit->toPlainText();
    QTextStream out(&file);
    out << content;
    file.close();
}

void TextEditor::setIndex(QVector<TextEditor *> *v, int i) {
    index = i;
    vec = v;
}
