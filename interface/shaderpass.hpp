#ifndef __SHADERPASS_HPP__
#define __SHADERPASS_HPP__

#include <QDialog>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QStringList>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

class ShaderPassDialog : public QDialog {
    Q_OBJECT
public:
    explicit ShaderPassDialog(const QStringList &shaderNames, QWidget *parent = nullptr);

    bool isShaderPassEnabled() const;
    QStringList getSelectedShaderIndices() const;
    QString getShaderPassArgument() const;
    QStringList getSelectedShaderNames() const;

    void setEnabled(bool enabled);
    void setSelectedIndices(const QStringList &indices);
    void setSelectedShaderNames(const QStringList &names);
    void updateShaderList(const QStringList &shaderNames);

public slots:
    void addShader();
    void removeShader();
    void moveUp();
    void moveDown();
    void clearAll();
    void filterSearchChanged(const QString &text);

private:
    void setupUI();
    void loadShaders(const QStringList &shaderNames);

    QCheckBox *enableCheckBox;
    QComboBox *shaderComboBox;
    QLineEdit *searchLineEdit;
    QListWidget *selectedShadersList;
    QPushButton *addButton;
    QPushButton *removeButton;
    QPushButton *upButton;
    QPushButton *downButton;
    QPushButton *clearButton;
    QPushButton *okButton;
    QPushButton *cancelButton;

    QStandardItemModel *shaderModel;
    QSortFilterProxyModel *proxyModel;

    QMap<QString, int> shaderNameToIndex;
    QStringList shaderNamesList;
};

#endif
