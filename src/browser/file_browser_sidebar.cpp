#include "file_browser_sidebar.h"
#include <QAbstractItemView>
#include <QHeaderView>
#include <QPainter>
#include <QFileSystemModel>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include "core/theme_manager.h"
#include "core/settings_manager.h"
#include "core/default_shortcuts.h"
#include "core/project_manager.h"

namespace Browser {

bool FileSortProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const {
    QFileSystemModel *fsModel = qobject_cast<QFileSystemModel*>(sourceModel());
    if (!fsModel) {
        return QSortFilterProxyModel::lessThan(source_left, source_right);
    }

    bool leftIsDir = fsModel->isDir(source_left);
    bool rightIsDir = fsModel->isDir(source_right);
    
    QString leftName = source_left.data(Qt::DisplayRole).toString();
    QString rightName = source_right.data(Qt::DisplayRole).toString();

    if (leftIsDir != rightIsDir) {
        return leftIsDir;
    }

    return leftName.compare(rightName, Qt::CaseInsensitive) < 0;
}

void FileBrowserView::mousePressEvent(QMouseEvent *event) {
    QModelIndex index = indexAt(event->pos());
    if (!index.isValid()) {
        clearSelection();
        setCurrentIndex(QModelIndex());
    }
    QTreeView::mousePressEvent(event);
}

void FileBrowserDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QString filePath = index.model()->data(index, QFileSystemModel::FilePathRole).toString();

    if (m_gitModifiedFiles.contains(filePath)) {
        QString status = m_gitModifiedFiles.value(filePath);
        QColor color = status == "(U)" ? 
            Core::ThemeManager::instance().getColor(Core::ThemeManager::GitUntrackedIndicator) :
            Core::ThemeManager::instance().getColor(Core::ThemeManager::GitModifiedIndicator);
        opt.palette.setColor(QPalette::Text, color);
        opt.palette.setColor(QPalette::WindowText, color);
    }

    if (!opt.icon.isNull()) {
        auto &tm = Core::ThemeManager::instance();
        QColor fg = tm.getColor(Core::ThemeManager::EditorForeground);
        bool isSelected = (opt.state & QStyle::State_Selected);
        
        QColor tintColor = fg;
        if (isSelected) {
            QColor selBg = tm.getColor(Core::ThemeManager::SelectionBackground);
            tintColor = (selBg.lightness() > 140) ? Qt::black : Qt::white;
        }

        QString cacheKey = filePath + (isSelected ? "_sel" : "_norm") + tintColor.name();
        
        if (m_iconCache.contains(cacheKey)) {
            opt.icon = m_iconCache.value(cacheKey);
        } else {
            QIcon originalIcon = opt.icon;
            QIcon tintedIcon;
            
            for (int s : {16, 24, 32}) {
                QPixmap pix = originalIcon.pixmap(s, s);
                if (pix.isNull()) continue;

                QPainter p(&pix);
                p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
                p.fillRect(pix.rect(), tintColor);
                p.end();
                tintedIcon.addPixmap(pix);
            }
            m_iconCache.insert(cacheKey, tintedIcon);
            opt.icon = tintedIcon;
        }
    }

    QStyledItemDelegate::paint(painter, opt, index);

    int dotSize = 6;
    QRect rect = opt.rect;

    if (m_modifiedFiles.contains(filePath)) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(Core::ThemeManager::instance().getColor(Core::ThemeManager::ModifiedIndicator));
        
        painter->drawEllipse(rect.right() - 15, rect.top() + (rect.height() - dotSize) / 2, dotSize, dotSize);
        painter->restore();
    } else if (m_gitModifiedFiles.contains(filePath)) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        
        QString status = m_gitModifiedFiles.value(filePath);
        QColor color = status == "(U)" ? 
            Core::ThemeManager::instance().getColor(Core::ThemeManager::GitUntrackedIndicator) :
            Core::ThemeManager::instance().getColor(Core::ThemeManager::GitModifiedIndicator);
            
        painter->setBrush(color);
        painter->drawEllipse(rect.right() - 15, rect.top() + (rect.height() - dotSize) / 2, dotSize, dotSize);
        painter->restore();
    }
}

