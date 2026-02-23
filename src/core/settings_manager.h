#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <QObject>
#include <QSettings>
#include <QVariant>
#include <QKeySequence>

namespace Core {

class SettingsManager : public QObject {
    Q_OBJECT

public:
    static SettingsManager& instance();

    void setValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;

    
    QString theme() const;
    void setTheme(const QString &theme);

    int tabSize() const;
    void setTabSize(int size);

    bool wordWrap() const;
    void setWordWrap(bool wrap);

    double uiScale() const;
    void setUIScale(double scale);

    QString shortcut(const QString &actionName, const QString &defaultShortcut = QString()) const;
    void setShortcut(const QString &actionName, const QString &shortcut);
signals:
    void settingsChanged(const QString &key);

private:
    SettingsManager();
    QSettings m_settings;

    
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;
};

} 

#endif 
