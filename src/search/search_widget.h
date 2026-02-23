#ifndef SEARCH_WIDGET_H
#define SEARCH_WIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QHash>
#include "search_engine.h"

namespace Search {

class SearchWidget : public QWidget {
    Q_OBJECT

public:
    explicit SearchWidget(QWidget *parent = nullptr);
    void applyTheme();
    void setProjectRoot(const QString &root);

signals:
    void fileSelected(const QString &filePath, int line, int column);

private slots:
    void onThemeChanged();
    void onSearch();
    void onReplace();
    void onReplaceAll();
    void onMatchFound(const SearchResult &result);
    void onSearchFinished(int totalMatches);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    void setupUI();

    QLineEdit *m_searchInput;
    QLineEdit *m_replaceInput;
    QPushButton *m_searchButton;
    QPushButton *m_replaceButton;
    QPushButton *m_replaceAllButton;
    QCheckBox *m_caseSensitiveCheck;
    QTreeWidget *m_resultsTree;
    QLabel *m_statusLabel;

    SearchEngine *m_engine;
    QString m_projectRoot;
    QVector<SearchResult> m_currentResults;
    QHash<QString, QTreeWidgetItem*> m_fileItemCache;
};

} 

#endif 
