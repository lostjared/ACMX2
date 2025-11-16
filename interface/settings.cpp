#include "settings.hpp"
#include<QMessageBox>

SettingsWindow::SettingsWindow(QWidget *parent)
    : QDialog(parent),
      selectedCameraIndex(0),
      selectedCameraResolution(640, 480),
      selectedScreenResolution(1280, 720),
      cameraFPS(30),
      saveFileKbps(23),
      inputVideoFile(""),
      outputVideoFile(""),
      useInputVideoFile(false),
      useGraphicsFile(false),
      saveOutputVideoFile(false),
      graphicsFile(""),
      graphicsDuration(10),
      modelFile("data/cube.mxmod.z") {
    init();
}

void SettingsWindow::init() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    cameraOptionRadioButton = new QRadioButton("Use Camera", this);
    inputVideoOptionRadioButton = new QRadioButton("Use Video File as Input", this);
    graphicsFileOptionRadioButton = new QRadioButton("Use Graphics File as Input", this);
    cameraOptionRadioButton->setChecked(true);
    mainLayout->addWidget(cameraOptionRadioButton);
    mainLayout->addWidget(inputVideoOptionRadioButton);
    mainLayout->addWidget(graphicsFileOptionRadioButton);
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

    QLabel *cameraFPSLabel = new QLabel("Set FPS:", this);
    cameraFPSSpinBox = new QSpinBox(this);
    cameraFPSSpinBox->setRange(1, 120);
    cameraFPSSpinBox->setValue(24);

    
    QHBoxLayout *inputVideoFileLayout = new QHBoxLayout;
    inputVideoFileLineEdit = new QLineEdit(this);
    inputVideoFileLineEdit->setReadOnly(true);
    browseInputVideoButton = new QPushButton("Browse", this);
    inputVideoFileLayout->addWidget(inputVideoFileLineEdit);
    inputVideoFileLayout->addWidget(browseInputVideoButton);

    QHBoxLayout *graphicsFileLayout = new QHBoxLayout;
    graphicsFileLineEdit = new QLineEdit(this);
    graphicsFileLineEdit->setReadOnly(true);
    browseGraphicsButton = new QPushButton("Browse", this);
    graphicsFileLayout->addWidget(graphicsFileLineEdit);
    graphicsFileLayout->addWidget(browseGraphicsButton);
    
    saveOutputVideoCheckBox = new QCheckBox("Save Output to Video File", this);

    QHBoxLayout *outputVideoFileLayout = new QHBoxLayout;
    outputVideoFileLineEdit = new QLineEdit(this);
    outputVideoFileLineEdit->setReadOnly(true);
    browseOutputVideoButton = new QPushButton("Browse", this);
    outputVideoFileLayout->addWidget(outputVideoFileLineEdit);
    outputVideoFileLayout->addWidget(browseOutputVideoButton);

    QLabel *saveFileKbpsLabel = new QLabel("Set Save File CRF:", this);
    saveFileKbpsSpinBox = new QSpinBox(this);
    saveFileKbpsSpinBox->setRange(0, 51);
    saveFileKbpsSpinBox->setValue(23);

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

    
    connect(cameraOptionRadioButton, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            cameraIndexComboBox->setEnabled(true);
            cameraResolutionComboBox->setEnabled(true);
            cameraFPSSpinBox->setEnabled(true);
            inputVideoFileLineEdit->setEnabled(false);
            browseInputVideoButton->setEnabled(false);
            graphicsFileLineEdit->setEnabled(false);
            browseGraphicsButton->setEnabled(false);
            textureCacheCheckBox->setEnabled(false);
            cacheDelaySpinBox->setEnabled(false);
        }
    });

    connect(inputVideoOptionRadioButton, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            cameraIndexComboBox->setEnabled(false);
            cameraResolutionComboBox->setEnabled(false);
            cameraFPSSpinBox->setEnabled(true);
            inputVideoFileLineEdit->setEnabled(true);
            browseInputVideoButton->setEnabled(true);
            graphicsFileLineEdit->setEnabled(false);
            browseGraphicsButton->setEnabled(false);
            textureCacheCheckBox->setEnabled(true);
            cacheDelaySpinBox->setEnabled(textureCacheCheckBox->isChecked());
        }
    });

    connect(graphicsFileOptionRadioButton, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            cameraIndexComboBox->setEnabled(false);
            cameraResolutionComboBox->setEnabled(false);
            cameraFPSSpinBox->setEnabled(true);
            inputVideoFileLineEdit->setEnabled(false);
            browseInputVideoButton->setEnabled(false);
            graphicsFileLineEdit->setEnabled(true);
            browseGraphicsButton->setEnabled(true);
            textureCacheCheckBox->setEnabled(false);
            cacheDelaySpinBox->setEnabled(false);
        }
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
    mainLayout->addLayout(graphicsFileLayout);
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

    QHBoxLayout *modelFileLayout = new QHBoxLayout;
    modelFileLineEdit = new QLineEdit(this);
    modelFileLineEdit->setText("data/cube.mxmod.z"); 
    modelFileLineEdit->setReadOnly(true);
    modelFileLineEdit->setEnabled(false);
    browseModelButton = new QPushButton("Model", this);
    browseModelButton->setEnabled(false);
    modelFileLayout->addWidget(modelFileLineEdit);
    modelFileLayout->addWidget(browseModelButton);

    connect(enable3dCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        modelFileLineEdit->setEnabled(checked);
        browseModelButton->setEnabled(checked);
        if (!checked) {
            modelFileLineEdit->clear();
        }
    });

    mainLayout->addLayout(outputVideoFileLayout);
    mainLayout->addWidget(saveFileKbpsLabel);
    mainLayout->addWidget(saveFileKbpsSpinBox);
    mainLayout->addWidget(screenResolutionLabel);
    mainLayout->addWidget(screenResolutionComboBox);
    copyAudioCheckBox->setEnabled(false);
    mainLayout->addLayout(textureCacheLayout);
    mainLayout->addLayout(fullScreenLayout);
    mainLayout->addLayout(enable3d_layout);
    mainLayout->addLayout(modelFileLayout);
    mainLayout->addLayout(buttonLayout);
 
  
    setLayout(mainLayout);
    setWindowTitle("Settings");

    connect(okButton, &QPushButton::clicked, this, &SettingsWindow::acceptSettings);
    connect(cancelButton, &QPushButton::clicked, this, &SettingsWindow::rejectSettings);
    connect(browseInputVideoButton, &QPushButton::clicked, this, &SettingsWindow::browseInputVideoFile);
    connect(browseOutputVideoButton, &QPushButton::clicked, this, &SettingsWindow::browseOutputVideoFile);
    connect(browseGraphicsButton, &QPushButton::clicked, this, &SettingsWindow::browseGraphicsFile);
    connect(browseModelButton, &QPushButton::clicked, this, &SettingsWindow::browseModelFile);

    
    inputVideoFileLineEdit->setEnabled(false);
    browseInputVideoButton->setEnabled(false);
    graphicsFileLineEdit->setEnabled(false);
    browseGraphicsButton->setEnabled(false);
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

