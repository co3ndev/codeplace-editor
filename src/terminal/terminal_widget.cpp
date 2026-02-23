#include "terminal_widget.h"
#include "core/theme_manager.h"
#include "core/project_manager.h"
#include <QKeyEvent>
#include <QScrollBar>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QPushButton>
#include <QVBoxLayout>

#include <signal.h>
#include <unistd.h>

namespace Terminal {

TerminalWidget::TerminalWidget(QWidget *parent, const QString &workingDirectory)
    : QPlainTextEdit(parent), m_workingDirectory(workingDirectory) {
    
    m_shell = detectShell();
    
    
    m_restartButton = new QPushButton("New Terminal Session?", this);
    m_restartButton->hide();
    connect(m_restartButton, &QPushButton::clicked, this, &TerminalWidget::restartShell);

    
    setReadOnly(false);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    
    updateTheme();
    connect(&Core::ThemeManager::instance(), &Core::ThemeManager::themeChanged, this, &TerminalWidget::updateTheme);
    
    m_process = new QProcess(this);
    if (!workingDirectory.isEmpty()) {
        m_process->setWorkingDirectory(workingDirectory);
    }
    
    connect(m_process, &QProcess::readyReadStandardOutput, this, &TerminalWidget::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &TerminalWidget::onReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &TerminalWidget::onProcessFinished);
    
    connect(&Core::ProjectManager::instance(), &Core::ProjectManager::projectRootChanged,
            this, &TerminalWidget::setWorkingDirectory);

    QStringList args;
    if (m_shell.contains("bash") || m_shell.contains("zsh")) {
        args << "-i";
    }

    m_process->start(m_shell, args);
    if (!m_process->waitForStarted()) {
        appendPlainText("Error: Could not start shell: " + m_shell);
    }
}

TerminalWidget::~TerminalWidget() {
    if (m_process->state() != QProcess::NotRunning) {
        ::kill(m_process->processId(), SIGHUP);
        m_process->terminate();
        if (!m_process->waitForFinished(1000)) {
            m_process->kill();
        }
    }
}

void TerminalWidget::setWorkingDirectory(const QString &dir) {
    m_workingDirectory = dir;
}

void TerminalWidget::sendInput(const QString &input) {
    if (m_process->state() == QProcess::Running) {
        m_process->write(input.toUtf8());
    }
}

void TerminalWidget::onReadyReadStandardOutput() {
    processData(m_process->readAllStandardOutput());
}

void TerminalWidget::onReadyReadStandardError() {
    processData(m_process->readAllStandardError());
}

void TerminalWidget::processData(const QByteArray &data) {
    m_ansiBuffer.append(data);
    
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    
    QTextCharFormat defaultFormat;
    defaultFormat.setForeground(Core::ThemeManager::instance().getColor(Core::ThemeManager::EditorForeground));
    
    QTextCharFormat currentFormat = cursor.charFormat();
    if (currentFormat.isEmpty()) currentFormat = defaultFormat;

    int lastProcessed = 0;
    for (int i = 0; i < m_ansiBuffer.size(); ++i) {
        char c = m_ansiBuffer[i];
        
        if (c == '\x1B') { 
            
            
            int end = -1;
            if (i + 1 < m_ansiBuffer.size()) {
                char next = m_ansiBuffer[i + 1];
                if (next == '[') { 
                    for (int j = i + 2; j < m_ansiBuffer.size(); ++j) {
                        char endChar = m_ansiBuffer[j];
                        if (endChar >= 0x40 && endChar <= 0x7E) {
                            end = j;
                            break;
                        }
                    }
                } else if (next == ']') { 
                    for (int j = i + 2; j < m_ansiBuffer.size(); ++j) {
                        if (m_ansiBuffer[j] == '\x07') {
                            end = j;
                            break;
                        }
                        if (m_ansiBuffer[j] == '\x1B' && j + 1 < m_ansiBuffer.size() && m_ansiBuffer[j+1] == '\\') {
                            end = j + 1;
                            break;
                        }
                    }
                } else if (next >= 0x40 && next <= 0x5F) { 
                    end = i + 1;
                }
            }
            
            if (end != -1) {
                
                if (i > lastProcessed) {
                    QString text = QString::fromUtf8(m_ansiBuffer.mid(lastProcessed, i - lastProcessed));
                    text.remove('\r');
                    cursor.insertText(text, currentFormat);
                }
                
                
                QByteArray seq = m_ansiBuffer.mid(i, end - i + 1);
                if (seq.startsWith("\x1B[") && seq.endsWith("m")) {
                    
                    QString params = QString::fromLatin1(seq.mid(2, seq.size() - 3));
                    for (const QString &p : params.split(';')) {
                        int val = p.toInt();
                        if (val == 0) currentFormat = defaultFormat;
                        else if (val == 1) currentFormat.setFontWeight(QFont::Bold);
                        else if (val >= 30 && val <= 37) {
                            static const Core::ThemeManager::EditorColor colors[] = { 
                                Core::ThemeManager::TerminalBlack, Core::ThemeManager::TerminalRed, 
                                Core::ThemeManager::TerminalGreen, Core::ThemeManager::TerminalYellow, 
                                Core::ThemeManager::TerminalBlue, Core::ThemeManager::TerminalMagenta, 
                                Core::ThemeManager::TerminalCyan, Core::ThemeManager::TerminalWhite 
                            };
                            currentFormat.setForeground(Core::ThemeManager::instance().getColor(colors[val - 30]));
                        } else if (val >= 90 && val <= 97) {
                            static const Core::ThemeManager::EditorColor colors[] = { 
                                Core::ThemeManager::TerminalBrightBlack, Core::ThemeManager::TerminalBrightRed, 
                                Core::ThemeManager::TerminalBrightGreen, Core::ThemeManager::TerminalBrightYellow, 
                                Core::ThemeManager::TerminalBrightBlue, Core::ThemeManager::TerminalBrightMagenta, 
                                Core::ThemeManager::TerminalBrightCyan, Core::ThemeManager::TerminalBrightWhite 
                            };
                            currentFormat.setForeground(Core::ThemeManager::instance().getColor(colors[val - 90]));
                        }
                    }
                }
                
                i = end;
                lastProcessed = i + 1;
            } else {
                
                break;
            }
        } else if (c == '\x08') { 
            if (i > lastProcessed) {
                QString text = QString::fromUtf8(m_ansiBuffer.mid(lastProcessed, i - lastProcessed));
                text.remove('\r');
                cursor.insertText(text, currentFormat);
            }
            cursor.deletePreviousChar();
            lastProcessed = i + 1;
        } else if (static_cast<unsigned char>(c) < 32 && c != '\n' && c != '\r' && c != '\t') {
            
            if (i > lastProcessed) {
                QString text = QString::fromUtf8(m_ansiBuffer.mid(lastProcessed, i - lastProcessed));
                text.remove('\r');
                cursor.insertText(text, currentFormat);
            }
            lastProcessed = i + 1;
        }
    }
    
    
    if (lastProcessed < m_ansiBuffer.size()) {
        
        bool inPartialEsc = false;
        for (int k = m_ansiBuffer.size() - 1; k >= lastProcessed; --k) {
            if (m_ansiBuffer[k] == '\x1B') {
                inPartialEsc = true;
                break;
            }
        }
        
        if (!inPartialEsc) {
            QString text = QString::fromUtf8(m_ansiBuffer.mid(lastProcessed));
            text.remove('\r');
            cursor.insertText(text, currentFormat);
            m_ansiBuffer.clear();
        } else {
            
            m_ansiBuffer = m_ansiBuffer.mid(lastProcessed);
        }
    } else {
        m_ansiBuffer.clear();
    }
    
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void TerminalWidget::appendAnsiText(const QString &text) {
    processData(text.toUtf8());
}

void TerminalWidget::updateTheme() {
    auto &tm = Core::ThemeManager::instance();
    QColor bg = tm.getColor(Core::ThemeManager::EditorBackground);
    QColor fg = tm.getColor(Core::ThemeManager::EditorForeground);
    QColor selection = tm.getColor(Core::ThemeManager::SelectionBackground);

    QString style = QString(
        "QPlainTextEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  font-family: 'Monospace', 'Courier New', monospace;"
        "  font-size: 13px;"
        "  border: none;"
        "  selection-background-color: %3;"
        "}"
    ).arg(bg.name(), fg.name(), selection.name());

    setStyleSheet(style);

    QColor statusBg = tm.getColor(Core::ThemeManager::StatusBarBackground);
    QColor statusFg = tm.getColor(Core::ThemeManager::StatusBarForeground);
    QColor btnHover = statusBg.lighter(120);
    QColor btnBorder = statusBg.darker(110);

    QString btnStyle = QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  padding: 8px 16px;"
        "  font-weight: bold;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: %4; }"
    
    ).arg(statusBg.name(), statusFg.name(), btnBorder.name(), btnHover.name());

    if (m_restartButton) m_restartButton->setStyleSheet(btnStyle);
}

QString TerminalWidget::detectShell() {
    char* shell = getenv("SHELL");
    if (shell) return QString::fromLocal8Bit(shell);
    return "/bin/bash";
}

void TerminalWidget::onProcessFinished(int exitCode, QProcess::ExitStatus ) {
    appendPlainText(QString("\nProcess finished with exit code %1").arg(exitCode));
    
    
    m_restartButton->show();
    m_restartButton->raise();
    
    m_restartButton->move((width() - m_restartButton->width()) / 2, 
                         (height() - m_restartButton->height()) / 2);
}

void TerminalWidget::restartShell() {
    m_restartButton->hide();
    clear();
    m_ansiBuffer.clear();
    
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
    }
    
