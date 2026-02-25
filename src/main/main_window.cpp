#include "main_window.h"
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QIcon>
#include <QMimeDatabase>
#include <QMimeType>
#include "preferences_dialog.h"
#include "core/settings_manager.h"
#include "core/theme_manager.h"
#include "core/session_manager.h"
#include "core/default_shortcuts.h"
#include "core/project_manager.h"
#include <QStandardPaths>
#include <QDir>
#include <QDockWidget>
#include "terminal/terminal_widget.h"
#include "search/search_widget.h"
#include "outline/outline_widget.h"
#include "gitscm/gitscm_widget.h"
#include "gitscm/git_manager.h"
#include "lspclient/lsp_manager.h"

namespace Main {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupLayout();
    createMenus();

    setWindowTitle("CodePlace Editor");
    setWindowIcon(QIcon(":/resources/codeplace.png"));

    QString sandboxPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/projects";
    QDir().mkpath(sandboxPath);
    QString initialDir = sandboxPath;
    Core::ProjectManager::instance().setProjectRoot(initialDir);

    applySettings();
    applyShortcuts();
    
    
    auto state = Core::SessionManager::instance().loadSession();
    if (!state.windowGeometry.isEmpty()) {
        restoreGeometry(state.windowGeometry);
    } else {
        resize(1000, 762);
    }
    if (!state.windowState.isEmpty()) restoreState(state.windowState);
    
    setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    applySettings();

    
    connect(&Core::SettingsManager::instance(), &Core::SettingsManager::settingsChanged,
            this, [this](const QString &key) {
                if (key.startsWith("shortcuts/")) {
                    applyShortcuts();
                } else {
                    applySettings();
                }
            });

    
    Core::ThemeManager::instance().applyTheme(Core::SettingsManager::instance().theme());

    
    connect(&Core::ThemeManager::instance(), &Core::ThemeManager::themeChanged, this, [this]() {
        updateStatusBar();
    });
    
}

