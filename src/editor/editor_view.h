#ifndef EDITORVIEW_H
#define EDITORVIEW_H

#include <QPointer>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QWidget>
#include <QCompleter>
#include <QAbstractItemView>
#include <QJsonArray>
#include "symbol_info_widget.h"

namespace Editor {

class EditorView final : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit EditorView(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    void applySettings();
    
    void goToLine();
    void goToLine(int line, int column);
    void toggleFold(int blockNumber);
    
    QString currentFilePath() const { return m_filePath; }
    void setFilePath(const QString &path) { m_filePath = path; }

public slots:
    void highlightCurrentLine();
    void zoomIn(int range = 1);
    void zoomOut(int range = 1);
    
    void setCompleter(QCompleter *c);
    QCompleter *completer() const;
    void insertCompletion(const QString &completion);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void onThemeChanged();
    void matchBrackets();
    void onHoverTimeout();

private:
    bool isHtmlLikeLanguage(const QString &fileType) const;
    QString textUnderCursor() const;
    QString getFileType() const;

    QWidget *m_lineNumberArea;
    QCompleter *m_completer = nullptr;
    QString m_filePath;

public:
    struct Diagnostic {
        int startLine, startCol;
        int endLine, endCol;
        int severity; 
        QString message;
        bool hasFixes = false;
        QJsonObject raw;
    };
public slots:
    void onDiagnosticsReady(const QString &filePath, const QJsonArray &diagnosticsArray);
    void handleHoverResult(const QString &filePath, const QJsonObject &hover);
    void handleCompletionResult(const QString &filePath, const QJsonArray &items);
    void handleDefinitionResult(const QString &filePath, const QJsonValue &result);
    void handleRenameResult(const QString &filePath, const QJsonObject &workspaceEdit);
    void handleFormattingResult(const QString &filePath, const QJsonArray &edits);
    void applyTextEdits(const QJsonArray &edits);
    
    void requestFormatting();
    void requestRename();
    void requestGoToDefinition();
    void commentSelection();
    void triggerSuggestion();

protected:
    bool event(QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    
private:
    void updateCompletionInfo();
    void updateCompleterTheme();
    
    
    bool handleCompleterKeys(QKeyEvent *event);
    bool handleShortcutKeys(QKeyEvent *event);
    bool handleBackspaceKey(QKeyEvent *event);
    bool handleEnterKey(QKeyEvent *event);
    bool handleAutoClosingPairs(QKeyEvent *event);
    bool handleTagClosing(QKeyEvent *event);
    void updateCompleterPopup(QKeyEvent *event);
    
    QList<Diagnostic> m_diagnostics;
    QJsonArray m_completionItems;
    QString m_hoverText;
    SymbolInfoWidget *m_infoWidget = nullptr;
    SymbolInfoWidget *m_diagnosticWidget = nullptr;
    QTimer *m_hoverTimer = nullptr;
    QPoint m_lastMousePos;
    QPoint m_hoverTriggerPos;

    QString getCommentPrefix() const;
};

class LineNumberArea final : public QWidget {
public:
    LineNumberArea(EditorView *editor) : QWidget(editor), m_editorView(editor) {}

    QSize sizeHint() const override {
        return QSize(m_editorView->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        m_editorView->lineNumberAreaPaintEvent(event);
    }

    void mousePressEvent(QMouseEvent *event) override;

private:
    EditorView *m_editorView;
};

} 

#endif 
