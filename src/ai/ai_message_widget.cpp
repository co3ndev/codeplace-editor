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
#include <QRegularExpression>
#include <QFontDatabase>
#include "../core/theme_manager.h"

class CodeBlockWidget : public QWidget {
    Q_OBJECT
public:
    CodeBlockWidget(const QString &code, const QString &lang, QWidget *parent = nullptr)
        : QWidget(parent), m_code(code), m_lang(lang), m_isFolded(true) {
        auto *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 5, 0, 5);
        mainLayout->setSpacing(0);

        m_headerGroup = new QWidget(this);
        m_headerGroup->setObjectName("CodeHeader");
        auto *headerLayout = new QHBoxLayout(m_headerGroup);
        headerLayout->setContentsMargins(10, 4, 10, 4);

        m_toggleButton = new QPushButton(m_isFolded ? "▶" : "▼", this);
        m_toggleButton->setFixedSize(20, 20);
        m_toggleButton->setFlat(true);
        m_toggleButton->setCursor(Qt::PointingHandCursor);

        m_langLabel = new QLabel(lang.isEmpty() ? "CODE" : lang.toUpper(), this);
        m_langLabel->setStyleSheet("color: gray; font-size: 10px; font-weight: bold;");

        headerLayout->addWidget(m_toggleButton);
        headerLayout->addWidget(m_langLabel);
        headerLayout->addStretch();

        m_codeBrowser = new QTextBrowser(this);
        m_codeBrowser->setReadOnly(true);
        m_codeBrowser->setFrameShape(QFrame::NoFrame);
        m_codeBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_codeBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_codeBrowser->setMarkdown(QString("```%1\n%2\n```").arg(lang, code));
        m_codeBrowser->setVisible(!m_isFolded);
        
        mainLayout->addWidget(m_headerGroup);
        mainLayout->addWidget(m_codeBrowser);

        connect(m_toggleButton, &QPushButton::clicked, this, &CodeBlockWidget::toggleFold);
        applyTheme();
    }

    void toggleFold() {
        m_isFolded = !m_isFolded;
        m_toggleButton->setText(m_isFolded ? "▶" : "▼");
        m_codeBrowser->setVisible(!m_isFolded);
        emit sizeChanged();
    }

    void applyTheme() {
        auto &tm = Core::ThemeManager::instance();
        QColor bg = tm.getColor(Core::ThemeManager::LineHighlight).darker(120);
        m_headerGroup->setStyleSheet(QString("#CodeHeader { background-color: %1; border-top-left-radius: 8px; border-top-right-radius: 8px; }").arg(bg.name()));
        m_codeBrowser->setStyleSheet(QString("QTextBrowser { background-color: %1; border-bottom-left-radius: 8px; border-bottom-right-radius: 8px; }").arg(bg.name()));
        m_toggleButton->setStyleSheet("border: none; color: gray; font-weight: bold;");
        
        QString docCss = QString(
            "body { color: %1; font-family: 'JetBrains Mono', 'Fira Code', monospace; font-size: 12px; }"
            "pre { margin: 0; padding: 10px; }"
        ).arg(tm.getColor(Core::ThemeManager::EditorForeground).name());
        m_codeBrowser->document()->setDefaultStyleSheet(docCss);
    }

    int calculateHeight(int width) {
        if (m_isFolded) {
            return m_headerGroup->sizeHint().height() + 10;
        }
        m_codeBrowser->document()->setTextWidth(width - 20);
        return m_headerGroup->sizeHint().height() + m_codeBrowser->document()->size().height() + 15;
    }

signals:
    void sizeChanged();

private:
    QString m_code;
    QString m_lang;
    bool m_isFolded;
    QPushButton *m_toggleButton;
    QLabel *m_langLabel;
    QTextBrowser *m_codeBrowser;
    QWidget *m_headerGroup;
};

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

    m_contentLayout = new QVBoxLayout();
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(0);
    mainLayout->addLayout(m_contentLayout);
}