void MainWindow::setupLayout() {
    
    
    m_tabContainer = new Editor::TabContainer(this);
    setCentralWidget(m_tabContainer);

    
    m_sidebar = new Browser::FileBrowserSidebar(this);
    m_sidebarDock = createDockWidget("Explorer", "sidebarDock", m_sidebar, Qt::LeftDockWidgetArea);



    setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::West);
    setTabPosition(Qt::RightDockWidgetArea, QTabWidget::East);

    m_terminalWidget = new Terminal::TerminalWidget(this);
    m_terminalDock = createDockWidget("Terminal", "terminalDock", m_terminalWidget, Qt::BottomDockWidgetArea);

    m_problemsWidget = new LspClient::ProblemsWidget(this);
    m_problemsDock = createDockWidget("Problems", "problemsDock", m_problemsWidget, Qt::BottomDockWidgetArea);

    tabifyDockWidget(m_terminalDock, m_problemsDock);
    m_terminalDock->raise();

    connect(&LspClient::LspManager::instance(), &LspClient::LspManager::diagnosticsReady, 
            m_problemsWidget, &LspClient::ProblemsWidget::onDiagnosticsReady);
            
    connect(m_tabContainer, &Editor::TabContainer::openFilesChanged, m_problemsWidget, &LspClient::ProblemsWidget::onOpenFilesChanged);
    
    connect(m_problemsWidget, &LspClient::ProblemsWidget::problemSelected, this, [this](const QString &path, int line, int col) {
        m_tabContainer->openFile(path, line, col);
    });

    connect(m_sidebar, &Browser::FileBrowserSidebar::fileSelected, this, &MainWindow::onFileSelected);
    
    auto *searchWidget = new Search::SearchWidget(this);
    m_searchDock = createDockWidget("Search", "searchDock", searchWidget, Qt::LeftDockWidgetArea);

    connect(searchWidget, &Search::SearchWidget::fileSelected, this, [this](const QString &path, int line, int column) {
        m_tabContainer->openFile(path, line, column);
    });
    
    auto *outlineWidget = new Outline::OutlineWidget(this);
    m_outlineDock = createDockWidget("Outline", "outlineDock", outlineWidget, Qt::LeftDockWidgetArea);

    connect(outlineWidget, &Outline::OutlineWidget::symbolSelected, [this](int line, int column) {
        if (auto editor = m_tabContainer->currentEditor()) {
            editor->goToLine(line, column);
        }
    });

    auto *gitscmWidget = new GitScm::GitScmWidget(this);
    m_gitscmDock = createDockWidget("Source Control", "gitscmDock", gitscmWidget, Qt::LeftDockWidgetArea);

    connect(gitscmWidget, &GitScm::GitScmWidget::fileSelected, this, [this](const QString &path) {
        m_tabContainer->openFile(path);
    });
    connect(gitscmWidget, &GitScm::GitScmWidget::diffRequested, this, [this](const QString &path) {
        m_tabContainer->openDiff(path);
    });
    connect(&GitScm::GitManager::instance(), &GitScm::GitManager::statusUpdated, m_sidebar, &Browser::FileBrowserSidebar::onGitModifiedFilesChanged);
    connect(&GitScm::GitManager::instance(), &GitScm::GitManager::statusUpdated, m_tabContainer, &Editor::TabContainer::onGitModifiedFilesChanged);

    tabifyDockWidget(m_sidebarDock, m_searchDock);
    tabifyDockWidget(m_searchDock, m_outlineDock);
    tabifyDockWidget(m_outlineDock, m_gitscmDock);
    
    m_aiSidebar = new AiChatSidebar(this);
    m_aiDock = createDockWidget("AI Chat", "aiDock", m_aiSidebar, Qt::RightDockWidgetArea);

    tabifyDockWidget(m_terminalDock, m_problemsDock);

    m_sidebarDock->show();
    m_searchDock->show();
    m_outlineDock->show();
    m_gitscmDock->show();
    m_aiDock->show();
    m_terminalDock->show();
    m_problemsDock->show();

    m_sidebarDock->raise();
    m_terminalDock->raise();

    
    

    connect(m_tabContainer, &Editor::TabContainer::currentFileChanged, this, [this, outlineWidget](const QString &path) {
        QString shownName = path.isEmpty() ? "untitled" : QFileInfo(path).fileName();
        setWindowTitle(QString("%1[*] - CodePlace Editor").arg(shownName));
        outlineWidget->setEditor(m_tabContainer->currentEditor());
        updateStatusBar();
        
    });

    connect(m_tabContainer, &Editor::TabContainer::activeEditorChanged, this, [this](Editor::EditorView *editor) {
        if (m_lastConnectedEditor) {
            disconnect(m_lastConnectedEditor, &Editor::EditorView::cursorPositionChanged, this, &MainWindow::updateStatusBar);
        }
        m_lastConnectedEditor = editor;
        if (editor) {
            connect(editor, &Editor::EditorView::cursorPositionChanged, this, &MainWindow::updateStatusBar);
        }
        updateStatusBar();
    });

    connect(m_tabContainer, &Editor::TabContainer::modifiedFilesChanged, m_sidebar, &Browser::FileBrowserSidebar::onModifiedFilesChanged);

    
    connect(m_tabContainer, &Editor::TabContainer::fileExtensionChanged, m_sidebar, &Browser::FileBrowserSidebar::refreshFileIcon);

    
    statusBar()->setSizeGripEnabled(false);
    statusBar()->setFixedHeight(24);

    m_statusCursorLabel = new QLabel("Ln 1, Col 1");
    m_statusFileTypeLabel = new QLabel("Plain Text");
    m_statusEncodingLabel = new QLabel("UTF-8");

    m_statusCursorLabel->setContentsMargins(8, 0, 8, 0);
    m_statusFileTypeLabel->setContentsMargins(8, 0, 8, 0);
    m_statusEncodingLabel->setContentsMargins(8, 0, 8, 0);

    statusBar()->addPermanentWidget(m_statusCursorLabel);
    statusBar()->addPermanentWidget(m_statusFileTypeLabel);
    statusBar()->addPermanentWidget(m_statusEncodingLabel);
}

