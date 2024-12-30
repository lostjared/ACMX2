#include"main_window.hpp"
#include<QLayout>
#include<QApplication>
void MainWindow::initControls() {
    setGeometry(150, 150, 1280, 720);
    setWindowTitle("ACMX2 - Interface");
    QMenuBar *menuBarPtr = menuBar();
    fileMenu = menuBarPtr->addMenu(tr("File"));
    helpMenu = menuBarPtr->addMenu(tr("Help"));
    fileMenu_prop = new QAction(tr("Properties"), this);
    fileMenu->addAction(fileMenu_prop);
    connect(fileMenu_prop, &QAction::triggered, this, &MainWindow::fileOpenProp);
    fileMenu->addSeparator();
    fileMenu_exit = new QAction(tr("Exit"), this);
    connect(fileMenu_exit, &QAction::triggered, this, &MainWindow::fileExit);
    fileMenu->addAction(fileMenu_exit);
    model = new ReadOnlyStringListModel(this);
    items << "Item 1" << "Item 2" << "Item 3";
    model->setStringList(items);
    list_view = new QListView(this);
    list_view->setModel(model);
    bottomTextBox = new QTextEdit(this);
    bottomTextBox->setHtml("<b>ACMX2</b> - Interface: Loaded.<br>\n");
    bottomTextBox->setStyleSheet("QTextEdit { background-color: black; color: green; font-size: 24px; font-family: monaco; }");
    bottomTextBox->setReadOnly(true);
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->addWidget(list_view, 3);   
    layout->addWidget(bottomTextBox, 1); 
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
}

void MainWindow::Log(const QString &message) {
    bottomTextBox->append(message);
    QTextCursor cursor = bottomTextBox->textCursor();
    cursor.movePosition(QTextCursor::End);
    bottomTextBox->setTextCursor(cursor);
}

void MainWindow::fileOpenProp() {

}

void MainWindow::fileExit() {
    QApplication::quit();
}