void AiMessageWidget::setMessage(const QString &text, bool isUser) {
    setLoading(false);
    m_isError = false;
    m_isUser = isUser;
    m_rawText = text;
    
    m_nameLabel->setText(isUser ? "You" : "AI");
    
    clearContentLayout();
    
    // Regex to split by code blocks: ```[lang]\n<code>\n```
    static QRegularExpression re("```([a-zA-Z0-9+#]*)?\\n([\\s\\S]*?)\\n```");
    
    int lastPos = 0;
    auto matches = re.globalMatch(text);
    while (matches.hasNext()) {
        auto match = matches.next();
        QString textPart = text.mid(lastPos, match.capturedStart() - lastPos);
        addTextPart(textPart);
        
        QString lang = match.captured(1);
        QString code = match.captured(2);
        addCodePart(code, lang);
        
        lastPos = match.capturedEnd();
    }
    
    QString remainingText = text.mid(lastPos);
    addTextPart(remainingText);
    
    updateStyleSheet();
    
    QTimer::singleShot(50, this, &AiMessageWidget::adjustHeight);
    
    Qt::Alignment alignment = isUser ? Qt::AlignRight : Qt::AlignLeft;
    m_headerLayout->setDirection(isUser ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight);
    
    for (int i = 0; i < m_contentLayout->count(); ++i) {
        if (auto *widget = m_contentLayout->itemAt(i)->widget()) {
            m_contentLayout->setAlignment(widget, alignment);
        }
    }
    
    if (isUser) {
        layout()->setContentsMargins(50, 5, 10, 10);
    } else {
        layout()->setContentsMargins(10, 5, 50, 10);
    }
}

void AiMessageWidget::setError(const QString &text) {
    setLoading(false);
    m_isError = true;
    m_isUser = false;
    m_rawText = text;
    
    m_nameLabel->setText("Error");
    
    clearContentLayout();
    addTextPart(text);
    
    updateStyleSheet();
    QTimer::singleShot(50, this, &AiMessageWidget::adjustHeight);
    
    m_headerLayout->setDirection(QBoxLayout::LeftToRight);
    for (int i = 0; i < m_contentLayout->count(); ++i) {
        if (auto *widget = m_contentLayout->itemAt(i)->widget()) {
            m_contentLayout->setAlignment(widget, Qt::AlignLeft);
        }
    }
    layout()->setContentsMargins(10, 5, 50, 10);
}

