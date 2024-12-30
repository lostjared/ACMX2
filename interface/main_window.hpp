#ifndef __APP_WINDOW_H_
#define __APP_WINDOW_H_

#include<QMainWindow.h>

class MainWindow : public QMainWindow {
Q_OBJECT
public:
    MainWindow(QWidget *parent = 0) : QMainWindow(parent) {
        initControls();
    }
    void initControls();
};




#endif