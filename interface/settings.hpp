#ifndef __SET__H_
#define __SET__H_

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QFileDialog>
#include <QRadioButton>
#include <QSpinBox>

class SettingsWindow : public QDialog {
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);
    void init();

    int getSelectedCameraIndex() const;
    QSize getSelectedCameraResolution() const;
    QSize getSelectedScreenResolution() const;
    int getCameraFPS() const;
    int getSaveFileKbps() const;
    QString getInputVideoFile() const;
    QString getOutputVideoFile() const;
    bool isUsingInputVideoFile() const;
    bool isSavingToOutputVideoFile() const;
    bool isTextureCacheEnabled() const;
    int getCacheDelay() const;
    bool isFullscreen() const;
    bool isCopyAudioEnabled() const;

private slots:
    void acceptSettings();
    void rejectSettings();
    void browseInputVideoFile();
    void browseOutputVideoFile();

private:
    QRadioButton *cameraOptionRadioButton;
    QRadioButton *inputVideoOptionRadioButton;

    QComboBox *cameraIndexComboBox;
    QComboBox *cameraResolutionComboBox;
    QComboBox *screenResolutionComboBox;

    QSpinBox *cameraFPSSpinBox;
    QSpinBox *saveFileKbpsSpinBox;

    QLineEdit *inputVideoFileLineEdit;
    QPushButton *browseInputVideoButton;

    QCheckBox *saveOutputVideoCheckBox;
    QLineEdit *outputVideoFileLineEdit;
    QPushButton *browseOutputVideoButton;

    QPushButton *okButton;
    QPushButton *cancelButton;

    QCheckBox *textureCacheCheckBox;
    QSpinBox *cacheDelaySpinBox;

    QCheckBox *fullscreenCheckBox;
    QCheckBox *copyAudioCheckBox;

    int selectedCameraIndex;
    QSize selectedCameraResolution;
    QSize selectedScreenResolution;
    int cameraFPS;
    int saveFileKbps;
    QString inputVideoFile;
    QString outputVideoFile;
    bool useInputVideoFile;
    bool saveOutputVideoFile;
};

#endif
