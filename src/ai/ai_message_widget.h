#ifndef AI_MESSAGE_WIDGET_H
#define AI_MESSAGE_WIDGET_H

#include <QWidget>
#include <QTextBrowser>
#include <QPushButton>

class QLabel;
class QHBoxLayout;

class AiMessageWidget : public QWidget {
    Q_OBJECT
public:
    explicit AiMessageWidget(QWidget *parent = nullptr);

    void setMessage(const QString &text, bool isUser);
    void applyTheme();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateStyleSheet();
    void adjustHeight();

private:
    QTextBrowser *m_textBrowser;
    QHBoxLayout *m_headerLayout;
    QLabel *m_nameLabel;
    QString m_rawText;
    bool m_isUser;
    
    void setupUi();
};

#endif // AI_MESSAGE_WIDGET_H
