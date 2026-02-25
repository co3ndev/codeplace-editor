#include "tab_container.h"
#include <QVBoxLayout>
#include <QFileInfo>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QMenu>
#include <QTabBar>
#include <QFileDialog>
#include <QStringListModel>
#include "core/theme_manager.h"
#include "core/settings_manager.h"
#include "core/simple_highlighter.h"
#include "lspclient/lsp_manager.h"
#include "lspclient/lspclient_widget.h"
#include "gitscm/diff_view.h"

namespace Editor {

TabContainer::TabContainer(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setUsesScrollButtons(true);
    m_tabWidget->setDocumentMode(true);
    
    m_findBar = new FindReplaceBar(this);
	
    m_stackedWidget = new QStackedWidget(this);
    m_quickStartWidget = new QuickStartWidget(this);

    m_stackedWidget->addWidget(m_quickStartWidget);
    m_stackedWidget->addWidget(m_tabWidget);

    layout->addWidget(m_stackedWidget);
    layout->addWidget(m_findBar);

    updateEmptyState();

    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &TabContainer::onTabCloseRequested);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &TabContainer::onCurrentChanged);

    
    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int) {
        if (m_findBar->isVisible()) {
            m_findBar->hideBar();
        }
    });
    
    m_tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tabWidget, &QTabWidget::customContextMenuRequested, this, &TabContainer::showContextMenu);

    auto &lsp = LspClient::LspManager::instance();
    connect(&lsp, &LspClient::LspManager::completionReady, this, [this](const QString &uri, const QJsonArray &items) {
        for (const auto &tab : m_tabs) {
            if (auto *editor = qobject_cast<EditorView*>(tab.widget)) {
                editor->handleCompletionResult(uri, items);
            }
        }
    });
    connect(&lsp, &LspClient::LspManager::diagnosticsReady, this, [this](const QString &uri, const QJsonArray &diags) {
        for (const auto &tab : m_tabs) {
            if (auto *editor = qobject_cast<EditorView*>(tab.widget)) {
                editor->onDiagnosticsReady(uri, diags);
            }
        }
    });
    connect(&lsp, &LspClient::LspManager::hoverReady, this, [this](const QString &uri, const QJsonObject &hover) {
        for (const auto &tab : m_tabs) {
            if (auto *editor = qobject_cast<EditorView*>(tab.widget)) {
                editor->handleHoverResult(uri, hover);
            }
        }
    });
    connect(&lsp, &LspClient::LspManager::renameReady, this, [this](const QString &uri, const QJsonObject &edit) {
        for (const auto &tab : m_tabs) {
            if (auto *editor = qobject_cast<EditorView*>(tab.widget)) {
                editor->handleRenameResult(uri, edit);
            }
        }
    });
    connect(&lsp, &LspClient::LspManager::formattingReady, this, [this](const QString &uri, const QJsonArray &edits) {
        for (const auto &tab : m_tabs) {
            if (auto *editor = qobject_cast<EditorView*>(tab.widget)) {
                editor->handleFormattingResult(uri, edits);
            }
        }
    });
    connect(&lsp, &LspClient::LspManager::definitionReady, this, [this](const QString &uri, const QJsonValue &result) {
        QJsonObject location;
        if (result.isArray()) {
            QJsonArray arr = result.toArray();
            if (arr.isEmpty()) return;
            location = arr[0].toObject();
        } else if (result.isObject()) {
            location = result.toObject();
        } else { return; }

        QString targetUri = location["uri"].toString();
        if (targetUri.startsWith("file://")) targetUri = QUrl(targetUri).toLocalFile();
        
        QJsonObject range = location["range"].toObject();
        QJsonObject start = range["start"].toObject();
        int line = start["line"].toInt() + 1;
        int col = start["character"].toInt();

        int tabIdx = findTabByPath(uri);
        if (tabIdx != -1) {
            if (targetUri == uri) {
                if (auto *editor = qobject_cast<EditorView*>(m_tabs[tabIdx].widget)) {
                    editor->goToLine(line, col);
                }
            } else {
                openFile(targetUri, line, col);
            }
        }
    });
}

