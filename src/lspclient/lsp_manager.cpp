#include "lsp_manager.h"
#include "lspclient_widget.h"
#include "core/settings_manager.h"
#include "core/project_manager.h"
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QDebug>

namespace LspClient {

LspManager& LspManager::instance() {
    static LspManager instance;
    return instance;
}

LspManager::LspManager() : QObject(nullptr) {
    m_indexTimer = new QTimer(this);
    m_indexTimer->setInterval(10);
    connect(m_indexTimer, &QTimer::timeout, this, &LspManager::indexNextBatch);

    connect(&Core::ProjectManager::instance(), &Core::ProjectManager::projectRootChanged,
            this, &LspManager::setProjectRoot);
    setProjectRoot(Core::ProjectManager::instance().projectRoot());
}

LspManager::~LspManager() {
    stopAllClients();
}

bool LspManager::isLanguageSupported(const QString &language) const {
    if (language.isEmpty() || language == "Unknown") return false;
    
    QString prefix = "lsp/" + language + "/";
    auto &settings = Core::SettingsManager::instance();
    
    bool enabled = settings.value(prefix + "enabled", false).toBool();
    QString executable = settings.value(prefix + "executable", "").toString();
    
    if (!enabled || executable.isEmpty()) {
        qDebug() << "LSP NOT Supported for" << language << "(enabled:" << enabled << "exe:" << executable << ")";
    }
    
    return enabled && !executable.isEmpty();
}

bool LspManager::hasActiveClient(const QString &filePath) const {
    QString language = m_openFiles.value(filePath);
    if (language.isEmpty()) return false;
    auto it = m_clients.find(language);
    LspClient *client = (it != m_clients.end()) ? it->second.get() : nullptr;
    return client && client->state() == LspClient::Initialized;
}

LspClient* LspManager::getClient(const QString &language, const QString &rootPath) {
    if (!isLanguageSupported(language)) {
        return nullptr;
    }

    auto it = m_clients.find(language);
    if (it != m_clients.end()) {
        LspClient *client = it->second.get();
        if (client->state() == LspClient::Error) {
            stopClient(language);
        } else {
            return client;
        }
    }

    QString finalRoot = rootPath;
    if (finalRoot.isEmpty()) finalRoot = m_projectRoot;
    if (finalRoot.isEmpty()) finalRoot = QDir::homePath();

    startClient(language, finalRoot);
    
    auto itFinal = m_clients.find(language);
    return (itFinal != m_clients.end()) ? itFinal->second.get() : nullptr;
}

void LspManager::setProjectRoot(const QString &path) {
    if (m_projectRoot == path) return;
    
    m_projectRoot = path;
    qDebug() << "LSP Project Root set to:" << m_projectRoot;
    
    if (!m_projectRoot.isEmpty()) {
        indexProject(m_projectRoot);
    }
}

void LspManager::startClient(const QString &language, const QString &rootPath) {
    if (!isLanguageSupported(language) || m_clients.count(language)) {
        return;
    }

    QString prefix = "lsp/" + language + "/";
    auto &settings = Core::SettingsManager::instance();
    
    QString executable = settings.value(prefix + "executable", "").toString();
    QString argsString = settings.value(prefix + "args", "").toString();
    QStringList args = getArguments(argsString);

    auto clientPtr = std::make_unique<LspClient>(language);
    LspClient *client = clientPtr.get();
    
    connect(client, &LspClient::publishDiagnostics, this, [this](const QString &uri, const QJsonArray &diagnostics) {
        emit diagnosticsReady(uri, diagnostics);
    });

    connect(client, &LspClient::stateChanged, this, [this, client, language](LspClient::State state) {
        if (state == LspClient::Initialized) {
            qDebug() << "LSP Client Initialized for" << language << "- Flushing pending documents";
            
            QStringList toSend;
            for (auto it = m_pendingDocuments.begin(); it != m_pendingDocuments.end(); ++it) {
                if (it.value().languageId == language) {
                    toSend << it.key();
                }
            }
            
            
            for (const QString &filePath : toSend) {
                if (m_addedFiles.contains(filePath)) {
                    m_pendingDocuments.remove(filePath);
                    continue;
                }
                
                PendingDocument doc = m_pendingDocuments.take(filePath);
                qDebug() << "Flushing didOpen for:" << filePath << "lang:" << doc.languageId;
                client->didOpen(filePath, doc.content, doc.languageId, doc.version);
                m_addedFiles.insert(filePath);
                
                if (doc.isFromEditor) {
                    m_openFiles[filePath] = language;
                    m_fileVersions[filePath] = doc.version;
                } else {
                    m_indexedFiles[filePath] = language;
                }
            }
        }
    });

    if (client->start(executable, args, rootPath)) {
        m_clients[language] = std::move(clientPtr);
        qDebug() << "Started LSP Server for" << language << "using" << executable << "root:" << rootPath;
        connect(client, &LspClient::completionResult, this, [this](const QString &uri, const QJsonArray &items) {
            emit completionReady(uri, items);
        });

        connect(client, &LspClient::hoverResult, this, [this](const QString &uri, const QJsonObject &hover) {
            emit hoverReady(uri, hover);
        });

        connect(client, &LspClient::definitionResult, this, [this](const QString &uri, const QJsonValue &result) {
            emit definitionReady(uri, result);
        });

        connect(client, &LspClient::referencesResult, this, [this](const QString &uri, const QJsonArray &references) {
            emit referencesReady(uri, references);
        });

        connect(client, &LspClient::renameResult, this, [this](const QString &uri, const QJsonObject &workspaceEdit) {
            emit renameReady(uri, workspaceEdit);
        });

        connect(client, &LspClient::formattingResult, this, [this](const QString &uri, const QJsonArray &edits) {
            emit formattingReady(uri, edits);
        });

        connect(client, &LspClient::documentSymbolsResult, this, [this](const QString &uri, const QJsonArray &symbols) {
            emit documentSymbolsReady(uri, symbols);
        });

        connect(client, &LspClient::codeActionResult, this, [this](const QString &uri, const QJsonArray &actions) {
            emit codeActionReady(uri, actions);
        });
        
    } else {
        qWarning() << "Failed to start LSP Server for" << language;
        client->deleteLater();
    }
}

void LspManager::stopClient(const QString &language) {
    auto it = m_clients.find(language);
    if (it != m_clients.end()) {
        auto client = std::move(it->second);
        m_clients.erase(it);
        client->stop();
        
        qDebug() << "Stopped LSP Server for" << language << "- Moving documents to pending";

        QStringList paths = m_addedFiles.values();
        for (const QString &path : paths) {
            QString lang = m_openFiles.value(path);
            bool isFromEditor = true;
            if (lang.isEmpty()) {
                lang = m_indexedFiles.value(path);
                isFromEditor = false;
            }
            
            if (lang == language) {
                m_addedFiles.remove(path);
                
                if (!isFromEditor) {
                    QFile file(path);
                    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        m_pendingDocuments[path] = {QString::fromUtf8(file.readAll()), language, 1, false};
                        file.close();
                    }
                } else {
                    qDebug() << "File" << path << "needs re-sync after LSP restart";
                }
            }
        }
    }
}

