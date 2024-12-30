#ifndef __CONSOLE__H_
#define __CONSOLE__H_

#include<QWidget>
#include<QDialog>

class PropWindow : public QDialog {
Q_OBJECT
public:
    PropWindow(QWidget *parent) : QDialog(parent) {

    }
    void init();
};





#endif