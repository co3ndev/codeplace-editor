#include "find_replace_bar.h"
#include "editor_view.h"
#include "core/theme_manager.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextEdit>

namespace Editor {

FindReplaceBar::FindReplaceBar(QWidget *parent) : QWidget(parent) {
    setVisible(false);
    setFixedHeight(0); 

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 6, 8, 6);
    mainLayout->setSpacing(4);

    
    findRow = new QWidget(this);
    auto *findLayout = new QHBoxLayout(findRow);
    findLayout->setContentsMargins(0, 0, 0, 0);
    findLayout->setSpacing(4);

    searchField = new QLineEdit(findRow);
    searchField->setPlaceholderText("Find...");
    searchField->setMinimumWidth(200);
    searchField->setClearButtonEnabled(true);
    searchField->installEventFilter(this);

    matchCountLabel = new QLabel("No results", findRow);
    matchCountLabel->setMinimumWidth(70);
    matchCountLabel->setAlignment(Qt::AlignCenter);

    caseSensitiveButton = new QToolButton(findRow);
    caseSensitiveButton->setText("Aa");
    caseSensitiveButton->setCheckable(true);
    caseSensitiveButton->setToolTip("Match Case");
    caseSensitiveButton->setFixedSize(24, 24);

    prevButton = new QToolButton(findRow);
    prevButton->setText("▲");
    prevButton->setToolTip("Previous Match (Shift+Enter)");
    prevButton->setFixedSize(24, 24);

    nextButton = new QToolButton(findRow);
    nextButton->setText("▼");
    nextButton->setToolTip("Next Match (Enter)");
    nextButton->setFixedSize(24, 24);

    closeButton = new QToolButton(findRow);
    closeButton->setText("✕");
    closeButton->setToolTip("Close (Escape)");
    closeButton->setFixedSize(24, 24);

    findLayout->addWidget(searchField, 1);
    findLayout->addWidget(matchCountLabel);
    findLayout->addWidget(caseSensitiveButton);
    findLayout->addWidget(prevButton);
    findLayout->addWidget(nextButton);
    findLayout->addWidget(closeButton);

    
    replaceRow = new QWidget(this);
    auto *replaceLayout = new QHBoxLayout(replaceRow);
    replaceLayout->setContentsMargins(0, 0, 0, 0);
    replaceLayout->setSpacing(4);

    replaceField = new QLineEdit(replaceRow);
    replaceField->setPlaceholderText("Replace with...");
    replaceField->setMinimumWidth(200);
    replaceField->setClearButtonEnabled(true);
    replaceField->installEventFilter(this);

    replaceButton = new QPushButton("Replace", replaceRow);
    replaceButton->setFixedHeight(24);

    replaceAllButton = new QPushButton("Replace All", replaceRow);
    replaceAllButton->setFixedHeight(24);

    replaceLayout->addWidget(replaceField, 1);
    replaceLayout->addWidget(replaceButton);
    replaceLayout->addWidget(replaceAllButton);
    replaceLayout->addStretch();

    mainLayout->addWidget(findRow);
    mainLayout->addWidget(replaceRow);

    
    connect(searchField, &QLineEdit::textChanged, this, &FindReplaceBar::onSearchTextChanged);
    connect(nextButton, &QToolButton::clicked, this, &FindReplaceBar::performFindNext);
    connect(prevButton, &QToolButton::clicked, this, &FindReplaceBar::performFindPrevious);
    connect(closeButton, &QToolButton::clicked, this, &FindReplaceBar::hideBar);
    connect(caseSensitiveButton, &QToolButton::toggled, this, &FindReplaceBar::onCaseSensitivityToggled);
    connect(replaceButton, &QPushButton::clicked, this, &FindReplaceBar::onReplace);
    connect(replaceAllButton, &QPushButton::clicked, this, &FindReplaceBar::onReplaceAll);

    connect(&Core::ThemeManager::instance(), &Core::ThemeManager::themeChanged,
            this, &FindReplaceBar::onThemeChanged);

    applyThemeColors();
}

bool FindReplaceBar::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Escape) {
            hideBar();
            return true;
        }

        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                performFindPrevious();
            } else {
                performFindNext();
            }
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void FindReplaceBar::showFind(EditorView *editor) {
    activeEditor = editor;
    replaceRow->setVisible(false);
    setFixedHeight(40);
    setVisible(true);

    
    if (editor) {
        QTextCursor cursor = editor->textCursor();
        if (cursor.hasSelection()) {
            searchField->setText(cursor.selectedText());
        }
    }

    searchField->setFocus();
    searchField->selectAll();

    
    if (!searchField->text().isEmpty()) {
        onSearchTextChanged(searchField->text());
    }

    emit visibilityChanged(true);
}

