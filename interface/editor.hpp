#ifndef __EDITOR__H__
#define __EDITOR__H__

#include<QDialog.h>

class TextEditor : public QDialog {
Q_OBJECT

public:
    TextEditor(QWidget *parent = 0) : QDialog(parent) { 
        init();
    }

    void init();
};

#endif