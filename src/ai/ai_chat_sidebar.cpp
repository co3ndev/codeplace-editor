#include "ai_chat_sidebar.h"
#include "ai_message_widget.h"
#include "../core/ai_client.h"
#include "../core/settings_manager.h"
#include "../core/theme_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QKeyEvent>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QToolButton>
#include <QFrame>
#include <QTimer>
#include <QMenu>
#include <QDirIterator>
#include "../editor/tab_container.h"
#include "../core/project_manager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

class AiContextChip : public QFrame {
    Q_OBJECT
public:
    AiContextChip(const QString &filePath, QWidget *parent = nullptr) : QFrame(parent) {
        setObjectName("contextChip");
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(8, 2, 4, 2);
        layout->setSpacing(4);

        QString fileName = QFileInfo(filePath).fileName();
        auto *label = new QLabel(fileName, this);
        label->setStyleSheet("font-size: 11px;");
        layout->addWidget(label);

        auto *closeBtn = new QToolButton(this);
        closeBtn->setText("×");
        closeBtn->setCursor(Qt::PointingHandCursor);
        closeBtn->setAutoRaise(true);
        closeBtn->setStyleSheet("QToolButton { border: none; font-weight: bold; font-size: 14px; } QToolButton:hover { color: red; }");
        layout->addWidget(closeBtn);

        connect(closeBtn, &QToolButton::clicked, [this, filePath]() {
            emit removed(filePath);
        });

        applyTheme();
    }

    void applyTheme() {
        auto &tm = Core::ThemeManager::instance();
        QColor bgColor = tm.getColor(Core::ThemeManager::EditorBackground);
        // Make it slightly different from background
        bgColor = bgColor.lightness() > 128 ? bgColor.darker(110) : bgColor.lighter(130);
        QColor borderColor = tm.getColor(Core::ThemeManager::FindBarBorder);
        QColor textColor = tm.getColor(Core::ThemeManager::EditorForeground);

        setStyleSheet(QString(
            "#contextChip { background-color: %1; border: 1px solid %2; border-radius: 10px; color: %3; }"
        ).arg(bgColor.name()).arg(borderColor.name()).arg(textColor.name()));
    }

signals:
    void removed(const QString &filePath);
};

AiChatSidebar::AiChatSidebar(QWidget *parent) : QWidget(parent) {
    m_aiClient = new AiClient(this);
    connect(m_aiClient, &AiClient::responseReceived, this, &AiChatSidebar::onAiResponseReceived);
    connect(m_aiClient, &AiClient::errorOccurred, this, &AiChatSidebar::onAiErrorOccurred);

    setupUi();
    connect(&Core::ThemeManager::instance(), &Core::ThemeManager::themeChanged, this, &AiChatSidebar::applyTheme);
    applyTheme();
}

