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
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextCursor>
#include <QClipboard>
#include <QApplication>
#include <QLabel>
#include <QStatusBar>

TextEditor::TextEditor(QWidget *parent)
    : QDialog(parent), m_modified(false), m_textEdit(nullptr), m_highlighter(nullptr), 
      m_statusBar(nullptr), m_lineColLabel(nullptr), vec(nullptr), index(-1), m_fontSize(24)
{
    init();
}

void TextEditor::setText(const QString &text) {
    m_textEdit->setPlainText(text);
    m_modified = false;
}

void TextEditor::setFileName(const QString &filen) {
    filename = filen;
    updateWindowTitle();
}

void TextEditor::updateWindowTitle() {
    QString title = "ACMX2";
    if (!filename.isEmpty()) {
        title += " - " + QFileInfo(filename).fileName();
    }
    if (m_modified) {
        title += " *";
    }
    setWindowTitle(title);
}

void TextEditor::init() {
    m_modified = false;
    m_fontSize = 24;
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    QMenuBar *menuBar = new QMenuBar(this);
    
    QMenu *fileMenu = menuBar->addMenu("&File");
    
    QAction *saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);
    
    QAction *saveAsAction = fileMenu->addAction("Save &As...");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    
    fileMenu->addSeparator();
    
    QAction *closeAction = fileMenu->addAction("&Close");
    closeAction->setShortcut(QKeySequence::Close);
    
    QMenu *editMenu = menuBar->addMenu("&Edit");
    
    QAction *undoAction = editMenu->addAction("&Undo");
    undoAction->setShortcut(QKeySequence::Undo);
    
    QAction *redoAction = editMenu->addAction("&Redo");
    redoAction->setShortcut(QKeySequence::Redo);
    
    editMenu->addSeparator();
    
    QAction *cutAction = editMenu->addAction("Cu&t");
    cutAction->setShortcut(QKeySequence::Cut);
    
    QAction *copyAction = editMenu->addAction("&Copy");
    copyAction->setShortcut(QKeySequence::Copy);
    
    QAction *pasteAction = editMenu->addAction("&Paste");
    pasteAction->setShortcut(QKeySequence::Paste);
    
    editMenu->addSeparator();
    
    QAction *selectAllAction = editMenu->addAction("Select &All");
    selectAllAction->setShortcut(QKeySequence::SelectAll);
    
    editMenu->addSeparator();
    
    QAction *findAction = editMenu->addAction("&Find...");
    findAction->setShortcut(QKeySequence::Find);
    
    QAction *findNextAction = editMenu->addAction("Find &Next");
    findNextAction->setShortcut(QKeySequence::FindNext);
    
    QAction *replaceAction = editMenu->addAction("&Replace...");
    replaceAction->setShortcut(QKeySequence::Replace);
    
    editMenu->addSeparator();
    
    QAction *gotoLineAction = editMenu->addAction("&Go to Line...");
    gotoLineAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
    
    QMenu *viewMenu = menuBar->addMenu("&View");
    
    QAction *increaseFontAction = viewMenu->addAction("Increase Font Size");
    increaseFontAction->setShortcut(QKeySequence::ZoomIn);
    
    QAction *decreaseFontAction = viewMenu->addAction("Decrease Font Size");
    decreaseFontAction->setShortcut(QKeySequence::ZoomOut);
    
    QAction *resetFontAction = viewMenu->addAction("Reset Font Size");
    resetFontAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    
    viewMenu->addSeparator();
    
    QAction *toggleWordWrapAction = viewMenu->addAction("Word Wrap");
    toggleWordWrapAction->setCheckable(true);
    toggleWordWrapAction->setChecked(false);
    
    layout->setMenuBar(menuBar);
    
    m_textEdit = new CustomTextEdit(this);
    m_textEdit->setTabStopDistance(4 * m_textEdit->fontMetrics().horizontalAdvance(' '));
    updateFontSize();
    
    layout->addWidget(m_textEdit);
    
    m_statusBar = new QStatusBar(this);
    m_lineColLabel = new QLabel("Line: 1, Col: 1", this);
    m_statusBar->addPermanentWidget(m_lineColLabel);
    layout->addWidget(m_statusBar);
    
    m_highlighter = new GlslSyntaxHighlighter(m_textEdit->document());
    
    setLayout(layout);
    setGeometry(300, 300, 1024, 768);
    
    connect(saveAction, &QAction::triggered, this, &TextEditor::saveContents);
    connect(saveAsAction, &QAction::triggered, this, &TextEditor::saveAs);
    connect(closeAction, &QAction::triggered, this, &TextEditor::close);
    
    connect(undoAction, &QAction::triggered, m_textEdit, &QPlainTextEdit::undo);
    connect(redoAction, &QAction::triggered, m_textEdit, &QPlainTextEdit::redo);
    connect(cutAction, &QAction::triggered, m_textEdit, &QPlainTextEdit::cut);
    connect(copyAction, &QAction::triggered, m_textEdit, &QPlainTextEdit::copy);
    connect(pasteAction, &QAction::triggered, m_textEdit, &QPlainTextEdit::paste);
    connect(selectAllAction, &QAction::triggered, m_textEdit, &QPlainTextEdit::selectAll);
    
    connect(findAction, &QAction::triggered, this, &TextEditor::findText);
    connect(findNextAction, &QAction::triggered, this, &TextEditor::findNext);
    connect(replaceAction, &QAction::triggered, this, &TextEditor::replaceText);
    connect(gotoLineAction, &QAction::triggered, this, &TextEditor::gotoLine);
    
    connect(increaseFontAction, &QAction::triggered, this, &TextEditor::increaseFontSize);
    connect(decreaseFontAction, &QAction::triggered, this, &TextEditor::decreaseFontSize);
    connect(resetFontAction, &QAction::triggered, this, &TextEditor::resetFontSize);
    
    connect(toggleWordWrapAction, &QAction::triggered, this, [this](bool checked) {
        m_textEdit->setLineWrapMode(checked ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    });
    
    connect(m_textEdit, &QPlainTextEdit::textChanged, this, [this]() {
        m_modified = true;
        updateWindowTitle();
    });
    
    connect(m_textEdit, &QPlainTextEdit::cursorPositionChanged, this, &TextEditor::updateCursorPosition);
    
    connect(this, &QDialog::finished, this, [this]() {
        if (vec && index >= 0 && index < vec->size()) {
            vec->removeAt(index);
        }
    });
    
    setAttribute(Qt::WA_DeleteOnClose);
}

