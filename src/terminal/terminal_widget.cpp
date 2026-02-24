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
#include <QMimeData>
#include "pty_process.h"

namespace Terminal {

TerminalWidget::TerminalWidget(QWidget *parent, const QString &workingDirectory)
    : QPlainTextEdit(parent), m_workingDirectory(workingDirectory) {
    
    if (m_workingDirectory.isEmpty()) {
        m_workingDirectory = Core::ProjectManager::instance().projectRoot();
    }
    
    m_shell = detectShell();
    
    m_restartButton = new QPushButton("New Terminal Session?", this);
    m_restartButton->hide();
    connect(m_restartButton, &QPushButton::clicked, this, &TerminalWidget::restartShell);

    setReadOnly(false); // Set to false to allow blinking cursor; input is manually intercepted
    setUndoRedoEnabled(false);
    setCursorWidth(2); 
    setLineWrapMode(QPlainTextEdit::NoWrap);
    
    updateTheme();
    connect(&Core::ThemeManager::instance(), &Core::ThemeManager::themeChanged, this, &TerminalWidget::updateTheme);
    
    m_pty = PtyProcess::create(this);
    
    connect(m_pty, &PtyProcess::readyRead, this, &TerminalWidget::processData);
    connect(m_pty, &PtyProcess::finished, this, &TerminalWidget::onProcessFinished);
    connect(m_pty, &PtyProcess::errorOccurred, this, [this](const QString &err) {
        appendPlainText("Error: " + err);
    });
    
    connect(&Core::ProjectManager::instance(), &Core::ProjectManager::projectRootChanged,
            this, &TerminalWidget::setWorkingDirectory);

    QStringList args;
    // Use login shell to ensure environment is correctly initialized (common issue on WSL)
    if (m_shell.contains("sh") || m_shell.contains("zsh")) {
        args << "-l";
    }
    
    if (!m_pty->start(m_shell, args, m_workingDirectory)) {
        appendPlainText("Error: Could not start shell: " + m_shell);
    }
}

TerminalWidget::~TerminalWidget() {
    if (m_pty) {
        m_pty->stop();
    }
}

void TerminalWidget::setWorkingDirectory(const QString &dir) {
    if (m_workingDirectory == dir) return;
    m_workingDirectory = dir;

    if (m_pty && m_pty->isRunning()) {
#ifdef Q_OS_WIN
        sendInput(QString("cd /d \"%1\"\r\n").arg(m_workingDirectory));
#else
        sendInput(QString("cd \"%1\"\n").arg(m_workingDirectory));
#endif
    }
}

void TerminalWidget::sendInput(const QString &input) {
    if (m_pty && m_pty->isRunning()) {
        m_pty->write(input.toUtf8());
    }
}

// Redundant readyRead slots removed, processData is now connected directly to PtyProcess::readyRead

void TerminalWidget::processData(const QByteArray &data) {
    m_ansiBuffer.append(data);
    
    QTextCursor cursor(document());
    
    static const QTextCharFormat defaultFormat = []() {
        QTextCharFormat fmt;
        return fmt;
    }();
    
    QTextCharFormat currentFormat = textCursor().charFormat();
    if (currentFormat.isEmpty()) {
        currentFormat.setForeground(Core::ThemeManager::instance().getColor(Core::ThemeManager::EditorForeground));
    }
    
    auto flushText = [&](int start, int end) {
        if (end <= start) return;
        QString text = QString::fromUtf8(m_ansiBuffer.mid(start, end - start));
        
        for (QChar c : text) {
            if (c == '\n') {
                cursor.movePosition(QTextCursor::End);
                cursor.insertText("\n", currentFormat);
                cursor.movePosition(QTextCursor::End);
            } else if (c == '\t') {
                cursor.insertText("    ", currentFormat);
            } else {
                // Overwrite logic: if not at end of line, replace next char
                if (!cursor.atBlockEnd()) {
                    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                }
                cursor.insertText(c, currentFormat);
            }
        }
    };

    int lastProcessed = 0;
    cursor = textCursor();

    for (int i = 0; i < m_ansiBuffer.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(m_ansiBuffer[i]);
        
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
                flushText(lastProcessed, i);
                
                QByteArray seq = m_ansiBuffer.mid(i, end - i + 1);
                if (seq.startsWith("\x1B[") && seq.endsWith("m")) {
                    
                    QString params = QString::fromLatin1(seq.mid(2, seq.size() - 3));
                    if (params.isEmpty() || params == "0") {
                        currentFormat = defaultFormat;
                        currentFormat.setForeground(Core::ThemeManager::instance().getColor(Core::ThemeManager::EditorForeground));
                    } else {
                        for (const QString &p : params.split(';')) {
                            int val = p.toInt();
                            if (val == 0) {
                                currentFormat = defaultFormat;
                                currentFormat.setForeground(Core::ThemeManager::instance().getColor(Core::ThemeManager::EditorForeground));
                            } else if (val == 1) currentFormat.setFontWeight(QFont::Bold);
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
                } else if (seq == "\x1B[K" || seq == "\x1B[0K") {
                    QTextCursor temp = cursor;
                    temp.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    temp.removeSelectedText();
                } else if (seq == "\x1B[2K") {
                    QTextCursor temp = cursor;
                    temp.movePosition(QTextCursor::StartOfBlock);
                    temp.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    temp.removeSelectedText();
                } else if (seq == "\x1B[H" || seq == "\x1B[1;1H") {
                    // Very basic home handling
                    cursor.movePosition(QTextCursor::Start);
                }
                
                i = end;
                lastProcessed = i + 1;
            } else {
                
                break;
            }
        } else if (c == '\r') {
            flushText(lastProcessed, i);
            cursor.movePosition(QTextCursor::StartOfBlock);
            lastProcessed = i + 1;
        } else if (c == '\x08') { 
            flushText(lastProcessed, i);
            cursor.deletePreviousChar();
            lastProcessed = i + 1;
        } else if (c < 32 && c != '\n' && c != '\t') {
            flushText(lastProcessed, i);
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
            flushText(lastProcessed, m_ansiBuffer.size());
            m_ansiBuffer.clear();
        } else {
            
            m_ansiBuffer = m_ansiBuffer.mid(lastProcessed);
        }
    } else {
        m_ansiBuffer.clear();
    }
    
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    setTextCursor(cursor);
    ensureCursorVisible();
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
#ifdef Q_OS_WIN
    char* comspec = getenv("COMSPEC");
    if (comspec) return QString::fromLocal8Bit(comspec);
    return "cmd.exe";
#else
    char* shell = getenv("SHELL");
    if (shell) return QString::fromLocal8Bit(shell);
#ifdef Q_OS_MAC
    return "/bin/zsh";
#else
    return "/bin/bash";
#endif
#endif
}

void TerminalWidget::onProcessFinished(int exitCode) {
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
    
    if (m_pty) {
        m_pty->stop();
    }
    
    QStringList args;
    if (m_shell.contains("sh") || m_shell.contains("zsh")) {
        args << "-l";
    }
    
    m_pty->start(m_shell, args, m_workingDirectory);
}

void TerminalWidget::resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);
    if (m_restartButton->isVisible()) {
        m_restartButton->move((width() - m_restartButton->width()) / 2, 
                             (height() - m_restartButton->height()) / 2);
    }

    // Notify PTY of size change
    if (m_pty && m_pty->isRunning()) {
        QFontMetrics fm(font());
        int charWidth = fm.horizontalAdvance('W');
        int charHeight = fm.height();
        
        if (charWidth > 0 && charHeight > 0) {
            int cols = viewport()->width() / charWidth;
            int rows = viewport()->height() / charHeight;
            if (cols != m_lastCols || rows != m_lastRows) {
                m_lastCols = cols;
                m_lastRows = rows;
                m_pty->resize(cols, rows);
            }
        }
    }
}

