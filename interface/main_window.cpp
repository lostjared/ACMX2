#include"main_window.hpp"
#include<QLayout>
#include<QApplication>
#include<QMessageBox>
#include<QFile>
#include<QTextStream>
#include"settings.hpp"

void MainWindow::initControls() {
    process = new QProcess(this);
    connect(process, &QProcess::readyReadStandardOutput, this, [=]() {
        QString output = process->readAllStandardOutput();
        output.replace("\n", "<br>");
        this->Write(output);
    });

    connect(process, &QProcess::readyReadStandardError, this, [=]() {
        QString errorOutput = process->readAllStandardError();
        if(!errorOutput.contains("GStreamer"))
            Log("<b style='color:red;'>Error:</b> " + errorOutput);
    });

    connect(process, &QProcess::finished, this, [=](int exitCode, QProcess::ExitStatus) {
        QString text;
        QTextStream stream(&text);
        stream << "acmx2: Exited with Code: " << exitCode;
        Log(text + "<br>");
    });
    camera_index = 0;
    camera_res = QSize(1280, 720);
    setGeometry(150, 150, 1280, 720);
    setWindowTitle("ACMX2 - Interface");
    QMenuBar *menuBarPtr = menuBar();
    fileMenu = menuBarPtr->addMenu(tr("File"));
    cameraMenu = menuBarPtr->addMenu(tr("Camera"));
    runMenu = menuBarPtr->addMenu(tr("Run"));
    helpMenu = menuBarPtr->addMenu(tr("Help"));
    fileMenu_prop = new QAction(tr("Properties"), this);
    fileMenu->addAction(fileMenu_prop);
    connect(fileMenu_prop, &QAction::triggered, this, &MainWindow::fileOpenProp);
    fileMenu->addSeparator();
    fileMenu_exit = new QAction(tr("Exit"), this);
    connect(fileMenu_exit, &QAction::triggered, this, &MainWindow::fileExit);
    fileMenu->addAction(fileMenu_exit);
    cameraSet = new QAction(tr("Settings"), this);
    connect(cameraSet, &QAction::triggered, this, &MainWindow::cameraSettings);
    cameraMenu->addAction(cameraSet);
    runMenu_select = new QAction(tr("Run Selected"), this);
    runMenu_select->setShortcut(QKeySequence("F5"));
    connect(runMenu_select, &QAction::triggered, this, &MainWindow::runSelected);
    runMenu->addAction(runMenu_select);
    runMenu->addSeparator();
    runMenu_all = new QAction(tr("Run All"), this);
    runMenu_all->setShortcut(QKeySequence("Ctrl+E"));
    connect(runMenu_all, &QAction::triggered, this, &MainWindow::runAll);
    runMenu->addAction(runMenu_all);
    model = new ReadOnlyStringListModel(this);
    model->setStringList(items);
    list_view = new QListView(this);
    list_view->setStyleSheet("QListView { background-color: black; color: lime; font-size: 24px; font-family: 'Courier New', Courier, monospace; }");
    list_view->setModel(model);
    bottomTextBox = new QTextEdit(this);
    bottomTextBox->setHtml("<b style='color:red;'>ACMX2</b> - Interface: Loaded.");
    bottomTextBox->setStyleSheet("QTextEdit { background-color: black; color: lime; font-size: 24px; font-family: 'Courier New', Courier, monospace;; }");
    bottomTextBox->setReadOnly(true);
    connect(list_view, &QListView::doubleClicked,
        this, &MainWindow::listClicked);
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->addWidget(list_view, 3);   
    layout->addWidget(bottomTextBox, 1); 
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
    QSettings appSettings("LostSideDead");
    QString path = appSettings.value("shaders", "").toString();
#ifdef _WIN32
        executable_path = appSettings.value("exePath", "acmx2.exe").toString();
#else
        executable_path = appSettings.value("exePath", "acmx2").toString();
#endif
    if(!path.isEmpty()) {
        shader_path = path;
        loadShaders(path);
    }
}

QString MainWindow::readFileContents(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Log("Failed to open file: " + filePath);
        return QString();
    }

    QTextStream in(&file);
    QString contents = in.readAll();
    file.close(); 
    return contents;
}

