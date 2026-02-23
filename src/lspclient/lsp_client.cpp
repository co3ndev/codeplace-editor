#include "lsp_client.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QUrl>
#include <QCoreApplication>

namespace LspClient {

LspClient::LspClient(const QString &language, QObject *parent)
    : QObject(parent), m_process(new LspProcess(this)), m_state(Uninitialized),
      m_language(language), m_requestId(1) {

    connect(m_process, &LspProcess::messageReceived, this, &LspClient::onMessageReceived);
    connect(m_process, &LspProcess::errorOccurred, this, &LspClient::onProcessError);
}

LspClient::~LspClient() {
    stop();
}

bool LspClient::start(const QString &program, const QStringList &arguments, const QString &rootPath) {
    if (m_state != Uninitialized && m_state != Error) return false;

    if (m_process->start(program, arguments)) {
        m_state = Initializing;
        emit stateChanged(m_state);
        sendInitialize(rootPath);
        return true;
    }
    
    m_state = Error;
    emit stateChanged(m_state);
    return false;
}

void LspClient::stop() {
    if (m_state == Uninitialized || m_state == Error) return;

    if (m_process->isRunning()) {
        QJsonObject shutdownReq;
        shutdownReq["jsonrpc"] = "2.0";
        shutdownReq["id"] = getNextRequestId();
        shutdownReq["method"] = "shutdown";
        m_process->sendMessage(shutdownReq);

        QJsonObject exitNotif;
        exitNotif["jsonrpc"] = "2.0";
        exitNotif["method"] = "exit";
        m_process->sendMessage(exitNotif);
        
        m_process->stop();
    }
    
    m_state = Uninitialized;
    emit stateChanged(m_state);
}

void LspClient::sendInitialize(const QString &rootPath) {
    QJsonObject params;
    params["processId"] = QCoreApplication::applicationPid();
    params["rootUri"] = QUrl::fromLocalFile(rootPath).toString();
    
    QJsonObject capabilities;
    QJsonObject workspace;
    workspace["applyEdit"] = true;
    
    QJsonObject textDocument;
    
    QJsonObject completion;
    QJsonObject completionItem;
    completionItem["snippetSupport"] = true;
    completion["completionItem"] = completionItem;
    textDocument["completion"] = completion;
    
    QJsonObject hover;
    hover["contentFormat"] = QJsonArray({"markdown", "plaintext"});
    textDocument["hover"] = hover;

    QJsonObject definition;
    definition["dynamicRegistration"] = false;
    textDocument["definition"] = definition;
    
    QJsonObject rename;
    rename["dynamicRegistration"] = false;
    rename["prepareSupport"] = true;
    textDocument["rename"] = rename;
    
    QJsonObject formatting;
    formatting["dynamicRegistration"] = false;
    textDocument["formatting"] = formatting;

    QJsonObject documentSymbol;
    documentSymbol["hierarchicalDocumentSymbolSupport"] = true;
    textDocument["documentSymbol"] = documentSymbol;

    capabilities["workspace"] = workspace;
    capabilities["textDocument"] = textDocument;
    params["capabilities"] = capabilities;

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["id"] = getNextRequestId();
    request["method"] = "initialize";
    request["params"] = params;

    m_process->sendMessage(request);
}

void LspClient::sendInitialized() {
    QJsonObject notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = "initialized";
    notification["params"] = QJsonObject();

    m_process->sendMessage(notification);
    
    m_state = Initialized;
    emit stateChanged(m_state);
}

void LspClient::didOpen(const QString &uri, const QString &text, const QString &languageId, int version) {
    if (m_state != Initialized) return;

    QJsonObject textDoc;
    textDoc["uri"] = QUrl::fromLocalFile(uri).toString();
    textDoc["languageId"] = languageId;
    textDoc["version"] = version;
    textDoc["text"] = text;

    QJsonObject params;
    params["textDocument"] = textDoc;

    QJsonObject notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = "textDocument/didOpen";
    notification["params"] = params;

    qDebug() << "LSP Send: didOpen" << uri;
    m_process->sendMessage(notification);
}

void LspClient::didChange(const QString &uri, const QString &text, int version) {
    if (m_state != Initialized) return;

    QJsonObject textDoc;
    textDoc["uri"] = QUrl::fromLocalFile(uri).toString();
    textDoc["version"] = version;

    QJsonObject contentChange;
    contentChange["text"] = text;

    QJsonArray contentChanges;
    contentChanges.append(contentChange);

    QJsonObject params;
    params["textDocument"] = textDoc;
    params["contentChanges"] = contentChanges;

    QJsonObject notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = "textDocument/didChange";
    notification["params"] = params;

    m_process->sendMessage(notification);
}

void LspClient::didClose(const QString &uri) {
    if (m_state != Initialized) return;

    QJsonObject textDoc;
    textDoc["uri"] = QUrl::fromLocalFile(uri).toString();

    QJsonObject params;
    params["textDocument"] = textDoc;

    QJsonObject notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = "textDocument/didClose";
    notification["params"] = params;

    m_process->sendMessage(notification);
}

void LspClient::requestCompletion(const QString &uri, int line, int column) {
    if (m_state != Initialized) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", QUrl::fromLocalFile(uri).toString()}};
    params["position"] = QJsonObject{{"line", line}, {"character", column}};

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    int id = getNextRequestId();
    m_pendingRequests[id] = {uri, "textDocument/completion"};
    request["id"] = id;

    request["method"] = "textDocument/completion";
    request["params"] = params;

    m_process->sendMessage(request);
}

void LspClient::requestHover(const QString &uri, int line, int column) {
    if (m_state != Initialized) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", QUrl::fromLocalFile(uri).toString()}};
    params["position"] = QJsonObject{{"line", line}, {"character", column}};

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    int id = getNextRequestId();
    m_pendingRequests[id] = {uri, "textDocument/hover"};
    request["id"] = id;
    request["method"] = "textDocument/hover";
    request["params"] = params;

    m_process->sendMessage(request);
}

void LspClient::requestDefinition(const QString &uri, int line, int column) {
    if (m_state != Initialized) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", QUrl::fromLocalFile(uri).toString()}};
    params["position"] = QJsonObject{{"line", line}, {"character", column}};

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    int id = getNextRequestId();
    m_pendingRequests[id] = {uri, "textDocument/definition"};
    request["id"] = id;
    request["method"] = "textDocument/definition";
    request["params"] = params;

    m_process->sendMessage(request);
}

void LspClient::requestReferences(const QString &uri, int line, int column) {
    if (m_state != Initialized) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", QUrl::fromLocalFile(uri).toString()}};
    params["position"] = QJsonObject{{"line", line}, {"character", column}};
    params["context"] = QJsonObject{{"includeDeclaration", true}};

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    int id = getNextRequestId();
    m_pendingRequests[id] = {uri, "textDocument/references"};
    request["id"] = id;
    request["method"] = "textDocument/references";
    request["params"] = params;

    m_process->sendMessage(request);
}

void LspClient::requestRename(const QString &uri, int line, int column, const QString &newName) {
    if (m_state != Initialized) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", QUrl::fromLocalFile(uri).toString()}};
    params["position"] = QJsonObject{{"line", line}, {"character", column}};
    params["newName"] = newName;

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    int id = getNextRequestId();
    m_pendingRequests[id] = {uri, "textDocument/rename"};
    request["id"] = id;
    request["method"] = "textDocument/rename";
    request["params"] = params;

    m_process->sendMessage(request);
}

void LspClient::requestFormatting(const QString &uri) {
    if (m_state != Initialized) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", QUrl::fromLocalFile(uri).toString()}};
    params["options"] = QJsonObject{{"tabSize", 4}, {"insertSpaces", true}};

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    int id = getNextRequestId();
    m_pendingRequests[id] = {uri, "textDocument/formatting"};
    request["id"] = id;
    request["method"] = "textDocument/formatting";
    request["params"] = params;

    m_process->sendMessage(request);
}

void LspClient::requestDocumentSymbols(const QString &uri) {
    if (m_state != Initialized) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", QUrl::fromLocalFile(uri).toString()}};

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    int id = getNextRequestId();
    m_pendingRequests[id] = {uri, "textDocument/documentSymbol"};
    request["id"] = id;
    request["method"] = "textDocument/documentSymbol";
    request["params"] = params;

    m_process->sendMessage(request);
}

void LspClient::requestCodeAction(const QString &uri, int startLine, int startCol, int endLine, int endCol, const QJsonArray &diagnostics) {
    if (m_state != Initialized) return;
    
    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", QUrl::fromLocalFile(uri).toString()}};
    
    QJsonObject range;
    range["start"] = QJsonObject{{"line", startLine}, {"character", startCol}};
    range["end"] = QJsonObject{{"line", endLine}, {"character", endCol}};
    params["range"] = range;
    
    QJsonObject context;
    context["diagnostics"] = diagnostics;
    params["context"] = context;

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    int id = getNextRequestId();
    m_pendingRequests[id] = {uri, "textDocument/codeAction"};
    request["id"] = id;
    request["method"] = "textDocument/codeAction";
    request["params"] = params;

    m_process->sendMessage(request);
}

void LspClient::onMessageReceived(const QJsonObject &message) {
    if (message.contains("id") && message.contains("result")) {
        handleResponse(message);
    } else if (message.contains("id") && message.contains("error")) {
        qDebug() << "LSP Response Error:" << message["error"];
    } else if (message.contains("method")) {
        qDebug() << "LSP Recv Notification:" << message["method"].toString();
        handleNotification(message);
    }
}

void LspClient::handleResponse(const QJsonObject &message) {
    int id = message["id"].toInt();
    
    if (m_state == Initializing) {
        sendInitialized();
        return;
    }

    if (!m_pendingRequests.contains(id)) return;
    
    RequestContext ctx = m_pendingRequests.take(id);
    QString uri = ctx.uri;
    QString method = ctx.method;
    QJsonValue result = message["result"];

    if (method == "textDocument/completion") {
        QJsonArray items;
        if (result.isArray()) {
            items = result.toArray();
        } else if (result.isObject() && result.toObject().contains("items")) {
            items = result.toObject()["items"].toArray();
        }
        emit completionResult(uri, items);
    } else if (method == "textDocument/hover") {
        emit hoverResult(uri, result.toObject());
    } else if (method == "textDocument/definition") {
        emit definitionResult(uri, result);
    } else if (method == "textDocument/references") {
        emit referencesResult(uri, result.toArray());
    } else if (method == "textDocument/rename") {
        emit renameResult(uri, result.toObject());
    } else if (method == "textDocument/formatting") {
        emit formattingResult(uri, result.toArray());
    } else if (method == "textDocument/documentSymbol") {
        emit documentSymbolsResult(uri, result.toArray());
    } else if (method == "textDocument/codeAction") {
        emit codeActionResult(uri, result.toArray());
    }
}

void LspClient::handleNotification(const QJsonObject &message) {
    QString method = message["method"].toString();
    QJsonObject params = message["params"].toObject();

    if (method == "textDocument/publishDiagnostics") {
        QString uri = params["uri"].toString();
        if (uri.startsWith("file://")) {
            uri = QUrl(uri).toLocalFile();
        }
        QJsonArray diagnostics = params["diagnostics"].toArray();
        emit publishDiagnostics(uri, diagnostics);
    } else if (method == "window/logMessage") {
        qDebug() << "LSP Log:" << params["message"].toString();
    }
}

void LspClient::onProcessError(const QString &error) {
    qDebug() << "LSP Client Process Error:" << error;
    m_state = Error;
    emit stateChanged(m_state);
}

int LspClient::getNextRequestId() {
    return m_requestId++;
}

} 