void AiMessageWidget::setLoading(bool loading) {
    if (m_isLoading == loading) return;
    m_isLoading = loading;
    if (loading) {
        m_isError = false;
        m_isUser = false;
        m_nameLabel->setText("AI");
        m_headerLayout->setDirection(QBoxLayout::LeftToRight);
        
        clearContentLayout();
        addTextPart("..."); // Placeholder
        
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
    if (!m_isLoading || m_contentLayout->count() == 0) return;
    auto *browser = qobject_cast<QTextBrowser*>(m_contentLayout->itemAt(0)->widget());
    if (!browser) return;

    static const QStringList spinnerFrames = {
        "⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"
    };
    
    QString frame = spinnerFrames[m_spinnerStep % spinnerFrames.size()];
    m_spinnerStep++;
    
    browser->setHtml(QString("<div style='color: gray; font-style: italic;'><span style='font-family: monospace;'>%1</span> Generating response...</div>").arg(frame));
    adjustHeight();
}

void AiMessageWidget::adjustHeight() {
    int totalHeight = m_headerLayout->sizeHint().height() + layout()->spacing() * 2;
    int layoutMargins = layout()->contentsMargins().left() + layout()->contentsMargins().right();
    int maxW = width() - layoutMargins - 10;
    if (maxW < 100) maxW = 300;

    int maxWidthReached = 0;

    for (int i = 0; i < m_contentLayout->count(); ++i) {
        auto *widget = m_contentLayout->itemAt(i)->widget();
        if (!widget) continue;

        if (auto *browser = qobject_cast<QTextBrowser*>(widget)) {
            browser->document()->setTextWidth(-1);
            int idealW = static_cast<int>(browser->document()->idealWidth()) + 25;
            int finalW = qMin(idealW, maxW);
            browser->document()->setTextWidth(finalW);
            
            int h = static_cast<int>(browser->document()->size().height()) + 5;
            browser->setFixedSize(finalW, h);
            totalHeight += h;
            maxWidthReached = qMax(maxWidthReached, finalW);
        } else if (auto *codeBlock = qobject_cast<CodeBlockWidget*>(widget)) {
            int h = codeBlock->calculateHeight(maxW);
            codeBlock->setFixedWidth(maxW);
            totalHeight += h;
            maxWidthReached = qMax(maxWidthReached, maxW);
        }
    }

    totalHeight += layout()->contentsMargins().top() + layout()->contentsMargins().bottom();
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
    
    for (int i = 0; i < m_contentLayout->count(); ++i) {
        auto *widget = m_contentLayout->itemAt(i)->widget();
        if (auto *codeBlock = qobject_cast<CodeBlockWidget*>(widget)) {
            codeBlock->applyTheme();
        }
    }
}

void AiMessageWidget::updateStyleSheet() {
    auto &tm = Core::ThemeManager::instance();
    QColor bg;
    QColor fg;
    
    if (m_isUser) {
        bg = tm.getColor(Core::ThemeManager::SelectionBackground);
        fg = (bg.lightness() > 140) ? QColor("#000000") : QColor("#ffffff");
    } else if (m_isError) {
        bg = QColor(255, 0, 0, 40);
        fg = QColor("#ff4c4c");
    } else {
        bg = tm.getColor(Core::ThemeManager::LineHighlight);
        fg = tm.getColor(Core::ThemeManager::EditorForeground);
    }

    QString docCss = QString(
        "body { color: %1; font-family: 'Inter', 'Segoe UI', sans-serif; font-size: 13px; }"
        "h1, h2, h3, h4 { color: %1; margin-top: 12px; margin-bottom: 8px; font-weight: bold; }"
        "h1 { font-size: 18px; border-bottom: 1px solid rgba(128,128,128,0.2); }"
        "h2 { font-size: 16px; }"
        "h3 { font-size: 14px; }"
        "code { font-family: 'JetBrains Mono', 'Fira Code', monospace; background-color: rgba(128,128,128,0.15); padding: 2px; border-radius: 3px; }"
        "pre { font-family: 'JetBrains Mono', 'Fira Code', monospace; background-color: rgba(0,0,0,0.2); padding: 10px; border-radius: 6px; margin-top: 8px; margin-bottom: 8px; }"
        "a { color: #3584e4; text-decoration: none; }"
    ).arg(fg.name());

    QString borderCss = m_isError ? "1px solid #ff4c4c" : QString("1px solid %1").arg(bg.darker(110).name());
    QString browserStyle = QString("QTextBrowser { background-color: %1; color: %2; border-radius: 12px; border: %3; }")
        .arg(bg.name(QColor::HexArgb), fg.name(QColor::HexArgb), borderCss);

    for (int i = 0; i < m_contentLayout->count(); ++i) {
        if (auto *browser = qobject_cast<QTextBrowser*>(m_contentLayout->itemAt(i)->widget())) {
            browser->document()->setDefaultStyleSheet(docCss);
            browser->setStyleSheet(browserStyle);
        }
    }
}

void AiMessageWidget::clearContentLayout() {
    while (m_contentLayout->count() > 0) {
        QLayoutItem *item = m_contentLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void AiMessageWidget::addTextPart(const QString &text) {
    if (text.trimmed().isEmpty()) return;
    auto *browser = createTextBrowser();
    browser->setMarkdown(text);
    m_contentLayout->addWidget(browser);
}

void AiMessageWidget::addCodePart(const QString &code, const QString &lang) {
    auto *codeBlock = new CodeBlockWidget(code, lang, this);
    connect(codeBlock, &CodeBlockWidget::sizeChanged, this, &AiMessageWidget::adjustHeight);
    m_contentLayout->addWidget(codeBlock);
}

QTextBrowser* AiMessageWidget::createTextBrowser() {
    auto *browser = new QTextBrowser(this);
    browser->setReadOnly(true);
    browser->setOpenExternalLinks(true);
    browser->setFrameShape(QFrame::NoFrame);
    browser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    browser->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    browser->document()->setDocumentMargin(10);
    return browser;
}

#include "ai_message_widget.moc"
