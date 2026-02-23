#ifndef GIT_MANAGER_H
#define GIT_MANAGER_H

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QTimer>
#include <QFileSystemWatcher>
#include <memory>
#include "../core/process_helper.h"

namespace GitScm {

class GitManager : public QObject {
    Q_OBJECT

public:
    static GitManager& instance();

    void setProjectRoot(const QString &root);
    QString projectRoot() const { return m_projectRoot; }

    void refreshStatus();
    void refreshBranches();
    void runAction(const QStringList &args);

    QMap<QString, QString> fileStatuses() const { return m_currentFileStatuses; }

signals:
    void statusUpdated(const QMap<QString, QString> &fileStatuses);
    void branchesUpdated(const QStringList &branches, const QString &currentBranch);
    void actionFinished(bool success, const QString &error);

private slots:
    void onStatusProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onBranchProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onActionProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onFileSystemChanged();

private:
    GitManager();
    ~GitManager() override;

    GitManager(const GitManager&) = delete;
    GitManager& operator=(const GitManager&) = delete;

    std::unique_ptr<Core::BackgroundProcess> m_statusProcess;
    std::unique_ptr<Core::BackgroundProcess> m_branchProcess;
    std::unique_ptr<Core::BackgroundProcess> m_actionProcess;
    std::unique_ptr<QTimer> m_debounceTimer;
    std::unique_ptr<QFileSystemWatcher> m_watcher;

    QString m_projectRoot;
    QMap<QString, QString> m_currentFileStatuses;
};

} 

#endif 