QString SettingsWindow::getGraphicsFile() const {
    return graphicsFile;
}

bool SettingsWindow::isUsingInputVideoFile() const {
    return useInputVideoFile;
}

bool SettingsWindow::isUsingGraphicsFile() const {
    return useGraphicsFile;
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

QString SettingsWindow::getModelFile() const {
    return modelFile;
}

void SettingsWindow::acceptSettings() {
    useInputVideoFile = inputVideoOptionRadioButton->isChecked();
    useGraphicsFile = graphicsFileOptionRadioButton->isChecked();
    saveOutputVideoFile = saveOutputVideoCheckBox->isChecked();

    if (useInputVideoFile) {
        if(inputVideoFileLineEdit->text().isEmpty()) {
            QMessageBox::information(this, "Video file required", "When using video file mode, a selected video file is required");
            return;
        }
        inputVideoFile = inputVideoFileLineEdit->text();
    } else if (useGraphicsFile) {
        if(graphicsFileLineEdit->text().isEmpty()) {
            QMessageBox::information(this, "Graphics file required", "When using graphics file mode, a selected graphics file is required");
            return;
        }
        graphicsFile = graphicsFileLineEdit->text();
    } else {
        
        selectedCameraIndex = cameraIndexComboBox->currentText().toInt();
        QStringList cameraResParts = cameraResolutionComboBox->currentText().split('x');
        if (cameraResParts.size() == 2) {
            selectedCameraResolution = QSize(cameraResParts[0].toInt(), cameraResParts[1].toInt());
        }
    }

    cameraFPS = cameraFPSSpinBox->value();

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
    
    
    if (enable3dCheckBox->isChecked()) {
        modelFile = modelFileLineEdit->text();
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

void SettingsWindow::browseGraphicsFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Select Graphics File", "", "Image Files (*.jpg *.jpeg *.png *.bmp *.tiff *.gif)");
    if (!fileName.isEmpty()) {
        graphicsFileLineEdit->setText(fileName);
    }
}

void SettingsWindow::browseModelFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Select 3D Model File", "", "Model Files (*.mxmod *.mxmod.z)");
    if (!fileName.isEmpty()) {
        modelFileLineEdit->setText(fileName);
    }
}
