#ifndef LSP_MANAGER_H
#define LSP_MANAGER_H

#include "lsp_client.h"
#include <QObject>
#include <QMap>
#include <map>
#include <QPointer>
#include <QStringList>
#include <QSet>
#include <QTimer>
#include <memory>

namespace LspClient {

class LspManager : public QObject {
    Q_OBJECT

public:
    static LspManager& instance();

    bool isLanguageSupported(const QString &language) const;
    bool hasActiveClient(const QString &filePath) const;
    LspClient* getClient(const QString &language, const QString &rootPath = QString());

    void setProjectRoot(const QString &path);
    void startClient(const QString &language, const QString &rootPath);
    void stopClient(const QString &language);
    void stopAllClients();

    void documentOpened(const QString &filePath, const QString &content, const QString &languageId);
    void documentChanged(const QString &filePath, const QString &content, int version);
    void documentClosed(const QString &filePath);

    void requestCompletion(const QString &filePath, int line, int column);
    void requestHover(const QString &filePath, int line, int column);
    void requestDefinition(const QString &filePath, int line, int column);
    void requestReferences(const QString &filePath, int line, int column);
    void requestRename(const QString &filePath, int line, int column, const QString &newName);
    void requestFormatting(const QString &filePath);
    void requestDocumentSymbols(const QString &filePath);
    void requestCodeAction(const QString &filePath, int startLine, int startCol, int endLine, int endCol, const QJsonArray &diagnostics);

    void indexProject(const QString &rootPath);
    void indexFile(const QString &filePath);

signals:
    void diagnosticsReady(const QString &filePath, const QJsonArray &diagnostics);
    void completionReady(const QString &filePath, const QJsonArray &items);
    void hoverReady(const QString &filePath, const QJsonObject &hover);
    void definitionReady(const QString &filePath, const QJsonValue &result);
    void referencesReady(const QString &filePath, const QJsonArray &references);
    void renameReady(const QString &filePath, const QJsonObject &workspaceEdit);
    void formattingReady(const QString &filePath, const QJsonArray &edits);
    void documentSymbolsReady(const QString &filePath, const QJsonArray &symbols);
    void codeActionReady(const QString &filePath, const QJsonArray &actions);

private:
    LspManager();
    ~LspManager() override;

    LspManager(const LspManager&) = delete;
    LspManager& operator=(const LspManager&) = delete;

    QTimer *m_cleanupTimer;  // For periodic cleanup
    static constexpr int CLEANUP_INTERVAL_MS = 60000;

    void cleanupStaleEntries();

    static constexpr int MAX_PENDING_BUFFER_SIZE = 104857600; // 100MB
    int m_pendingBufferSize = 0;

    bool canAddToPending(const QString &content, const QString &filePath);
    void evictOldestPending();

    struct PendingDocument {
        QString content;
        QString languageId;
        int version;
        bool isFromEditor;
        qint64 timestamp;
    };

    static constexpr int MAX_INDEXED_FILES = 5000;  // Reasonable limit
    QStringList m_excludePatterns = {
        "*/node_modules/*",
        "*/.git/*",
        "*/build/*",
        "*/dist/*",
        "*/target/*",
        "*/.venv/*",
        "*/venv/*",
        "*/.o",
        "*/.a",
        "*/.so",
        "*/.dylib",
    };

    bool shouldIndexFile(const QString &filePath) const;

    void indexNextBatch();

    QTimer *m_indexTimer;
    QStringList m_indexQueue;

    QMap<QString, PendingDocument> m_pendingDocuments;
    QSet<QString> m_addedFiles;

    QString getLanguageForFile(const QString &filePath) const;
    QStringList getArguments(const QString &argsString) const;

    std::map<QString, std::unique_ptr<LspClient>> m_clients;

    QMap<QString, QString> m_openFiles;
    QMap<QString, QString> m_indexedFiles;
    QMap<QString, int> m_fileVersions;
    QString m_projectRoot;
};

}

#endif
