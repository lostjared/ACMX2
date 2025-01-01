#ifndef __SET__H_
#define __SET__H_

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QRadioButton>

class SettingsWindow : public QDialog {
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);
    void init();

    int getSelectedCameraIndex() const;
    QSize getSelectedCameraResolution() const;
    QSize getSelectedScreenResolution() const;
    QString getSelectedVideoFile() const;
    bool isUsingVideoFile() const;

private slots:
    void acceptSettings();
    void rejectSettings();
    void browseVideoFile();

private:
    QRadioButton *cameraOptionRadioButton;
    QRadioButton *videoOptionRadioButton;

    QComboBox *cameraIndexComboBox;
    QComboBox *cameraResolutionComboBox;
    QComboBox *screenResolutionComboBox;

    QLineEdit *videoFileLineEdit;
    QPushButton *browseButton;

    QPushButton *okButton;
    QPushButton *cancelButton;

    int selectedCameraIndex;
    QSize selectedCameraResolution;
    QSize selectedScreenResolution;
    QString selectedVideoFile;
    bool useVideoFile;
};

#endif
