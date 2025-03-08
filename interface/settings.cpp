#include "settings.hpp"
#include<QMessageBox>

SettingsWindow::SettingsWindow(QWidget *parent)
    : QDialog(parent),
      selectedCameraIndex(0),
      selectedCameraResolution(640, 480),
      selectedScreenResolution(1280, 720),
      cameraFPS(30),
      saveFileKbps(2500),
      useInputVideoFile(false),
      saveOutputVideoFile(false) {
    init();
}

void SettingsWindow::init() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    cameraOptionRadioButton = new QRadioButton("Use Camera", this);
    inputVideoOptionRadioButton = new QRadioButton("Use Video File as Input", this);
    cameraOptionRadioButton->setChecked(true);

    mainLayout->addWidget(cameraOptionRadioButton);
    mainLayout->addWidget(inputVideoOptionRadioButton);

    QLabel *cameraIndexLabel = new QLabel("Select Camera Index:", this);
    cameraIndexComboBox = new QComboBox(this);
    for (int i = 0; i <= 9; ++i) {
        cameraIndexComboBox->addItem(QString::number(i));
    }

    QLabel *cameraResolutionLabel = new QLabel("Select Camera Resolution:", this);
    cameraResolutionComboBox = new QComboBox(this);
    QStringList cameraResolutions = {
        "Default", "640x360", "640x480", "720x480", "480x720", "960x720", "1280x720", "720x1280", "1440x1080", 
        "1920x1080", "1080x1920", "2560x1440", "1440x2560", "3840x2160", "2160x3840"
    };
    cameraResolutionComboBox->addItems(cameraResolutions);
    cameraResolutionComboBox->setCurrentIndex(6);

    QLabel *cameraFPSLabel = new QLabel("Set Camera FPS:", this);
    cameraFPSSpinBox = new QSpinBox(this);
    cameraFPSSpinBox->setRange(1, 120);
    cameraFPSSpinBox->setValue(24);

    QHBoxLayout *inputVideoFileLayout = new QHBoxLayout;
    inputVideoFileLineEdit = new QLineEdit(this);
    inputVideoFileLineEdit->setReadOnly(true);
    browseInputVideoButton = new QPushButton("Browse", this);
    inputVideoFileLayout->addWidget(inputVideoFileLineEdit);
    inputVideoFileLayout->addWidget(browseInputVideoButton);

    saveOutputVideoCheckBox = new QCheckBox("Save Output to Video File", this);

    QHBoxLayout *outputVideoFileLayout = new QHBoxLayout;
    outputVideoFileLineEdit = new QLineEdit(this);
    outputVideoFileLineEdit->setReadOnly(true);
    browseOutputVideoButton = new QPushButton("Browse", this);
    outputVideoFileLayout->addWidget(outputVideoFileLineEdit);
    outputVideoFileLayout->addWidget(browseOutputVideoButton);

    QLabel *saveFileKbpsLabel = new QLabel("Set Save File Kbps:", this);
    saveFileKbpsSpinBox = new QSpinBox(this);
    saveFileKbpsSpinBox->setRange(100, 50000);
    saveFileKbpsSpinBox->setValue(10000);

    QLabel *screenResolutionLabel = new QLabel("Select Screen Resolution:", this);
    screenResolutionComboBox = new QComboBox(this);
    QStringList screenResolutions = {
        "Default", "640x360", "640x480", "720x480", "480x720", "960x720", "1280x720", "720x1280", "1440x1080", 
        "1920x1080", "1080x1920", "2560x1440", "1440x2560", "3840x2160", "2160x3840"
    };
    screenResolutionComboBox->addItems(screenResolutions);
    screenResolutionComboBox->setCurrentIndex(0);

    textureCacheCheckBox = new QCheckBox("Enable Texture Cache", this);
    cacheDelaySpinBox = new QSpinBox(this);
    cacheDelaySpinBox->setRange(1, 8); 
    cacheDelaySpinBox->setValue(1);
    cacheDelaySpinBox->setEnabled(false);
    textureCacheCheckBox->setEnabled(false);

    connect(textureCacheCheckBox, &QCheckBox::toggled, cacheDelaySpinBox, &QSpinBox::setEnabled);

    connect(inputVideoOptionRadioButton, &QRadioButton::toggled, this, [this](bool checked) {
        cameraIndexComboBox->setEnabled(!checked);
        cameraResolutionComboBox->setEnabled(!checked);
        cameraFPSSpinBox->setEnabled(!checked);
        inputVideoFileLineEdit->setEnabled(checked);
        browseInputVideoButton->setEnabled(checked);
        browseOutputVideoButton->setEnabled(checked);
        textureCacheCheckBox->setEnabled(checked);
        cacheDelaySpinBox->setEnabled(checked && textureCacheCheckBox->isChecked());
    });

    connect(saveOutputVideoCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        outputVideoFileLineEdit->setEnabled(checked);
        browseOutputVideoButton->setEnabled(checked);
        saveFileKbpsSpinBox->setEnabled(checked);
    });

    QHBoxLayout *textureCacheLayout = new QHBoxLayout;
    textureCacheLayout->addWidget(textureCacheCheckBox);
    textureCacheLayout->addWidget(cacheDelaySpinBox);
    QHBoxLayout *fullScreenLayout = new QHBoxLayout;
    fullscreenCheckBox = new QCheckBox("Fullscreen", this);
    fullScreenLayout->addWidget(fullscreenCheckBox);
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    okButton = new QPushButton("OK", this);
    cancelButton = new QPushButton("Cancel", this);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addWidget(cameraIndexLabel);
    mainLayout->addWidget(cameraIndexComboBox);
    mainLayout->addWidget(cameraResolutionLabel);
    mainLayout->addWidget(cameraResolutionComboBox);
    mainLayout->addWidget(cameraFPSLabel);
    mainLayout->addWidget(cameraFPSSpinBox);
    mainLayout->addLayout(inputVideoFileLayout);
    mainLayout->addWidget(saveOutputVideoCheckBox);
    copyAudioCheckBox = new QCheckBox("Copy Audio Track", this);
    mainLayout->addWidget(copyAudioCheckBox);
    copyAudioCheckBox->setChecked(false);
    connect(inputVideoOptionRadioButton, &QRadioButton::toggled, this, [this](bool checked) {
        bool enableAudio = checked && saveOutputVideoCheckBox->isChecked();
        copyAudioCheckBox->setEnabled(enableAudio);
        if (!enableAudio) {
            copyAudioCheckBox->setChecked(false);
        }
    });
    connect(saveOutputVideoCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        bool enableAudio = checked && inputVideoOptionRadioButton->isChecked();
        copyAudioCheckBox->setEnabled(enableAudio);
        if (!enableAudio) {
            copyAudioCheckBox->setChecked(false);
        }
    });

    QHBoxLayout *enable3d_layout = new QHBoxLayout;
    enable3dCheckBox = new QCheckBox("Enable 3D", this);
    enable3dCheckBox->setChecked(false);
    enable3d_layout->addWidget(enable3dCheckBox);
    

    mainLayout->addLayout(outputVideoFileLayout);
    mainLayout->addWidget(saveFileKbpsLabel);
    mainLayout->addWidget(saveFileKbpsSpinBox);
    mainLayout->addWidget(screenResolutionLabel);
    mainLayout->addWidget(screenResolutionComboBox);
    copyAudioCheckBox->setEnabled(false);
    connect(saveOutputVideoCheckBox, &QCheckBox::toggled, copyAudioCheckBox, &QCheckBox::setEnabled);
    mainLayout->addLayout(textureCacheLayout);
    mainLayout->addLayout(fullScreenLayout);
    mainLayout->addLayout(enable3d_layout);        
    mainLayout->addLayout(buttonLayout);
 
  
    setLayout(mainLayout);
    setWindowTitle("Settings");

    connect(okButton, &QPushButton::clicked, this, &SettingsWindow::acceptSettings);
    connect(cancelButton, &QPushButton::clicked, this, &SettingsWindow::rejectSettings);
    connect(browseInputVideoButton, &QPushButton::clicked, this, &SettingsWindow::browseInputVideoFile);
    connect(browseOutputVideoButton, &QPushButton::clicked, this, &SettingsWindow::browseOutputVideoFile);

    inputVideoFileLineEdit->setEnabled(false);
    browseInputVideoButton->setEnabled(false);
    outputVideoFileLineEdit->setEnabled(false);
    browseOutputVideoButton->setEnabled(false);
    saveFileKbpsSpinBox->setEnabled(false);
}

