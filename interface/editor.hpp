#ifndef EDITOR_HPP
#define EDITOR_HPP

#include <QDialog>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QVector>
#include <QStatusBar>
#include <QLabel>
#include <QCloseEvent>
#include "syntax.hpp"

class CustomTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    using QPlainTextEdit::QPlainTextEdit;
};

class TextEditor : public QDialog
{
    Q_OBJECT

public:
    explicit TextEditor(QWidget *parent = nullptr);
    void setText(const QString &text);
    void setFileName(const QString &filename);
    void setIndex(QVector<TextEditor *> *v, int i);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void init();
    void saveContents();
    void saveAs();
    void findText();
    void findNext();
    void replaceText();
    void gotoLine();
    void increaseFontSize();
    void decreaseFontSize();
    void resetFontSize();
    void updateFontSize();
    void updateCursorPosition();
    void updateWindowTitle();
    
    bool m_modified = false;
    CustomTextEdit *m_textEdit = nullptr; 
    GlslSyntaxHighlighter *m_highlighter = nullptr;
    QStatusBar *m_statusBar = nullptr;
    QLabel *m_lineColLabel = nullptr;
    QVector<TextEditor *> *vec = nullptr;
    QString filename;
    QString m_lastSearchText;
    int index = -1;
    int m_fontSize = 24;
};

#endif
