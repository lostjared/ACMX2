#ifndef SHADER_H
#define SHADER_H

#include <QDialog>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>


class ShaderDialog : public QDialog {
    Q_OBJECT

public:
    ShaderDialog(QWidget *parent = nullptr);
    void setShaderPath(const QString &path);

private:
    QLineEdit *shaderNameEdit;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QCheckBox *defaultCodeCheckBox;
    QString shaderPath;

    void init();
    void createShaderFile(const QString &shaderName, bool includeDefaultCode);
private slots:
    void onOkButtonClicked();
    void onCancelButtonClicked();
};

#endif