#ifndef GITSCM_WIDGET_H
#define GITSCM_WIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QProcess>
#include <QComboBox>
#include <QTextEdit>
#include <QMap>
#include <QString>
#include <QFileSystemWatcher>

namespace GitScm {

class GitScmWidget : public QWidget {
    Q_OBJECT

public:
    explicit GitScmWidget(QWidget *parent = nullptr);
    void setProjectRoot(const QString &root);
    QMap<QString, QString> getFileStatuses() const;

signals:
    void fileSelected(const QString &filePath);
    void diffRequested(const QString &filePath);
    void statusUpdated(const QMap<QString, QString> &fileStatuses);

private slots:
    void onGitStatusUpdated(const QMap<QString, QString> &fileStatuses);
    void onGitBranchesUpdated(const QStringList &branches, const QString &currentBranch);
    void onGitActionFinished(bool success, const QString &error);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onThemeChanged();
    
    void onBranchChanged(int index);
    void onStageSelected();
    void onUnstageSelected();
    void onCommitClicked();
    void showContextMenu(const QPoint &pos);
    void showCreateBranchDialog();

private:
    void setupUI();

    QComboBox *m_branchComboBox;
    QTreeWidget *m_scmTree;
    QLabel *m_statusLabel;
    
    QPushButton *m_stageButton;
    QPushButton *m_unstageButton;
    QTextEdit *m_commitMessageEdit;
    QPushButton *m_commitButton;
    QWidget *m_controlsWidget;
    
    QString m_projectRoot;
    QMap<QString, QString> m_currentFileStatuses;
    bool m_updatingBranches = false;
};

} 

#endif 
