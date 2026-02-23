#ifndef PROCESS_HELPER_H
#define PROCESS_HELPER_H

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <memory>

namespace Core {

class BackgroundProcess : public QObject {
    Q_OBJECT

public:
    explicit BackgroundProcess(QObject *parent = nullptr);
    ~BackgroundProcess() override;

    bool start(const QString &program, const QStringList &arguments, const QString &workingDir = QString());
    void stop();
    bool isRunning() const;
    
    QByteArray readAllStandardOutput();
    QByteArray readAllStandardError();
    void write(const QByteArray &data);

    QString errorString() const;
    int exitCode() const;
    QProcess::ExitStatus exitStatus() const;
    QProcess::ProcessState state() const;

signals:
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void readyReadStandardOutput();
    void readyReadStandardError();
    void errorOccurred(const QString &error);
    void stateChanged(QProcess::ProcessState newState);

private:
    std::unique_ptr<QProcess> m_process;
};

} 

#endif 