void AiChatSidebar::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // History Area
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_historyContainer = new QWidget();
    m_historyLayout = new QVBoxLayout(m_historyContainer);
    m_historyLayout->setAlignment(Qt::AlignTop);
    
    // Add a stretch at the end to keep messages at the top if there aren't many
    m_historyLayout->addStretch();
    
    m_scrollArea->setWidget(m_historyContainer);
    mainLayout->addWidget(m_scrollArea, 1);

    // Input Area
    m_inputContainer = new QWidget(this);
    auto *inputLayout = new QVBoxLayout(m_inputContainer);
    inputLayout->setContentsMargins(10, 10, 10, 10);

    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setPlaceholderText("Type a message... (Shift+Enter for newline)");
    m_inputEdit->setMaximumHeight(100);
    m_inputEdit->installEventFilter(this);
    inputLayout->addWidget(m_inputEdit);

    auto *buttonLayout = new QHBoxLayout();
    m_contextButton = new QPushButton("Add Context", this);
    m_newChatButton = new QPushButton("New Chat", this);
    m_sendButton = new QPushButton("Send", this);
    m_sendButton->setDefault(true);
    
    buttonLayout->addWidget(m_contextButton);
    buttonLayout->addWidget(m_newChatButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_sendButton);
    inputLayout->addLayout(buttonLayout);

    mainLayout->addWidget(m_inputContainer);

    connect(m_sendButton, &QPushButton::clicked, this, &AiChatSidebar::onSendClicked);
    connect(m_contextButton, &QPushButton::clicked, this, &AiChatSidebar::onContextClicked);
    connect(m_newChatButton, &QPushButton::clicked, this, &AiChatSidebar::onNewChatClicked);

    // Chips Area
    m_chipsScrollArea = new QScrollArea(this);
    m_chipsScrollArea->setWidgetResizable(true);
    m_chipsScrollArea->setFrameShape(QFrame::NoFrame);
    m_chipsScrollArea->setFixedHeight(35);
    m_chipsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_chipsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_chipsContainer = new QWidget();
    m_chipsLayout = new QHBoxLayout(m_chipsContainer);
    m_chipsLayout->setContentsMargins(5, 5, 5, 5);
    m_chipsLayout->setSpacing(5);
    m_chipsLayout->addStretch();

    m_chipsScrollArea->setWidget(m_chipsContainer);
    m_chipsScrollArea->hide(); // Hide initially until we have chips

    inputLayout->insertWidget(0, m_chipsScrollArea);
}

void AiChatSidebar::applyTheme() {
    auto &tm = Core::ThemeManager::instance();
    QColor borderColor = tm.getColor(Core::ThemeManager::FindBarBorder);
    
    // Add a subtle border at the top of the input container
    m_inputContainer->setStyleSheet(
        QString("#inputContainer { border-top: 1px solid %1; }").arg(borderColor.name())
    );
    m_inputContainer->setObjectName("inputContainer");

    // Update chips theme if any
    for (int i = 0; i < m_chipsLayout->count(); ++i) {
        if (auto *chip = qobject_cast<AiContextChip*>(m_chipsLayout->itemAt(i)->widget())) {
            chip->applyTheme();
        }
    }
}

void AiChatSidebar::setTabContainer(Editor::TabContainer *container) {
    m_tabContainer = container;
}

void AiChatSidebar::addAttachedFolder(const QString &folderPath) {
    if (m_attachedFiles.contains(folderPath)) return; // Use same list for simplicity but mark as folder
    m_attachedFiles.append(folderPath);
    
    auto *chip = new AiContextChip(folderPath, m_chipsContainer);
    // Maybe make folder chips look slightly different? Or just same is fine.
    
    m_chipsLayout->insertWidget(m_chipsLayout->count() - 1, chip);
    connect(chip, &AiContextChip::removed, this, &AiChatSidebar::removeAttachedFile); // Re-use removeAttachedFile
    m_chipsScrollArea->show();
}

void AiChatSidebar::removeAttachedFolder(const QString &folderPath) {
    removeAttachedFile(folderPath);
}

QString AiChatSidebar::getFullContext() const {
    QString context;

    // 1. Open Files
    if (m_tabContainer) {
        QStringList paths = m_tabContainer->openFilePaths();
        if (!paths.isEmpty()) {
            context += "Currently open files:\n";
            for (int i = 0; i < m_tabContainer->count(); ++i) {
                QString path = paths[i];
                if (path.isEmpty()) continue;
                context += QString("--- %1 ---\n").arg(QFileInfo(path).fileName());
                context += m_tabContainer->getFileContent(i);
                context += "\n";
            }
            context += "\n";
        }
    }

    // 2. Attached Files & Folders
    if (!m_attachedFiles.isEmpty()) {
        context += "Additional context:\n";
        for (const QString &path : m_attachedFiles) {
            QFileInfo fi(path);
            if (fi.isDir()) {
                context += QString("--- Folder: %1 ---\n").arg(fi.fileName());
                QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
                while (it.hasNext()) {
                    QString filePath = it.next();
                    QFile file(filePath);
                    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        context += QString("[File: %1]\n").arg(QDir(path).relativeFilePath(filePath));
                        context += file.readAll();
                        context += "\n";
                    }
                }
            } else {
                QFile file(path);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    context += QString("--- %1 ---\n").arg(fi.fileName());
                    context += file.readAll();
                    context += "\n";
                }
            }
        }
        context += "\n";
    }

    // 3. Snippets
    if (!m_attachedSnippets.isEmpty()) {
        context += "Selected snippets:\n";
        for (const QString &snippet : m_attachedSnippets) {
            context += "--- Snippet ---\n";
            context += snippet;
            context += "\n";
        }
        context += "\n";
    }

    return context;
}

