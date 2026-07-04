#pragma once

#include <QDialog>
#include <QListWidget>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QMap>
#include <QString>
#include <QLineEdit>
#include <QPushButton>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private slots:
    void addImage();
    void removeImage();
    void selectStartupSound();
    void selectSwitchSound();
    void saveSettings();
    void loadSettings();

private:
    QListWidget* m_imageList;
    QSlider* m_opacitySlider;
    QDoubleSpinBox* m_scaleSpin;
    QComboBox* m_modCombo;
    QComboBox* m_prevKeyCombo;
    QComboBox* m_nextKeyCombo;
    
    QLineEdit* m_startupSoundEdit;
    QLineEdit* m_switchSoundEdit;

    QMap<QString, int> keyMap;
    int getKey(const QString& name);
    QString getKeyName(int key);
};
