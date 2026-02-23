#include "preferences_dialog.h"
#include "core/settings_manager.h"
#include "core/theme_manager.h"
#include "core/default_shortcuts.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QInputDialog>
#include <QTimer>
#include <QCompleter>

namespace Main {

PreferencesDialog::PreferencesDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Preferences");
    
    defaultShortcuts = Core::defaultShortcuts();

    setupUi();
    loadSettings();
    resize(600, 500);
}

void PreferencesDialog::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    
    tabWidget = new QTabWidget();
    setupGeneralTab();
    setupShortcutsTab();
    setupLspTab();
    
    mainLayout->addWidget(tabWidget);
    
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &PreferencesDialog::saveAndClose);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void PreferencesDialog::setupGeneralTab() {
    auto *generalWidget = new QWidget();
    auto *mainLayout = new QVBoxLayout(generalWidget);
    auto *formLayout = new QFormLayout();
    
    themeCombo = new QComboBox();
    themeCombo->addItems(Core::ThemeManager::instance().availableThemes());
    formLayout->addRow("Theme:", themeCombo);

    tabSizeSpin = new QSpinBox();
    tabSizeSpin->setRange(1, 8);
    formLayout->addRow("Tab Indentation:", tabSizeSpin);
    
    wordWrapCheck = new QCheckBox("Enable Word Wrap");
    formLayout->addRow("", wordWrapCheck);
    
    uiScaleSpin = new QDoubleSpinBox();
    uiScaleSpin->setRange(0.5, 3.0);
    uiScaleSpin->setSingleStep(0.1);
    uiScaleSpin->setDecimals(2);
    formLayout->addRow("UI Scale:", uiScaleSpin);

    restartWarningLabel = new QLabel("(Restart required for scaling changes)");
        restartWarningLabel->setStyleSheet("font-style: italic; font-size: 10px;");
        connect(&Core::ThemeManager::instance(), &Core::ThemeManager::themeChanged,
            this, &PreferencesDialog::applyThemeColors);
        applyThemeColors();
    formLayout->addRow("", restartWarningLabel);
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();
    
    tabWidget->addTab(generalWidget, "General");
    connect(themeCombo, &QComboBox::currentTextChanged, this, &PreferencesDialog::onThemeChanged);
}

void PreferencesDialog::applyThemeColors() {
    auto &tm = Core::ThemeManager::instance();
    QColor fg = tm.getColor(Core::ThemeManager::FindBarForeground);
    restartWarningLabel->setStyleSheet(QString("color: %1; font-style: italic; font-size: 10px;").arg(fg.name()));
}

void PreferencesDialog::setupShortcutsTab() {
    auto *shortcutsWidget = new QWidget();
    auto *layout = new QVBoxLayout(shortcutsWidget);
    
    
    shortcutsTable = new QTableWidget();
    shortcutsTable->setColumnCount(2);
    shortcutsTable->setHorizontalHeaderLabels({"Action", "Shortcut"});
    shortcutsTable->horizontalHeader()->setStretchLastSection(true);
    shortcutsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    shortcutsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    shortcutsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    layout->addWidget(shortcutsTable);
    
    
    auto *buttonLayout = new QHBoxLayout();
    
    auto *resetButton = new QPushButton("Reset to Defaults");
    connect(resetButton, &QPushButton::clicked, this, &PreferencesDialog::resetShortcuts);
    buttonLayout->addStretch();
    buttonLayout->addWidget(resetButton);
    
    layout->addLayout(buttonLayout);
    
    tabWidget->addTab(shortcutsWidget, "Shortcuts");
    
    connect(shortcutsTable, &QTableWidget::cellDoubleClicked, this, &PreferencesDialog::onShortcutCellDoubleClicked);
}

void PreferencesDialog::setupLspTab() {
    lspWidget = new LspClient::LspClientWidget();
    tabWidget->addTab(lspWidget, "LSP Settings");
}

void PreferencesDialog::populateShortcutsTable() {
    shortcutsTable->setRowCount(0);
    
    int row = 0;
    for (auto it = shortcutMap.begin(); it != shortcutMap.end(); ++it) {
        shortcutsTable->insertRow(row);
        
        auto *actionItem = new QTableWidgetItem(it.key());
        actionItem->setFlags(actionItem->flags() & ~Qt::ItemIsEditable);
        shortcutsTable->setItem(row, 0, actionItem);
        
        auto *shortcutItem = new QTableWidgetItem(it.value());
        shortcutItem->setFlags(shortcutItem->flags() & ~Qt::ItemIsEditable);
        shortcutsTable->setItem(row, 1, shortcutItem);
        
        row++;
    }
    
    shortcutsTable->resizeColumnsToContents();
}

void PreferencesDialog::loadSettings() {
    auto &settings = Core::SettingsManager::instance();
    
    themeCombo->setCurrentText(settings.theme());
    tabSizeSpin->setValue(settings.tabSize());
    wordWrapCheck->setChecked(settings.wordWrap());
    uiScaleSpin->setValue(settings.uiScale());
    
    
    shortcutMap = defaultShortcuts;
    for (auto it = defaultShortcuts.begin(); it != defaultShortcuts.end(); ++it) {
        shortcutMap[it.key()] = settings.shortcut(it.key(), it.value());
    }
    
    populateShortcutsTable();
}

void PreferencesDialog::onThemeChanged(const QString &themeName) {
    Core::ThemeManager::instance().applyTheme(themeName);
}

void PreferencesDialog::onShortcutCellDoubleClicked(int row, int column) {
    if (column != 1) return; 
    
    QTableWidgetItem *actionItem = shortcutsTable->item(row, 0);
    QTableWidgetItem *shortcutItem = shortcutsTable->item(row, 1);
    
    if (!actionItem || !shortcutItem) return;
    
    QString actionName = actionItem->text();
    QString currentShortcut = shortcutItem->text();
    
    bool ok;
    QString newShortcut = QInputDialog::getText(this, "Edit Shortcut",
        QString("Enter new shortcut for '%1':\n(e.g., Ctrl+S, Alt+F, F2)").arg(actionName),
        QLineEdit::Normal, currentShortcut, &ok);
    
    if (ok && !newShortcut.isEmpty()) {
        shortcutMap[actionName] = newShortcut;
        shortcutItem->setText(newShortcut);
    }
}

void PreferencesDialog::resetShortcuts() {
    shortcutMap = defaultShortcuts;
    populateShortcutsTable();
}

void PreferencesDialog::saveAndClose() {
    auto &settings = Core::SettingsManager::instance();
    
    settings.setTabSize(tabSizeSpin->value());
    settings.setWordWrap(wordWrapCheck->isChecked());
    settings.setUIScale(uiScaleSpin->value());
    
    
    for (auto it = shortcutMap.begin(); it != shortcutMap.end(); ++it) {
        settings.setShortcut(it.key(), it.value());
    }
    
    accept();
}

} 
