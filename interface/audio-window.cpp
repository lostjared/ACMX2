#include "audio-window.hpp"

AudioSettings::AudioSettings(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Audio Settings");

    audioReactivityCheckBox = new QCheckBox("Enable Audio Reactivity", this);

    QLabel *channelLabel = new QLabel("Number of Channels:", this);
    channelSpinBox = new QSpinBox(this);
    channelSpinBox->setRange(1, 32);
    channelSpinBox->setValue(2);

    QLabel *sensitivityLabel = new QLabel("Sensitivity:", this);
    sensitivitySlider = new QSlider(Qt::Horizontal, this);
    sensitivitySlider->setRange(1, 100); // Map floating-point range [0.1, 2.0] to integers [1, 20]
    sensitivitySlider->setValue(5);    // Default value for sensitivity (e.g., 0.5)

    QLabel *sensitivityValueLabel = new QLabel("0.5", this); // Label to show floating-point value
    connect(sensitivitySlider, &QSlider::valueChanged, this, [this, sensitivityValueLabel](int value) {
        double floatValue = value / 10.0; // Convert integer value to float
        sensitivityValueLabel->setText(QString::number(floatValue, 'f', 1));
    });

    okButton = new QPushButton("OK", this);
    cancelButton = new QPushButton("Cancel", this);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(audioReactivityCheckBox);

    QHBoxLayout *channelLayout = new QHBoxLayout();
    channelLayout->addWidget(channelLabel);
    channelLayout->addWidget(channelSpinBox);
    mainLayout->addLayout(channelLayout);

    QHBoxLayout *sensitivityLayout = new QHBoxLayout();
    sensitivityLayout->addWidget(sensitivityLabel);
    sensitivityLayout->addWidget(sensitivitySlider);
    sensitivityLayout->addWidget(sensitivityValueLabel); // Show the floating-point value
    mainLayout->addLayout(sensitivityLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

bool AudioSettings::isAudioReactivityEnabled() const {
    return audioReactivityCheckBox->isChecked();
}

int AudioSettings::getNumberOfChannels() const {
    return channelSpinBox->value();
}

double AudioSettings::getSensitivity() const {
    return sensitivitySlider->value() / 10.0; // Convert integer value to float
}