bool SettingsWindow::is3dEnabled() const {
    return enable3dCheckBox->isChecked();
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

int SettingsWindow::getCameraFPS() const {
    return cameraFPS;
}

int SettingsWindow::getSaveFileKbps() const {
    return saveFileKbps;
}

QString SettingsWindow::getInputVideoFile() const {
    return inputVideoFile;
}

QString SettingsWindow::getOutputVideoFile() const {
    return outputVideoFile;
}

bool SettingsWindow::isUsingInputVideoFile() const {
    return useInputVideoFile;
}

bool SettingsWindow::isSavingToOutputVideoFile() const {
    return saveOutputVideoFile;
}

bool SettingsWindow::isTextureCacheEnabled() const {
    return textureCacheCheckBox->isChecked();
}

int SettingsWindow::getCacheDelay() const {
    return cacheDelaySpinBox->value();
}

bool SettingsWindow::isFullscreen() const {
    return fullscreenCheckBox->isChecked();
}

bool SettingsWindow::isCopyAudioEnabled() const {
    return copyAudioCheckBox->isChecked();
}

void SettingsWindow::acceptSettings() {
    useInputVideoFile = inputVideoOptionRadioButton->isChecked();
    saveOutputVideoFile = saveOutputVideoCheckBox->isChecked();

    if (useInputVideoFile) {
        
        if(inputVideoFileLineEdit->text().isEmpty()) {
            QMessageBox::information(this, "Video file required", "When using video file mode, a selected video file is required");
            return;
        }

        inputVideoFile = inputVideoFileLineEdit->text();

    } else {
        selectedCameraIndex = cameraIndexComboBox->currentText().toInt();
        QStringList cameraResParts = cameraResolutionComboBox->currentText().split('x');
        if (cameraResParts.size() == 2) {
            selectedCameraResolution = QSize(cameraResParts[0].toInt(), cameraResParts[1].toInt());
        }
        cameraFPS = cameraFPSSpinBox->value();
    }

    QStringList screenResParts = screenResolutionComboBox->currentText().split('x');
    if (screenResParts.size() == 2) {
        selectedScreenResolution = QSize(screenResParts[0].toInt(), screenResParts[1].toInt());
    } else {
        selectedScreenResolution = QSize(0, 0);
    }

    if (saveOutputVideoFile) {
        outputVideoFile = outputVideoFileLineEdit->text();
        if(outputVideoFile.isEmpty()) {
            QMessageBox::information(this, "Output required", "Requires you set a output filename");
            reject();
            return;
        }
        saveFileKbps = saveFileKbpsSpinBox->value();
    }
    accept();
}

void SettingsWindow::rejectSettings() {
    reject();
}

void SettingsWindow::browseInputVideoFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Select Input Video File", "", "Video Files (*.mp4 *.avi *.mkv *.mov)");
    if (!fileName.isEmpty()) {
        inputVideoFileLineEdit->setText(fileName);
    }
}

void SettingsWindow::browseOutputVideoFile() {
    QString fileName = QFileDialog::getSaveFileName(this, "Select Output Video File", "", "MP4 Files (*.mp4)");
    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".mp4")) {
            fileName += ".mp4";
        }
        outputVideoFileLineEdit->setText(fileName);
    }
}
