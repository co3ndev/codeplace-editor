#ifndef LSPCLIENT_WIDGET_H
#define LSPCLIENT_WIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include "core/simple_highlighter.h"

namespace LspClient {

class LspClientWidget : public QWidget {
    Q_OBJECT

public:
    explicit LspClientWidget(QWidget *parent = nullptr);
    ~LspClientWidget() override = default;
    
    static QString languageToString(Core::SimpleHighlighter::Language lang);
    static QString languageToDisplayName(Core::SimpleHighlighter::Language lang);

private slots:
    void onLanguageChanged(int index);
    void onEnableToggled(bool checked);
    void onBrowseExecutable();
    void saveCurrentSettings();
    void loadSettingsForLanguage(Core::SimpleHighlighter::Language lang);

private:
    void setupUi();
    void populateLanguages();

    QComboBox *m_languageCombo;
    QCheckBox *m_enableCheckBox;
    QLineEdit *m_executableEdit;
    QPushButton *m_browseButton;
    QLineEdit *m_argsEdit;
    
    Core::SimpleHighlighter::Language m_currentLang;
};

} 

#endif 
