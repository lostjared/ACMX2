#include<QApplication.h>
#include"main_window.hpp"


int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QApplication::setWindowIcon(QIcon(":/win-icon.png"));
    MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}