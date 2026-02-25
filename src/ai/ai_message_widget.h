#ifndef AI_MESSAGE_WIDGET_H
#define AI_MESSAGE_WIDGET_H

#include <QWidget>
#include <QTextBrowser>
#include <QPushButton>

class AiMessageWidget : public QWidget {
    Q_OBJECT
public:
    explicit AiMessageWidget(QWidget *parent = nullptr);

    void setMessage(const QString &text, bool isUser);
    void applyTheme();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private slots:
    void onCopyClicked();

    void updateStyleSheet();

private:
    QTextBrowser *m_textBrowser;
    QPushButton *m_copyButton;
    QString m_rawText;
    bool m_isUser;
};

#endif // AI_MESSAGE_WIDGET_H