void FindReplaceBar::showReplace(EditorView *editor) {
    activeEditor = editor;
    replaceRow->setVisible(true);
    setFixedHeight(70);
    setVisible(true);

    
    if (editor) {
        QTextCursor cursor = editor->textCursor();
        if (cursor.hasSelection()) {
            searchField->setText(cursor.selectedText());
        }
    }

    searchField->setFocus();
    searchField->selectAll();

    
    if (!searchField->text().isEmpty()) {
        onSearchTextChanged(searchField->text());
    }

    emit visibilityChanged(true);
}

void FindReplaceBar::hideBar() {
    setVisible(false);
    setFixedHeight(0);
    clearHighlights();

    if (activeEditor) {
        activeEditor->setFocus();
    }
    activeEditor = nullptr;

    emit visibilityChanged(false);
}

void FindReplaceBar::onSearchTextChanged(const QString &text) {
    lastSearchText = text;

    if (text.isEmpty() || !activeEditor) {
        matchCountLabel->setText("No results");
        currentMatchIndex = 0;
        totalMatches = 0;
        clearHighlights();
        return;
    }

    highlightAllMatches();
    updateMatchCount();

    
    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;

    QTextCursor cursor = activeEditor->textCursor();
    cursor.movePosition(QTextCursor::Start);
    activeEditor->setTextCursor(cursor);

    QTextCursor found = activeEditor->document()->find(text, cursor, flags);
    if (!found.isNull()) {
        activeEditor->setTextCursor(found);
        activeEditor->ensureCursorVisible();
        
        currentMatchIndex = 1;
        QTextCursor counter = activeEditor->document()->find(text, 0, flags);
        while (!counter.isNull() && counter.selectionEnd() <= found.selectionStart()) {
            currentMatchIndex++;
            counter = activeEditor->document()->find(text, counter, flags);
        }
        updateMatchCount();
    }
}

void FindReplaceBar::performFindNext() {
    if (lastSearchText.isEmpty() || !activeEditor) return;

    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;

    QTextCursor cursor = activeEditor->textCursor();
    QTextCursor found = activeEditor->document()->find(lastSearchText, cursor, flags);

    if (found.isNull()) {
        
        cursor.movePosition(QTextCursor::Start);
        found = activeEditor->document()->find(lastSearchText, cursor, flags);
        currentMatchIndex = 1;
    } else {
        currentMatchIndex++;
        if (currentMatchIndex > totalMatches) currentMatchIndex = 1;
    }

    if (!found.isNull()) {
        activeEditor->setTextCursor(found);
        activeEditor->ensureCursorVisible();
    }

    updateMatchCount();
}

void FindReplaceBar::performFindPrevious() {
    if (lastSearchText.isEmpty() || !activeEditor) return;

    QTextDocument::FindFlags flags = QTextDocument::FindBackward;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;

    QTextCursor cursor = activeEditor->textCursor();
    
    cursor.setPosition(cursor.selectionStart());
    QTextCursor found = activeEditor->document()->find(lastSearchText, cursor, flags);

    if (found.isNull()) {
        
        cursor.movePosition(QTextCursor::End);
        found = activeEditor->document()->find(lastSearchText, cursor, flags);
        currentMatchIndex = totalMatches;
    } else {
        currentMatchIndex--;
        if (currentMatchIndex < 1) currentMatchIndex = totalMatches;
    }

    if (!found.isNull()) {
        activeEditor->setTextCursor(found);
        activeEditor->ensureCursorVisible();
    }

    updateMatchCount();
}

void FindReplaceBar::onReplace() {
    if (lastSearchText.isEmpty() || !activeEditor) return;

    QTextCursor cursor = activeEditor->textCursor();
    if (cursor.hasSelection()) {
        QString selected = cursor.selectedText();
        bool matches = caseSensitive
            ? (selected == lastSearchText)
            : (selected.compare(lastSearchText, Qt::CaseInsensitive) == 0);

        if (matches) {
            cursor.beginEditBlock();
            cursor.insertText(replaceField->text());
            cursor.endEditBlock();
        }
    }

    
    performFindNext();
    highlightAllMatches();
}

