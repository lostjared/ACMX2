#ifndef __EDITOR__H__
#define __EDITOR__H__

#include<QDialog>
#include<QPlainTextEdit>
#include"syntax.hpp"
#include<QVector>

#include <QKeyEvent>

class CustomTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CustomTextEdit(QWidget *parent = nullptr) : QPlainTextEdit(parent)
    {
        setTabStopDistance(4 * fontMetrics().horizontalAdvance(' '));
    }

protected:
    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Tab) {
            insertPlainText("    "); 
        } else {
            QPlainTextEdit::keyPressEvent(event);
        }
    }
};

class TextEditor : public QDialog
{
    Q_OBJECT

public:
    explicit TextEditor(QWidget *parent = nullptr);
    void setText(const QString &text);
    void setFileName(const QString &filen);
    void saveContents();
    void setIndex(QVector<TextEditor *> *v, int index);
private:
    void init();
    int index;
    CustomTextEdit*      m_textEdit  = nullptr;
    GlslSyntaxHighlighter* m_highlighter = nullptr;
    QString filename;
    QVector<TextEditor *> *vec;
};



#endif
