#include "audio-window.hpp"
#ifdef AUDIO_ENABLED
#include <RtAudio.h>
#endif

AudioSettings::AudioSettings(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Audio Settings");

    audioReactivityCheckBox = new QCheckBox("Enable Audio Reactivity", this);
    audioPassThroughCheckBox = new QCheckBox("Enable Audio Pass Through", this); 

    QLabel *channelLabel = new QLabel("Number of Channels:", this);
    channelSpinBox = new QSpinBox(this);
    channelSpinBox->setRange(1, 32);
    channelSpinBox->setValue(2);

    QLabel *sensitivityLabel = new QLabel("Sensitivity:", this);
    sensitivitySlider = new QSlider(Qt::Horizontal, this);
    sensitivitySlider->setRange(1, 200);
    sensitivitySlider->setValue(5);    

    QLabel *sensitivityValueLabel = new QLabel("0.5", this); 
    connect(sensitivitySlider, &QSlider::valueChanged, this, [this, sensitivityValueLabel](int value) {
        double floatValue = value / 10.0; 
        sensitivityValueLabel->setText(QString::number(floatValue, 'f', 1));
    });

    
    QLabel *inputDeviceLabel = new QLabel("Input Device:", this);
    inputDeviceComboBox = new QComboBox(this);
    
    
    QLabel *outputDeviceLabel = new QLabel("Output Device:", this);
    outputDeviceComboBox = new QComboBox(this);
    
    
    populateAudioDevices();

    okButton = new QPushButton("OK", this);
    cancelButton = new QPushButton("Cancel", this);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(audioReactivityCheckBox);
    mainLayout->addWidget(audioPassThroughCheckBox); 

    QHBoxLayout *channelLayout = new QHBoxLayout();
    channelLayout->addWidget(channelLabel);
    channelLayout->addWidget(channelSpinBox);
    mainLayout->addLayout(channelLayout);

    QHBoxLayout *sensitivityLayout = new QHBoxLayout();
    sensitivityLayout->addWidget(sensitivityLabel);
    sensitivityLayout->addWidget(sensitivitySlider);
    sensitivityLayout->addWidget(sensitivityValueLabel); 
    mainLayout->addLayout(sensitivityLayout);

    
    QHBoxLayout *inputDeviceLayout = new QHBoxLayout();
    inputDeviceLayout->addWidget(inputDeviceLabel);
    inputDeviceLayout->addWidget(inputDeviceComboBox);
    mainLayout->addLayout(inputDeviceLayout);

    
    QHBoxLayout *outputDeviceLayout = new QHBoxLayout();
    outputDeviceLayout->addWidget(outputDeviceLabel);
    outputDeviceLayout->addWidget(outputDeviceComboBox);
    mainLayout->addLayout(outputDeviceLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void AudioSettings::populateAudioDevices() {
    inputDeviceComboBox->addItem("Default", -1);
    outputDeviceComboBox->addItem("Default", -1);
    for(int i = 0; i < 10; i++) {
        inputDeviceComboBox->addItem(QString::number(i), i);
        outputDeviceComboBox->addItem(QString::number(i), i);
    }
    inputDeviceComboBox->setCurrentIndex(0);
    outputDeviceComboBox->setCurrentIndex(0);
}

bool AudioSettings::isAudioReactivityEnabled() const {
    return audioReactivityCheckBox->isChecked();
}

bool AudioSettings::isAudioPassThroughEnabled() const { 
    return audioPassThroughCheckBox->isChecked();
}

int AudioSettings::getNumberOfChannels() const {
    return channelSpinBox->value();
}

double AudioSettings::getSensitivity() const {
    return sensitivitySlider->value() / 10.0; 
}

int AudioSettings::getInputDeviceIndex() const {
    return inputDeviceComboBox->currentData().toInt();
}

int AudioSettings::getOutputDeviceIndex() const {
    return outputDeviceComboBox->currentData().toInt();
}
