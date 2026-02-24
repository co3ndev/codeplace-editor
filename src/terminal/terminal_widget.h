#ifndef TERMINAL_WIDGET_H
#define TERMINAL_WIDGET_H

#include <QPlainTextEdit>
#include <QProcess>
#include <QByteArray>
#include <QFocusEvent>

class QPushButton;

namespace Terminal {

class PtyProcess;

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
    void onProcessFinished(int exitCode);
    void restartShell();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void insertFromMimeData(const QMimeData *source) override;

private:
    void processData(const QByteArray &data);
    QString detectShell();
    void moveCursorToEnd();

    PtyProcess *m_pty;
    QString m_shell;
    QString m_workingDirectory;
    QByteArray m_ansiBuffer;
    QPushButton *m_restartButton;
    int m_lastCols = -1;
    int m_lastRows = -1;
};

} 

#endif 