FileBrowserSidebar::FileBrowserSidebar(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_treeView = new FileBrowserView(this);
    m_model = new QFileSystemModel(this);
    m_model->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
    
    auto *proxyModel = new FileSortProxyModel(this);
    proxyModel->setSourceModel(m_model);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->sort(0, Qt::AscendingOrder);
    
    m_treeView->setModel(proxyModel);
    m_treeView->setAnimated(false);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    m_treeView->setIndentation(20);
    m_treeView->setSortingEnabled(false);
    m_treeView->header()->hide();
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    for (int i = 1; i < m_model->columnCount(); ++i) {
        m_treeView->hideColumn(i);
    }

    auto *delegate = new FileBrowserDelegate(m_modifiedFiles, this);
    m_treeView->setItemDelegate(delegate);

    layout->addWidget(m_treeView);

    connect(m_treeView, &QTreeView::doubleClicked, this, &FileBrowserSidebar::onItemDoubleClicked);
    connect(m_treeView, &QTreeView::clicked, this, &FileBrowserSidebar::onItemClicked);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &FileBrowserSidebar::onContextMenuRequested);

    m_treeView->setFrameStyle(QFrame::NoFrame);

    connect(&Core::ProjectManager::instance(), &Core::ProjectManager::projectRootChanged,
            this, &FileBrowserSidebar::setProjectRoot);
    
    createBrowserActions();
    
    updateTheme();
    connect(&Core::ThemeManager::instance(), &Core::ThemeManager::themeChanged, this, &FileBrowserSidebar::updateTheme);
}

void FileBrowserSidebar::createBrowserActions() {
    
    auto &settings = Core::SettingsManager::instance();

    m_renameAction = new QAction("&Rename", this);
    m_renameAction->setShortcut(QKeySequence(settings.shortcut("Rename", "F2")));
    m_renameAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_renameAction, &QAction::triggered, this, &FileBrowserSidebar::onRenameFile);
    addAction(m_renameAction);

    m_deleteAction = new QAction("&Delete", this);
    m_deleteAction->setShortcut(QKeySequence(settings.shortcut("Delete", "Delete")));
    m_deleteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_deleteAction, &QAction::triggered, this, &FileBrowserSidebar::onDeleteFile);
    addAction(m_deleteAction);

    m_newFileAction = new QAction("&New File", this);
    m_newFileAction->setShortcut(QKeySequence(settings.shortcut("New File (Browser)", "Ctrl+Alt+N")));
    m_newFileAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_newFileAction, &QAction::triggered, this, &FileBrowserSidebar::onNewFile);
    addAction(m_newFileAction);

    m_newFolderAction = new QAction("New &Folder", this);
    m_newFolderAction->setShortcut(QKeySequence(settings.shortcut("New Folder (Browser)", "Ctrl+Alt+Shift+N")));
    m_newFolderAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_newFolderAction, &QAction::triggered, this, &FileBrowserSidebar::onNewFolder);
    addAction(m_newFolderAction);

    m_copyPathAction = new QAction("&Copy Path", this);
    m_copyPathAction->setShortcut(QKeySequence(settings.shortcut("Copy Path", "Ctrl+Shift+C")));
    m_copyPathAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_copyPathAction, &QAction::triggered, this, &FileBrowserSidebar::onCopyPath);
    addAction(m_copyPathAction);

    m_revealAction = new QAction("&Reveal in Explorer", this);
    m_revealAction->setShortcut(QKeySequence(settings.shortcut("Reveal in Explorer", "Ctrl+Alt+R")));
    m_revealAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_revealAction, &QAction::triggered, this, &FileBrowserSidebar::onRevealInExplorer);
    addAction(m_revealAction);
}

void FileBrowserSidebar::setProjectRoot(const QString &path) {
    m_projectRoot = path;
    m_model->setRootPath(path);
    
    
    QModelIndex sourceIndex = m_model->index(path);
    auto *proxyModel = qobject_cast<QSortFilterProxyModel*>(m_treeView->model());
    QModelIndex proxyIndex = proxyModel ? proxyModel->mapFromSource(sourceIndex) : sourceIndex;
    m_treeView->setRootIndex(proxyIndex);
}

QString FileBrowserSidebar::currentProjectRoot() const {
    return m_projectRoot;
}

void FileBrowserSidebar::refreshFileIcon(const QString &filePath) {
    
    QModelIndex sourceIndex = m_model->index(filePath);
    if (!sourceIndex.isValid()) return;
    
    
    auto *proxyModel = qobject_cast<QSortFilterProxyModel*>(m_treeView->model());
    QModelIndex proxyIndex = proxyModel ? proxyModel->mapFromSource(sourceIndex) : sourceIndex;
    
    if (proxyIndex.isValid()) {
        
        m_treeView->model()->dataChanged(proxyIndex, proxyIndex);
    }
}

void FileBrowserSidebar::onItemDoubleClicked(const QModelIndex &index) {
    QModelIndex sourceIndex = resolveSourceIndex(index);
    QString filePath = m_model->filePath(sourceIndex);
    if (QFileInfo(filePath).isFile()) {
        emit fileSelected(filePath);
    }
}

