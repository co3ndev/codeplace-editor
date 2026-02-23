#include "search_engine.h"
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QtConcurrent>
#include <QFuture>

namespace Search {

SearchEngine::SearchEngine(QObject *parent) : QObject(parent) {
    m_excludePatterns << ".git" << "build" << "node_modules" << ".vscode" << ".idea";
}

void SearchEngine::search(const QString &rootPath, const QString &query, bool caseSensitive) {
    m_cancelled = false;
    emit searchStarted();

    
    (void)QtConcurrent::run([this, rootPath, query, caseSensitive]() {
        runSearch(rootPath, query, caseSensitive);
    });
}

void SearchEngine::cancel() {
    m_cancelled = true;
}

void SearchEngine::runSearch(const QString &rootPath, const QString &query, bool caseSensitive) {
    if (query.isEmpty()) {
        emit searchFinished(0);
        return;
    }

    int totalMatches = 0;
    QDirIterator it(rootPath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    
    
    
    
    Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    while (it.hasNext() && !m_cancelled) {
        QString filePath = it.next();
        QFileInfo fileInfo(filePath);
        
        if (shouldExclude(fileInfo)) continue;
        if (isBinaryFile(filePath)) continue;

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        QTextStream in(&file);
        int lineNumber = 1;
        while (!in.atEnd() && !m_cancelled) {
            QString lineContent = in.readLine();
            int index = 0;
            while ((index = lineContent.indexOf(query, index, cs)) != -1 && !m_cancelled) {
                if (totalMatches >= MAX_TOTAL_MATCHES) {
                    emit searchFinished(totalMatches);
                    return;
                }

                SearchResult result;
                result.filePath = filePath;
                result.line = lineNumber;
                result.column = index;
                
                if (lineContent.length() > MAX_LINE_LENGTH) {
                    result.lineContent = lineContent.left(MAX_LINE_LENGTH).trimmed() + "...";
                } else {
                    result.lineContent = lineContent.trimmed();
                }
                
                result.matchLength = query.length();
                
                emit matchFound(result);
                totalMatches++;
                index += query.length();
            }
            lineNumber++;
        }
    }

    emit searchFinished(totalMatches);
}

bool SearchEngine::shouldExclude(const QFileInfo &fileInfo) {
    QString path = fileInfo.absoluteFilePath();
    for (const QString &pattern : m_excludePatterns) {
        if (path.contains("/" + pattern + "/") || path.endsWith("/" + pattern) || path.startsWith(pattern + "/")) {
            return true;
        }
    }
    return false;
}

bool SearchEngine::isBinaryFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    char buffer[1024];
    qint64 bytesRead = file.read(buffer, sizeof(buffer));
    file.close();

    for (qint64 i = 0; i < bytesRead; ++i) {
        if (buffer[i] == '\0') {
            return true;
        }
    }
    return false;
}

bool SearchEngine::replaceInFile(const QString &filePath, int line, int column, int length, const QString &replacement) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) return false;

    QStringList lines;
    QTextStream in(&file);
    while (!in.atEnd()) {
        lines.append(in.readLine());
    }

    if (line <= 0 || line > lines.size()) {
        file.close();
        return false;
    }

    QString &lineToModify = lines[line - 1];
    if (column < 0 || column > lineToModify.length()) {
        file.close();
        return false;
    }

    lineToModify.replace(column, length, replacement);

    file.resize(0);
    QTextStream out(&file);
    for (const QString &l : lines) {
        out << l << "\n";
    }
    file.close();
    return true;
}

int SearchEngine::replaceAll(const QVector<SearchResult> &results, const QString &replacement) {
    int count = 0;
    
    QMap<QString, QVector<SearchResult>> fileResults;
    for (const SearchResult &res : results) {
        fileResults[res.filePath].append(res);
    }

    for (auto it = fileResults.begin(); it != fileResults.end(); ++it) {
        QString filePath = it.key();
        QVector<SearchResult> &resForFile = it.value();
        
        
        std::sort(resForFile.begin(), resForFile.end(), [](const SearchResult &a, const SearchResult &b) {
            if (a.line != b.line) return a.line > b.line;
            return a.column > b.column;
        });

        QFile file(filePath);
        if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) continue;

        QStringList lines;
        QTextStream in(&file);
        while (!in.atEnd()) {
            lines.append(in.readLine());
        }

        for (const SearchResult &res : resForFile) {
            if (res.line > 0 && res.line <= lines.size()) {
                lines[res.line - 1].replace(res.column, res.matchLength, replacement);
                count++;
            }
        }

        file.resize(0);
        QTextStream out(&file);
        for (const QString &l : lines) {
            out << l << "\n";
        }
        file.close();
    }
    return count;
}

} 
