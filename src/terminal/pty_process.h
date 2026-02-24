#ifndef PTY_PROCESS_H
#define PTY_PROCESS_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSize>

namespace Terminal {

class PtyProcess : public QObject {
    Q_OBJECT

public:
    explicit PtyProcess(QObject *parent = nullptr);
    virtual ~PtyProcess();

    virtual bool start(const QString &program, const QStringList &arguments, const QString &workingDir = QString()) = 0;
    virtual void stop() = 0;
    virtual void write(const QByteArray &data) = 0;
    virtual bool isRunning() const = 0;
    virtual void resize(int cols, int rows) = 0;

    static PtyProcess* create(QObject *parent = nullptr);

signals:
    void readyRead(const QByteArray &data);
    void finished(int exitCode);
    void errorOccurred(const QString &error);
};

} // namespace Terminal

#endif // PTY_PROCESS_H
