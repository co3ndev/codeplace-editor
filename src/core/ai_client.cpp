#include "ai_client.h"
#include <QUrl>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

AiClient::AiClient(QObject *parent) : QObject(parent) {
    m_networkManager = new QNetworkAccessManager(this);
}

void AiClient::fetchModels(const QString &baseUrl, const QString &apiKey) {
    QString fullUrl = baseUrl;
    if (!fullUrl.endsWith("/")) fullUrl += "/";
    fullUrl += "models";

    QNetworkRequest request;
    request.setUrl(QUrl(fullUrl));
    if (!apiKey.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());
    }

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QList<QJsonObject> models;
            if (doc.isObject()) {
                QJsonArray dataArray = doc.object()["data"].toArray();
                for (const QJsonValue &value : dataArray) {
                    if (value.isObject()) {
                        models << value.toObject();
                    }
                }
            }
            emit modelsFetched(models);
        } else {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject() && doc.object().contains("error")) {
                QJsonObject errorObj = doc.object()["error"].toObject();
                QString msg = errorObj.contains("message") ? errorObj["message"].toString() : reply->errorString();
                emit errorOccurred(msg);
            } else {
                emit errorOccurred(reply->errorString());
            }
        }
        reply->deleteLater();
    });
}

void AiClient::sendChatRequest(const QString &baseUrl, const QString &apiKey, const QJsonObject &payload) {
    QString fullUrl = baseUrl;
    if (!fullUrl.endsWith("/")) fullUrl += "/";
    fullUrl += "chat/completions";

    QNetworkRequest request;
    request.setUrl(QUrl(fullUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!apiKey.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());
    }

    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                QJsonArray choices = doc.object()["choices"].toArray();
                if (!choices.isEmpty()) {
                    QString content = choices[0].toObject()["message"].toObject()["content"].toString();
                    emit responseReceived(content);
                } else {
                    emit errorOccurred("No choices in response");
                }
            } else {
                emit errorOccurred("Invalid JSON response");
            }
        } else {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject() && doc.object().contains("error")) {
                QJsonObject errorObj = doc.object()["error"].toObject();
                QString msg = errorObj.contains("message") ? errorObj["message"].toString() : reply->errorString();
                emit errorOccurred(msg);
            } else {
                emit errorOccurred(reply->errorString());
            }
        }
        reply->deleteLater();
    });
}