void AiChatSidebar::addAttachedSnippet(const QString &text) {
    if (text.isEmpty()) return;
    m_attachedSnippets.append(text);
    
    QString display = text.left(20);
    if (text.length() > 20) display += "...";
    auto *chip = new AiContextChip("Snippet: " + display, m_chipsContainer);
    
    m_chipsLayout->insertWidget(m_chipsLayout->count() - 1, chip);
    connect(chip, &AiContextChip::removed, [this, text](const QString &) {
        m_attachedSnippets.removeAll(text);
        if (auto *c = qobject_cast<AiContextChip*>(sender())) {
            c->deleteLater();
        }
    });

    m_chipsScrollArea->show();
}

void AiChatSidebar::addAttachedFile(const QString &filePath) {
    if (m_attachedFiles.contains(filePath)) return;

    m_attachedFiles.append(filePath);
    
    auto *chip = new AiContextChip(filePath, m_chipsContainer);
    
    // Insert before the stretch at the end
    m_chipsLayout->insertWidget(m_chipsLayout->count() - 1, chip);
    
    connect(chip, &AiContextChip::removed, this, &AiChatSidebar::removeAttachedFile);

    m_chipsScrollArea->show();
}

void AiChatSidebar::removeAttachedFile(const QString &filePath) {
    m_attachedFiles.removeAll(filePath);
    if (auto *chip = qobject_cast<AiContextChip*>(sender())) {
        chip->deleteLater();
    }
    
    // Hide scroll area if no more chips (delayed check because deleteLater)
    QTimer::singleShot(0, this, [this]() {
        if (m_attachedFiles.isEmpty()) {
            m_chipsScrollArea->hide();
        }
    });
}

