#ifndef TAB_CONTAINER_H
#define TAB_CONTAINER_H

#include <QWidget>
#include <QTabWidget>
#include <QMap>
#include <QStackedWidget>
#include <optional>
#include "editor_view.h"
#include "find_replace_bar.h"
#include "quick_start_widget.h"
#include "core/simple_highlighter.h"

namespace Editor {

class TabContainer final : public QWidget {
    Q_OBJECT

public:
    explicit TabContainer(QWidget *parent = nullptr);

    void openFile(const QString &filePath);
    void openFile(const QString &filePath, int line, int column);
    void openDiff(const QString &filePath);

    void openNewFile();
    void closeCurrentTab();
    bool closeAllTabs();
    void closeOtherTabs(int index);
    
    EditorView* currentEditor() const;
    int currentTabIndex() const;
    int count() const;
    void setCurrentIndex(int index);
    QString currentFilePath() const;
    QStringList openFilePaths() const;
    
    void applySettings();
    
    void selectNextTab();
    void selectPreviousTab();

    void showFind();
    void showReplace();
    void findNext();
    void findPrevious();

    
    bool maybeSaveAll();
    bool saveFile(int index);
    bool saveCurrentFile();
    bool saveFileAs(int index);

signals:
    void currentFileChanged(const QString &filePath);
    void activeEditorChanged(EditorView *editor);
    void fileClosed(const QString &filePath);
    void modifiedFilesChanged(const QStringList &modifiedFiles);
    void fileExtensionChanged(const QString &filePath);
    void openFilesChanged(const QStringList &openFiles);

public slots:
    void onGitModifiedFilesChanged(const QMap<QString, QString> &files);

private slots:
    void onTabCloseRequested(int index);
    void onCurrentChanged(int index);
    void onModificationChanged(bool modified);
    void showContextMenu(const QPoint &pos);

private:
    struct TabInfo {
         QWidget *widget;
         QString filePath;
         Core::SimpleHighlighter *highlighter = nullptr;
         bool isDiff = false;
     };

    QTabWidget *m_tabWidget;
    FindReplaceBar *m_findBar;
    QStackedWidget *m_stackedWidget;
    QuickStartWidget *m_quickStartWidget;
    QList<TabInfo> m_tabs;

    void updateTabTitle(int index);
    int findTabByPath(const QString &filePath);
    bool maybeSaveTab(int index);
    void updateEmptyState();

    static std::optional<Core::SimpleHighlighter::Language> languageForExtension(const QString &ext);
    QStringList modifiedFilePaths() const;
    QMap<QString, QString> m_gitModifiedFiles;
};

} 

#endif 
