#ifndef AI_CHAT_SIDEBAR_H
#define AI_CHAT_SIDEBAR_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QHBoxLayout>

namespace Editor { class TabContainer; }

class AiClient;
class AiMessageWidget;

struct AiChatMessage {
    QString role; // "user" or "assistant"
    QString content;
};

class AiChatSidebar : public QWidget {
    Q_OBJECT
public:
    explicit AiChatSidebar(QWidget *parent = nullptr);

    AiMessageWidget* addMessage(const QString &text, bool isUser);
    void applyTheme();
    void setTabContainer(Editor::TabContainer *container);

    void addAttachedFile(const QString &filePath);
    void addAttachedFolder(const QString &folderPath);
    void addAttachedSnippet(const QString &text);
    void removeAttachedFile(const QString &filePath);
    void removeAttachedFolder(const QString &folderPath);

    QString getFullContext() const;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onSendClicked();
    void onContextClicked();
    void onAiResponseReceived(const QString &message);
    void onAiErrorOccurred(const QString &error);
    void onNewChatClicked();

private:
    QScrollArea *m_scrollArea;
    QWidget *m_historyContainer;
    QVBoxLayout *m_historyLayout;
    QTextEdit *m_inputEdit;
    QPushButton *m_sendButton;
    QPushButton *m_contextButton;
    QPushButton *m_newChatButton;
    QWidget *m_inputContainer;
    QWidget *m_chipsContainer;
    QHBoxLayout *m_chipsLayout;
    QScrollArea *m_chipsScrollArea;

    Editor::TabContainer *m_tabContainer = nullptr;
    AiClient *m_aiClient;
    AiMessageWidget *m_pendingMessage = nullptr;
    QList<QString> m_attachedFiles;
    QList<QString> m_attachedSnippets;
    QList<AiChatMessage> m_chatHistory;

    void setupUi();
    void clearChips();
    void clearHistory();
};

#endif // AI_CHAT_SIDEBAR_H
