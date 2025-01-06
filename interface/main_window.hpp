//#define BUILD_BUNDLE 
//uncomment above if building BUNDLE

#ifndef __APP_WINDOW_H_
#define __APP_WINDOW_H_

#include<QMainWindow>
#include<QListView>
#include<QStringListModel>
#include<QTextEdit>
#include<QMenuBar>
#include<QProcess>
#include<QSettings>
#include"prop.hpp"
#include"editor.hpp"
#include"shader.hpp"
#include"shaderlibrary.hpp"

class ReadOnlyStringListModel : public QStringListModel {
    Q_OBJECT
public:
    using QStringListModel::QStringListModel; 
    Qt::ItemFlags flags(const QModelIndex &index) const override {
        return QStringListModel::flags(index) & ~Qt::ItemIsEditable;
    }
};

class MainWindow : public QMainWindow {
Q_OBJECT
public:
    MainWindow(QWidget *parent = 0) : QMainWindow(parent) {
        initControls();
    }
    void initControls();
    void Log(const QString &message);
    void Write(const QString &message);
    bool loadShaders(const QString &path);
    void updateIndex();
    
public slots:
    void fileOpenProp();
    void fileExit();
    void runSelected();
    void runAll();
    void cameraSettings();
    void listClicked(const QModelIndex &i);
    void newList();
    void newShader();
    void menuUp();
    void menuDown();
    void menuRemove();
protected:

     void closeEvent(QCloseEvent *event) override {
        if (process->state() == QProcess::Running) {
            process->terminate();
            if (!process->waitForFinished(10000)) {
                process->kill(); 
            }
        }
        QMainWindow::closeEvent(event); 
    }
private:
    QListView        *list_view;
    QStringList       items;
    ReadOnlyStringListModel *model;
    QTextEdit   *bottomTextBox;
    QMenu *fileMenu;
    QMenu *cameraMenu;
    QMenu *runMenu;
    QMenu *playbackMenu;
    QMenu *listMenu;
    QMenu *helpMenu;
    QAction *fileMenu_prop, *fileMenu_exit;
    QAction *cameraSet;
    QAction *runMenu_select, *runMenu_all;
    QAction *play_repeat, *play_stop;
    QAction *listMenu_new,*listMenu_shader, *listMenu_remove, *listMenu_up, *listMenu_down;
    QAction *helpMenu_about;
    QString executable_path;
    QString shader_path;
    QProcess *process;
    QSize camera_res, screen_res;
    unsigned int camera_index;
    QString video_file;
    QString prefix_path;
    QString output_file;
    int output_kbps = 10000;
    double output_fps = 24.0f;
    QString concatList(const QStringList lst);
    QVector<TextEditor *> open_files;
    QString readFileContents(const QString &filePath);
};


#endif
