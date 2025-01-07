#ifndef __AUDIO_WINDOW__H_
#define __AUDIO_WINDOW__H_

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class AudioSettings : public QDialog {
    Q_OBJECT

public:
    explicit AudioSettings(QWidget *parent = nullptr);

    bool isAudioReactivityEnabled() const;
    int getNumberOfChannels() const;
    double getSensitivity() const;

private:
    QCheckBox *audioReactivityCheckBox;
    QSpinBox *channelSpinBox;
    QSlider *sensitivitySlider;
    QPushButton *okButton;
    QPushButton *cancelButton;
};

#endif