#ifndef __APP_WINDOW_H_
#define __APP_WINDOW_H_

#include<QMainWindow>
#include<QListView>
#include<QStringListModel>
#include<QTextEdit>
#include<QMenuBar>
#include<QProcess>
#include"prop.hpp"

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

public slots:
    void fileOpenProp();
    void fileExit();
    void runSelected();
    void runAll();
private:
    QListView        *list_view;
    QStringList       items;
    ReadOnlyStringListModel *model;
    QTextEdit   *bottomTextBox;
    QMenu *fileMenu;
    QMenu *runMenu;
    QMenu *helpMenu;
    QAction *fileMenu_prop, *fileMenu_exit;
    QAction *runMenu_select, *runMenu_all;
    QString executable_path;
    QString shader_path;
    QProcess *process;
};


#endif