void TabContainer::openFile(const QString &filePath) {
    int existingIndex = findTabByPath(filePath);
    if (existingIndex != -1) {
        m_tabWidget->setCurrentIndex(existingIndex);
        return;
    }

    auto *editor = new EditorView(this);
    
    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();
    auto lang = Core::SimpleHighlighter::languageForExtension(ext);
    Core::SimpleHighlighter *highlighter = nullptr;
    if (lang.has_value()) {
        highlighter = new Core::SimpleHighlighter(editor->document(), lang.value());
    }
    editor->setFilePath(filePath);
    
    QFileInfo fileInfo(filePath);
    if (fileInfo.size() > 10 * 1024 * 1024) {
        QMessageBox::StandardButton ret = QMessageBox::warning(this, "Large File",
            QString("The file '%1' is %2 MB which may cause slowdowns.\nOpen anyway?")
            .arg(fileInfo.fileName())
            .arg(fileInfo.size() / (1024.0 * 1024.0), 0, 'f', 1),
            QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) return;
    }

    QFile file(filePath);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&file);
        editor->setPlainText(in.readAll());
    } else {
        QMessageBox::warning(this, "Error", "Could not open file: " + filePath);
    }
    editor->document()->setModified(false);

    TabInfo info = {editor, filePath, highlighter};
    m_tabs.append(info);
    
    int index = m_tabWidget->addTab(editor, "");
    updateTabTitle(index);
    updateEmptyState();
    m_tabWidget->setCurrentIndex(index);
    emit openFilesChanged(openFilePaths());

    if (lang.has_value()) {
        QString langStr = LspClient::LspClientWidget::languageToString(lang.value());
        qDebug() << "TabContainer: calling documentOpened for" << filePath << "lang:" << langStr;
        LspClient::LspManager::instance().documentOpened(
            filePath, editor->toPlainText(), langStr);
            
        QCompleter *completer = new QCompleter(editor);
        QStringListModel *model = new QStringListModel(completer);
        completer->setModel(model);
        editor->setCompleter(completer);
    }

    connect(editor->document(), &QTextDocument::modificationChanged, this, &TabContainer::onModificationChanged);
    
    connect(editor->document(), &QTextDocument::contentsChange, this, [this, filePath, lang, editor](int , int , int charsAdded) {
        if (lang.has_value()) {
            LspClient::LspManager::instance().documentChanged(
                filePath, editor->toPlainText(), 1);
                
            if (charsAdded > 0) {
                QTextCursor cursor = editor->textCursor();
                LspClient::LspManager::instance().requestCompletion(filePath, cursor.blockNumber(), cursor.positionInBlock());
            }
        }
    });
}

void TabContainer::openDiff(const QString &filePath) {
    QFileInfo fi(filePath);
    QString absFilePath = fi.absoluteFilePath();
    
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (QFileInfo(m_tabs[i].filePath).absoluteFilePath() == absFilePath && m_tabs[i].isDiff) {
            m_tabWidget->setCurrentIndex(i);
            return;
        }
    }

    auto *diffView = new GitScm::DiffView(this);
    
    QString gitRoot;
    QDir dir = fi.absoluteDir();
    while (!dir.isRoot()) {
        if (dir.exists(".git")) {
            gitRoot = dir.absolutePath();
            break;
        }
        dir.cdUp();
    }

    if (gitRoot.isEmpty()) gitRoot = fi.absolutePath();

    QString relPath = QDir(gitRoot).relativeFilePath(absFilePath);

    QProcess git;
    git.setWorkingDirectory(gitRoot);
    git.start("git", QStringList() << "show" << "HEAD:" + relPath);
    git.waitForFinished();
    QString oldContent = QString::fromUtf8(git.readAllStandardOutput());

    QFile file(absFilePath);
    QString newContent;
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        newContent = QTextStream(&file).readAll();
    }

    diffView->setDiff(oldContent, newContent, absFilePath);

    TabInfo info = {diffView, absFilePath, nullptr, true};
    m_tabs.append(info);
    
    int index = m_tabWidget->addTab(diffView, "Diff: " + fi.fileName());
    m_tabWidget->setCurrentIndex(index);
    updateEmptyState();
    emit openFilesChanged(openFilePaths());
}

