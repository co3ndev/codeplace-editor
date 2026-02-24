#include "pty_process.h"
#include <QSocketNotifier>
#include <QDebug>

#ifdef Q_OS_UNIX
#include <pty.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#endif

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <thread>
#include <atomic>
#endif

namespace Terminal {

#ifdef Q_OS_UNIX
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
#endif

#ifdef Q_OS_WIN
// Missing defines for some compilers/SDK versions
#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE 0x00020016
#endif

typedef HRESULT (WINAPI *CreatePseudoConsolePtr)(COORD, HANDLE, HANDLE, DWORD, HPCON*);
typedef HRESULT (WINAPI *ResizePseudoConsolePtr)(HPCON, COORD);
typedef VOID (WINAPI *ClosePseudoConsolePtr)(HPCON);

class WinPtyProcess : public PtyProcess {
public:
    WinPtyProcess(QObject *parent) : PtyProcess(parent), 
        m_hpc(nullptr), 
        m_hInputWrite(INVALID_HANDLE_VALUE), 
        m_hOutputRead(INVALID_HANDLE_VALUE),
        m_hProcess(INVALID_HANDLE_VALUE) {}

    ~WinPtyProcess() override { stop(); }

    bool start(const QString &program, const QStringList &arguments, const QString &workingDir) override {
        stop();
        m_isStopping = false;

        HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
        auto pCreatePseudoConsole = (CreatePseudoConsolePtr)GetProcAddress(hKernel32, "CreatePseudoConsole");
        if (!pCreatePseudoConsole) {
            emit errorOccurred("ConPTY is not supported on this Windows version (requires Windows 10 1809+).");
            return false;
        }

        HANDLE hInR, hInW, hOutR, hOutW;
        
        // Input pipe: child reads, parent writes
        if (!CreatePipe(&hInR, &hInW, NULL, 0)) return false;
        // Output pipe: child writes, parent reads
        if (!CreatePipe(&hOutR, &hOutW, NULL, 0)) {
            CloseHandle(hInR); CloseHandle(hInW);
            return false;
        }

        m_hInputWrite = hInW;
        m_hOutputRead = hOutR;

        COORD size = { 80, 24 }; // Initial size
        HRESULT hr = pCreatePseudoConsole(size, hInR, hOutW, 0, &m_hpc);
        
        // Close handles used by ConPTY in the child side
        CloseHandle(hInR);
        CloseHandle(hOutW);

        if (FAILED(hr)) {
            emit errorOccurred("Failed to create PseudoConsole: 0x" + QString::number(hr, 16));
            return false;
        }

        // Setup process attributes
        STARTUPINFOEXW siEx = { 0 };
        siEx.StartupInfo.cb = sizeof(STARTUPINFOEXW);
        
        SIZE_T attrListSize = 0;
        InitializeProcThreadAttributeList(NULL, 1, 0, &attrListSize);
        siEx.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, attrListSize);
        InitializeProcThreadAttributeList(siEx.lpAttributeList, 1, 0, &attrListSize);
        UpdateProcThreadAttribute(siEx.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, m_hpc, sizeof(HPCON), NULL, NULL);

        QString fullCmd = "\"" + program + "\"";
        for (const QString &arg : arguments) {
            QString escapedArg = arg;
            escapedArg.replace("\"", "\\\"");
            fullCmd += " \"" + escapedArg + "\"";
        }
        std::wstring wCmd = fullCmd.toStdWString();
        std::wstring wDir = workingDir.toStdWString();

        PROCESS_INFORMATION pi = { 0 };
        BOOL success = CreateProcessW(
            NULL, (LPWSTR)wCmd.c_str(), NULL, NULL, FALSE,
            EXTENDED_STARTUPINFO_PRESENT, NULL, 
            wDir.empty() ? NULL : wDir.c_str(), 
            &siEx.StartupInfo, &pi
        );

        DeleteProcThreadAttributeList(siEx.lpAttributeList);
        HeapFree(GetProcessHeap(), 0, siEx.lpAttributeList);

        if (!success) {
            emit errorOccurred("Failed to create process: " + QString::number(GetLastError()));
            auto pClosePseudoConsole = (ClosePseudoConsolePtr)GetProcAddress(hKernel32, "ClosePseudoConsole");
            if (pClosePseudoConsole) pClosePseudoConsole(m_hpc);
            m_hpc = nullptr;
            return false;
        }

        m_hProcess = pi.hProcess;
        CloseHandle(pi.hThread);

        m_readThread = std::thread([this]() {
            char buffer[4096];
            DWORD bytesRead;
            while (!m_isStopping) {
                if (ReadFile(m_hOutputRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
                    QByteArray data(buffer, bytesRead);
                    QMetaObject::invokeMethod(this, [this, data]() {
                        emit readyRead(data);
                    }, Qt::QueuedConnection);
                } else {
                    break;
                }
            }
            
            if (!m_isStopping) {
                DWORD exitCode = 0;
                GetExitCodeProcess(m_hProcess, &exitCode);
                QMetaObject::invokeMethod(this, [this, exitCode]() {
                    stop();
                    emit finished(exitCode);
                }, Qt::QueuedConnection);
            }
        });

        return true;
    }

    void stop() override {
        if (!isRunning()) return;

        m_isStopping = true;
        
        if (m_hProcess != INVALID_HANDLE_VALUE) {
            TerminateProcess(m_hProcess, 1);
            CloseHandle(m_hProcess);
            m_hProcess = INVALID_HANDLE_VALUE;
        }

        if (m_hpc) {
            auto pClosePseudoConsole = (ClosePseudoConsolePtr)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "ClosePseudoConsole");
            if (pClosePseudoConsole) pClosePseudoConsole(m_hpc);
            m_hpc = nullptr;
        }

        if (m_hInputWrite != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hInputWrite);
            m_hInputWrite = INVALID_HANDLE_VALUE;
        }

        if (m_hOutputRead != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hOutputRead);
            m_hOutputRead = INVALID_HANDLE_VALUE;
        }

        if (m_readThread.joinable()) {
            m_readThread.join();
        }

        emit finished(0);
    }

    void write(const QByteArray &data) override {
        if (m_hInputWrite != INVALID_HANDLE_VALUE) {
            DWORD written;
            WriteFile(m_hInputWrite, data.constData(), data.size(), &written, NULL);
        }
    }

    bool isRunning() const override {
        if (m_hProcess == INVALID_HANDLE_VALUE) return false;
        DWORD exitCode;
        if (GetExitCodeProcess(m_hProcess, &exitCode)) {
            return exitCode == STILL_ACTIVE;
        }
        return false;
    }

    void resize(int cols, int rows) override {
        if (m_hpc) {
            COORD size = { (SHORT)cols, (SHORT)rows };
            auto pResizePseudoConsole = (ResizePseudoConsolePtr)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "ResizePseudoConsole");
            if (pResizePseudoConsole) pResizePseudoConsole(m_hpc, size);
        }
    }

private:
    HPCON m_hpc;
    HANDLE m_hInputWrite;
    HANDLE m_hOutputRead;
    HANDLE m_hProcess;
    std::thread m_readThread;
    std::atomic<bool> m_isStopping{false};
};
#endif

PtyProcess::PtyProcess(QObject *parent) : QObject(parent) {}
PtyProcess::~PtyProcess() {}

PtyProcess* PtyProcess::create(QObject *parent) {
#ifdef Q_OS_UNIX
    return new UnixPtyProcess(parent);
#elif defined(Q_OS_WIN)
    return new WinPtyProcess(parent);
#else
    return nullptr;
#endif
}

} // namespace Terminal
