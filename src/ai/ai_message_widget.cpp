#include "ai_message_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QClipboard>
#include <QApplication>
#include <QStyle>
#include <QEnterEvent>
#include <QPushButton>
#include <QTimer>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextDocument>
#include "../core/theme_manager.h"

AiMessageWidget::AiMessageWidget(QWidget *parent) 
    : QWidget(parent)
    , m_isUser(false)
    , m_isLoading(false)
    , m_isError(false)
    , m_spinnerStep(0)
{
    m_spinnerTimer = new QTimer(this);
    connect(m_spinnerTimer, &QTimer::timeout, this, &AiMessageWidget::updateSpinner);

    setupUi();
    connect(&Core::ThemeManager::instance(), &Core::ThemeManager::themeChanged, this, &AiMessageWidget::applyTheme);
    applyTheme();
}

void AiMessageWidget::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 5, 10, 5);
    mainLayout->setSpacing(4);

    m_headerLayout = new QHBoxLayout();
    m_headerLayout->setContentsMargins(5, 0, 5, 0);
    
    m_nameLabel = new QLabel(this);
    m_nameLabel->setStyleSheet("font-weight: bold; font-size: 11px; color: gray;");
    
    m_headerLayout->addWidget(m_nameLabel);
    m_headerLayout->addStretch();
    mainLayout->addLayout(m_headerLayout);

    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setReadOnly(true);
    m_textBrowser->setOpenExternalLinks(true);
    m_textBrowser->setFrameShape(QFrame::NoFrame);
    m_textBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textBrowser->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_textBrowser->document()->setDocumentMargin(10);
    
    mainLayout->addWidget(m_textBrowser);
}

void AiMessageWidget::setMessage(const QString &text, bool isUser) {
    setLoading(false);
    m_isError = false;
    m_isUser = isUser;
    m_rawText = text;
    
    m_nameLabel->setText(isUser ? "You" : "AI");
    
    // Set markdown with some base styling for code blocks
    updateStyleSheet();
    
    m_textBrowser->setMarkdown(text);
    
    // Initial height adjustment after layout
    QTimer::singleShot(50, this, &AiMessageWidget::adjustHeight);
    
    if (isUser) {
        m_headerLayout->setDirection(QBoxLayout::RightToLeft);
        if (auto *vbox = qobject_cast<QVBoxLayout*>(layout())) {
            vbox->setAlignment(m_textBrowser, Qt::AlignRight);
        }
        layout()->setContentsMargins(50, 5, 10, 10);
    } else {
        m_headerLayout->setDirection(QBoxLayout::LeftToRight);
        if (auto *vbox = qobject_cast<QVBoxLayout*>(layout())) {
            vbox->setAlignment(m_textBrowser, Qt::AlignLeft);
        }
        layout()->setContentsMargins(10, 5, 50, 10);
    }
}

void AiMessageWidget::setError(const QString &text) {
    setLoading(false);
    m_isError = true;
    m_isUser = false;
    m_rawText = text;
    
    m_nameLabel->setText("Error");
    
    updateStyleSheet();
    m_textBrowser->setMarkdown(text);
    
    QTimer::singleShot(50, this, &AiMessageWidget::adjustHeight);
    
    m_headerLayout->setDirection(QBoxLayout::LeftToRight);
    if (auto *vbox = qobject_cast<QVBoxLayout*>(layout())) {
        vbox->setAlignment(m_textBrowser, Qt::AlignLeft);
    }
    layout()->setContentsMargins(10, 5, 50, 10);
}

void AiMessageWidget::setLoading(bool loading) {
    m_isLoading = loading;
    if (loading) {
        m_isError = false;
        m_isUser = false;
        m_nameLabel->setText("AI");
        m_headerLayout->setDirection(QBoxLayout::LeftToRight);
        if (auto *vbox = qobject_cast<QVBoxLayout*>(layout())) {
            vbox->setAlignment(m_textBrowser, Qt::AlignLeft);
        }
        layout()->setContentsMargins(10, 5, 50, 10);
        
        updateStyleSheet();
        m_spinnerStep = 0;
        updateSpinner();
        m_spinnerTimer->start(100);
    } else {
        m_spinnerTimer->stop();
    }
}