void TabContainer::openFile(const QString &filePath, int line, int column) {
    openFile(filePath);
    int index = findTabByPath(filePath);
    if (index != -1) {
        if (auto *editor = qobject_cast<EditorView*>(m_tabs[index].widget)) {
            editor->goToLine(line, column);
        }
    }
}

void TabContainer::openNewFile() {
    auto *editor = new EditorView(this);
    

    TabInfo info = {editor, QString(), nullptr, false};
    m_tabs.append(info);

    int index = m_tabWidget->addTab(editor, "");
    updateTabTitle(index);
    updateEmptyState();
    m_tabWidget->setCurrentIndex(index);
    emit openFilesChanged(openFilePaths());

    connect(editor->document(), &QTextDocument::modificationChanged, this, &TabContainer::onModificationChanged);
}

void TabContainer::closeCurrentTab() {
    int index = m_tabWidget->currentIndex();
    if (index != -1) {
        onTabCloseRequested(index);
    }
}

void TabContainer::onTabCloseRequested(int index) {
    if (maybeSaveTab(index)) {

        QWidget *widget = m_tabWidget->widget(index);
        QString filePath = m_tabs[index].filePath;
        
        if (m_tabs[index].highlighter) {
            LspClient::LspManager::instance().documentClosed(filePath);
        }

        m_tabs.removeAt(index);
        m_tabWidget->removeTab(index);
        updateEmptyState();
        delete widget;
        emit openFilesChanged(openFilePaths());
    }
}

void TabContainer::onCurrentChanged(int index) {
    EditorView *editor = nullptr;
    QString filePath;
    if (index >= 0 && index < m_tabs.size()) {
        filePath = m_tabs[index].filePath;
        editor = qobject_cast<EditorView*>(m_tabs[index].widget);
    }
    emit currentFileChanged(filePath);
    emit activeEditorChanged(editor);
}

void TabContainer::onModificationChanged(bool ) {
    
    QTextDocument *changedDoc = qobject_cast<QTextDocument*>(sender());
    int changedIndex = -1;
    
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (auto *editor = qobject_cast<EditorView*>(m_tabs[i].widget)) {
            if (editor->document() == changedDoc) {
                changedIndex = i;
                break;
            }
        }
    }
    
    if (changedIndex != -1) {
        updateTabTitle(changedIndex);
    }

    emit modifiedFilesChanged(modifiedFilePaths());
}

void TabContainer::updateTabTitle(int index) {
    if (index < 0 || index >= m_tabs.size()) return;
    const auto &info = m_tabs[index];
    QString name = QFileInfo(info.filePath).fileName();
    if (name.isEmpty()) name = "untitled";
    
    if (info.isDiff) {
        name = "Diff: " + name;
    } else if (auto *editor = qobject_cast<EditorView*>(info.widget)) {
        if (editor->document()->isModified()) {
            name += "*";
        }
    }

    m_tabWidget->setTabText(index, name);
    
    QColor normalColor = Core::ThemeManager::instance().getColor(Core::ThemeManager::EditorForeground);
    if (m_gitModifiedFiles.contains(info.filePath)) {
        QString status = m_gitModifiedFiles.value(info.filePath);
        if (status == "(U)") {
            name += " (U)";
            normalColor = Core::ThemeManager::instance().getColor(Core::ThemeManager::GitUntrackedIndicator);
        } else {
            name += " (M)";
            normalColor = Core::ThemeManager::instance().getColor(Core::ThemeManager::GitModifiedIndicator);
        }
    }
    m_tabWidget->tabBar()->setTabTextColor(index, normalColor);
}

