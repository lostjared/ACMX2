#ifndef AUDIOSETTINGS_HPP
#define AUDIOSETTINGS_HPP

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>

class AudioSettings : public QDialog {
    Q_OBJECT

public:
    explicit AudioSettings(QWidget *parent = nullptr);

    bool isAudioReactivityEnabled() const;
    bool isAudioPassThroughEnabled() const;
    int getNumberOfChannels() const;
    double getSensitivity() const;
    int getInputDeviceIndex() const;
    int getOutputDeviceIndex() const;

private:
    void populateAudioDevices();

    QCheckBox *audioReactivityCheckBox;
    QCheckBox *audioPassThroughCheckBox;
    QSpinBox *channelSpinBox;
    QSlider *sensitivitySlider;
    QComboBox *inputDeviceComboBox;
    QComboBox *outputDeviceComboBox;
    QPushButton *okButton;
    QPushButton *cancelButton;
};

#endif
