#include "prop.hpp"

PropWindow::PropWindow(QWidget *parent) : QDialog(parent) {
    init();
}

void PropWindow::init() {
    setWindowTitle("Properties");
    setFixedSize(400, 250);

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

    QLabel *screenshotDirLabel = new QLabel("Screenshot Directory:");
    screenshotDirLineEdit = new QLineEdit(this);
    screenshotDirLineEdit->setReadOnly(true);
    QPushButton *screenshotDirBrowseButton = new QPushButton("Browse");

    QPushButton *okButton = new QPushButton("OK");
    QPushButton *cancelButton = new QPushButton("Cancel");

    QHBoxLayout *exeLayout = new QHBoxLayout();
    exeLayout->addWidget(exePathLineEdit, 1);
    exeLayout->addWidget(exeBrowseButton);

    QHBoxLayout *shaderDirLayout = new QHBoxLayout();
    shaderDirLayout->addWidget(shaderDirLineEdit, 1);
    shaderDirLayout->addWidget(shaderDirBrowseButton);

    QHBoxLayout *screenshotDirLayout = new QHBoxLayout();
    screenshotDirLayout->addWidget(screenshotDirLineEdit, 1);
    screenshotDirLayout->addWidget(screenshotDirBrowseButton);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(exeLabel);
    mainLayout->addLayout(exeLayout);
    mainLayout->addWidget(shaderDirLabel);
    mainLayout->addLayout(shaderDirLayout);
    mainLayout->addWidget(screenshotDirLabel);
    mainLayout->addLayout(screenshotDirLayout);
    mainLayout->addStretch(1);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    connect(exeBrowseButton, &QPushButton::clicked, this, &PropWindow::selectExecutable);
    connect(shaderDirBrowseButton, &QPushButton::clicked, this, &PropWindow::selectShaderDirectory);
    connect(screenshotDirBrowseButton, &QPushButton::clicked, this, &PropWindow::selectScreenshotDirectory);
    connect(okButton, &QPushButton::clicked, this, [this]() {
        QSettings appSettings("LostSideDead");
        appSettings.setValue("exePath", exePathLineEdit->text());
        appSettings.setValue("shaders", shaderDirLineEdit->text());
        appSettings.setValue("prefix_path", screenshotDirLineEdit->text());
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
    QString screenshotDir = appSettings.value("prefix_path", ".").toString();

    exePathLineEdit->setText(filePath);
    shaderDirLineEdit->setText(shader_);
    screenshotDirLineEdit->setText(screenshotDir);
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

void PropWindow::selectScreenshotDirectory() {
    QString dirPath = QFileDialog::getExistingDirectory(
        this, "Select Screenshot Directory", "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dirPath.isEmpty()) {
        screenshotDirLineEdit->setText(dirPath);
    }
}
