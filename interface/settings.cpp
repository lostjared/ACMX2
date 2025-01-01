#include "settings.hpp"

SettingsWindow::SettingsWindow(QWidget *parent)
    : QDialog(parent),
      selectedCameraIndex(0),
      selectedCameraResolution(640, 480),
      selectedScreenResolution(1280, 720),
      useVideoFile(false) {
    init();
}

void SettingsWindow::init() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Option to select camera or video file
    cameraOptionRadioButton = new QRadioButton("Use Camera", this);
    videoOptionRadioButton = new QRadioButton("Use Video File", this);
    cameraOptionRadioButton->setChecked(true);

    mainLayout->addWidget(cameraOptionRadioButton);
    mainLayout->addWidget(videoOptionRadioButton);

    // Camera selection
    QLabel *cameraIndexLabel = new QLabel("Select Camera Index:", this);
    cameraIndexComboBox = new QComboBox(this);
    for (int i = 0; i <= 9; ++i) {
        cameraIndexComboBox->addItem(QString::number(i));
    }
    mainLayout->addWidget(cameraIndexLabel);
    mainLayout->addWidget(cameraIndexComboBox);

    // Camera resolution selection
    QLabel *cameraResolutionLabel = new QLabel("Select Camera Resolution:", this);
    cameraResolutionComboBox = new QComboBox(this);
    QStringList cameraResolutions = {
        "640x360", "640x480", "960x720", "1280x720", "1920x1080", "3840x2160"
    };
    cameraResolutionComboBox->addItems(cameraResolutions);
    cameraResolutionComboBox->setCurrentIndex(3); // Default: 1280x720
    mainLayout->addWidget(cameraResolutionLabel);
    mainLayout->addWidget(cameraResolutionComboBox);

    // Video file selection
    QHBoxLayout *videoFileLayout = new QHBoxLayout;
    videoFileLineEdit = new QLineEdit(this);
    browseButton = new QPushButton("Browse", this);
    videoFileLayout->addWidget(videoFileLineEdit);
    videoFileLayout->addWidget(browseButton);

    QLabel *videoFileLabel = new QLabel("Select Video File:", this);
    mainLayout->addWidget(videoFileLabel);
    mainLayout->addLayout(videoFileLayout);

    // Screen resolution selection
    QLabel *screenResolutionLabel = new QLabel("Select Screen Resolution:", this);
    screenResolutionComboBox = new QComboBox(this);
    QStringList screenResolutions = {
        "Default","640x360", "640x480", "960x720", "1280x720", "1440x1080", 
        "1920x1080", "2560x1440", "3840x2160"
    };
    screenResolutionComboBox->addItems(screenResolutions);
    screenResolutionComboBox->setCurrentIndex(0); 
    mainLayout->addWidget(screenResolutionLabel);
    mainLayout->addWidget(screenResolutionComboBox);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    okButton = new QPushButton("OK", this);
    cancelButton = new QPushButton("Cancel", this);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
    setWindowTitle("Settings");

    connect(okButton, &QPushButton::clicked, this, &SettingsWindow::acceptSettings);
    connect(cancelButton, &QPushButton::clicked, this, &SettingsWindow::rejectSettings);
    connect(browseButton, &QPushButton::clicked, this, &SettingsWindow::browseVideoFile);
}

int SettingsWindow::getSelectedCameraIndex() const {
    return selectedCameraIndex;
}

QSize SettingsWindow::getSelectedCameraResolution() const {
    return selectedCameraResolution;
}

QSize SettingsWindow::getSelectedScreenResolution() const {
    return selectedScreenResolution;
}

QString SettingsWindow::getSelectedVideoFile() const {
    return selectedVideoFile;
}

bool SettingsWindow::isUsingVideoFile() const {
    return useVideoFile;
}

void SettingsWindow::acceptSettings() {
    useVideoFile = videoOptionRadioButton->isChecked();

    if (useVideoFile) {
        selectedVideoFile = videoFileLineEdit->text();
    } else {
        selectedCameraIndex = cameraIndexComboBox->currentText().toInt();
        QStringList cameraResParts = cameraResolutionComboBox->currentText().split('x');
        if (cameraResParts.size() == 2) {
            selectedCameraResolution = QSize(cameraResParts[0].toInt(), cameraResParts[1].toInt());
        }
    }

    QStringList screenResParts = screenResolutionComboBox->currentText().split('x');
    if (screenResParts.size() == 2) {
        selectedScreenResolution = QSize(screenResParts[0].toInt(), screenResParts[1].toInt());
    } else {
        selectedScreenResolution = QSize(0, 0);
    }

    accept();
}

void SettingsWindow::rejectSettings() {
    reject();
}

void SettingsWindow::browseVideoFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Select Video File", "", "Video Files (*.mp4 *.avi *.mkv *.mov)");
    if (!fileName.isEmpty()) {
        videoFileLineEdit->setText(fileName);
    }
}
