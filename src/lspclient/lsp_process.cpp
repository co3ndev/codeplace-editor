#include "lsp_process.h"
#include <QJsonDocument>
#include <QDebug>
#include <QRegularExpression>

namespace LspClient {

LspProcess::LspProcess(QObject *parent)
    : QObject(parent), m_process(std::make_unique<Core::BackgroundProcess>()), m_expectedContentLength(-1) {
    connect(m_process.get(), &Core::BackgroundProcess::readyReadStandardOutput, this, &LspProcess::onReadyReadStandardOutput);
    connect(m_process.get(), &Core::BackgroundProcess::readyReadStandardError, this, &LspProcess::onReadyReadStandardError);
    connect(m_process.get(), &Core::BackgroundProcess::errorOccurred, this, [this](const QString &error) {
        emit errorOccurred(error);
    });
    connect(m_process.get(), &Core::BackgroundProcess::stateChanged, this, &LspProcess::stateChanged);
}

LspProcess::~LspProcess() {
    stop();
}

bool LspProcess::start(const QString &program, const QStringList &arguments) {
    if (m_process->isRunning()) {
        return false;
    }
    m_buffer.clear();
    m_expectedContentLength = -1;
    return m_process->start(program, arguments);
}

void LspProcess::stop() {
    m_process->stop();
}

bool LspProcess::isRunning() const {
    return m_process->state() == QProcess::Running;
}

void LspProcess::sendMessage(const QJsonObject &message) {
    if (!isRunning()) return;

    QJsonDocument doc(message);
    QByteArray payload = doc.toJson(QJsonDocument::Compact);
    
    QByteArray rpcMessage;
    rpcMessage.append(QString("Content-Length: %1\r\n\r\n").arg(payload.size()).toUtf8());
    rpcMessage.append(payload);

    m_process->write(rpcMessage);
}

void LspProcess::onReadyReadStandardOutput() {
    m_buffer.append(m_process->readAllStandardOutput());
    processBuffer();
}

void LspProcess::onReadyReadStandardError() {
    QByteArray err = m_process->readAllStandardError();
    qDebug() << "LSP STDERR:" << QString::fromUtf8(err.trimmed());
}

void LspProcess::processBuffer() {
    while (true) {
        if (m_expectedContentLength == -1) {
            int headerEnd = m_buffer.indexOf("\r\n\r\n");
            if (headerEnd == -1) {
                return;
            }

            QByteArray headers = m_buffer.left(headerEnd);
            static QRegularExpression clRegex("Content-Length: (\\d+)");
            QRegularExpressionMatch match = clRegex.match(QString::fromUtf8(headers));
            
            if (match.hasMatch()) {
                m_expectedContentLength = match.captured(1).toInt();
            } else {
                qWarning() << "LSP missing Content-Length header. Discarding: " << headers;
                m_buffer.remove(0, headerEnd + 4);
                continue;
            }

            m_buffer.remove(0, headerEnd + 4);
        }

        if (m_expectedContentLength != -1 && m_buffer.size() >= m_expectedContentLength) {
            QByteArray payload = m_buffer.left(m_expectedContentLength);
            m_buffer.remove(0, m_expectedContentLength);
            m_expectedContentLength = -1;

            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
            
            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                emit messageReceived(doc.object());
            } else {
                qWarning() << "LSP JSON parse error:" << parseError.errorString() << "Payload:" << payload;
            }
        } else {
            return;
        }
    }
}

} 
