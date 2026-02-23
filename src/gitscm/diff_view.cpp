#include "diff_view.h"
#include <QVBoxLayout>
#include <QFileInfo>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include "core/theme_manager.h"

namespace GitScm {

DiffView::DiffView(QWidget *parent) : QWidget(parent) {
    setupUI();
    connect(&Core::ThemeManager::instance(), &Core::ThemeManager::themeChanged, this, &DiffView::onThemeChanged);
}

void DiffView::onThemeChanged() {
    auto &tm = Core::ThemeManager::instance();
    QPalette p = palette();
    p.setColor(QPalette::Base, tm.getColor(Core::ThemeManager::EditorBackground));
    p.setColor(QPalette::Text, tm.getColor(Core::ThemeManager::EditorForeground));
    
    leftEditor->setPalette(p);
    rightEditor->setPalette(p);
    
    applyChangeHighlights(m_oldContent, m_newContent);
}

void DiffView::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto *editorsLayout = new QHBoxLayout();
    editorsLayout->setContentsMargins(0, 0, 0, 0);
    editorsLayout->setSpacing(2);

    leftEditor = new QPlainTextEdit(this);
    rightEditor = new QPlainTextEdit(this);

    leftEditor->setReadOnly(true);
    rightEditor->setReadOnly(true);

    QFont font("Cascadia Code", 10);
    if (QFontInfo(font).family() != "Cascadia Code") {
        font.setFamily("Monospace");
    }
    leftEditor->setFont(font);
    rightEditor->setFont(font);

    onThemeChanged();

    connect(leftEditor->verticalScrollBar(), &QScrollBar::valueChanged, this, &DiffView::syncScrollRight);
    connect(rightEditor->verticalScrollBar(), &QScrollBar::valueChanged, this, &DiffView::syncScrollLeft);

    editorsLayout->addWidget(leftEditor);
    editorsLayout->addWidget(rightEditor);

    mainLayout->addLayout(editorsLayout);
}

void DiffView::setDiff(const QString &oldContent, const QString &newContent, const QString &fileName) {
    m_fileName = fileName;
    m_oldContent = oldContent;
    m_newContent = newContent;
    
    leftEditor->setPlainText(oldContent);
    rightEditor->setPlainText(newContent);
    
    QFileInfo fi(fileName);
    QString ext = fi.suffix().toLower();
    auto lang = Core::SimpleHighlighter::languageForExtension(ext);
    
    if (lang.has_value()) {
        if (leftHighlighter) delete leftHighlighter;
        if (rightHighlighter) delete rightHighlighter;
        
        leftHighlighter = new Core::SimpleHighlighter(leftEditor->document(), lang.value());
        rightHighlighter = new Core::SimpleHighlighter(rightEditor->document(), lang.value());
        
        leftHighlighter->rehighlight();
        rightHighlighter->rehighlight();
    }
    
    applyChangeHighlights(oldContent, newContent);
}

void DiffView::applyChangeHighlights(const QString &oldContent, const QString &newContent) {
    QStringList oldLines = oldContent.split('\n');
    QStringList newLines = newContent.split('\n');
    
    QList<DiffEdit> edits = computeMyersDiff(oldLines, newLines);
    
    QList<QTextEdit::ExtraSelection> leftSelections;
    QList<QTextEdit::ExtraSelection> rightSelections;
    
    QColor addedBg = Core::ThemeManager::instance().getColor(Core::ThemeManager::DiffAddedBackground);
    QColor removedBg = Core::ThemeManager::instance().getColor(Core::ThemeManager::DiffRemovedBackground);
    
    for (const auto &edit : edits) {
        if (edit.type == DiffEdit::Insert) {
            QTextEdit::ExtraSelection sel;
            sel.format.setBackground(addedBg);
            sel.format.setProperty(QTextFormat::FullWidthSelection, true);
            sel.cursor = QTextCursor(rightEditor->document()->findBlockByNumber(edit.newLine));
            sel.cursor.select(QTextCursor::BlockUnderCursor);
            rightSelections.append(sel);
        } else if (edit.type == DiffEdit::Delete) {
            QTextEdit::ExtraSelection sel;
            sel.format.setBackground(removedBg);
            sel.format.setProperty(QTextFormat::FullWidthSelection, true);
            sel.cursor = QTextCursor(leftEditor->document()->findBlockByNumber(edit.oldLine));
            sel.cursor.select(QTextCursor::BlockUnderCursor);
            leftSelections.append(sel);
        } else if (edit.type == DiffEdit::Keep) {
        }
    }
    
    leftEditor->setExtraSelections(leftSelections);
    rightEditor->setExtraSelections(rightSelections);
}

void DiffView::syncScrollLeft(int value) {
    if (leftEditor->verticalScrollBar()->value() != value) {
        leftEditor->verticalScrollBar()->setValue(value);
    }
}

void DiffView::syncScrollRight(int value) {
    if (rightEditor->verticalScrollBar()->value() != value) {
        rightEditor->verticalScrollBar()->setValue(value);
    }
}

}
