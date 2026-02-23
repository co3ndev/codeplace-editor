#ifndef LSP_PROCESS_H
#define LSP_PROCESS_H

#include <QObject>
#include <QStringList>
#include <QByteArray>
#include <memory>
#include <QJsonObject>
#include "../core/process_helper.h"

namespace LspClient {

class LspProcess : public QObject {
    Q_OBJECT

public:
    explicit LspProcess(QObject *parent = nullptr);
    ~LspProcess() override;

    bool start(const QString &program, const QStringList &arguments);
    void stop();
    bool isRunning() const;

    void sendMessage(const QJsonObject &message);

signals:
    void messageReceived(const QJsonObject &message);
    void errorOccurred(const QString &error);
    void stateChanged(QProcess::ProcessState state);

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();

private:
    void processBuffer();

    std::unique_ptr<Core::BackgroundProcess> m_process;
    QByteArray m_buffer;
    int m_expectedContentLength = -1;
};

} 

#endif 
