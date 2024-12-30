#include "prop.hpp"

PropWindow::PropWindow(QWidget *parent) : QDialog(parent) {
    init();
}

void PropWindow::init() {
    setWindowTitle("Properties");
    setFixedSize(400, 200);

    QLabel *exeLabel = new QLabel("Program Executable:");
    exePathLineEdit = new QLineEdit(this);
#ifdef _WIN32
    exePathLineEdit->setText("acmx2.exe");
#else
    exePathLineEdit->setText("acmx2");
#endif
    exePathLineEdit->setReadOnly(true);
    QPushButton *exeBrowseButton = new QPushButton("Browse");

    QLabel *shaderDirLabel = new QLabel("Shader Directory:");
    shaderDirLineEdit = new QLineEdit(this);
    shaderDirLineEdit->setReadOnly(true);
    QPushButton *shaderDirBrowseButton = new QPushButton("Browse");

    QPushButton *okButton = new QPushButton("OK");
    QPushButton *cancelButton = new QPushButton("Cancel");

    QHBoxLayout *exeLayout = new QHBoxLayout();
    exeLayout->addWidget(exePathLineEdit, 1);
    exeLayout->addWidget(exeBrowseButton);

    QHBoxLayout *shaderDirLayout = new QHBoxLayout();
    shaderDirLayout->addWidget(shaderDirLineEdit, 1);
    shaderDirLayout->addWidget(shaderDirBrowseButton);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(exeLabel);
    mainLayout->addLayout(exeLayout);
    mainLayout->addWidget(shaderDirLabel);
    mainLayout->addLayout(shaderDirLayout);
    mainLayout->addStretch(1);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    connect(exeBrowseButton, &QPushButton::clicked, this, &PropWindow::selectExecutable);
    connect(shaderDirBrowseButton, &QPushButton::clicked, this, &PropWindow::selectShaderDirectory);
    connect(okButton, &QPushButton::clicked, this, [this]() {
        QSettings appSettings("LostSideDead");
        appSettings.setValue("exePath", exePathLineEdit->text());
        appSettings.setValue("shaders", shaderDirLineEdit->text());
        accept();
    });
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    QSettings appSettings("LostSideDead");
#ifndef _WIN32
    QString filePath = appSettings.value("exePath", "acmx2").toString();
#else
    QString filePath = appSettings.value("exePath", "acmx2.exe").toString();
#endif    
    QString shader_ = appSettings.value("shaders", "").toString();
    exePathLineEdit->setText(filePath);
    shaderDirLineEdit->setText(shader_);
}

void PropWindow::selectExecutable() {
    QString filePath = QFileDialog::getOpenFileName(
        this, "Select Program Executable", "", "Executable Files (*.exe);;All Files (*)");
    if (!filePath.isEmpty()) {
        exePathLineEdit->setText(filePath);
    }
}

void PropWindow::selectShaderDirectory() {
    QString dirPath = QFileDialog::getExistingDirectory(
        this, "Select Shader Directory", "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dirPath.isEmpty()) {
        shaderDirLineEdit->setText(dirPath);
    }
}