void MainWindow::listClicked(const QModelIndex &i) {
   if (!i.isValid())
        return;
    QString itemText = i.data(Qt::DisplayRole).toString();
    open_files.append(new TextEditor(this));
    QString filePath = shader_path + "/" + itemText;
    open_files.back()->setText(readFileContents(filePath));
    open_files.back()->setFileName(filePath);
    open_files.back()->setIndex(&open_files, open_files.size()-1);
    open_files.back()->show();
}

void MainWindow::Log(const QString &message) {
    bottomTextBox->append(message);
    QTextCursor cursor = bottomTextBox->textCursor();
    cursor.movePosition(QTextCursor::End);
    bottomTextBox->setTextCursor(cursor);
}

void MainWindow::Write(const QString &message) {
    QTextCursor cursor = bottomTextBox->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertHtml(message);
    bottomTextBox->setTextCursor(cursor);
}

void MainWindow::fileOpenProp() {
    PropWindow propWindow(this);
    if (propWindow.exec() == QDialog::Accepted) {
        QString exePath = propWindow.exePathLineEdit->text();
        QString shaderDir = propWindow.shaderDirLineEdit->text();
        if(exePath.length()==0) {
            QMessageBox::information(this, "No Path", "Requires Executable path");
            return;
        }
        if(shaderDir.length()==0) {
            QMessageBox::information(this, "Shader Path", "Requires Shader Path");
            return;
        }
        if(loadShaders(shaderDir)) {
            Log("Executable Path: " + exePath);
            Log("Shader Directory: " + shaderDir + "<br>");
            executable_path = exePath;
            shader_path = shaderDir;
        }
    } else {
        Log("Canceled");
    }
}

bool MainWindow::loadShaders(const QString &path) {
    QFile file(path+"/index.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Could not open index file", "Failed to open file:" + file.errorString());
        return false;
    }
    items.clear();
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        items.append(line);
    }
    file.close();
    model->setStringList(items);
    return true;
}

void MainWindow::fileExit() {
    QApplication::quit();
}

void MainWindow::runSelected() {
   if(shader_path.length()==0) {
        QMessageBox::information(this, "Select Shaders", "Select Shader Path");
        return;
   }
   QItemSelectionModel *selectionModel = list_view->selectionModel();
    if (!selectionModel->hasSelection()) {
        Log("<b>No item selected.</b>");
        return;
    }
    QModelIndex selectedIndex = selectionModel->currentIndex();
    QString data = selectedIndex.data(Qt::DisplayRole).toString();
    QStringList arguments;
    QString dirPath = QCoreApplication::applicationDirPath();
    QString shader_file = shader_path + "/" + data;
    arguments << "--path" << dirPath << "--fragment" << shader_file;
    QString res;
    QTextStream stream(&res);
    stream << camera_res.width() << "x" << camera_res.height();
    arguments << "--resolution" << res;
    arguments << "--device" << QString::number(camera_index);
    Log("shell: acmx2 " + concatList(arguments) + "<br>");
    process->start(executable_path, arguments);
    if(!process->waitForStarted()) {
        Log("<b style='color:red;'>Failed to start the program.</b>");
        QMessageBox::critical(this, "Error", "Failed to start the program.");
    }
}
void MainWindow::runAll() {
    if(shader_path.length()==0) {
        QMessageBox::information(this, "Select Shaders", "Select Shader Path");
        return;
   }
    QStringList arguments;
    QString dirPath = QCoreApplication::applicationDirPath();
    QString shader_file = shader_path;
    arguments << "--path" << dirPath << "--shaders" << shader_file;
    QString res;
    QTextStream stream(&res);
    stream << camera_res.width() << "x" << camera_res.height();
    arguments << "--resolution" << res;
    arguments << "--device" << QString::number(camera_index);
    Log("shell: acmx2 " + concatList(arguments) + "<br>");
    process->start(executable_path, arguments);
    if(!process->waitForStarted()) {
        Log("<b style='color:red;'>Failed to start the program.</b>");
        QMessageBox::critical(this, "Error", "Failed to start the program.");
    }
}

void MainWindow::cameraSettings() {
    SettingsWindow settings(this);
    if(settings.exec() == QDialog::Accepted) {
        camera_index = settings.getSelectedCameraIndex();
        camera_res = settings.getSelectedResolution();
    }
}

QString MainWindow::concatList(const QStringList lst) {
     QString text;
     QTextStream stream(&text);
     for(auto &i : lst) {
        stream << i << " ";
     }
     return text;
}