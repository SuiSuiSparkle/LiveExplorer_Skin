#include "SettingsDialog.h"
#include "Config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <windows.h> // For VK codes

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Settings - FileMngr BG Image");
    setMinimumWidth(400);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Image List
    mainLayout->addWidget(new QLabel("Images:"));
    m_imageList = new QListWidget();
    mainLayout->addWidget(m_imageList);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* btnAdd = new QPushButton("Add Image");
    QPushButton* btnRemove = new QPushButton("Remove Selected");
    btnLayout->addWidget(btnAdd);
    btnLayout->addWidget(btnRemove);
    mainLayout->addLayout(btnLayout);

    connect(btnAdd, &QPushButton::clicked, this, &SettingsDialog::addImage);
    connect(btnRemove, &QPushButton::clicked, this, &SettingsDialog::removeImage);

    // Opacity
    QHBoxLayout* opacityLayout = new QHBoxLayout();
    opacityLayout->addWidget(new QLabel("Opacity:"));
    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(0, 100);
    opacityLayout->addWidget(m_opacitySlider);
    mainLayout->addLayout(opacityLayout);

    // Scale
    QHBoxLayout* scaleLayout = new QHBoxLayout();
    scaleLayout->addWidget(new QLabel("Scale Factor:"));
    m_scaleSpin = new QDoubleSpinBox();
    m_scaleSpin->setRange(0.1, 5.0);
    m_scaleSpin->setSingleStep(0.1);
    scaleLayout->addWidget(m_scaleSpin);
    mainLayout->addLayout(scaleLayout);

    // Keys
    QHBoxLayout* keyLayout = new QHBoxLayout();
    keyLayout->addWidget(new QLabel("Modifier:"));
    m_modCombo = new QComboBox();
    m_modCombo->addItems({"None", "Shift", "Ctrl", "Alt"});
    keyLayout->addWidget(m_modCombo);
    
    keyLayout->addWidget(new QLabel("Prev:"));
    m_prevKeyCombo = new QComboBox();
    m_prevKeyCombo->addItems({"Left", "Right", "Up", "Down", "A", "D"});
    keyLayout->addWidget(m_prevKeyCombo);

    keyLayout->addWidget(new QLabel("Next:"));
    m_nextKeyCombo = new QComboBox();
    m_nextKeyCombo->addItems({"Left", "Right", "Up", "Down", "A", "D"});
    keyLayout->addWidget(m_nextKeyCombo);
    
    mainLayout->addLayout(keyLayout);

    // Audio settings
    mainLayout->addWidget(new QLabel("Startup Sound (WAV):"));
    QHBoxLayout* startSoundLayout = new QHBoxLayout();
    m_startupSoundEdit = new QLineEdit();
    QPushButton* btnBrowseStart = new QPushButton("Browse");
    startSoundLayout->addWidget(m_startupSoundEdit);
    startSoundLayout->addWidget(btnBrowseStart);
    mainLayout->addLayout(startSoundLayout);
    connect(btnBrowseStart, &QPushButton::clicked, this, &SettingsDialog::selectStartupSound);

    mainLayout->addWidget(new QLabel("Image Switch Sound (WAV):"));
    QHBoxLayout* switchSoundLayout = new QHBoxLayout();
    m_switchSoundEdit = new QLineEdit();
    QPushButton* btnBrowseSwitch = new QPushButton("Browse");
    switchSoundLayout->addWidget(m_switchSoundEdit);
    switchSoundLayout->addWidget(btnBrowseSwitch);
    mainLayout->addLayout(switchSoundLayout);
    connect(btnBrowseSwitch, &QPushButton::clicked, this, &SettingsDialog::selectSwitchSound);

    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::saveSettings);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Key Mapping
    keyMap["Left"] = VK_LEFT;
    keyMap["Right"] = VK_RIGHT;
    keyMap["Up"] = VK_UP;
    keyMap["Down"] = VK_DOWN;
    keyMap["A"] = 'A';
    keyMap["D"] = 'D';

    loadSettings();
}

// Helper not needed
// void SettingsDialog::loadImage() {}

void SettingsDialog::addImage() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Select Images", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (!files.isEmpty()) {
        m_imageList->addItems(files);
    }
}

void SettingsDialog::removeImage() {
    qDeleteAll(m_imageList->selectedItems());
}

void SettingsDialog::selectStartupSound() {
    QString file = QFileDialog::getOpenFileName(this, "Select Startup Sound", "", "WAV Files (*.wav)");
    if (!file.isEmpty()) m_startupSoundEdit->setText(file);
}

void SettingsDialog::selectSwitchSound() {
    QString file = QFileDialog::getOpenFileName(this, "Select Switch Sound", "", "WAV Files (*.wav)");
    if (!file.isEmpty()) m_switchSoundEdit->setText(file);
}

void SettingsDialog::saveSettings() {
    Config& cfg = Config::instance();
    cfg.imagePaths.clear();
    for (int i = 0; i < m_imageList->count(); ++i) {
        cfg.imagePaths << m_imageList->item(i)->text();
    }
    cfg.opacity = m_opacitySlider->value() / 100.0;
    cfg.scale = m_scaleSpin->value();
    
    // Keys
    cfg.keyPrev = getKey(m_prevKeyCombo->currentText());
    cfg.keyNext = getKey(m_nextKeyCombo->currentText());
    
    QString modTxt = m_modCombo->currentText();
    if (modTxt == "Shift") cfg.keyModifiers = VK_SHIFT;
    else if (modTxt == "Ctrl") cfg.keyModifiers = VK_CONTROL;
    else if (modTxt == "Alt") cfg.keyModifiers = VK_MENU;
    else cfg.keyModifiers = 0;

    cfg.startupSound = m_startupSoundEdit->text();
    cfg.switchSound = m_switchSoundEdit->text();

    cfg.save();
    accept();
}

void SettingsDialog::loadSettings() {
    Config& cfg = Config::instance();
    // cfg.load(); // Don't reload inside loadSettings, it might overwrite unsaved changes if called weirdly, but here it's fine. 
                   // Actually, loadSettings is called on Init, so load is good.
    cfg.load();

    m_imageList->clear();
    m_imageList->addItems(cfg.imagePaths);

    m_opacitySlider->setValue((int)(cfg.opacity * 100));
    m_scaleSpin->setValue(cfg.scale);
    
    m_startupSoundEdit->setText(cfg.startupSound);
    m_switchSoundEdit->setText(cfg.switchSound);

    // Map keys back to combo
    // Modifier
    if (cfg.keyModifiers == VK_SHIFT) m_modCombo->setCurrentText("Shift");
    else if (cfg.keyModifiers == VK_CONTROL) m_modCombo->setCurrentText("Ctrl");
    else if (cfg.keyModifiers == VK_MENU) m_modCombo->setCurrentText("Alt");
    else m_modCombo->setCurrentText("None");

    m_prevKeyCombo->setCurrentText(getKeyName(cfg.keyPrev));
    m_nextKeyCombo->setCurrentText(getKeyName(cfg.keyNext));

    // Modifiers
    if (cfg.keyModifiers == VK_SHIFT) m_modCombo->setCurrentText("Shift");
    else if (cfg.keyModifiers == VK_CONTROL) m_modCombo->setCurrentText("Ctrl");
    else if (cfg.keyModifiers == VK_MENU) m_modCombo->setCurrentText("Alt");
    else m_modCombo->setCurrentText("None");
}

int SettingsDialog::getKey(const QString& name) {
    return keyMap.value(name, 0);
}

QString SettingsDialog::getKeyName(int key) {
    return keyMap.key(key, "Unknown");
}