void MainWindow::createMenus() {
    QMenu *fileMenu = menuBar()->addMenu("&File");

    auto *newAction = fileMenu->addAction("&New", this, &MainWindow::onNewFile);
    m_actionMap["New"] = newAction;

    auto *openAction = fileMenu->addAction("&Open...", this, &MainWindow::onOpenFile);
    m_actionMap["Open"] = openAction;

    auto *openFolderAction = fileMenu->addAction("Open &Folder...", this, &MainWindow::onOpenFolder);
    m_actionMap["Open Folder"] = openFolderAction;

    fileMenu->addSeparator();

    auto *saveAction = fileMenu->addAction("&Save", this, &MainWindow::onSaveFile);
    m_actionMap["Save"] = saveAction;

    auto *saveAsAction = fileMenu->addAction("Save &As...", this, &MainWindow::onSaveAsFile);
    m_actionMap["Save As"] = saveAsAction;

    auto *saveAllAction = fileMenu->addAction("Save A&ll", this, &MainWindow::onSaveAll);
    m_actionMap["Save All"] = saveAllAction;

    fileMenu->addSeparator();

    auto *exitAction = fileMenu->addAction("E&xit", this, &QWidget::close);
    m_actionMap["Exit"] = exitAction;

    QMenu *editMenu = menuBar()->addMenu("&Edit");

    auto *undoAction = editMenu->addAction("&Undo", [this]() {
        if (m_tabContainer->currentEditor()) m_tabContainer->currentEditor()->undo();
    });
    undoAction->setShortcutContext(Qt::ApplicationShortcut);
    m_actionMap["Undo"] = undoAction;
    
    auto *redoAction = editMenu->addAction("&Redo", [this]() {
        if (m_tabContainer->currentEditor()) m_tabContainer->currentEditor()->redo();
    });
    redoAction->setShortcutContext(Qt::ApplicationShortcut);
    m_actionMap["Redo"] = redoAction;
    
    editMenu->addSeparator();
    
    auto *cutAction = editMenu->addAction("Cu&t", [this]() {
        if (m_tabContainer->currentEditor()) m_tabContainer->currentEditor()->cut();
    });
    cutAction->setShortcutContext(Qt::ApplicationShortcut);
    m_actionMap["Cut"] = cutAction;
    
    auto *copyAction = editMenu->addAction("&Copy", [this]() {
        if (m_tabContainer->currentEditor()) m_tabContainer->currentEditor()->copy();
    });
    copyAction->setShortcutContext(Qt::ApplicationShortcut);
    m_actionMap["Copy"] = copyAction;
    
    auto *pasteAction = editMenu->addAction("&Paste", [this]() {
        if (m_tabContainer->currentEditor()) m_tabContainer->currentEditor()->paste();
    });
    pasteAction->setShortcutContext(Qt::ApplicationShortcut);
    m_actionMap["Paste"] = pasteAction;
    
    editMenu->addSeparator();
    
    auto *selectAllAction = editMenu->addAction("Select &All", [this]() {
        if (m_tabContainer->currentEditor()) m_tabContainer->currentEditor()->selectAll();
    });
    selectAllAction->setShortcutContext(Qt::ApplicationShortcut);
    m_actionMap["Select All"] = selectAllAction;
    
    auto *findAction = editMenu->addAction("&Find", [this]() {
        m_tabContainer->showFind();
    });
    m_actionMap["Find"] = findAction;
    
    auto *findNextAction = editMenu->addAction("Find &Next", [this]() {
        m_tabContainer->findNext();
    });
    m_actionMap["Find Next"] = findNextAction;

    auto *findPrevAction = editMenu->addAction("Find Pre&vious", [this]() {
        m_tabContainer->findPrevious();
    });
    m_actionMap["Find Previous"] = findPrevAction;

    auto *replaceAction = editMenu->addAction("&Replace", [this]() {
        m_tabContainer->showReplace();
    });
    m_actionMap["Replace"] = replaceAction;

    auto *goToLineAction = editMenu->addAction("&Go to Line", [this]() {
        if (m_tabContainer->currentEditor()) m_tabContainer->currentEditor()->goToLine();
    });
    m_actionMap["Go to Line"] = goToLineAction;

    editMenu->addSeparator();

    auto *commentAction = editMenu->addAction("Toggle &Comment", [this]() {
        if (auto *editor = m_tabContainer->currentEditor()) editor->commentSelection();
    });
    m_actionMap["Comment/Uncomment"] = commentAction;

    auto *formatAction = editMenu->addAction("&Format Document", [this]() {
        if (auto *editor = m_tabContainer->currentEditor()) editor->requestFormatting();
    });
    m_actionMap["Format Document"] = formatAction;

    auto *defAction = editMenu->addAction("Go to &Definition", [this]() {
        if (auto *editor = m_tabContainer->currentEditor()) editor->requestGoToDefinition();
    });
    m_actionMap["Go to Definition"] = defAction;

    auto *renameSymAction = editMenu->addAction("Rena&me Symbol", [this]() {
        if (auto *editor = m_tabContainer->currentEditor()) editor->requestRename();
    });
    m_actionMap["Rename Symbol"] = renameSymAction;

    auto *suggestAction = editMenu->addAction("Trigger Su&ggestion", [this]() {
        if (auto *editor = m_tabContainer->currentEditor()) editor->triggerSuggestion();
    });
    m_actionMap["Trigger Suggestion"] = suggestAction;

    
    QMenu *viewMenu = menuBar()->addMenu("&View");

    auto *closeTabAction = viewMenu->addAction("&Close Tab", [this]() {
        m_tabContainer->closeCurrentTab();
    });
    m_actionMap["Close Tab"] = closeTabAction;
    
    auto *nextTabAction = viewMenu->addAction("&Next Tab", [this]() {
        m_tabContainer->selectNextTab();
    });
    m_actionMap["Next Tab"] = nextTabAction;
    
    auto *prevTabAction = viewMenu->addAction("&Previous Tab", [this]() {
        m_tabContainer->selectPreviousTab();
    });
    m_actionMap["Previous Tab"] = prevTabAction;

    viewMenu->addSeparator();

    auto *zoomInAction = viewMenu->addAction("Zoom &In", [this]() {
        if (auto *editor = qobject_cast<Editor::EditorView*>(m_tabContainer->currentEditor())) {
            editor->zoomIn();
        }
    });
    m_actionMap["Zoom In"] = zoomInAction;

    auto *zoomOutAction = viewMenu->addAction("Zoom &Out", [this]() {
        if (auto *editor = qobject_cast<Editor::EditorView*>(m_tabContainer->currentEditor())) {
            editor->zoomOut();
        }
    });
    m_actionMap["Zoom Out"] = zoomOutAction;

    viewMenu->addSeparator();

    auto *appearanceSubMenu = viewMenu->addMenu("&Appearance");

    auto *toggleFileBrowserAction = appearanceSubMenu->addAction("&File Browser", [this](bool checked) { m_sidebarDock->setVisible(checked); });
    toggleFileBrowserAction->setCheckable(true);
    toggleFileBrowserAction->setChecked(m_sidebarDock->isVisible());
    m_actionMap["Toggle File Browser"] = toggleFileBrowserAction;

    auto *toggleSearchAction = appearanceSubMenu->addAction("S&earch", [this](bool checked) { m_searchDock->setVisible(checked); });
    toggleSearchAction->setCheckable(true);
    toggleSearchAction->setChecked(m_searchDock->isVisible());
    m_actionMap["Toggle Search"] = toggleSearchAction;

    auto *toggleOutlineAction = appearanceSubMenu->addAction("&Outline", [this](bool checked) { m_outlineDock->setVisible(checked); });
    toggleOutlineAction->setCheckable(true);
    toggleOutlineAction->setChecked(m_outlineDock->isVisible());
    m_actionMap["Toggle Outline"] = toggleOutlineAction;

    auto *toggleGitScmAction = appearanceSubMenu->addAction("Source &Control", [this](bool checked) { m_gitscmDock->setVisible(checked); });
    toggleGitScmAction->setCheckable(true);
    toggleGitScmAction->setChecked(m_gitscmDock->isVisible());
    m_actionMap["Toggle Source Control"] = toggleGitScmAction;

    auto *toggleAiChatAction = appearanceSubMenu->addAction("&AI Chat", [this](bool checked) { m_aiDock->setVisible(checked); });
    toggleAiChatAction->setCheckable(true);
    toggleAiChatAction->setChecked(m_aiDock->isVisible());
    m_actionMap["Toggle AI Chat"] = toggleAiChatAction;

    appearanceSubMenu->addSeparator();

    auto *toggleTerminalAction = appearanceSubMenu->addAction("&Terminal", [this](bool checked) { m_terminalDock->setVisible(checked); });
    toggleTerminalAction->setCheckable(true);
    toggleTerminalAction->setChecked(m_terminalDock->isVisible());
    m_actionMap["Toggle Terminal"] = toggleTerminalAction;

    auto *toggleProblemsAction = appearanceSubMenu->addAction("&Problems", [this](bool checked) { m_problemsDock->setVisible(checked); });
    toggleProblemsAction->setCheckable(true);
    toggleProblemsAction->setChecked(m_problemsDock->isVisible());
    m_actionMap["Toggle Problems"] = toggleProblemsAction;

    appearanceSubMenu->addSeparator();

    auto *toggleStatusBarAction = appearanceSubMenu->addAction("&Status Bar", [this](bool checked) { statusBar()->setVisible(checked); });
    toggleStatusBarAction->setCheckable(true);
    toggleStatusBarAction->setChecked(statusBar()->isVisible());
    m_actionMap["Toggle Status Bar"] = toggleStatusBarAction;

    // Sync menu states before showing
    connect(appearanceSubMenu, &QMenu::aboutToShow, [=]() {
        toggleFileBrowserAction->setChecked(!m_sidebarDock->isHidden());
        toggleSearchAction->setChecked(!m_searchDock->isHidden());
        toggleOutlineAction->setChecked(!m_outlineDock->isHidden());
        toggleGitScmAction->setChecked(!m_gitscmDock->isHidden());
        toggleAiChatAction->setChecked(!m_aiDock->isHidden());
        toggleTerminalAction->setChecked(!m_terminalDock->isHidden());
        toggleProblemsAction->setChecked(!m_problemsDock->isHidden());
        toggleStatusBarAction->setChecked(statusBar()->isVisible());
    });

    viewMenu->addSeparator();

    auto *toggleLeftSidebarAction = viewMenu->addAction("Toggle Left Sidebar", [this](bool checked) {
        if (checked) {
            m_sidebarDock->show();
            m_sidebarDock->raise();
        } else {
            m_sidebarDock->hide();
            m_searchDock->hide();
            m_outlineDock->hide();
            m_gitscmDock->hide();
        }
    });
    toggleLeftSidebarAction->setCheckable(true);
    toggleLeftSidebarAction->setChecked(m_sidebarDock->isVisible() || m_searchDock->isVisible() || m_outlineDock->isVisible() || m_gitscmDock->isVisible());
    m_actionMap["Toggle Left Sidebar"] = toggleLeftSidebarAction;

    auto updateLeftSidebarState = [this, toggleLeftSidebarAction]() {
        bool anyVisible = m_sidebarDock->isVisible() || m_searchDock->isVisible() || m_outlineDock->isVisible() || m_gitscmDock->isVisible();
        toggleLeftSidebarAction->setChecked(anyVisible);
    };
    connect(m_sidebarDock, &QDockWidget::visibilityChanged, this, updateLeftSidebarState);
    connect(m_searchDock, &QDockWidget::visibilityChanged, this, updateLeftSidebarState);
    connect(m_outlineDock, &QDockWidget::visibilityChanged, this, updateLeftSidebarState);
    connect(m_gitscmDock, &QDockWidget::visibilityChanged, this, updateLeftSidebarState);

    auto *toggleBottomPanelAction = viewMenu->addAction("Toggle Bottom Panel", [this](bool checked) {
        if (checked) {
            m_terminalDock->show();
            m_terminalDock->raise();
        } else {
            m_terminalDock->hide();
            m_problemsDock->hide();
        }
    });
    toggleBottomPanelAction->setCheckable(true);
    toggleBottomPanelAction->setChecked(m_terminalDock->isVisible() || m_problemsDock->isVisible());
    m_actionMap["Toggle Bottom Panel"] = toggleBottomPanelAction;

    viewMenu->addSeparator();

    auto *focusSidebarAction = viewMenu->addAction("Focus File Browser", [this]() {
        m_sidebarDock->show();
        m_sidebarDock->raise();
        m_sidebar->setFocus();
    });
    m_actionMap["Focus File Browser"] = focusSidebarAction;

    auto *focusTerminalAction = viewMenu->addAction("Focus Terminal", [this]() {
        m_terminalDock->show();
        m_terminalDock->raise();
        m_terminalWidget->setFocus();
    });
    m_actionMap["Focus Terminal"] = focusTerminalAction;

    auto *focusSearchAction = viewMenu->addAction("Focus Search", [this]() {
        m_searchDock->show();
        m_searchDock->raise();
        m_searchDock->widget()->setFocus();
    });
    m_actionMap["Focus Search"] = focusSearchAction;

    auto *focusOutlineAction = viewMenu->addAction("Focus Outline", [this]() {
        m_outlineDock->show();
        m_outlineDock->raise();
        m_outlineDock->widget()->setFocus();
    });
    m_actionMap["Focus Outline"] = focusOutlineAction;

    auto updateBottomPanelState = [this, toggleBottomPanelAction]() {
        bool anyVisible = m_terminalDock->isVisible() || m_problemsDock->isVisible();
        toggleBottomPanelAction->setChecked(anyVisible);
    };
    connect(m_terminalDock, &QDockWidget::visibilityChanged, this, updateBottomPanelState);
    connect(m_problemsDock, &QDockWidget::visibilityChanged, this, updateBottomPanelState);

    viewMenu->addSeparator();

    auto *toggleFullScreenAction = viewMenu->addAction("&Full Screen", [this](bool checked) {
        if (checked) showFullScreen();
        else showNormal();
    });
    toggleFullScreenAction->setCheckable(true);
    toggleFullScreenAction->setChecked(isFullScreen());
    m_actionMap["Toggle Full Screen"] = toggleFullScreenAction;
    
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About", [this]() {
        QMessageBox::about(this, "About CodePlace Editor",
            "CodePlace Editor\n"
            "Version: 0.1.3\n\n"
            "A modern, fast, and lightweight code editor.\n"
            "Developed by Michael Coen.\n"
            "https://github.com/co3ndev/codeplace-editor" );
    });

    
    QMenu *prefMenu = menuBar()->addMenu("&Preferences");
    auto *prefAction = prefMenu->addAction("&Settings", this, &MainWindow::onPreferences);
    m_actionMap["Settings"] = prefAction;
}

