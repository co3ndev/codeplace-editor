#ifndef SYMBOL_INFO_WIDGET_H
#define SYMBOL_INFO_WIDGET_H

#include <QWidget>
#include <QTextBrowser>
#include <QPushButton>
#include <QListWidget>
#include <QJsonArray>
#include <QJsonObject>

namespace Editor {

class SymbolInfoWidget : public QWidget {
    Q_OBJECT

public:
    explicit SymbolInfoWidget(QWidget *parent = nullptr);
    ~SymbolInfoWidget() override = default;

    void setContent(const QString &content);
    void showAt(const QPoint &pos);
    void setDiagnosticMode(bool isDiagnostic);
    
    void updateTheme();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QTextBrowser *m_browser;
    bool m_isDiagnostic = false;
};

} 

#endif 