// Better implementation using sender()
void AiChatSidebar::clearChips() {
    m_attachedFiles.clear();
    m_attachedSnippets.clear();
    while (m_chipsLayout->count() > 1) { // Keep the stretch
        auto *item = m_chipsLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    m_chipsScrollArea->hide();
}

void AiChatSidebar::clearHistory() {
    m_chatHistory.clear();
    // Keep the last item which is the stretch
    while (m_historyLayout->count() > 1) {
        auto *item = m_historyLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void AiChatSidebar::onNewChatClicked() {
    clearHistory();
    clearChips();
    m_inputEdit->clear();
    m_pendingMessage = nullptr;
}

bool AiChatSidebar::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_inputEdit && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (!(keyEvent->modifiers() & Qt::ShiftModifier)) {
                onSendClicked();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

AiMessageWidget* AiChatSidebar::addMessage(const QString &text, bool isUser) {
    auto *msg = new AiMessageWidget(this);
    msg->setMessage(text, isUser);
    
    // Insert before the stretch (which is at index count() - 1)
    m_historyLayout->insertWidget(m_historyLayout->count() - 1, msg);
    
    // Scroll to bottom after layout update
    QTimer::singleShot(50, this, [this]() {
        m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->maximum());
    });
    return msg;
}

void AiChatSidebar::onSendClicked() {
    QString text = m_inputEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;

    // Assemble Prompt
    QString context = getFullContext();
    
    QString systemPrompt = "You are an AI assistant built into CodePlace Editor. Your goal is to help the user with their code. You cannot edit files directly. Use the provided context to answer questions.";
    
    QString finalPrompt = context;
    if (!context.isEmpty()) {
        finalPrompt += "\n";
    }
    finalPrompt += "User Message: " + text;

    addMessage(text, true);
    m_chatHistory.append({"user", finalPrompt});
    m_inputEdit->clear();
    clearChips();

    // Prepare Payload
    auto &settings = Core::SettingsManager::instance();
    QString model = settings.aiSelectedModel();
    if (model.isEmpty()) {
        addMessage("", false)->setError("No model selected. Please go to Preferences > AI and select a model.");
        return;
    }

    QJsonObject payload;
    payload["model"] = model;
    payload["stream"] = false;

    QJsonArray messages;
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = systemPrompt;
    messages.append(systemMsg);

    // Provide the last 5 messages from history
    int historyCount = m_chatHistory.size();
    int startIdx = std::max(0, historyCount - 5);
    
    for (int i = startIdx; i < historyCount; ++i) {
        QJsonObject msg;
        msg["role"] = m_chatHistory[i].role;
        msg["content"] = m_chatHistory[i].content;
        messages.append(msg);
    }

    payload["messages"] = messages;

    // Send Request
    QString baseUrl = settings.aiProvider() == "OpenRouter" ? "https://openrouter.ai/api/v1" : settings.aiLocalUrl();
    QString apiKey = settings.aiProvider() == "OpenRouter" ? settings.aiOpenRouterKey() : "";

    m_sendButton->setEnabled(false);
    m_sendButton->setText("Thinking...");
    
    // Create pending message with loading spinner
    m_pendingMessage = addMessage("", false);
    m_pendingMessage->setLoading(true);

    m_aiClient->sendChatRequest(baseUrl, apiKey, payload);
}

void AiChatSidebar::onAiResponseReceived(const QString &message) {
    m_sendButton->setEnabled(true);
    m_sendButton->setText("Send");
    
    if (m_pendingMessage) {
        m_pendingMessage->setMessage(message, false);
        m_pendingMessage = nullptr;
    } else {
        addMessage(message, false);
    }
    m_chatHistory.append({"assistant", message});
}

void AiChatSidebar::onAiErrorOccurred(const QString &error) {
    m_sendButton->setEnabled(true);
    m_sendButton->setText("Send");
    
    QString errorMsg = "Error: " + error;
    if (m_pendingMessage) {
        m_pendingMessage->setError(errorMsg);
        m_pendingMessage = nullptr;
    } else {
        addMessage("", false)->setError(errorMsg);
    }
}

void AiChatSidebar::onContextClicked() {
    QMenu menu(this);
    menu.addAction("Add File(s)...", [this]() {
        QString root = Core::ProjectManager::instance().projectRoot();
        QStringList files = QFileDialog::getOpenFileNames(this, "Add Context Files", root);
        for (const QString &file : files) {
            addAttachedFile(file);
        }
    });
    menu.addAction("Add Folder...", [this]() {
        QString root = Core::ProjectManager::instance().projectRoot();
        QString folder = QFileDialog::getExistingDirectory(this, "Add Context Folder", root);
        if (!folder.isEmpty()) {
            addAttachedFolder(folder);
        }
    });

    menu.addAction("Add Selection", [this]() {
        if (m_tabContainer && m_tabContainer->currentEditor()) {
            QString selection = m_tabContainer->currentEditor()->textCursor().selectedText();
            // QPlainTextEdit selectedText() uses U+2029 for paragraph separator
            selection.replace(QChar(0x2029), '\n');
            if (!selection.isEmpty()) {
                addAttachedSnippet(selection);
            }
        }
    });
    
    menu.exec(m_contextButton->mapToGlobal(QPoint(0, -menu.sizeHint().height())));
}

#include "ai_chat_sidebar.moc"