void MainWindow::applyShortcuts() {
    auto &settings = Core::SettingsManager::instance();
    const auto defaults = Core::defaultShortcuts();

    for (auto it = m_actionMap.begin(); it != m_actionMap.end(); ++it) {
        const QString &name = it.key();
        QAction *action = it.value();
        QString defaultShortcut = defaults.value(name);
        QString shortcutStr = settings.shortcut(name, defaultShortcut);
        action->setShortcut(QKeySequence(shortcutStr));
    }
}

void MainWindow::onNewFile() {
    m_tabContainer->openNewFile();
}

void MainWindow::onOpenFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open File", Core::ProjectManager::instance().projectRoot());
    if (!fileName.isEmpty()) {
        onFileSelected(fileName);
    }
}

void MainWindow::onOpenFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Project Folder");
    if (!dir.isEmpty()) {
        openFolder(dir);
    }
}

void MainWindow::openFolder(const QString &folderPath) {
    if (m_tabContainer->closeAllTabs()) {
        Core::ProjectManager::instance().setProjectRoot(folderPath);
    }
}

void MainWindow::openFile(const QString &filePath) {
    onFileSelected(filePath);
}

void MainWindow::onSaveFile() {
    QString currentPath = m_tabContainer->currentFilePath();
    if (currentPath.isEmpty()) {
        onSaveAsFile();
    } else {
        m_tabContainer->saveCurrentFile();
    }
}

