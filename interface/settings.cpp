#include "settings.hpp"

SettingsWindow::SettingsWindow(QWidget *parent) : QDialog(parent), selectedCameraIndex(0), selectedResolution(640, 480) {
    init();
}

void SettingsWindow::init() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QLabel *cameraIndexLabel = new QLabel("Select Camera Index:", this);
    cameraIndexComboBox = new QComboBox(this);
    for (int i = 0; i <= 9; ++i) {
        cameraIndexComboBox->addItem(QString::number(i));
    }
    mainLayout->addWidget(cameraIndexLabel);
    mainLayout->addWidget(cameraIndexComboBox);
    QLabel *resolutionLabel = new QLabel("Select Screen Resolution:", this);
    resolutionComboBox = new QComboBox(this);
    QStringList resolutions = {
        "640x360", "640x480", "960x720", "1280x720", "1440x1080", 
        "1920x1080", "1920x1440", "2560x1440", "3840x2160" 
    };
    resolutionComboBox->addItems(resolutions);
    resolutionComboBox->setCurrentIndex(3);
    mainLayout->addWidget(resolutionLabel);
    mainLayout->addWidget(resolutionComboBox);
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    okButton = new QPushButton("OK", this);
    cancelButton = new QPushButton("Cancel", this);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);
    connect(okButton, &QPushButton::clicked, this, &SettingsWindow::acceptSettings);
    connect(cancelButton, &QPushButton::clicked, this, &SettingsWindow::rejectSettings);
    setLayout(mainLayout);
    setWindowTitle("Settings");
}

int SettingsWindow::getSelectedCameraIndex() const {
    return selectedCameraIndex;
}

QSize SettingsWindow::getSelectedResolution() const {
    return selectedResolution;
}

void SettingsWindow::acceptSettings() {
    selectedCameraIndex = cameraIndexComboBox->currentText().toInt();

    QStringList resolutionParts = resolutionComboBox->currentText().split('x');
    if (resolutionParts.size() == 2) {
        selectedResolution = QSize(resolutionParts[0].toInt(), resolutionParts[1].toInt());
    }

    accept();
}

void SettingsWindow::rejectSettings() {
    reject();
}