void AiMessageWidget::updateSpinner() {
    if (!m_isLoading) return;

    static const QStringList spinnerFrames = {
        "⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"
    };
    
    QString frame = spinnerFrames[m_spinnerStep % spinnerFrames.size()];
    m_spinnerStep++;
    
    m_textBrowser->setHtml(QString("<div style='color: gray; font-style: italic;'><span style='font-family: monospace;'>%1</span> Generating response...</div>").arg(frame));
    adjustHeight();
}

void AiMessageWidget::adjustHeight() {
    // Get available width for text
    int maxW = m_textBrowser->parentWidget()->width() - layout()->contentsMargins().left() - layout()->contentsMargins().right() - 10;
    if (maxW < 100) {
        maxW = this->width() - layout()->contentsMargins().left() - layout()->contentsMargins().right() - 22;
    }
    if (maxW < 100) maxW = 250;

    // First, find the ideal width for the text
    m_textBrowser->document()->setTextWidth(-1); // Allow document to find its ideal width
    int idealW = static_cast<int>(m_textBrowser->document()->idealWidth()) + 25; // + padding
    
    int finalW = qMin(idealW, maxW);

    // Force a layout update with the final width
    m_textBrowser->document()->setTextWidth(finalW);
    
    // Calculate required height
    int height = static_cast<int>(m_textBrowser->document()->size().height()) + 5;
    
    m_textBrowser->setFixedSize(finalW, height);
    
    // Calculate total height including name label and margins
    int totalHeight = height + m_nameLabel->height() + 
                      layout()->contentsMargins().top() + 
                      layout()->contentsMargins().bottom() + 
                      layout()->spacing();
                      
    this->setFixedHeight(totalHeight);
    updateGeometry();
}

void AiMessageWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    adjustHeight();
}

void AiMessageWidget::applyTheme() {
    auto &tm = Core::ThemeManager::instance();
    QColor fg = tm.getColor(Core::ThemeManager::EditorForeground);
    
    m_nameLabel->setStyleSheet(QString("font-weight: bold; font-size: 11px; color: %1;").arg(fg.name()));
    
    updateStyleSheet();
    
    // If we have text, we need to refresh the document's internal stylesheet
    if (!m_rawText.isEmpty()) {
        m_textBrowser->setMarkdown(m_rawText);
        adjustHeight();
    }
}

void AiMessageWidget::updateStyleSheet() {
    auto &tm = Core::ThemeManager::instance();
    
    QColor bg;
    QColor fg;
    
    if (m_isUser) {
        bg = tm.getColor(Core::ThemeManager::SelectionBackground);
        // Ensure contrast for text on selection background
        fg = (bg.lightness() > 140) ? QColor("#000000") : QColor("#ffffff");
    } else if (m_isError) {
        bg = QColor(255, 0, 0, 40); // Subtle red
        fg = QColor("#ff4c4c"); // Brighter red for text
    } else {
        bg = tm.getColor(Core::ThemeManager::LineHighlight);
        fg = tm.getColor(Core::ThemeManager::EditorForeground);
    }

    // Markdown styling via document's default stylesheet
    QString docCss = QString(
        "body { color: %1; font-family: 'Inter', 'Segoe UI', sans-serif; font-size: 13px; }"
        "code { font-family: 'JetBrains Mono', 'Fira Code', monospace; background-color: rgba(0,0,0,0.2); padding: 2px; border-radius: 3px; }"
        "pre { font-family: 'JetBrains Mono', 'Fira Code', monospace; background-color: rgba(0,0,0,0.3); padding: 8px; border-radius: 5px; margin-top: 5px; margin-bottom: 5px; }"
        "a { color: #3584e4; text-decoration: none; }"
    ).arg(fg.name());
    
    m_textBrowser->document()->setDefaultStyleSheet(docCss);

    QString borderCss;
    if (m_isError) {
        borderCss = QString("1px solid #ff4c4c");
    } else {
        borderCss = QString("1px solid %1").arg(bg.darker(110).name());
    }

    m_textBrowser->setStyleSheet(
        QString("QTextBrowser {"
        "  background-color: %1;"
        "  color: %2;"
        "  border-radius: 12px;"
        "  border: %3;"
        "}").arg(bg.name(QColor::HexArgb), fg.name(QColor::HexArgb), borderCss)
    );
}
