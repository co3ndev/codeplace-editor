#ifndef AI_CLIENT_H
#define AI_CLIENT_H

#include <QObject>
#include <QStringList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

class AiClient : public QObject {
    Q_OBJECT
public:
    explicit AiClient(QObject *parent = nullptr);

    void fetchModels(const QString &baseUrl, const QString &apiKey);
    void sendChatRequest(const QString &baseUrl, const QString &apiKey, const QJsonObject &payload);

signals:
    void modelsFetched(const QList<QJsonObject> &models);
    void responseReceived(const QString &message);
    void errorOccurred(const QString &error);

private:
    QNetworkAccessManager *m_networkManager;
};

#endif // AI_CLIENT_H

