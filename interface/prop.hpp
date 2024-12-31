#ifndef PROP_WINDOW_HPP
#define PROP_WINDOW_HPP

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QSettings>

class PropWindow : public QDialog {
    Q_OBJECT

public:
    PropWindow(QWidget *parent = nullptr);
    void init();

private slots:
    void selectExecutable();
    void selectShaderDirectory();
    void selectScreenshotDirectory();

public:
    QLineEdit *exePathLineEdit;
    QLineEdit *shaderDirLineEdit;
    QLineEdit *screenshotDirLineEdit;
};

#endif