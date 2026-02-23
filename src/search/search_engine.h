#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QFileInfo>

namespace Search {

struct SearchResult {
    QString filePath;
    int line;
    int column;
    QString lineContent;
    int matchLength;
};

class SearchEngine : public QObject {
    Q_OBJECT

public:
    explicit SearchEngine(QObject *parent = nullptr);

    void search(const QString &rootPath, const QString &query, bool caseSensitive);
    void cancel();

    
    bool replaceInFile(const QString &filePath, int line, int column, int length, const QString &replacement);
    int replaceAll(const QVector<SearchResult> &results, const QString &replacement);

    static bool isBinaryFile(const QString &filePath);
    static constexpr int MAX_TOTAL_MATCHES = 10000;
    static constexpr int MAX_LINE_LENGTH = 1000;

signals:
    void matchFound(const SearchResult &result);
    void searchStarted();
    void searchFinished(int totalMatches);
    void progressUpdated(int filesProcessed, int totalFiles);

private:
    void runSearch(const QString &rootPath, const QString &query, bool caseSensitive);
    bool shouldExclude(const QFileInfo &fileInfo);

    std::atomic<bool> m_cancelled = false;
    QStringList m_excludePatterns;
};

} 

#endif 
