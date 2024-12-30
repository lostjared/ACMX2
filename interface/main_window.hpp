#ifndef __APP_WINDOW_H_
#define __APP_WINDOW_H_

#include<QMainWindow>
#include<QListView>
#include<QStringListModel>
#include<QTextEdit>
#include<QMenuBar>


class ReadOnlyStringListModel : public QStringListModel {
    Q_OBJECT
public:
    using QStringListModel::QStringListModel; 
    Qt::ItemFlags flags(const QModelIndex &index) const override {
        // Exclude the editable flag
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

public slots:
    void fileOpenProp();
    void fileExit();

private:
    QListView        *list_view;
    QStringList       items;
    ReadOnlyStringListModel *model;
    QTextEdit   *bottomTextBox;
    QMenu *fileMenu;
    QMenu *helpMenu;
    QAction *fileMenu_prop, *fileMenu_exit;
};


#endif