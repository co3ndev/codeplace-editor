#ifndef LSP_CLIENT_H
#define LSP_CLIENT_H

#include "lsp_process.h"
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace LspClient {

class LspClient : public QObject {
    Q_OBJECT

public:
    enum State {
        Uninitialized,
        Initializing,
        Initialized,
        Error
    };

    explicit LspClient(const QString &language, QObject *parent = nullptr);
    ~LspClient() override;

    bool start(const QString &program, const QStringList &arguments, const QString &rootPath);
    void stop();

    State state() const { return m_state; }
    QString language() const { return m_language; }

    void didOpen(const QString &uri, const QString &text, const QString &languageId, int version = 1);
    void didChange(const QString &uri, const QString &text, int version);
    void didClose(const QString &uri);
    
    void requestCompletion(const QString &uri, int line, int column);
    void requestHover(const QString &uri, int line, int column);
    void requestDefinition(const QString &uri, int line, int column);
    void requestReferences(const QString &uri, int line, int column);
    void requestRename(const QString &uri, int line, int column, const QString &newName);
    void requestFormatting(const QString &uri);
    void requestDocumentSymbols(const QString &uri);
    void requestCodeAction(const QString &uri, int startLine, int startCol, int endLine, int endCol, const QJsonArray &diagnostics);

signals:
    void stateChanged(State newState);
    void publishDiagnostics(const QString &uri, const QJsonArray &diagnostics);
    void completionResult(const QString &uri, const QJsonArray &items);
    void hoverResult(const QString &uri, const QJsonObject &hover);
    void definitionResult(const QString &uri, const QJsonValue &result);
    void referencesResult(const QString &uri, const QJsonArray &references);
    void renameResult(const QString &uri, const QJsonObject &workspaceEdit);
    void formattingResult(const QString &uri, const QJsonArray &edits);
    void documentSymbolsResult(const QString &uri, const QJsonArray &symbols);
    void codeActionResult(const QString &uri, const QJsonArray &actions);

private slots:
    void onMessageReceived(const QJsonObject &message);
    void onProcessError(const QString &error);

private:
    void handleResponse(const QJsonObject &message);
    void handleNotification(const QJsonObject &message);

    void sendInitialize(const QString &rootPath);
    void sendInitialized();

    int getNextRequestId();

    LspProcess *m_process;
    State m_state;
    QString m_language;
    int m_requestId;
    
    struct RequestContext {
        QString uri;
        QString method;
    };
    QMap<int, RequestContext> m_pendingRequests;
};

} 

#endif 
