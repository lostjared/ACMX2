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
        if(!errorOutput.contains("GStreamer")) {
            errorOutput.replace("\n", "<br>");
            this->Write("<b style='color:red;'>Error:</b> " + errorOutput);
        }
    });

    connect(process, &QProcess::finished, this, [=](int exitCode, QProcess::ExitStatus) {
        QString text;
        QTextStream stream(&text);
        stream << "acmx2: Exited with Code: " << exitCode;
        Log(text + "<br>");
    });
    camera_index = 0;
    camera_res = QSize(1280, 720);
    screen_res = QSize(0, 0);
    setGeometry(150, 150, 1280, 720);
    setWindowTitle("ACMX2 - Interface");
    QMenuBar *menuBarPtr = menuBar();
    fileMenu = menuBarPtr->addMenu(tr("File"));
    cameraMenu = menuBarPtr->addMenu(tr("Session"));
    runMenu = menuBarPtr->addMenu(tr("Run"));
    listMenu = menuBarPtr->addMenu(tr("List"));
    helpMenu = menuBarPtr->addMenu(tr("Help"));
    fileMenu_prop = new QAction(tr("Properties"), this);
    fileMenu->addAction(fileMenu_prop);
    connect(fileMenu_prop, &QAction::triggered, this, &MainWindow::fileOpenProp);
    fileMenu->addSeparator();
    fileMenu_exit = new QAction(tr("Exit"), this);
    connect(fileMenu_exit, &QAction::triggered, this, &MainWindow::fileExit);
    fileMenu->addAction(fileMenu_exit);
    cameraSet = new QAction(tr("Session Properties"), this);
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
    listMenu_new = new QAction(tr("New Shader Library"), this);
    connect(listMenu_new,  &QAction::triggered, this, &MainWindow::newList);
    listMenu->addAction(listMenu_new);
    listMenu_shader = new QAction(tr("New Shader GLSL File"), this);
    connect(listMenu_shader,  &QAction::triggered, this, &MainWindow::newShader);
    listMenu->addAction(listMenu_shader);
    listMenu->addSeparator();
    listMenu_remove = new QAction(tr("Remove Shader"), this);
    connect(listMenu_remove, &QAction::triggered, this, &MainWindow::menuRemove);
    listMenu->addAction(listMenu_remove);
    listMenu->addSeparator();
    listMenu_up = new QAction(tr("Shift Shader Up"), this);
    connect(listMenu_up,  &QAction::triggered, this, &MainWindow::menuUp);
    listMenu->addAction(listMenu_up);
    listMenu_down = new QAction(tr("Shift Shader Down"), this);
    connect(listMenu_down,  &QAction::triggered, this, &MainWindow::menuDown);
    listMenu->addAction(listMenu_down);
    helpMenu_about = new QAction("About", this);
    connect(helpMenu_about, &QAction::triggered, this, [=](){
        QMessageBox::information(this, "About", "ACMX2 - (C) 2025 LostSideDead Software\n");
    });
    helpMenu->addAction(helpMenu_about);
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
    prefix_path = appSettings.value("prefix_path", "ACMX2_Snapshot").toString();
    if(!path.isEmpty()) {
        shader_path = path;
        loadShaders(path);
    }
}

void MainWindow::newList() {
    LibraryWindow library(this);

    if(library.exec() == QDialog::Accepted) {
        shader_path = library.getShaderPath();
        loadShaders(shader_path);
        QSettings appSettings("LostSideDead");
        appSettings.setValue("shaders", shader_path);
    }
}

void MainWindow::newShader() {
    ShaderDialog new_shader(this);
    new_shader.setShaderPath(shader_path);
    if(new_shader.exec() == QDialog::Accepted) {
        loadShaders(shader_path);
        QSettings appSettings("LostSideDead");
        appSettings.setValue("shaders", shader_path);
    }
}

void MainWindow::menuRemove() {
    QModelIndex currentIndex = list_view->selectionModel()->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }
    model->removeRow(currentIndex.row());
    updateIndex();
}

