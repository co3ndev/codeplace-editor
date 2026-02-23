#include "lspclient_widget.h"
#include "core/settings_manager.h"
#include <QFileDialog>
#include <QFormLayout>

namespace LspClient {

LspClientWidget::LspClientWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
    populateLanguages();
    if (m_languageCombo->count() > 0) {
        onLanguageChanged(0);
    }
}

void LspClientWidget::setupUi() {
    auto *layout = new QVBoxLayout(this);

    auto *langLayout = new QHBoxLayout();
    langLayout->addWidget(new QLabel("Language:"));
    m_languageCombo = new QComboBox();
    langLayout->addWidget(m_languageCombo);
    layout->addLayout(langLayout);

    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LspClientWidget::onLanguageChanged);

    auto *group = new QGroupBox("LSP Configuration");
    auto *form = new QFormLayout(group);

    m_enableCheckBox = new QCheckBox("Enable LSP");
    form->addRow(m_enableCheckBox);

    connect(m_enableCheckBox, &QCheckBox::toggled, this, &LspClientWidget::onEnableToggled);

    auto *exeLayout = new QHBoxLayout();
    m_executableEdit = new QLineEdit();
    m_browseButton = new QPushButton("Browse...");
    exeLayout->addWidget(m_executableEdit);
    exeLayout->addWidget(m_browseButton);
    form->addRow("Executable:", exeLayout);

    connect(m_browseButton, &QPushButton::clicked, this, &LspClientWidget::onBrowseExecutable);
    
    m_argsEdit = new QLineEdit();
    form->addRow("Arguments:", m_argsEdit);

    layout->addWidget(group);
    layout->addStretch();
    
    connect(m_enableCheckBox, &QCheckBox::clicked, this, &LspClientWidget::saveCurrentSettings);
    connect(m_executableEdit, &QLineEdit::editingFinished, this, &LspClientWidget::saveCurrentSettings);
    connect(m_argsEdit, &QLineEdit::editingFinished, this, &LspClientWidget::saveCurrentSettings);
}

void LspClientWidget::populateLanguages() {
    for (int i = 0; i <= static_cast<int>(Core::SimpleHighlighter::Lang_Perl); ++i) {
        auto lang = static_cast<Core::SimpleHighlighter::Language>(i);
        m_languageCombo->addItem(languageToDisplayName(lang), QVariant::fromValue(i));
    }
}

QString LspClientWidget::languageToString(Core::SimpleHighlighter::Language lang) {
    switch (lang) {
        case Core::SimpleHighlighter::Lang_Cpp: return "cpp";
        case Core::SimpleHighlighter::Lang_Python: return "python";
        case Core::SimpleHighlighter::Lang_Rust: return "rust";
        case Core::SimpleHighlighter::Lang_Go: return "go";
        case Core::SimpleHighlighter::Lang_HTML: return "html";
        case Core::SimpleHighlighter::Lang_CSS: return "css";
        case Core::SimpleHighlighter::Lang_JS: return "javascript";
        case Core::SimpleHighlighter::Lang_TypeScript: return "typescript";
        case Core::SimpleHighlighter::Lang_Java: return "java";
        case Core::SimpleHighlighter::Lang_CSharp: return "csharp";
        case Core::SimpleHighlighter::Lang_Ruby: return "ruby";
        case Core::SimpleHighlighter::Lang_PHP: return "php";
        case Core::SimpleHighlighter::Lang_Shell: return "shellscript";
        case Core::SimpleHighlighter::Lang_YAML: return "yaml";
        case Core::SimpleHighlighter::Lang_JSON: return "json";
        case Core::SimpleHighlighter::Lang_XML: return "xml";
        case Core::SimpleHighlighter::Lang_SQL: return "sql";
        case Core::SimpleHighlighter::Lang_Markdown: return "markdown";
        case Core::SimpleHighlighter::Lang_Lua: return "lua";
        case Core::SimpleHighlighter::Lang_Kotlin: return "kotlin";
        case Core::SimpleHighlighter::Lang_Swift: return "swift";
        case Core::SimpleHighlighter::Lang_Dart: return "dart";
        case Core::SimpleHighlighter::Lang_Perl: return "perl";
        default: return "unknown";
    }
}

