#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QMap>
#include <QTabWidget>
#include <QTableWidget>
#include <QLabel>
#include "lspclient/lspclient_widget.h"

namespace Main {

class PreferencesDialog final : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);

private slots:
    void onThemeChanged(const QString &themeName);
    void applyThemeColors();
    void saveAndClose();
    void onShortcutCellDoubleClicked(int row, int column);
    void resetShortcuts();

private:
    QTabWidget *tabWidget;
    
    
    QComboBox *themeCombo;
    QSpinBox *tabSizeSpin;
    QCheckBox *wordWrapCheck;
    QDoubleSpinBox *uiScaleSpin;
    QLabel *restartWarningLabel;
    
    
    QTableWidget *shortcutsTable;
    QMap<QString, QString> shortcutMap;
    QMap<QString, QString> defaultShortcuts;

    LspClient::LspClientWidget *lspWidget;

    void setupUi();
    void setupGeneralTab();
    void setupShortcutsTab();
    void setupLspTab();
    void loadSettings();
    void populateShortcutsTable();
};

} 

#endif 