bool TerminalWidget::event(QEvent *event) {
    if (event->type() == QEvent::ShortcutOverride) {
        event->accept();
        return true;
    }
    return QPlainTextEdit::event(event);
}

void TerminalWidget::focusInEvent(QFocusEvent *event) {
    QPlainTextEdit::focusInEvent(event);
    // Ensure cursor is always at the end when gaining focus
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
}

void TerminalWidget::insertFromMimeData(const QMimeData *source) {
    if (source->hasText()) {
        sendInput(source->text());
    }
}

void TerminalWidget::keyPressEvent(QKeyEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        int key = event->key();

        if (key == Qt::Key_C && textCursor().hasSelection()) {
            copy();
            return;
        }

        if (key == Qt::Key_V) {
            paste();
            return;
        }

        if (key >= Qt::Key_A && key <= Qt::Key_Z) {
            char ctrlChar = static_cast<char>(key - Qt::Key_A + 1);
            sendInput(QByteArray(1, ctrlChar));
            return;
        }
    }

    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        sendInput("\r");
    } else if (event->key() == Qt::Key_Backspace) {
        sendInput("\x7f"); // Sending DEL which is standard for most modern terminals
    } else if (event->key() == Qt::Key_Tab) {
        sendInput("\t");
    } else if (event->key() == Qt::Key_Left) {
        sendInput("\x1B[D");
    } else if (event->key() == Qt::Key_Right) {
        sendInput("\x1B[C");
    } else if (event->key() == Qt::Key_Up) {
        sendInput("\x1B[A");
    } else if (event->key() == Qt::Key_Down) {
        sendInput("\x1B[B");
    } else {
        QString text = event->text();
        if (!text.isEmpty()) {
            sendInput(text);
        }
    }
    event->accept();
}

} 
