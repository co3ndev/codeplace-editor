#ifndef AI_CHAT_SIDEBAR_H
#define AI_CHAT_SIDEBAR_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>

class AiChatSidebar : public QWidget {
    Q_OBJECT
public:
    explicit AiChatSidebar(QWidget *parent = nullptr);

    void addMessage(const QString &text, bool isUser);

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

    void setupUi();
};

#endif // AI_CHAT_SIDEBAR_H