void FileBrowserSidebar::onItemClicked(const QModelIndex &index) {
    QModelIndex sourceIndex = resolveSourceIndex(index);
    QString filePath = m_model->filePath(sourceIndex);
    if (QFileInfo(filePath).isFile()) {
        emit fileSelected(filePath);
    }
}

void FileBrowserSidebar::onModifiedFilesChanged(const QStringList &files) {
    m_modifiedFiles = files;
    auto *delegate = qobject_cast<FileBrowserDelegate*>(m_treeView->itemDelegate());
    if (delegate) {
        delegate->setModifiedFiles(m_modifiedFiles);
        m_treeView->viewport()->update();
    }
}

void FileBrowserSidebar::onGitModifiedFilesChanged(const QMap<QString, QString> &files) {
    m_gitModifiedFiles = files;
    
    
    for (auto it = files.begin(); it != files.end(); ++it) {
        QFileInfo fi(it.key());
        QDir dir = fi.dir();
        while (dir.absolutePath() != m_projectRoot && dir.absolutePath().length() >= m_projectRoot.length()) {
            if (!m_gitModifiedFiles.contains(dir.absolutePath())) {
                m_gitModifiedFiles[dir.absolutePath()] = it.value();
            } else if (m_gitModifiedFiles[dir.absolutePath()] == "(U)" && it.value() == "(M)") {
                m_gitModifiedFiles[dir.absolutePath()] = "(M)"; 
            }
            if (!dir.cdUp()) break;
        }
    }
    
    auto *delegate = qobject_cast<FileBrowserDelegate*>(m_treeView->itemDelegate());
    if (delegate) {
        delegate->setGitModifiedFiles(m_gitModifiedFiles);
        m_treeView->viewport()->update();
    }
}

void FileBrowserSidebar::onContextMenuRequested(const QPoint &pos) {
    QModelIndex index = m_treeView->indexAt(pos);
    if (index.isValid()) {
        m_treeView->setCurrentIndex(index);
    }
    
    if (!index.isValid()) {
        m_contextMenuIndex = QModelIndex();
        showRootContextMenu(m_treeView->viewport()->mapToGlobal(pos));
        return;
    }
    m_contextMenuIndex = resolveSourceIndex(index);
    showContextMenu(m_treeView->viewport()->mapToGlobal(pos), index);
}

void FileBrowserSidebar::showContextMenu(const QPoint &globalPos, const QModelIndex &index) {
    QModelIndex sourceIndex = resolveSourceIndex(index);
    QString filePath = m_model->filePath(sourceIndex);
    QFileInfo fileInfo(filePath);
    
    QMenu contextMenu;
    
    
    contextMenu.addAction("&Open", this, &FileBrowserSidebar::onOpenFile);
    contextMenu.addSeparator();
    
    if (fileInfo.isDir()) {
        contextMenu.addAction(m_newFileAction);
        contextMenu.addAction(m_newFolderAction);
        contextMenu.addSeparator();
    }
    
    contextMenu.addAction(m_renameAction);
    contextMenu.addAction(m_deleteAction);
    contextMenu.addSeparator();
    contextMenu.addAction(m_copyPathAction);
    contextMenu.addAction(m_revealAction);
    
    contextMenu.exec(globalPos);
}

void FileBrowserSidebar::showRootContextMenu(const QPoint &globalPos) {
    QMenu contextMenu;
    
    contextMenu.addAction(m_newFileAction);
    contextMenu.addAction(m_newFolderAction);
    contextMenu.addSeparator();
    contextMenu.addAction(m_revealAction);
    
    contextMenu.exec(globalPos);
}

void FileBrowserSidebar::onOpenFile() {
    QString filePath = getSelectedPath();
    if (!filePath.isEmpty() && QFileInfo(filePath).isFile()) {
        emit fileSelected(filePath);
    }
}

void FileBrowserSidebar::onDeleteFile() {
    ensureContextIndex();

    QString filePath = getSelectedPath();
    if (filePath.isEmpty()) {
        return;
    }
    
    QFileInfo fileInfo(filePath);
    QString itemType = fileInfo.isDir() ? "folder" : "file";
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Delete " + itemType,
        "Are you sure you want to delete:\n" + fileInfo.fileName() + "?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        bool success = false;
        if (fileInfo.isDir()) {
            QDir dir(filePath);
            success = dir.removeRecursively();
        } else {
            success = QFile::remove(filePath);
        }
        
        if (success) {
            emit fileDeleted(filePath);
        } else {
            QMessageBox::warning(this, "Error", "Failed to delete " + itemType + ".");
        }
    }
    m_contextMenuIndex = QModelIndex();
}

