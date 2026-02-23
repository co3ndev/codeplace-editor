#include "settings_manager.h"

namespace Core {

SettingsManager& SettingsManager::instance() {
    static SettingsManager inst;
    return inst;
}

SettingsManager::SettingsManager() 
    : m_settings("CodePlace", "CodePlaceEditor") {
}

void SettingsManager::setValue(const QString &key, const QVariant &value) {
    m_settings.setValue(key, value);
    emit settingsChanged(key);
}

QVariant SettingsManager::value(const QString &key, const QVariant &defaultValue) const {
    return m_settings.value(key, defaultValue);
}

QString SettingsManager::theme() const {
    return value("theme", "Tokyo Night").toString();
}

void SettingsManager::setTheme(const QString &theme) {
    setValue("theme", theme);
}

int SettingsManager::tabSize() const {
    return value("editor/tabSize", 4).toInt();
}

void SettingsManager::setTabSize(int size) {
    setValue("editor/tabSize", size);
}

bool SettingsManager::wordWrap() const {
    return value("editor/wordWrap", false).toBool();
}

void SettingsManager::setWordWrap(bool wrap) {
    setValue("editor/wordWrap", wrap);
}

double SettingsManager::uiScale() const {
    return value("ui/scale", 1.0).toDouble();
}

void SettingsManager::setUIScale(double scale) {
    setValue("ui/scale", scale);
}

QString SettingsManager::shortcut(const QString &actionName, const QString &defaultShortcut) const {
    return value("shortcuts/" + actionName, defaultShortcut).toString();
}

void SettingsManager::setShortcut(const QString &actionName, const QString &shortcut) {
    setValue("shortcuts/" + actionName, shortcut);
}
} 
