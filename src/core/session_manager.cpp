#include "session_manager.h"
#include <QSettings>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>
#include <QCryptographicHash>

namespace Core {

SessionManager& SessionManager::instance() {
    static SessionManager inst;
    return inst;
}

SessionManager::SessionManager() {
}

void SessionManager::saveSession(const SessionState &state) {
    QSettings settings(sessionFilePath(), QSettings::IniFormat);
    settings.setValue("openFiles", state.openFiles);
    settings.setValue("activeTabIndex", state.activeTabIndex);
    settings.setValue("projectRoot", state.projectRoot);
    settings.setValue("windowGeometry", state.windowGeometry);
    settings.setValue("windowState", state.windowState);
    settings.setValue("splitterState", state.splitterState);
}

SessionState SessionManager::loadSession() const {
    SessionState state;
    QSettings settings(sessionFilePath(), QSettings::IniFormat);
    state.openFiles = settings.value("openFiles").toStringList();
    state.activeTabIndex = settings.value("activeTabIndex", 0).toInt();
    state.projectRoot = settings.value("projectRoot").toString();
    state.windowGeometry = settings.value("windowGeometry").toByteArray();
    state.windowState = settings.value("windowState").toByteArray();
    state.splitterState = settings.value("splitterState").toByteArray();
    return state;
}

QString SessionManager::sessionFilePath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/session.ini";
}

} 