void MainWindow::updateIndex() {
     QFile file(shader_path + "/index.txt");
     if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        QAbstractItemModel *model = list_view->model();
        if (!model) {
            return;
        }
        int rowCount = model->rowCount();
        for (int row = 0; row < rowCount; ++row) {
            QModelIndex index = model->index(row, 0);
            QVariant data = model->data(index, Qt::DisplayRole);
            out << data.toString() << "\n"; 
        }
        file.close();   
    } else {
        
    }
}

void MainWindow::menuUp() {
    QStringListModel *model = qobject_cast<QStringListModel *>(list_view->model());
    if (!model) {
        QMessageBox::warning(list_view, "Error", "The model is not a QStringListModel.");
        return;
    }
    QModelIndex currentIndex = list_view->selectionModel()->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }
    int currentRow = currentIndex.row();
    if (currentRow == 0) {
        return;
    }
    QStringList items = model->stringList();
    items.swapItemsAt(currentRow, currentRow - 1);
    model->setStringList(items);
    QModelIndex newIndex = model->index(currentRow - 1);
    list_view->selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select);
    updateIndex();
}

void MainWindow::menuDown() {
    QStringListModel *model = qobject_cast<QStringListModel *>(list_view->model());
    if (!model) {
        return;
    }
    QModelIndex currentIndex = list_view->selectionModel()->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }
    int currentRow = currentIndex.row();
    int rowCount = model->rowCount();

    if (currentRow >= rowCount - 1) {
        return;
    }

    QStringList items = model->stringList();
    items.swapItemsAt(currentRow, currentRow + 1);
    model->setStringList(items);
    QModelIndex newIndex = model->index(currentRow + 1);
    list_view->selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::Select);
    updateIndex();
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
        QString prefix = propWindow.screenshotDirLineEdit->text();

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
            Log("Prefix Path: " + prefix);
            Log("Shader Directory: " + shaderDir + "<br>");
            executable_path = exePath;
            shader_path = shaderDir;
            prefix_path = prefix;
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
    QString scr_res;
    QTextStream stream_r(&scr_res);
    stream_r << screen_res.width() << "x" << screen_res.height();
    if(video_file.isEmpty()) {
        arguments << "--camera-res" << res;
        if(screen_res.width() != 0)
            arguments << "--resolution" << scr_res;
        arguments << "--device" << QString::number(camera_index);
    } else {
        arguments << "--input" << video_file;
        if(screen_res.width() != 0)
            arguments << "--resolution" << scr_res;
    }
    arguments << "--prefix" << prefix_path;
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
    QString scr_res;
    QTextStream stream_r(&scr_res);
    stream_r << screen_res.width() << "x" << screen_res.height();
    if(video_file.isEmpty()) {
        arguments << "--camera-res" << res;
        if(screen_res.width() != 0)
            arguments << "--resolution" << scr_res;
        arguments << "--device" << QString::number(camera_index);
    } else {
        arguments << "--input" << video_file;
        if(screen_res.width() != 0)
            arguments << "--resolution" << scr_res;
    }
    Log("shell: acmx2 " + concatList(arguments) + "<br>");
    process->start(executable_path, arguments);
    if(!process->waitForStarted()) {
        Log("<b style='color:red;'>Failed to start the program.</b>");
        QMessageBox::critical(this, "Error", "Failed to start the program.");
    }
}

void MainWindow::cameraSettings() {
    SettingsWindow settingsWindow(this);
    if(settingsWindow.exec() == QDialog::Accepted) {
        if (settingsWindow.isUsingVideoFile()) {
            QString videoFile = settingsWindow.getSelectedVideoFile();
            QSize screenResolution = settingsWindow.getSelectedScreenResolution();
            screen_res = screenResolution;
            video_file = videoFile;
        } else {
            int cameraIndex = settingsWindow.getSelectedCameraIndex();
            QSize cameraResolution = settingsWindow.getSelectedCameraResolution();
            QSize screenResolution = settingsWindow.getSelectedScreenResolution();
            screen_res = screenResolution;
            camera_index  = cameraIndex;
            video_file = "";
            camera_res = cameraResolution;
        }
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