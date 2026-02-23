#include "project_manager.h"

namespace Core {

ProjectManager& ProjectManager::instance() {
    static ProjectManager instance;
    return instance;
}

ProjectManager::ProjectManager() : QObject(nullptr) {
}

void ProjectManager::setProjectRoot(const QString &path) {
    if (m_projectRoot == path) return;
    
    m_projectRoot = path;
    emit projectRootChanged(m_projectRoot);
}

} 