void FindReplaceBar::onReplaceAll() {
    if (lastSearchText.isEmpty() || !activeEditor) return;

    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;

    QTextCursor cursor(activeEditor->document());
    cursor.beginEditBlock();

    int count = 0;
    cursor.movePosition(QTextCursor::Start);
    QTextCursor found = activeEditor->document()->find(lastSearchText, cursor, flags);

    while (!found.isNull()) {
        found.insertText(replaceField->text());
        count++;
        found = activeEditor->document()->find(lastSearchText, found, flags);
    }

    cursor.endEditBlock();

    totalMatches = 0;
    currentMatchIndex = 0;
    matchCountLabel->setText(QString("Replaced %1").arg(count));
    clearHighlights();
}

void FindReplaceBar::onCaseSensitivityToggled(bool checked) {
    caseSensitive = checked;
    if (!lastSearchText.isEmpty()) {
        onSearchTextChanged(lastSearchText);
    }
}

void FindReplaceBar::onThemeChanged() {
    applyThemeColors();
}

void FindReplaceBar::applyThemeColors() {
    auto &tm = Core::ThemeManager::instance();
    QColor bg = tm.getColor(Core::ThemeManager::FindBarBackground);
    QColor fg = tm.getColor(Core::ThemeManager::FindBarForeground);
    QColor border = tm.getColor(Core::ThemeManager::FindBarBorder);

    setStyleSheet(
        QString(
            "#findReplaceBar { "
            "    background-color: %1; "
            "    border-top: 1px solid %3; "
            "} "
            "#findReplaceBar QLineEdit { "
            "    background-color: %1; "
            "    color: %2; "
            "    border: 1px solid %3; "
            "    border-radius: 3px; "
            "    padding: 3px 6px; "
            "    font-size: 12px; "
            "} "
            "#findReplaceBar QLineEdit:focus { "
            "    border: 1px solid %4; "
            "} "
            "#findReplaceBar QLabel { "
            "    color: %5; "
            "    font-size: 11px; "
            "    background: transparent; "
            "} "
            "#findReplaceBar QToolButton { "
            "    background: transparent; "
            "    color: %2; "
            "    border: none; "
            "    border-radius: 3px; "
            "    font-size: 12px; "
            "} "
            "#findReplaceBar QToolButton:hover { "
            "    background-color: %3; "
            "} "
            "#findReplaceBar QToolButton:checked { "
            "    background-color: %3; "
            "    border: 1px solid %4; "
            "} "
            "#findReplaceBar QPushButton { "
            "    background-color: %3; "
            "    color: %2; "
            "    border: 1px solid %3; "
            "    border-radius: 3px; "
            "    padding: 2px 10px; "
            "    font-size: 11px; "
            "} "
            "#findReplaceBar QPushButton:hover { "
            "    background-color: %4; "
            "} "
        )
        .arg(bg.name())              
        .arg(fg.name())              
        .arg(border.name())          
        .arg(border.lighter(130).name()) 
        .arg(fg.lighter(140).name())     
    );

    setObjectName("findReplaceBar");
}

void FindReplaceBar::updateMatchCount() {
    if (totalMatches == 0) {
        matchCountLabel->setText("No results");
    } else {
        matchCountLabel->setText(QString("%1 of %2").arg(currentMatchIndex).arg(totalMatches));
    }
}

void FindReplaceBar::highlightAllMatches() {
    if (!activeEditor || lastSearchText.isEmpty()) {
        clearHighlights();
        return;
    }

    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;

    QList<QTextEdit::ExtraSelection> extraSelections;

    
    
    QTextCursor searchCursor(activeEditor->document());
    searchCursor.movePosition(QTextCursor::Start);

    totalMatches = 0;
    auto &tm = Core::ThemeManager::instance();
    QColor highlightColor = tm.getColor(Core::ThemeManager::SelectionBackground);
    highlightColor.setAlpha(100);

    QTextCursor found = activeEditor->document()->find(lastSearchText, searchCursor, flags);
    while (!found.isNull()) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(highlightColor);
        sel.cursor = found;
        extraSelections.append(sel);
        totalMatches++;
        found = activeEditor->document()->find(lastSearchText, found, flags);
    }

    
    
    
    activeEditor->setProperty("findHighlights",
        QVariant::fromValue(extraSelections));
    activeEditor->highlightCurrentLine();
}

void FindReplaceBar::clearHighlights() {
    if (activeEditor) {
        QList<QTextEdit::ExtraSelection> empty;
        activeEditor->setProperty("findHighlights",
            QVariant::fromValue(empty));
        activeEditor->highlightCurrentLine();
    }
    totalMatches = 0;
    currentMatchIndex = 0;
}

} 
