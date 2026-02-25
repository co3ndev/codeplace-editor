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

class AiClient;
namespace LspClient { class LspClientWidget; }

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
    
    // AI Slots
    void onAiProviderChanged(int index);
    void onAiFetchModelsClicked();
    void onAiModelsReceived(const QStringList &models);

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

    // AI Tab members
    QComboBox *aiProviderCombo;
    QWidget *aiOpenRouterKeyWidget;
    QLineEdit *aiOpenRouterKeyEdit;
    QWidget *aiLocalUrlWidget;
    QLineEdit *aiLocalUrlEdit;
    QComboBox *aiModelCombo;
    QPushButton *aiFetchModelsButton;
    AiClient *aiClient;

    void setupUi();
    void setupGeneralTab();
    void setupShortcutsTab();
    void setupLspTab();
    void setupAiTab();
    void loadSettings();
    void populateShortcutsTable();
};

} 

#endif 