void TextEditor::saveContents() {
    if (filename.isEmpty()) {
        saveAs();
        return;
    }
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not save file: " + filename);
        return;
    }
    
    QString content = m_textEdit->toPlainText();
    QTextStream out(&file);
    out << content;
    file.close();
    
    m_modified = false;
    updateWindowTitle();
    m_statusBar->showMessage("File saved", 2000);
}

void TextEditor::saveAs() {
    QString newFileName = QFileDialog::getSaveFileName(
        this, "Save File As", filename, "GLSL Files (*.glsl *.frag *.vert);;All Files (*)");
    
    if (!newFileName.isEmpty()) {
        filename = newFileName;
        saveContents();
    }
}

void TextEditor::findText() {
    bool ok;
    QString searchText = QInputDialog::getText(this, "Find", "Enter text to find:", 
                                                QLineEdit::Normal, m_lastSearchText, &ok);
    if (ok && !searchText.isEmpty()) {
        m_lastSearchText = searchText;
        findNext();
    }
}

void TextEditor::findNext() {
    if (m_lastSearchText.isEmpty()) {
        findText();
        return;
    }
    
    QTextCursor cursor = m_textEdit->textCursor();
    QTextDocument::FindFlags flags;
    
    QTextCursor found = m_textEdit->document()->find(m_lastSearchText, cursor, flags);
    
    if (found.isNull()) {
        
        cursor.movePosition(QTextCursor::Start);
        found = m_textEdit->document()->find(m_lastSearchText, cursor, flags);
        
        if (found.isNull()) {
            m_statusBar->showMessage("Text not found: " + m_lastSearchText, 3000);
            return;
        }
    }
    
    m_textEdit->setTextCursor(found);
    m_statusBar->showMessage("Found: " + m_lastSearchText, 2000);
}

void TextEditor::replaceText() {
    bool ok;
    QString searchText = QInputDialog::getText(this, "Replace", "Find:", 
                                                QLineEdit::Normal, m_lastSearchText, &ok);
    if (!ok || searchText.isEmpty()) return;
    
    QString replaceWith = QInputDialog::getText(this, "Replace", "Replace with:", 
                                                 QLineEdit::Normal, "", &ok);
    if (!ok) return;
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Replace All", 
        "Replace all occurrences of '" + searchText + "' with '" + replaceWith + "'?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QString text = m_textEdit->toPlainText();
        int count = text.count(searchText);
        text.replace(searchText, replaceWith);
        m_textEdit->setPlainText(text);
        m_statusBar->showMessage("Replaced " + QString::number(count) + " occurrence(s)", 3000);
    }
}

void TextEditor::gotoLine() {
    bool ok;
    int lineNumber = QInputDialog::getInt(this, "Go to Line", "Line number:", 
                                           1, 1, m_textEdit->document()->blockCount(), 1, &ok);
    if (ok) {
        QTextCursor cursor(m_textEdit->document()->findBlockByLineNumber(lineNumber - 1));
        m_textEdit->setTextCursor(cursor);
        m_textEdit->centerCursor();
    }
}

void TextEditor::increaseFontSize() {
    m_fontSize += 2;
    if (m_fontSize > 72) m_fontSize = 72;
    updateFontSize();
}

void TextEditor::decreaseFontSize() {
    m_fontSize -= 2;
    if (m_fontSize < 8) m_fontSize = 8;
    updateFontSize();
}

void TextEditor::resetFontSize() {
    m_fontSize = 24;
    updateFontSize();
}

void TextEditor::updateFontSize() {
    QString styleSheet = QString(
        "QPlainTextEdit { "
        "color: white; "
        "font-size: %1px; "
        "font-family: 'Courier New', Courier, monospace; "
        "background-color: black; "
        "}"
    ).arg(m_fontSize);
    
    m_textEdit->setStyleSheet(styleSheet);
    m_textEdit->setTabStopDistance(4 * m_textEdit->fontMetrics().horizontalAdvance(' '));
}

void TextEditor::updateCursorPosition() {
    QTextCursor cursor = m_textEdit->textCursor();
    int line = cursor.blockNumber() + 1;
    int col = cursor.columnNumber() + 1;
    m_lineColLabel->setText(QString("Line: %1, Col: %2").arg(line).arg(col));
}

void TextEditor::setIndex(QVector<TextEditor *> *v, int i) {
    index = i;
    vec = v;
}

void TextEditor::closeEvent(QCloseEvent *event) {
    if (m_modified) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Unsaved Changes", 
            "The document has been modified. Do you want to save your changes?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (reply == QMessageBox::Save) {
            saveContents();
            event->accept();
        } else if (reply == QMessageBox::Discard) {
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}