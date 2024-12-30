#ifndef __SHADER_LIB_H__Y
#define __SHADER_LIB_H__Y

#include <QDialog>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>

class LibraryWindow : public QDialog {
    Q_OBJECT

public:
    LibraryWindow(QWidget *parent = nullptr);

private:
    QLineEdit *folderPathEdit;
    QPushButton *browseButton;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QCheckBox *createDefaultShaderCheckBox;
    void init();
    void createShaderIndexFile(const QString &folderPath);
public:
    QString getShaderPath();

private slots:
    void onBrowseButtonClicked();
    void onOkButtonClicked();
    void onCancelButtonClicked();
private:
    QString path;
};

#endif 
