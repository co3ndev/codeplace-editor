#include "ai_message_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QClipboard>
#include <QApplication>
#include <QStyle>
#include <QEnterEvent>
#include <QPushButton>
#include "../core/theme_manager.h"

AiMessageWidget::AiMessageWidget(QWidget *parent) : QWidget(parent), m_isUser(false) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(2);

    auto *headerLayout = new QHBoxLayout();
    headerLayout->addStretch();
    
    m_copyButton = new QPushButton(this);
    m_copyButton->setFixedSize(50, 24);
    m_copyButton->setCursor(Qt::PointingHandCursor);
    m_copyButton->setToolTip("Copy message");
    m_copyButton->setFlat(true);
    m_copyButton->setText("Copy");
    
    m_copyButton->hide();
    
    headerLayout->addWidget(m_copyButton);
    layout->addLayout(headerLayout);

    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setReadOnly(true);
    m_textBrowser->setOpenExternalLinks(true);
    m_textBrowser->setFrameShape(QFrame::NoFrame);
    m_textBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    
    m_textBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    layout->addWidget(m_textBrowser);

    connect(m_copyButton, &QPushButton::clicked, this, &AiMessageWidget::onCopyClicked);
    connect(&Core::ThemeManager::instance(), &Core::ThemeManager::themeChanged, this, &AiMessageWidget::applyTheme);
    
    applyTheme();
}

void AiMessageWidget::setMessage(const QString &text, bool isUser) {
    m_isUser = isUser;
    m_rawText = text;
    m_textBrowser->setMarkdown(text);

    // Dynamic height adjustment
    m_textBrowser->document()->setDocumentMargin(0);
    int height = m_textBrowser->document()->size().height() + 10;
    m_textBrowser->setMinimumHeight(height);
    m_textBrowser->setMaximumHeight(height);

    updateStyleSheet();

    if (isUser) {
        layout()->setContentsMargins(40, 2, 5, 5);
    } else {
        layout()->setContentsMargins(5, 2, 40, 5);
    }
}

void AiMessageWidget::applyTheme() {
    updateStyleSheet();
    
    auto &tm = Core::ThemeManager::instance();
    QColor fg = tm.getColor(Core::ThemeManager::EditorForeground);
    QColor hoverBg = tm.getColor(Core::ThemeManager::LineHighlight);
    
    m_copyButton->setStyleSheet(
        QString("QPushButton {"
        "  border: none;"
        "  background: transparent;"
        "  color: %1;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %2;"
        "  border-radius: 4px;"
        "  color: %3;"
        "}").arg(fg.name(QColor::HexArgb), hoverBg.name(QColor::HexArgb), fg.name())
    );
}

void AiMessageWidget::updateStyleSheet() {
    auto &tm = Core::ThemeManager::instance();
    
    QColor bg;
    QColor fg;
    
    if (m_isUser) {
        bg = tm.getColor(Core::ThemeManager::SelectionBackground);
        fg = (bg.lightness() > 140) ? QColor("#000000") : QColor("#ffffff");
    } else {
        bg = tm.getColor(Core::ThemeManager::LineHighlight);
        fg = tm.getColor(Core::ThemeManager::EditorForeground);
    }

    m_textBrowser->setStyleSheet(
        QString("QTextBrowser {"
        "  background-color: %1;"
        "  color: %2;"
        "  border-radius: 8px;"
        "  padding: 8px;"
        "  font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;"
        "  font-size: 13px;"
        "}").arg(bg.name(QColor::HexArgb), fg.name(QColor::HexArgb))
    );
}

void AiMessageWidget::enterEvent(QEnterEvent *event) {
    if (!m_isUser && m_copyButton) {
        m_copyButton->show();
    }
    QWidget::enterEvent(event);
}

void AiMessageWidget::leaveEvent(QEvent *event) {
    if (m_copyButton) {
        m_copyButton->hide();
    }
    QWidget::leaveEvent(event);
}

void AiMessageWidget::onCopyClicked() {
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(m_rawText);
}
