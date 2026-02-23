#ifndef FILEBROWSERSIDEBAR_H
#define FILEBROWSERSIDEBAR_H

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QFileIconProvider>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>
#include <QLabel>
#include <QStyledItemDelegate>
#include <QMenu>
#include <QAction>
#include <QMouseEvent>

namespace Browser {

class FileBrowserView final : public QTreeView {
    Q_OBJECT
public:
    explicit FileBrowserView(QWidget *parent = nullptr) : QTreeView(parent) {}

protected:
    void mousePressEvent(QMouseEvent *event) override;
};

class FileSortProxyModel final : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit FileSortProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent) {}

protected:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;
};

class FileBrowserDelegate final : public QStyledItemDelegate {
    Q_OBJECT
public:
    FileBrowserDelegate(const QStringList &modifiedFiles, QObject *parent = nullptr)
        : QStyledItemDelegate(parent), m_modifiedFiles(modifiedFiles) {}

    void setModifiedFiles(const QStringList &files) { m_modifiedFiles = files; }
    void setGitModifiedFiles(const QMap<QString, QString> &files) { m_gitModifiedFiles = files; }
    void clearCache() { m_iconCache.clear(); }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QStringList m_modifiedFiles;
    QMap<QString, QString> m_gitModifiedFiles;
    mutable QMap<QString, QIcon> m_iconCache;
};

class FileBrowserSidebar final : public QWidget {
    Q_OBJECT

public:
    explicit FileBrowserSidebar(QWidget *parent = nullptr);

    void setProjectRoot(const QString &path);
    QString currentProjectRoot() const;
    void refreshFileIcon(const QString &filePath);

signals:
    void fileSelected(const QString &filePath);
    void fileDeleted(const QString &filePath);
    void fileRenamed(const QString &oldPath, const QString &newPath);
    void folderCreated(const QString &folderPath);

public slots:
    void onModifiedFilesChanged(const QStringList &modifiedFiles);
    void onGitModifiedFilesChanged(const QMap<QString, QString> &files);
    void updateTheme();

private slots:
    void onItemDoubleClicked(const QModelIndex &index);
    void onItemClicked(const QModelIndex &index);
    void onContextMenuRequested(const QPoint &pos);
    void onOpenFile();
    void onDeleteFile();
    void onRenameFile();
    void onNewFolder();
    void onNewFile();
    void onCopyPath();
    void onRevealInExplorer();

private:
    void createBrowserActions();
    void showContextMenu(const QPoint &pos, const QModelIndex &index);
    void showRootContextMenu(const QPoint &pos);
    QString getSelectedPath() const;
    QString getContextPath() const;
    QModelIndex resolveSourceIndex(const QModelIndex &proxyIndex) const;
    void ensureContextIndex();

    QLabel *m_headerLabel;
    FileBrowserView *m_treeView;
    QFileSystemModel *m_model;
    QString m_projectRoot;
    QStringList m_modifiedFiles;
    QMap<QString, QString> m_gitModifiedFiles;
    QModelIndex m_contextMenuIndex;

    
    QAction *m_renameAction;
    QAction *m_deleteAction;
    QAction *m_newFileAction;
    QAction *m_newFolderAction;
    QAction *m_copyPathAction;
    QAction *m_revealAction;
};

} 

#endif 