void LspManager::stopAllClients() {
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        it->second->stop();
    }
    m_clients.clear();
}

void LspManager::documentOpened(const QString &filePath, const QString &content, const QString &languageId) {
    if (!isLanguageSupported(languageId)) return;

    qDebug() << "LSP Document Opened:" << filePath << "(" << languageId << ")";
    m_openFiles[filePath] = languageId;
    m_fileVersions[filePath] = 1;

    if (m_addedFiles.contains(filePath)) {
        qDebug() << "Document already added, converting to didChange";
        documentChanged(filePath, content, 1);
        return;
    }

    QString fallbackRoot = QFileInfo(filePath).absolutePath();
    LspClient *client = getClient(languageId, fallbackRoot);
    
    if (client && client->state() == LspClient::Initialized) {
        qDebug() << "Sending didOpen immediately";
        client->didOpen(filePath, content, languageId, 1);
        m_addedFiles.insert(filePath);
    } else {
        qDebug() << "Buffering didOpen (client state:" << (client ? (int)client->state() : -1) << ")";
        m_pendingDocuments[filePath] = {content, languageId, 1, true};
    }
}

void LspManager::documentChanged(const QString &filePath, const QString &content, int version) {
    QString languageId = m_openFiles.value(filePath);
    if (languageId.isEmpty()) return;

    auto it = m_clients.find(languageId);
    LspClient *client = (it != m_clients.end()) ? it->second.get() : nullptr;
    if (client && client->state() == LspClient::Initialized) {
        if (m_addedFiles.contains(filePath)) {
            m_fileVersions[filePath] = version;
            client->didChange(filePath, content, version);
        } else {
            qDebug() << "Buffering didChange as pending didOpen (client Initialized but file not added)";
            m_pendingDocuments[filePath] = {content, languageId, version, true};
        }
    } else {
        m_pendingDocuments[filePath] = {content, languageId, version, true};
    }
}

void LspManager::documentClosed(const QString &filePath) {
    QString languageId = m_openFiles.take(filePath);
    m_fileVersions.remove(filePath);
    m_pendingDocuments.remove(filePath);

    if (languageId.isEmpty()) return;

    if (m_addedFiles.contains(filePath)) {
        auto it = m_clients.find(languageId);
        LspClient *client = (it != m_clients.end()) ? it->second.get() : nullptr;
        if (client && client->state() == LspClient::Initialized) {
            client->didClose(filePath);
        }
        m_addedFiles.remove(filePath);
    }
    
    m_indexedFiles.remove(filePath);
}

