#ifndef PROP_HPP
#define PROP_HPP
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>
#include <QDir>

class PropWindow : public QDialog {
    Q_OBJECT
public:
    explicit PropWindow(QWidget *parent = nullptr);

private:
    void init();
    void selectExecutable();
    void selectShaderDirectory();
    void selectScreenshotDirectory();
    void restoreDefaults();
    QString getDefaultPicturesDirectory();  
public:
    QLineEdit *exePathLineEdit;
    QLineEdit *shaderDirLineEdit;
    QLineEdit *screenshotDirLineEdit;
};

#endif 