#include "pty_process.h"
#include <QSocketNotifier>
#include <QDebug>

#include <pty.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>

namespace Terminal {

class UnixPtyProcess : public PtyProcess {
public:
    UnixPtyProcess(QObject *parent) : PtyProcess(parent), m_masterFd(-1), m_pid(-1), m_notifier(nullptr) {}
    ~UnixPtyProcess() override { stop(); }

    bool start(const QString &program, const QStringList &arguments, const QString &workingDir) override {
        int master;
        pid_t pid = forkpty(&master, nullptr, nullptr, nullptr);

        if (pid < 0) {
            emit errorOccurred("Failed to fork PTY: " + QString::fromLocal8Bit(strerror(errno)));
            return false;
        }

        if (pid == 0) { // Child
            setenv("TERM", "xterm-256color", 1);
            setenv("COLORTERM", "truecolor", 1);

            if (!workingDir.isEmpty()) {
                if (chdir(workingDir.toLocal8Bit().constData()) != 0) {
                    perror("chdir failed");
                }
            }

            QByteArray prog = program.toLocal8Bit();
            QVector<QByteArray> argStorage;
            QVector<char*> argv;
            
            argv.push_back(prog.data());
            for (const QString &arg : arguments) {
                argStorage.push_back(arg.toLocal8Bit());
                argv.push_back(argStorage.last().data());
            }
            argv.push_back(nullptr);

            execvp(prog.constData(), argv.data());
            _exit(1);
        }

        // Parent
        m_masterFd = master;
        m_pid = pid;

        m_notifier = new QSocketNotifier(m_masterFd, QSocketNotifier::Read, this);
        connect(m_notifier, &QSocketNotifier::activated, this, [this](int) {
            char buffer[4096];
            ssize_t bytes = read(m_masterFd, buffer, sizeof(buffer));
            if (bytes > 0) {
                emit readyRead(QByteArray(buffer, bytes));
            } else if (bytes < 0 && (errno == EINTR || errno == EAGAIN)) {
                return;
            } else {
                stop();
            }
        });

        return true;
    }

    void stop() override {
        if (m_pid > 0) {
            kill(m_pid, SIGHUP);
            m_pid = -1;
        }
        if (m_masterFd != -1) {
            delete m_notifier;
            m_notifier = nullptr;
            close(m_masterFd);
            m_masterFd = -1;
            emit finished(0);
        }
    }

    void write(const QByteArray &data) override {
        if (m_masterFd != -1) {
            ::write(m_masterFd, data.constData(), data.size());
        }
    }

    bool isRunning() const override {
        return m_pid > 0;
    }

    void resize(int cols, int rows) override {
        if (m_masterFd != -1) {
            struct winsize ws;
            ws.ws_col = cols;
            ws.ws_row = rows;
            ws.ws_xpixel = 0;
            ws.ws_ypixel = 0;
            ioctl(m_masterFd, TIOCSWINSZ, &ws);
        }
    }

private:
    int m_masterFd;
    pid_t m_pid;
    QSocketNotifier *m_notifier;
};

PtyProcess::PtyProcess(QObject *parent) : QObject(parent) {}
PtyProcess::~PtyProcess() {}

PtyProcess* PtyProcess::create(QObject *parent) {
    return new UnixPtyProcess(parent);
}

} // namespace Terminal
