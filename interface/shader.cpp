#include"shader.hpp"

ShaderDialog::ShaderDialog(QWidget *parent) : QDialog(parent) {
    init();
}

void ShaderDialog::init() {
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *instructionLabel = new QLabel("Enter the name of the shader file:", this);
    layout->addWidget(instructionLabel);

    shaderNameEdit = new QLineEdit(this);
    shaderNameEdit->setPlaceholderText("Shader name (e.g., myshader.glsl)");
    layout->addWidget(shaderNameEdit);

    defaultCodeCheckBox = new QCheckBox("Include default shader code", this);
    layout->addWidget(defaultCodeCheckBox);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    okButton = new QPushButton("OK", this);
    connect(okButton, &QPushButton::clicked, this, &ShaderDialog::onOkButtonClicked);
    buttonLayout->addWidget(okButton);

    cancelButton = new QPushButton("Cancel", this);
    connect(cancelButton, &QPushButton::clicked, this, &ShaderDialog::onCancelButtonClicked);
    buttonLayout->addWidget(cancelButton);

    layout->addLayout(buttonLayout);

    setLayout(layout);
    setWindowTitle("Create New Shader");
    resize(400, 150);
}

void ShaderDialog::onOkButtonClicked() {
    QString shaderName = shaderNameEdit->text().trimmed();
    if (shaderName.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter a shader name.");
        return;
    }

    if(!shaderName.contains(".glsl")) {
        shaderName += ".glsl";
    }

    bool includeDefaultCode = defaultCodeCheckBox->isChecked();
    createShaderFile(shaderPath + "/" + shaderName, includeDefaultCode);
    QFile file(shaderPath + "/index.txt");
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << shaderName << "\n";
        file.close();
    } 
    QMessageBox::information(this, "Success", "Shader file created successfully.");
    accept();
}

void ShaderDialog::setShaderPath(const QString &path) {
    shaderPath = path;
}

void ShaderDialog::onCancelButtonClicked() {
    reject();
}

const char *defaultShFile = R"(#version 330 core
in vec2 tc;
out vec4 color;
uniform float time_f;
uniform sampler2D samp;
uniform vec2 iResolution;
uniform vec4 iMouse;
uniform float amp;
uniform float uamp;

void main(void) {
    color = texture(samp, tc);
}
)";


void ShaderDialog::createShaderFile(const QString &shaderName, bool includeDefaultCode) {
    QFile file(shaderName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        if (includeDefaultCode) {
            out << defaultShFile << "\n";;
        }
        file.close();
    } else {
        QMessageBox::critical(this, "Error", "Failed to create shader file.");
    }
}
