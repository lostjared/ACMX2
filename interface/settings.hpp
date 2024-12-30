#ifndef __SET__H_
#define __SET__H_

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class SettingsWindow : public QDialog {
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);
    void init();

    int getSelectedCameraIndex() const;
    QSize getSelectedResolution() const;

private slots:
    void acceptSettings();
    void rejectSettings();

private:
    QComboBox *cameraIndexComboBox;
    QComboBox *resolutionComboBox;
    QPushButton *okButton;
    QPushButton *cancelButton;

    int selectedCameraIndex;
    QSize selectedResolution;
};

#endif