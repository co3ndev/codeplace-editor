#ifndef PROJECT_MANAGER_H
#define PROJECT_MANAGER_H

#include <QObject>
#include <QString>

namespace Core {

class ProjectManager : public QObject {
    Q_OBJECT

public:
    static ProjectManager& instance();

    void setProjectRoot(const QString &path);
    QString projectRoot() const { return m_projectRoot; }

signals:
    void projectRootChanged(const QString &newRoot);

private:
    ProjectManager();
    ~ProjectManager() override = default;
    
    ProjectManager(const ProjectManager&) = delete;
    ProjectManager& operator=(const ProjectManager&) = delete;

    QString m_projectRoot;
};

} 

#endif 
