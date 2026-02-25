#ifndef AI_CLIENT_H
#define AI_CLIENT_H

#include <QObject>
#include <QStringList>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class AiClient : public QObject {
    Q_OBJECT
public:
    explicit AiClient(QObject *parent = nullptr);
};

#endif // AI_CLIENT_H
