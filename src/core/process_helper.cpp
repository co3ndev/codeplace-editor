#include "process_helper.h"
#include <QDebug>

namespace Core {

BackgroundProcess::BackgroundProcess(QObject *parent) : QObject(parent), m_process(std::make_unique<QProcess>()) {
    connect(m_process.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &BackgroundProcess::finished);
    connect(m_process.get(), &QProcess::readyReadStandardOutput, this, &BackgroundProcess::readyReadStandardOutput);
    connect(m_process.get(), &QProcess::readyReadStandardError, this, &BackgroundProcess::readyReadStandardError);
    connect(m_process.get(), &QProcess::stateChanged, this, &BackgroundProcess::stateChanged);
    connect(m_process.get(), &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        emit errorOccurred(m_process->errorString());
    });
}

BackgroundProcess::~BackgroundProcess() {
    stop();
}

bool BackgroundProcess::start(const QString &program, const QStringList &arguments, const QString &workingDir) {
    if (!workingDir.isEmpty()) {
        m_process->setWorkingDirectory(workingDir);
    }
    m_process->start(program, arguments);
    return m_process->waitForStarted();
}

void BackgroundProcess::stop() {
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
        }
    }
}

bool BackgroundProcess::isRunning() const {
    return m_process->state() == QProcess::Running;
}

QByteArray BackgroundProcess::readAllStandardOutput() {
    return m_process->readAllStandardOutput();
}

QByteArray BackgroundProcess::readAllStandardError() {
    return m_process->readAllStandardError();
}

void BackgroundProcess::write(const QByteArray &data) {
    m_process->write(data);
}

QString BackgroundProcess::errorString() const {
    return m_process->errorString();
}

int BackgroundProcess::exitCode() const {
    return m_process->exitCode();
}

QProcess::ExitStatus BackgroundProcess::exitStatus() const {
    return m_process->exitStatus();
}

QProcess::ProcessState BackgroundProcess::state() const {
    return m_process->state();
}

} 
