#include "symbol_info_widget.h"
#include "core/theme_manager.h"
#include <QVBoxLayout>
#include <QApplication>
#include <QScreen>

namespace Editor {

SymbolInfoWidget::SymbolInfoWidget(QWidget *parent) : QWidget(parent, Qt::ToolTip | Qt::FramelessWindowHint) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1);
    
    m_browser = new QTextBrowser(this);
    m_browser->setReadOnly(true);
    m_browser->setOpenExternalLinks(true);
    m_browser->setFrameStyle(QFrame::NoFrame);
    m_browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_browser->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    layout->addWidget(m_browser);
	
    updateTheme();
    
    qApp->installEventFilter(this);
}

void SymbolInfoWidget::setContent(const QString &content) {
    m_browser->setMarkdown(content);
    
    m_browser->document()->setTextWidth(500);
    QSize baseSize = m_browser->document()->size().toSize();
    baseSize.setWidth(500);
    baseSize.setHeight(qMin(baseSize.height() + 20, 450));
    setFixedSize(baseSize);
}

void SymbolInfoWidget::showAt(const QPoint &pos) {
    move(pos);
    show();
}

void SymbolInfoWidget::setDiagnosticMode(bool isDiagnostic) {
    m_isDiagnostic = isDiagnostic;
    updateTheme();
}

void SymbolInfoWidget::updateTheme() {
    auto &tm = Core::ThemeManager::instance();
    QColor bg = m_isDiagnostic ? tm.getColor(Core::ThemeManager::DiagnosticHoverBackground) 
                               : tm.getColor(Core::ThemeManager::EditorBackground);
    QColor fg = m_isDiagnostic ? tm.getColor(Core::ThemeManager::DiagnosticHoverForeground) 
                               : tm.getColor(Core::ThemeManager::EditorForeground);
    QColor border = bg.lighter(130);
    
    QPalette p = m_browser->palette();
    p.setColor(QPalette::Base, bg);
    p.setColor(QPalette::Text, fg);
    m_browser->setPalette(p);
    
    setStyleSheet(QString(
        "QTextBrowser {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-bottom: none;"
        "  padding: 8px;"
      "  selection-background-color: %4;\n"
      "  selection-color: %2;\n"
      "}\n"
    ).arg(bg.name())
     .arg(fg.name())
     .arg(border.name())
     .arg(tm.getColor(Core::ThemeManager::LineHighlight).name()));
}

bool SymbolInfoWidget::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::KeyPress) {
        if (!rect().contains(mapFromGlobal(QCursor::pos())) && isVisible()) {
            hide();
        }
    }
    return QWidget::eventFilter(obj, event);
}

void SymbolInfoWidget::leaveEvent(QEvent *event) {
    hide();
    QWidget::leaveEvent(event);
}

} 
