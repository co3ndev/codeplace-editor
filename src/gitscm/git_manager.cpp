#include "git_manager.h"
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include "../core/project_manager.h"

namespace GitScm {

GitManager& GitManager::instance() {
    static GitManager instance;
    return instance;
}

GitManager::GitManager() : QObject(nullptr) {
    m_statusProcess = std::make_unique<Core::BackgroundProcess>();
    connect(m_statusProcess.get(), &Core::BackgroundProcess::finished,
            this, &GitManager::onStatusProcessFinished);

    m_branchProcess = std::make_unique<Core::BackgroundProcess>();
    connect(m_branchProcess.get(), &Core::BackgroundProcess::finished,
            this, &GitManager::onBranchProcessFinished);

    m_actionProcess = std::make_unique<Core::BackgroundProcess>();
    connect(m_actionProcess.get(), &Core::BackgroundProcess::finished,
            this, &GitManager::onActionProcessFinished);

    m_debounceTimer = std::make_unique<QTimer>();
    m_debounceTimer->setSingleShot(true);
    connect(m_debounceTimer.get(), &QTimer::timeout, this, &GitManager::refreshStatus);

    m_watcher = std::make_unique<QFileSystemWatcher>();
    connect(m_watcher.get(), &QFileSystemWatcher::directoryChanged, this, &GitManager::onFileSystemChanged);
    connect(m_watcher.get(), &QFileSystemWatcher::fileChanged, this, &GitManager::onFileSystemChanged);

    connect(&Core::ProjectManager::instance(), &Core::ProjectManager::projectRootChanged,
            this, &GitManager::setProjectRoot);
            
    setProjectRoot(Core::ProjectManager::instance().projectRoot());
}

GitManager::~GitManager() {
}

void GitManager::setProjectRoot(const QString &root) {
    if (m_projectRoot == root) return;
    
    m_projectRoot = root;
    m_currentFileStatuses.clear();
    
    if (!m_watcher->directories().isEmpty()) m_watcher->removePaths(m_watcher->directories());
    if (!m_watcher->files().isEmpty()) m_watcher->removePaths(m_watcher->files());
    
    if (m_statusProcess->isRunning()) m_statusProcess->stop();
    if (m_branchProcess->isRunning()) m_branchProcess->stop();

    if (m_projectRoot.isEmpty()) {
        emit statusUpdated(m_currentFileStatuses);
        emit branchesUpdated({}, "");
        return;
    }
    
    m_watcher->addPath(m_projectRoot);
    QDir gitDir(QDir(m_projectRoot).filePath(".git"));
    if (gitDir.exists()) {
        m_watcher->addPath(gitDir.absolutePath());
        if (gitDir.exists("index")) m_watcher->addPath(gitDir.absoluteFilePath("index"));
        if (gitDir.exists("HEAD")) m_watcher->addPath(gitDir.absoluteFilePath("HEAD"));
        if (gitDir.exists("refs")) m_watcher->addPath(gitDir.absoluteFilePath("refs"));
        if (gitDir.exists("refs/heads")) m_watcher->addPath(gitDir.absoluteFilePath("refs/heads"));
    }
    
    refreshStatus();
}

void GitManager::refreshStatus() {
    if (m_projectRoot.isEmpty()) return;
    if (m_statusProcess->isRunning()) return;
    
    m_statusProcess->start("git", QStringList() << "status" << "--porcelain", m_projectRoot);
    
    refreshBranches();
}

void GitManager::refreshBranches() {
    if (m_projectRoot.isEmpty()) return;
    if (m_branchProcess->isRunning()) return;
    
    m_branchProcess->start("git", QStringList() << "branch" << "--format=%(refname:short) %(HEAD)", m_projectRoot);
}

void GitManager::runAction(const QStringList &args) {
    if (m_projectRoot.isEmpty()) return;
    if (m_actionProcess->isRunning()) {
        emit actionFinished(false, "Another Git action is currently running.");
        return;
    }
    
    m_actionProcess->start("git", args, m_projectRoot);
}

void GitManager::onStatusProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString output = QString::fromUtf8(m_statusProcess->readAllStandardOutput());
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        
        QMap<QString, QString> newFileStatuses;
        for (const QString &line : lines) {
            if (line.size() > 3) {
                QString state = line.left(2);
                QString fileRelPath = line.mid(3).trimmed();
                
                if (fileRelPath.startsWith("\"")) {
                    fileRelPath = fileRelPath.mid(1, fileRelPath.length() - 2);
                }
                
                QString fullPath = QDir(m_projectRoot).filePath(fileRelPath);
                newFileStatuses[fullPath] = state;
            }
        }
        
        if (newFileStatuses != m_currentFileStatuses) {
            m_currentFileStatuses = newFileStatuses;
            emit statusUpdated(m_currentFileStatuses);
        }
    } else {
        m_currentFileStatuses.clear();
        emit statusUpdated(m_currentFileStatuses);
    }
}

void GitManager::onBranchProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString output = QString::fromUtf8(m_branchProcess->readAllStandardOutput());
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        
        QString currentBranch;
        QStringList branches;
        
        for (QString line : lines) {
            line = line.trimmed();
            if (line.endsWith("*")) {
                currentBranch = line.left(line.length() - 1).trimmed();
                branches.append(currentBranch);
            } else if (!line.isEmpty()) {
                branches.append(line);
            }
        }
        
        emit branchesUpdated(branches, currentBranch);
    }
}

void GitManager::onActionProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    bool success = (exitStatus == QProcess::NormalExit && exitCode == 0);
    QString error;
    if (!success) {
        error = QString::fromUtf8(m_actionProcess->readAllStandardError());
        if (error.isEmpty()) {
            error = QString::fromUtf8(m_actionProcess->readAllStandardOutput());
        }
    } else {
        refreshStatus();
    }
    emit actionFinished(success, error);
}

void GitManager::onFileSystemChanged() {
    m_debounceTimer->start(500);
}

} 