int TabContainer::findTabByPath(const QString &filePath) {
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i].filePath == filePath && !m_tabs[i].isDiff) return i;
    }
    return -1;
}

EditorView* TabContainer::currentEditor() const {
    int index = m_tabWidget->currentIndex();
    if (index >= 0 && index < m_tabs.size()) return qobject_cast<EditorView*>(m_tabs[index].widget);
    return nullptr;
}

int TabContainer::currentTabIndex() const {
    return m_tabWidget->currentIndex();
}

int TabContainer::count() const {
    return m_tabWidget->count();
}

void TabContainer::setCurrentIndex(int index) {
    m_tabWidget->setCurrentIndex(index);
}

QString TabContainer::currentFilePath() const {
    int index = m_tabWidget->currentIndex();
    if (index >= 0 && index < m_tabs.size()) return m_tabs[index].filePath;
    return QString();
}

QStringList TabContainer::openFilePaths() const {
    QStringList paths;
    for (const auto &info : m_tabs) {
        paths.append(info.filePath);
    }
    return paths;
}

QString TabContainer::getFileContent(int index) const {
    if (index < 0 || index >= m_tabs.size()) return QString();
    if (auto *editor = qobject_cast<EditorView*>(m_tabs[index].widget)) {
        return editor->toPlainText();
    }
    return QString();
}

void TabContainer::applySettings() {
    for (auto &info : m_tabs) {
        if (auto *editor = qobject_cast<EditorView*>(info.widget)) {
            editor->applySettings();
        }
    }
}

void TabContainer::selectNextTab() {
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex != -1 && currentIndex < m_tabs.size() - 1) {
        m_tabWidget->setCurrentIndex(currentIndex + 1);
    } else if (m_tabs.size() > 0) {
        m_tabWidget->setCurrentIndex(0);
    }
}

void TabContainer::selectPreviousTab() {
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex > 0) {
        m_tabWidget->setCurrentIndex(currentIndex - 1);
    } else if (m_tabs.size() > 0) {
        m_tabWidget->setCurrentIndex(m_tabs.size() - 1);
    }
}

bool TabContainer::maybeSaveTab(int index) {
    auto *editor = qobject_cast<EditorView*>(m_tabs[index].widget);
    if (!editor || !editor->document()->isModified())
        return true;

    QMessageBox::StandardButton ret = QMessageBox::warning(this, "Unsaved Changes",
        QString("The document '%1' has been modified.\nDo you want to save your changes?")
        .arg(QFileInfo(m_tabs[index].filePath).fileName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save) {
        if (m_tabs[index].filePath.isEmpty()) {
            return saveFileAs(index);
        }
        return saveFile(index);
    }
    if (ret == QMessageBox::Cancel)
        return false;
    
    
    editor->document()->setModified(false);
    updateTabTitle(index);
    
    emit modifiedFilesChanged(modifiedFilePaths());
    
    return true;
}

bool TabContainer::maybeSaveAll() {
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (!maybeSaveTab(i)) return false;
    }
    return true;
}

bool TabContainer::saveFile(int index) {
    QString filePath = m_tabs[index].filePath;
    if (filePath.isEmpty()) return false; 

    QFile file(filePath);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream out(&file);
        if (auto *editor = qobject_cast<EditorView*>(m_tabs[index].widget)) {
            out << editor->toPlainText();
            editor->document()->setModified(false);
            updateTabTitle(index);
            return true;
        }
    }
    QMessageBox::warning(this, "Error", "Could not save file: " + filePath);
    return false;
}

bool TabContainer::saveCurrentFile() {
    int index = m_tabWidget->currentIndex();
    if (index != -1) return saveFile(index);
    return false;
}

void TabContainer::showContextMenu(const QPoint &pos) {
    int index = m_tabWidget->tabBar()->tabAt(pos);
    if (index == -1) return;

    QMenu menu(this);
    menu.addAction("Close", [this, index]() { onTabCloseRequested(index); });
    menu.addAction("Close Others", [this, index]() { closeOtherTabs(index); });
    menu.addAction("Close All", [this]() { closeAllTabs(); });
    
    menu.exec(m_tabWidget->mapToGlobal(pos));
}