void MainWindow::onSaveAsFile() {
    if (m_tabContainer->currentEditor()) {
        int index = m_tabContainer->currentTabIndex();
        if (index != -1) {
            m_tabContainer->saveFileAs(index);
        }
    }
}

void MainWindow::onSaveAll() {
    m_tabContainer->maybeSaveAll();
}

void MainWindow::onFileSelected(const QString &filePath) {
    m_tabContainer->openFile(filePath);
}

void MainWindow::onPreferences() {
    PreferencesDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        applySettings();
        applyShortcuts();
    }
}

void MainWindow::applySettings() {
    
    m_tabContainer->applySettings();
}

void MainWindow::loadSession() {
    auto state = Core::SessionManager::instance().loadSession();
    
    
    if (!state.projectRoot.isEmpty()) {
        openFolder(state.projectRoot);
    }
    for (const QString &file : state.openFiles) {
        openFile(file);
    }
    
    if (state.activeTabIndex >= 0 && state.activeTabIndex < m_tabContainer->count()) {
        m_tabContainer->setCurrentIndex(state.activeTabIndex);
    }
}

void MainWindow::saveSession() {
    Core::SessionState state;
    state.projectRoot = Core::ProjectManager::instance().projectRoot();
    state.openFiles = m_tabContainer->openFilePaths();
    state.activeTabIndex = m_tabContainer->currentTabIndex();

    state.windowGeometry = saveGeometry();
    state.windowState = saveState();

    Core::SessionManager::instance().saveSession(state);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_tabContainer->maybeSaveAll()) {
        saveSession();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::updateStatusBar() {
    auto *editor = m_tabContainer->currentEditor();
    if (editor) {
        QTextCursor cursor = editor->textCursor();
        int line = cursor.blockNumber() + 1;
        int col = cursor.columnNumber() + 1;
        m_statusCursorLabel->setText(QString("Ln %1, Col %2").arg(line).arg(col));
    } else {
        m_statusCursorLabel->setText("Ln 1, Col 1");
    }

    QString filePath = m_tabContainer->currentFilePath();
    if (filePath.isEmpty()) {
        m_statusFileTypeLabel->setText("Plain Text");
    } else {
        QMimeDatabase mimeDb;
        QMimeType mime = mimeDb.mimeTypeForFile(filePath);
        QString desc = mime.comment();
        if (desc.isEmpty()) {
            desc = QFileInfo(filePath).suffix().toUpper();
            if (desc.isEmpty()) desc = "Plain Text";
        }
        m_statusFileTypeLabel->setText(desc);
    }

    m_statusEncodingLabel->setText("UTF-8");
}

QDockWidget* MainWindow::createDockWidget(const QString &title, const QString &objectName, QWidget *widget, Qt::DockWidgetArea area) {
    auto *dock = new QDockWidget(title, this);
    dock->setObjectName(objectName);
    dock->setWidget(widget);
    dock->setFeatures(QDockWidget::DockWidgetMovable);
    addDockWidget(area, dock);
    return dock;
}

} 
