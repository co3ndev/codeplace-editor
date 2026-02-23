#ifndef FIND_REPLACE_BAR_H
#define FIND_REPLACE_BAR_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QPointer>

namespace Editor {

class EditorView;

class FindReplaceBar final : public QWidget {
    Q_OBJECT

public:
    explicit FindReplaceBar(QWidget *parent = nullptr);

    void showFind(EditorView *editor);
    void showReplace(EditorView *editor);
    void hideBar();

    void performFindNext();
    void performFindPrevious();

signals:
    void visibilityChanged(bool visible);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onSearchTextChanged(const QString &text);
    void onReplace();
    void onReplaceAll();
    void onCaseSensitivityToggled(bool checked);
    void onThemeChanged();

private:
    void applyThemeColors();
    void updateMatchCount();
    void highlightAllMatches();
    void clearHighlights();

    QLineEdit *searchField;
    QLineEdit *replaceField;
    QLabel *matchCountLabel;
    QToolButton *prevButton;
    QToolButton *nextButton;
    QToolButton *closeButton;
    QToolButton *caseSensitiveButton;
    QPushButton *replaceButton;
    QPushButton *replaceAllButton;

    QWidget *findRow;
    QWidget *replaceRow;

    QPointer<EditorView> activeEditor;
    QString lastSearchText;
    bool caseSensitive = false;
    int currentMatchIndex = 0;
    int totalMatches = 0;
};

} 

#endif 