void TabContainer::closeOtherTabs(int index) {
    for (int i = m_tabs.size() - 1; i >= 0; --i) {
        if (i != index) {
            onTabCloseRequested(i);
        }
    }
}

bool TabContainer::closeAllTabs() {
    
    for (int i = m_tabs.size() - 1; i >= 0; --i) {
        if (!maybeSaveTab(i)) return false;
        

        QWidget *widget = m_tabWidget->widget(i);
        QString filePath = m_tabs[i].filePath;
        
        if (m_tabs[i].highlighter) {
            LspClient::LspManager::instance().documentClosed(filePath);
        }

        m_tabs.removeAt(i);
        m_tabWidget->removeTab(i);
        delete widget;
    }
    
    updateEmptyState();
    emit currentFileChanged(QString());
    emit openFilesChanged(openFilePaths());
    
    return true;
}

bool TabContainer::saveFileAs(int index) {
     QString fileName = QFileDialog::getSaveFileName(this, "Save File",
         m_tabs[index].filePath.isEmpty() ? "untitled.txt" : m_tabs[index].filePath);
     
     if (fileName.isEmpty()) return false;

     auto *editor = qobject_cast<EditorView*>(m_tabs[index].widget);
     if (!editor) return false;

     QFile file(fileName);
     if (file.open(QFile::WriteOnly | QFile::Text)) {
         QTextStream out(&file);
         out << editor->toPlainText();
         m_tabs[index].filePath = fileName;
         editor->setFilePath(fileName);
         editor->document()->setModified(false);
         
         
         QFileInfo fi(fileName);
         QString ext = fi.suffix().toLower();
         auto lang = Core::SimpleHighlighter::languageForExtension(ext);
         if (lang.has_value()) {
             
             if (m_tabs[index].highlighter) {
                 delete m_tabs[index].highlighter;
                 m_tabs[index].highlighter = nullptr;
             }
             
             m_tabs[index].highlighter = new Core::SimpleHighlighter(editor->document(), lang.value());
         }
         
         updateTabTitle(index);
         
         
         emit fileExtensionChanged(fileName);
         emit openFilesChanged(openFilePaths());
         
         return true;
     }
     
     QMessageBox::warning(this, "Error", "Could not save file: " + fileName);
     return false;
 }

void TabContainer::showFind() {
    if (currentEditor()) {
        m_findBar->showFind(currentEditor());
    }
}

void TabContainer::showReplace() {
    if (currentEditor()) {
        m_findBar->showReplace(currentEditor());
    }
}

void TabContainer::findNext() {
    if (m_findBar->isVisible()) {
        m_findBar->performFindNext();
    } else if (currentEditor()) {
        m_findBar->showFind(currentEditor());
    }
}

void TabContainer::findPrevious() {
    if (m_findBar->isVisible()) {
        m_findBar->performFindPrevious();
    } else if (currentEditor()) {
        m_findBar->showFind(currentEditor());
    }
}

void TabContainer::updateEmptyState() {
    if (m_tabs.isEmpty()) {
        m_stackedWidget->setCurrentWidget(m_quickStartWidget);
    } else {
        m_stackedWidget->setCurrentWidget(m_tabWidget);
    }
}

QStringList TabContainer::modifiedFilePaths() const {
    QStringList paths;
    for (const auto &info : m_tabs) {
        if (auto *editor = qobject_cast<EditorView*>(info.widget)) {
            if (editor->document()->isModified()) {
                paths.append(info.filePath);
            }
        }
    }
    return paths;
}

void TabContainer::onGitModifiedFilesChanged(const QMap<QString, QString> &files) {
    m_gitModifiedFiles = files;
    for (int i = 0; i < m_tabs.size(); ++i) {
        updateTabTitle(i);
    }
}

} 
