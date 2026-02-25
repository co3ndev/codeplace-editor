#include "ai_chat_sidebar.h"
#include "ai_message_widget.h"
#include "../core/ai_client.h"
#include "../core/settings_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QKeyEvent>
#include <QApplication>

AiChatSidebar::AiChatSidebar(QWidget *parent) : QWidget(parent) {
    setupUi();
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
    auto *inputContainer = new QWidget(this);
    auto *inputLayout = new QVBoxLayout(inputContainer);
    inputLayout->setContentsMargins(10, 10, 10, 10);

    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setPlaceholderText("Type a message... (Shift+Enter for newline)");
    m_inputEdit->setMaximumHeight(100);
    m_inputEdit->installEventFilter(this);
    inputLayout->addWidget(m_inputEdit);

    auto *buttonLayout = new QHBoxLayout();
    m_contextButton = new QPushButton("Add Context", this);
    m_sendButton = new QPushButton("Send", this);
    m_sendButton->setDefault(true);
    
    buttonLayout->addWidget(m_contextButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_sendButton);
    inputLayout->addLayout(buttonLayout);

    mainLayout->addWidget(inputContainer);

    connect(m_sendButton, &QPushButton::clicked, this, &AiChatSidebar::onSendClicked);
    connect(m_contextButton, &QPushButton::clicked, this, &AiChatSidebar::onContextClicked);
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

void AiChatSidebar::addMessage(const QString &text, bool isUser) {
    auto *msg = new AiMessageWidget(this);
    msg->setMessage(text, isUser);
    
    // Insert before the stretch (which is at index count() - 1)
    m_historyLayout->insertWidget(m_historyLayout->count() - 1, msg);
    
    // Scroll to bottom after layout update
    QApplication::processEvents();
    m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->maximum());
}

void AiChatSidebar::onSendClicked() {
    QString text = m_inputEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;

    addMessage(text, true);
    m_inputEdit->clear();

    // Actual logic for sending to AI will be in Phase 5/6
}

void AiChatSidebar::onContextClicked() {
    // Logic for adding context will be in Phase 5
}
