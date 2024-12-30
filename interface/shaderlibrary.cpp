
#include "shaderlibrary.hpp"

LibraryWindow::LibraryWindow(QWidget *parent) : QDialog(parent) {
    init();
}

void LibraryWindow::init() {
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *instructionLabel = new QLabel("Select a folder to create a shader index file:", this);
    layout->addWidget(instructionLabel);

    folderPathEdit = new QLineEdit(this);
    folderPathEdit->setPlaceholderText("Folder path");
    layout->addWidget(folderPathEdit);

    browseButton = new QPushButton("Browse", this);
    connect(browseButton, &QPushButton::clicked, this, &LibraryWindow::onBrowseButtonClicked);
    layout->addWidget(browseButton);

    createDefaultShaderCheckBox = new QCheckBox("Create default shader", this);
    layout->addWidget(createDefaultShaderCheckBox);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    okButton = new QPushButton("OK", this);
    connect(okButton, &QPushButton::clicked, this, &LibraryWindow::onOkButtonClicked);
    buttonLayout->addWidget(okButton);

    cancelButton = new QPushButton("Cancel", this);
    connect(cancelButton, &QPushButton::clicked, this, &LibraryWindow::onCancelButtonClicked);
    buttonLayout->addWidget(cancelButton);

    layout->addLayout(buttonLayout);

    setLayout(layout);
    setWindowTitle("Shader Library Folder Selector");
    resize(400, 200);
}

void LibraryWindow::onBrowseButtonClicked() {
    QString folderPath = QFileDialog::getExistingDirectory(this, "Select Folder");
    if (!folderPath.isEmpty()) {
        folderPathEdit->setText(folderPath + "/shaders");
    }
}

const char *defaultFile = R"(#version 330 core
in vec2 tc;
out vec4 color;
uniform float time_f;
uniform sampler2D samp;
uniform vec2 iResolution;
void main(void) {
    color = texture(samp, tc);
}
)";

void LibraryWindow::onOkButtonClicked() {
    QString folderPath = folderPathEdit->text().trimmed();

    if (folderPath.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please select a folder.");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm", 
        QString("Do you want to create a shader index file in the folder: %1?").arg(folderPath),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
            QDir dir;
            dir.mkpath(folderPath);
            createShaderIndexFile(folderPath);
            path = folderPath;
        
        if (createDefaultShaderCheckBox->isChecked()) {
              QFile file(folderPath + "/default.glsl");
              if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << defaultFile << "\n";
                file.close();
              }
        } else {

            QFile file(folderPath + "/default.glsl");
            if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << "\n";
                file.close();    
            }
        }

        QMessageBox::information(this, "Success", "Shader index file created successfully.");
        accept();
    }
}

QString LibraryWindow::getShaderPath() {
    return path;
}

void LibraryWindow::onCancelButtonClicked() {
    reject();
}

void LibraryWindow::createShaderIndexFile(const QString &folderPath) {
    QFile file(folderPath + "/index.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "default.glsl\n";
        file.close();
    } else {
        QMessageBox::critical(this, "Error", "Failed to create shader index file.");
    }
}
