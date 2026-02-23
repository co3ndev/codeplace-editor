#ifndef TERMINAL_WIDGET_H
#define TERMINAL_WIDGET_H

#include <QPlainTextEdit>
#include <QProcess>
#include <QByteArray>

class QPushButton;

namespace Terminal {

class TerminalWidget : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget *parent = nullptr, const QString &workingDirectory = QString());
    ~TerminalWidget() override;

    void sendInput(const QString &input);
    void appendAnsiText(const QString &text);
    void setWorkingDirectory(const QString &dir);

public slots:
    void updateTheme();

signals:
    void inputReceived(const QString &input);
    void outputReceived(const QString &data);

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void restartShell();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void processData(const QByteArray &data);
    QString detectShell();

    QProcess *m_process;
    QString m_shell;
    QString m_workingDirectory;
    QByteArray m_ansiBuffer;
    QPushButton *m_restartButton;
};

} 

#endif 
