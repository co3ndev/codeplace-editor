#ifndef AI_CHAT_SIDEBAR_H
#define AI_CHAT_SIDEBAR_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QHBoxLayout>

namespace Editor { class TabContainer; }

class AiChatSidebar : public QWidget {
    Q_OBJECT
public:
    explicit AiChatSidebar(QWidget *parent = nullptr);

    void addMessage(const QString &text, bool isUser);
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

private:
    QScrollArea *m_scrollArea;
    QWidget *m_historyContainer;
    QVBoxLayout *m_historyLayout;
    QTextEdit *m_inputEdit;
    QPushButton *m_sendButton;
    QPushButton *m_contextButton;
    QWidget *m_inputContainer;
    QWidget *m_chipsContainer;
    QHBoxLayout *m_chipsLayout;
    QScrollArea *m_chipsScrollArea;

    Editor::TabContainer *m_tabContainer = nullptr;
    QList<QString> m_attachedFiles;
    QList<QString> m_attachedSnippets;

    void setupUi();
    void clearChips();
};

#endif // AI_CHAT_SIDEBAR_H