void LspManager::indexProject(const QString &rootPath) {
    qDebug() << "Indexing project:" << rootPath;
    m_indexQueue.clear();
    m_indexTimer->stop();

    if (rootPath.isEmpty()) return;

    QDirIterator it(rootPath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString path = it.next();
        
        
        if (path.contains("/.") || path.contains("\\.")) {
            
            if (path.contains("/.git/") || path.contains("\\.git\\") || 
                path.contains("/.git") || path.contains("\\.git")) {
                continue;
            }
        }
        
        m_indexQueue.append(path);
    }

    if (!m_indexQueue.isEmpty()) {
        m_indexTimer->start();
    }
}

void LspManager::indexNextBatch() {
    const int batchSize = 10;
    int processed = 0;

    while (!m_indexQueue.isEmpty() && processed < batchSize) {
        indexFile(m_indexQueue.takeFirst());
        processed++;
    }

    if (m_indexQueue.isEmpty()) {
        m_indexTimer->stop();
        qDebug() << "LSP Project Indexing completed";
    }
}

void LspManager::indexFile(const QString &filePath) {
    if (m_addedFiles.contains(filePath) || m_pendingDocuments.contains(filePath)) return;

    QString langId = getLanguageForFile(filePath);
    if (langId == "Unknown" || !isLanguageSupported(langId)) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    
    QString content = QString::fromUtf8(file.readAll());
    file.close();

    LspClient *client = getClient(langId, m_projectRoot);
    if (client && client->state() == LspClient::Initialized) {
        qDebug() << "Indexing file (sending didOpen):" << filePath;
        client->didOpen(filePath, content, langId, 1);
        m_addedFiles.insert(filePath);
        m_indexedFiles[filePath] = langId;
    } else {
        qDebug() << "Indexing file (buffering):" << filePath;
        m_pendingDocuments[filePath] = {content, langId, 1, false};
    }
}

void LspManager::requestCompletion(const QString &filePath, int line, int column) {
    QString language = m_openFiles.value(filePath);
    if (!language.isEmpty()) {
        auto it = m_clients.find(language);
        if (it != m_clients.end()) {
            it->second->requestCompletion(filePath, line, column);
        }
    }
}

void LspManager::requestHover(const QString &filePath, int line, int column) {
    QString language = m_openFiles.value(filePath);
    if (!language.isEmpty()) {
        auto it = m_clients.find(language);
        if (it != m_clients.end()) {
            it->second->requestHover(filePath, line, column);
        }
    }
}

void LspManager::requestDefinition(const QString &filePath, int line, int column) {
    QString language = m_openFiles.value(filePath);
    if (!language.isEmpty()) {
        auto it = m_clients.find(language);
        if (it != m_clients.end()) {
            it->second->requestDefinition(filePath, line, column);
        }
    }
}

void LspManager::requestReferences(const QString &filePath, int line, int column) {
    QString language = m_openFiles.value(filePath);
    if (!language.isEmpty()) {
        auto it = m_clients.find(language);
        if (it != m_clients.end()) {
            it->second->requestReferences(filePath, line, column);
        }
    }
}

void LspManager::requestRename(const QString &filePath, int line, int column, const QString &newName) {
    QString language = m_openFiles.value(filePath);
    if (!language.isEmpty()) {
        auto it = m_clients.find(language);
        if (it != m_clients.end()) {
            it->second->requestRename(filePath, line, column, newName);
        }
    }
}

void LspManager::requestFormatting(const QString &filePath) {
    QString language = m_openFiles.value(filePath);
    if (!language.isEmpty()) {
        auto it = m_clients.find(language);
        if (it != m_clients.end()) {
            it->second->requestFormatting(filePath);
        }
    }
}

void LspManager::requestDocumentSymbols(const QString &filePath) {
    QString language = m_openFiles.value(filePath);
    if (!language.isEmpty()) {
        auto it = m_clients.find(language);
        if (it != m_clients.end()) {
            it->second->requestDocumentSymbols(filePath);
        }
    }
}

void LspManager::requestCodeAction(const QString &filePath, int startLine, int startCol, int endLine, int endCol, const QJsonArray &diagnostics) {
    QString language = m_openFiles.value(filePath);
    if (!language.isEmpty()) {
        auto it = m_clients.find(language);
        if (it != m_clients.end()) {
            it->second->requestCodeAction(filePath, startLine, startCol, endLine, endCol, diagnostics);
        }
    }
}

QString LspManager::getLanguageForFile(const QString &filePath) const {
    QString ext = QFileInfo(filePath).suffix().toLower();
    auto lang = Core::SimpleHighlighter::languageForExtension(ext);
    if (lang.has_value()) {
        return LspClientWidget::languageToString(lang.value());
    }
    return "Unknown";
}

QStringList LspManager::getArguments(const QString &argsString) const {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    return argsString.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
#else
    return argsString.split(QRegExp("\\s+"), QString::SkipEmptyParts);
#endif
}

} 