QString LspClientWidget::languageToDisplayName(Core::SimpleHighlighter::Language lang) {
    switch (lang) {
        case Core::SimpleHighlighter::Lang_Cpp: return "C++";
        case Core::SimpleHighlighter::Lang_Python: return "Python";
        case Core::SimpleHighlighter::Lang_Rust: return "Rust";
        case Core::SimpleHighlighter::Lang_Go: return "Go";
        case Core::SimpleHighlighter::Lang_HTML: return "HTML";
        case Core::SimpleHighlighter::Lang_CSS: return "CSS";
        case Core::SimpleHighlighter::Lang_JS: return "JavaScript";
        case Core::SimpleHighlighter::Lang_TypeScript: return "TypeScript";
        case Core::SimpleHighlighter::Lang_Java: return "Java";
        case Core::SimpleHighlighter::Lang_CSharp: return "C#";
        case Core::SimpleHighlighter::Lang_Ruby: return "Ruby";
        case Core::SimpleHighlighter::Lang_PHP: return "PHP";
        case Core::SimpleHighlighter::Lang_Shell: return "Shell Script";
        case Core::SimpleHighlighter::Lang_YAML: return "YAML";
        case Core::SimpleHighlighter::Lang_JSON: return "JSON";
        case Core::SimpleHighlighter::Lang_XML: return "XML";
        case Core::SimpleHighlighter::Lang_SQL: return "SQL";
        case Core::SimpleHighlighter::Lang_Markdown: return "Markdown";
        case Core::SimpleHighlighter::Lang_Lua: return "Lua";
        case Core::SimpleHighlighter::Lang_Kotlin: return "Kotlin";
        case Core::SimpleHighlighter::Lang_Swift: return "Swift";
        case Core::SimpleHighlighter::Lang_Dart: return "Dart";
        case Core::SimpleHighlighter::Lang_Perl: return "Perl";
        default: return "Unknown";
    }
}

void LspClientWidget::onLanguageChanged(int index) {
    if (index < 0) return;
    auto lang = static_cast<Core::SimpleHighlighter::Language>(m_languageCombo->itemData(index).toInt());
    m_currentLang = lang;
    loadSettingsForLanguage(lang);
}

void LspClientWidget::loadSettingsForLanguage(Core::SimpleHighlighter::Language lang) {
    QString prefix = "lsp/" + languageToString(lang) + "/";
    auto &settings = Core::SettingsManager::instance();
    
    bool blockedE = m_enableCheckBox->blockSignals(true);
    bool blockedX = m_executableEdit->blockSignals(true);
    bool blockedA = m_argsEdit->blockSignals(true);

    m_enableCheckBox->setChecked(settings.value(prefix + "enabled", false).toBool());
    m_executableEdit->setText(settings.value(prefix + "executable", "").toString());
    m_argsEdit->setText(settings.value(prefix + "args", "").toString());

    if (lang == Core::SimpleHighlighter::Lang_Cpp) m_executableEdit->setPlaceholderText("clangd");
    else if (lang == Core::SimpleHighlighter::Lang_Python) m_executableEdit->setPlaceholderText("pyright-langserver");
    else if (lang == Core::SimpleHighlighter::Lang_Rust) m_executableEdit->setPlaceholderText("rust-analyzer");
    else m_executableEdit->setPlaceholderText("path/to/lsp-server");

    m_enableCheckBox->blockSignals(blockedE);
    m_executableEdit->blockSignals(blockedX);
    m_argsEdit->blockSignals(blockedA);
    
    onEnableToggled(m_enableCheckBox->isChecked());
}

void LspClientWidget::saveCurrentSettings() {
    QString prefix = "lsp/" + languageToString(m_currentLang) + "/";
    auto &settings = Core::SettingsManager::instance();
    
    settings.setValue(prefix + "enabled", m_enableCheckBox->isChecked());
    settings.setValue(prefix + "executable", m_executableEdit->text());
    settings.setValue(prefix + "args", m_argsEdit->text());
}

void LspClientWidget::onEnableToggled(bool checked) {
    m_executableEdit->setEnabled(checked);
    m_browseButton->setEnabled(checked);
    m_argsEdit->setEnabled(checked);
    saveCurrentSettings();
}

void LspClientWidget::onBrowseExecutable() {
    QString file = QFileDialog::getOpenFileName(this, "Select LSP Executable");
    if (!file.isEmpty()) {
        m_executableEdit->setText(file);
        saveCurrentSettings();
    }
}

} 
