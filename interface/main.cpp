#include<QApplication>
#include"main_window.hpp"


int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QApplication::setWindowIcon(QIcon(":/win-icon.png"));
    QFile styleFile(":/stylesheet.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleFile.readAll());
        app.setStyle("Fusion");
        app.setStyleSheet(style);
        styleFile.close();
    }
    MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}