void FileBrowserSidebar::onRenameFile() {
    ensureContextIndex();

    QString filePath = getSelectedPath();
    if (filePath.isEmpty()) {
        return;
    }
    
    QFileInfo fileInfo(filePath);
    QString currentName = fileInfo.fileName();
    
    bool ok;
    QString newName = QInputDialog::getText(
        this,
        "Rename",
        "New name:",
        QLineEdit::Normal,
        currentName,
        &ok
    );
    
    if (ok && !newName.isEmpty() && newName != currentName) {
        QString newPath = fileInfo.dir().filePath(newName);
        if (QFile::rename(filePath, newPath)) {
            emit fileRenamed(filePath, newPath);
        } else {
            QMessageBox::warning(this, "Error", "Failed to rename file.");
        }
    }
    m_contextMenuIndex = QModelIndex();
}

void FileBrowserSidebar::onNewFolder() {
    ensureContextIndex();

    QString parentPath = getContextPath();
    if (parentPath.isEmpty()) {
        return;
    }
    
    bool ok;
    QString folderName = QInputDialog::getText(
        this,
        "New Folder",
        "Folder name:",
        QLineEdit::Normal,
        "New Folder",
        &ok
    );
    
    if (ok && !folderName.isEmpty()) {
        QString newFolderPath = QDir(parentPath).filePath(folderName);
        if (QDir().mkdir(newFolderPath)) {
            emit folderCreated(newFolderPath);
        } else {
            QMessageBox::warning(this, "Error", "Failed to create folder.");
        }
    }
    m_contextMenuIndex = QModelIndex();
}

void FileBrowserSidebar::onNewFile() {
    ensureContextIndex();

    QString parentPath = getContextPath();
    if (parentPath.isEmpty()) {
        return;
    }
    
    bool ok;
    QString fileName = QInputDialog::getText(
        this,
        "New File",
        "File name:",
        QLineEdit::Normal,
        "untitled.txt",
        &ok
    );
    
    if (ok && !fileName.isEmpty()) {
        QString newFilePath = QDir(parentPath).filePath(fileName);
        QFile file(newFilePath);
        if (file.open(QFile::WriteOnly)) {
            file.close();
            emit fileSelected(newFilePath);
        } else {
            QMessageBox::warning(this, "Error", "Failed to create file.");
        }
    }
    m_contextMenuIndex = QModelIndex();
}

void FileBrowserSidebar::onCopyPath() {
    ensureContextIndex();

    QString filePath = getSelectedPath();
    if (!filePath.isEmpty()) {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(filePath);
    }
    m_contextMenuIndex = QModelIndex();
}

void FileBrowserSidebar::onRevealInExplorer() {
    ensureContextIndex();

    QString filePath = getSelectedPath();
    if (filePath.isEmpty()) {
        return;
    }
    
    QFileInfo fileInfo(filePath);
    QString pathToOpen = fileInfo.isDir() ? filePath : fileInfo.dir().path();
    
    QDesktopServices::openUrl(QUrl::fromLocalFile(pathToOpen));
    m_contextMenuIndex = QModelIndex();
}

QString FileBrowserSidebar::getSelectedPath() const {
    if (!m_contextMenuIndex.isValid()) {
        QModelIndex current = m_treeView->currentIndex();
        if (current.isValid()) {
            return m_model->filePath(resolveSourceIndex(current));
        }
        return QString();
    }
    return m_model->filePath(m_contextMenuIndex);
}

QString FileBrowserSidebar::getContextPath() const {
    if (!m_contextMenuIndex.isValid()) {
        return m_projectRoot;
    }
    
    QString selectedPath = m_model->filePath(m_contextMenuIndex);
    QFileInfo fileInfo(selectedPath);
    return fileInfo.isDir() ? selectedPath : fileInfo.dir().path();
}

QModelIndex FileBrowserSidebar::resolveSourceIndex(const QModelIndex &proxyIndex) const {
    auto *proxyModel = qobject_cast<QSortFilterProxyModel*>(m_treeView->model());
    return proxyModel ? proxyModel->mapToSource(proxyIndex) : proxyIndex;
}

void FileBrowserSidebar::ensureContextIndex() {
    if (m_contextMenuIndex.isValid()) return;
    QModelIndex current = m_treeView->currentIndex();
    if (current.isValid()) {
        m_contextMenuIndex = resolveSourceIndex(current);
    }
}

void FileBrowserSidebar::updateTheme() {
    auto *delegate = qobject_cast<FileBrowserDelegate*>(m_treeView->itemDelegate());
    if (delegate) {
        delegate->clearCache();
    }
    m_treeView->viewport()->update();
}

} 
