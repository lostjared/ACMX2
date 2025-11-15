#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QCheckBox>
#include <QFileDialog>

class SettingsWindow : public QDialog {
    Q_OBJECT
public:
    explicit SettingsWindow(QWidget *parent = nullptr);
    int getSelectedCameraIndex() const;
    QSize getSelectedCameraResolution() const;
    QSize getSelectedScreenResolution() const;
    int getCameraFPS() const;
    int getSaveFileKbps() const;
    QString getInputVideoFile() const;
    QString getOutputVideoFile() const;
    QString getGraphicsFile() const;
    bool isUsingInputVideoFile() const;
    bool isUsingGraphicsFile() const;
    bool isSavingToOutputVideoFile() const;
    bool isTextureCacheEnabled() const;
    int getCacheDelay() const;
    bool isFullscreen() const;
    bool isCopyAudioEnabled() const;
    bool is3dEnabled() const;
    QString getModelFile() const;

private slots:
    void acceptSettings();
    void rejectSettings();
    void browseInputVideoFile();
    void browseOutputVideoFile();
    void browseGraphicsFile();
    void browseModelFile();

private:
    void init();

    QComboBox *cameraIndexComboBox;
    QComboBox *cameraResolutionComboBox;
    QComboBox *screenResolutionComboBox;
    QSpinBox *cameraFPSSpinBox;
    QSpinBox *saveFileKbpsSpinBox;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QLineEdit *inputVideoFileLineEdit;
    QPushButton *browseInputVideoButton;
    QLineEdit *outputVideoFileLineEdit;
    QPushButton *browseOutputVideoButton;
    QLineEdit *graphicsFileLineEdit;
    QPushButton *browseGraphicsButton;
    QRadioButton *cameraOptionRadioButton;
    QRadioButton *inputVideoOptionRadioButton;
    QRadioButton *graphicsFileOptionRadioButton;
    QCheckBox *saveOutputVideoCheckBox;
    QCheckBox *textureCacheCheckBox;
    QSpinBox *cacheDelaySpinBox;
    QCheckBox *fullscreenCheckBox;
    QCheckBox *copyAudioCheckBox;
    QCheckBox *enable3dCheckBox;
    QLineEdit *modelFileLineEdit;
    QPushButton *browseModelButton;

    int selectedCameraIndex;
    QSize selectedCameraResolution;
    QSize selectedScreenResolution;
    int cameraFPS;
    int saveFileKbps;
    QString inputVideoFile;
    QString outputVideoFile;
    bool useInputVideoFile;
    bool useGraphicsFile;
    bool saveOutputVideoFile;
    QString graphicsFile;
    int graphicsDuration;
    QString modelFile;
};

#endif
