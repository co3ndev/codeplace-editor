#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QMap>
#include <QAction>
#include <QLabel>
#include <QStatusBar>
#include "editor/tab_container.h"
#include "browser/file_browser_sidebar.h"
#include "terminal/terminal_widget.h"
#include "lspclient/problems_widget.h"
#include "ai/ai_chat_sidebar.h"
class QDockWidget;

namespace Main {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    void openFile(const QString &filePath);
    void openFolder(const QString &folderPath);

    QMenu* createPopupMenu() override {
        return nullptr;
    }

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onNewFile();
    void onOpenFile();
    void onOpenFolder();
    void onSaveFile();
    void onSaveAsFile();
    void onSaveAll();
    void onPreferences();

    void onFileSelected(const QString &filePath);
    void updateStatusBar();

private:
    void createMenus();
    void setupLayout();
    void applyShortcuts();
    Q_INVOKABLE void loadSession();
    Q_INVOKABLE void saveSession();
    void applySettings();

    bool maybeSave();
    bool saveFile(const QString &filePath);
    void setCurrentFile(const QString &filePath);
    QDockWidget* createDockWidget(const QString &title, const QString &objectName, QWidget *widget, Qt::DockWidgetArea area);

    Editor::TabContainer *m_tabContainer;
    Browser::FileBrowserSidebar *m_sidebar;
    Terminal::TerminalWidget *m_terminalWidget;
    QDockWidget *m_terminalDock;
    QDockWidget *m_problemsDock;
    LspClient::ProblemsWidget *m_problemsWidget;
    Editor::EditorView *m_lastConnectedEditor = nullptr;
    QDockWidget *m_sidebarDock;
    QDockWidget *m_searchDock;
    QDockWidget *m_outlineDock;
    QDockWidget *m_gitscmDock;
    QDockWidget *m_aiDock;

    AiChatSidebar *m_aiSidebar;

    
    QMap<QString, QAction*> m_actionMap;

    
    QLabel *m_statusCursorLabel;
    QLabel *m_statusFileTypeLabel;
    QLabel *m_statusEncodingLabel;};

} 

#endif 
