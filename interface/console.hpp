#ifndef __CONSOLE__H_
#define __CONSOLE__H_

#include<QWidget.h>
#include<QDialog.h>

class Console : public QWidget {
Q_OBJECT
public:
    Console(QWidget *parent) : QWidget(parent) {

    }
    void init();
};





#endif