    QStringList args;
    if (m_shell.contains("bash") || m_shell.contains("zsh")) {
        args << "-i";
    }

    if (!m_workingDirectory.isEmpty()) {
        m_process->setWorkingDirectory(m_workingDirectory);
    }
    
    m_process->start(m_shell, args);
}

void TerminalWidget::resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);
    if (m_restartButton->isVisible()) {
        m_restartButton->move((width() - m_restartButton->width()) / 2, 
                             (height() - m_restartButton->height()) / 2);
    }
}

bool TerminalWidget::event(QEvent *event) {
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->modifiers() & Qt::ControlModifier) {
            int key = keyEvent->key();
            
            
            if (key >= Qt::Key_A && key <= Qt::Key_Z) {
                event->accept();
                return true;
            }
        }
    }
    return QPlainTextEdit::event(event);
}

void TerminalWidget::keyPressEvent(QKeyEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        int key = event->key();

        
        if (key == Qt::Key_C && textCursor().hasSelection()) {
            copy();
            return;
        }

        
        if (key == Qt::Key_C && !textCursor().hasSelection()) {
            if (m_process->state() == QProcess::Running) {
                
                QTextCursor cursor = textCursor();
                cursor.movePosition(QTextCursor::End);
                cursor.insertText("^C\n");
                verticalScrollBar()->setValue(verticalScrollBar()->maximum());
                
                ::kill(m_process->processId(), SIGINT);
                return;
            }
        }

        if (key == Qt::Key_V) {
            paste();
            return;
        }

        if (key >= Qt::Key_A && key <= Qt::Key_Z) {
            char ctrlChar = static_cast<char>(key - Qt::Key_A + 1);
            sendInput(QString(ctrlChar));
            return;
        }
    }

    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        sendInput("\n");
    } else if (event->key() == Qt::Key_Backspace) {
        sendInput("\b");
    } else if (event->key() == Qt::Key_Tab) {
        sendInput("\t");
    } else {
        QString text = event->text();
        if (!text.isEmpty()) {
            sendInput(text);
        }
    }
    
}

} 
