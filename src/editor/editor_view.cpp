#include "editor_view.h"
#include <QPainter>
#include <QTextBlock>
#include <QInputDialog>
#include <QTextDocument>
#include <QTextCursor>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QFontInfo>
#include <QFileInfo>
#include <QPainterPath>
#include <QTimer>
#include <QStringListModel>
#include <QItemSelectionModel>
#include <QPainter>
#include <QScrollBar>
#include <QHelpEvent>
#include <QToolTip>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QFileInfo>
#include <QMenu>
#include "core/theme_manager.h"
#include "core/settings_manager.h"
#include "core/simple_highlighter.h"
#include "lspclient/lsp_manager.h"

namespace Editor {

EditorView::EditorView(QWidget *parent) : QPlainTextEdit(parent) {
    m_lineNumberArea = new LineNumberArea(this);
    	
    connect(this, &EditorView::blockCountChanged, this, &EditorView::updateLineNumberAreaWidth);
    connect(this, &EditorView::updateRequest, this, &EditorView::updateLineNumberArea);
    connect(this, &EditorView::cursorPositionChanged, this, &EditorView::highlightCurrentLine);
    connect(&Core::ThemeManager::instance(), &Core::ThemeManager::themeChanged, this, &EditorView::onThemeChanged);

    m_infoWidget = new SymbolInfoWidget(this);
    connect(this, &EditorView::cursorPositionChanged, m_infoWidget, &SymbolInfoWidget::hide);

    m_diagnosticWidget = new SymbolInfoWidget(this);
    m_diagnosticWidget->setDiagnosticMode(true);
    connect(this, &EditorView::cursorPositionChanged, m_diagnosticWidget, &SymbolInfoWidget::hide);
	
    setMouseTracking(true);
    m_hoverTimer = new QTimer(this);
    m_hoverTimer->setSingleShot(true);
    connect(m_hoverTimer, &QTimer::timeout, this, &EditorView::onHoverTimeout);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    setFrameStyle(QFrame::NoFrame);
    QFont font("Cascadia Code", 10);
    QFontInfo fi(font);
    if (fi.family() != "Cascadia Code") {
        font.setFamily("Monospace");
    }
    setFont(font);

    applySettings();
}

void EditorView::applySettings() {
    auto &settings = Core::SettingsManager::instance();
    
    
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * settings.tabSize());
    
    
    setLineWrapMode(settings.wordWrap() ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
}

void EditorView::setCompleter(QCompleter *completer) {
    if (m_completer)
        QObject::disconnect(m_completer, nullptr, this, nullptr);

    m_completer = completer;

    if (!m_completer)
        return;

    m_completer->setWidget(this);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    QObject::connect(m_completer, QOverload<const QString &>::of(&QCompleter::activated),
                     this, &EditorView::insertCompletion);
    
    if (m_completer->popup()) {
        connect(m_completer->popup()->selectionModel(), &QItemSelectionModel::currentChanged,
                this, &EditorView::updateCompletionInfo);
        updateCompleterTheme();
    }
}

QCompleter *EditorView::completer() const {
    return m_completer;
}

void EditorView::insertCompletion(const QString &completion) {
    if (m_completer->widget() != this)
        return;
    QTextCursor tc = textCursor();
    QString prefix = textUnderCursor();
    tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, prefix.length());
    tc.insertText(completion);
    setTextCursor(tc);
}

QString EditorView::textUnderCursor() const {
    QTextCursor tc = textCursor();
    QString blockText = tc.block().text();
    int pos = tc.positionInBlock();
    int start = pos;
    while (start > 0 && (blockText[start-1].isLetterOrNumber() || blockText[start-1] == '_')) {
        start--;
    }
    return blockText.mid(start, pos - start);
}

int EditorView::lineNumberAreaWidth() {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int digitWidth = fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return 5 + digitWidth + 5 + 15; 
}

void EditorView::updateLineNumberAreaWidth(int ) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void EditorView::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void EditorView::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void EditorView::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(Core::ThemeManager::instance().getColor(Core::ThemeManager::LineHighlight));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    
    QVariant findHighlightsVar = property("findHighlights");
    if (findHighlightsVar.isValid()) {
        auto findHighlights = findHighlightsVar.value<QList<QTextEdit::ExtraSelection>>();
        extraSelections.append(findHighlights);
    }

    setExtraSelections(extraSelections);
    matchBrackets();
}

void EditorView::onThemeChanged() {
    auto &tm = Core::ThemeManager::instance();
    QPalette p = palette();
    p.setColor(QPalette::Base, tm.getColor(Core::ThemeManager::EditorBackground));
    p.setColor(QPalette::Text, tm.getColor(Core::ThemeManager::EditorForeground));
    setPalette(p);

    update();
    m_lineNumberArea->update();
    highlightCurrentLine();
    if (m_infoWidget) m_infoWidget->updateTheme();
    if (m_diagnosticWidget) m_diagnosticWidget->updateTheme();
    updateCompleterTheme();
}

void EditorView::updateCompleterTheme() {
    if (!m_completer || !m_completer->popup()) return;
    
    auto &tm = Core::ThemeManager::instance();
    QColor bg = tm.getColor(Core::ThemeManager::EditorBackground);
    QColor fg = tm.getColor(Core::ThemeManager::EditorForeground);
    QColor selBg = tm.getColor(Core::ThemeManager::LineHighlight);
    QColor border = bg.lighter(130);
    
    QString style = QString(
        "QListView {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  selection-background-color: %4;"
        "  selection-color: %2;"
        "  outline: 0;"
        "}"
    ).arg(bg.name(), fg.name(), border.name(), selBg.name());
    
    m_completer->popup()->setStyleSheet(style);
    m_completer->popup()->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
}

void EditorView::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(m_lineNumberArea);
    auto &tm = Core::ThemeManager::instance();
    
    painter.fillRect(event->rect(), tm.getColor(Core::ThemeManager::LineNumberBackground));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    painter.setPen(tm.getColor(Core::ThemeManager::LineNumberForeground));

    int areaWidth = lineNumberAreaWidth();
    
    int digitWidth = areaWidth - 5 - 5 - 15;

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.drawText(5, top, digitWidth, fontMetrics().height(),
                             Qt::AlignRight | Qt::AlignVCenter, number);
            
            
            auto *data = static_cast<Core::BlockUserData*>(block.userData());
            if (data && data->foldStart) {
                painter.save();
                QFont f = painter.font();
                f.setBold(true);
                painter.setFont(f);
                painter.drawText(5 + digitWidth + 5, top, 15, fontMetrics().height(),
                                 Qt::AlignLeft | Qt::AlignVCenter, data->folded ? "+" : "-");
                painter.restore();
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void EditorView::toggleFold(int blockNumber) {
    QTextBlock block = document()->findBlockByNumber(blockNumber);
    if (!block.isValid()) return;
    
    auto *data = static_cast<Core::BlockUserData*>(block.userData());
    if (!data || !data->foldStart) return;
    
    data->folded = !data->folded;
    
    bool hide = data->folded;
    int startLevel = data->foldingLevel;
    
    QTextBlock nextBlock = block.next();
    while (nextBlock.isValid()) {
        auto *nextData = static_cast<Core::BlockUserData*>(nextBlock.userData());
        
        if (nextData && nextData->foldingLevel < startLevel) break;
        
        if (!nextData) { nextBlock = nextBlock.next(); continue; }
        
        nextBlock.setVisible(!hide);
        nextBlock = nextBlock.next();
    }
    
    document()->markContentsDirty(block.position(), document()->characterCount() - block.position());
    viewport()->update();
    updateLineNumberAreaWidth(0);
}

void LineNumberArea::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        int blockNumber = m_editorView->cursorForPosition(QPoint(0, (int)event->position().y())).blockNumber();
        m_editorView->toggleFold(blockNumber);
    }
    QWidget::mousePressEvent(event);
}

void EditorView::goToLine() {
    bool ok;
    int lineNumber = QInputDialog::getInt(this, "Go to Line", "Line number:",
                                          textCursor().blockNumber() + 1, 1,
                                          document()->blockCount(), 1, &ok);
    if (ok) {
        goToLine(lineNumber, 1);
    }
}

void EditorView::goToLine(int line, int column) {
    QTextBlock block = document()->findBlockByNumber(line - 1);
    if (!block.isValid()) return;

    QTextCursor cursor(block);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);
    setTextCursor(cursor);
    ensureCursorVisible();
    setFocus();
}

void EditorView::keyPressEvent(QKeyEvent *event) {
    if (handleCompleterKeys(event))
        return;

    if (handleShortcutKeys(event))
        return;
    
    if (handleBackspaceKey(event))
        return;
    
    if (handleEnterKey(event))
        return;
    
    if (handleAutoClosingPairs(event))
        return;
    
    if (handleTagClosing(event))
        return;

    QPlainTextEdit::keyPressEvent(event);
    updateCompleterPopup(event);
}

void EditorView::handleCompletionResult(const QString &filePath, const QJsonArray &items) {
    if (filePath != m_filePath || !m_completer) return;

    if (items.isEmpty()) {
        m_completer->popup()->hide();
        return;
    }

    m_completionItems = items;
    QStringList completions;
    for (const QJsonValue &val : items) {
        if (val.isObject()) {
            QJsonObject item = val.toObject();
            if (item.contains("insertText")) completions.append(item["insertText"].toString());
            else if (item.contains("label")) completions.append(item["label"].toString());
        }
    }
    
    auto *model = static_cast<QStringListModel*>(m_completer->model());
    model->setStringList(completions);
    
    QRect cr = cursorRect();
    int prefixLen = m_completer->completionPrefix().length();
    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, prefixLen);
    cr.setLeft(cursorRect(tc).left());
    
    cr.translate(viewportMargins().left(), 0);
    cr.setWidth(m_completer->popup()->sizeHintForColumn(0)
                + m_completer->popup()->verticalScrollBar()->sizeHint().width() + 20);
    m_completer->complete(cr);
}

void EditorView::updateCompletionInfo() {
    if (!m_completer || !m_completer->popup() || !m_completer->popup()->isVisible()) {
        m_infoWidget->hide();
        return;
    }

    int index = m_completer->popup()->currentIndex().row();
    if (index < 0 || index >= m_completionItems.size()) {
        m_infoWidget->hide();
        return;
    }

    QJsonObject item = m_completionItems[index].toObject();
    QString doc;
    if (item.contains("documentation")) {
        QJsonValue docVal = item["documentation"];
        if (docVal.isString()) doc = docVal.toString();
        else if (docVal.isObject()) doc = docVal.toObject()["value"].toString();
    }
    
    if (doc.isEmpty() && item.contains("detail")) {
        doc = "*" + item["detail"].toString() + "*";
    }

    if (!doc.isEmpty()) {
        m_infoWidget->setContent(doc);
        QRect popupRect = m_completer->popup()->geometry();
        m_infoWidget->showAt(popupRect.topRight() + QPoint(5, 0));
    } else {
        m_infoWidget->hide();
    }
}

QString EditorView::getFileType() const {
    if (m_filePath.isEmpty()) return "text";
    return QFileInfo(m_filePath).suffix().toLower();
}

bool EditorView::isHtmlLikeLanguage(const QString &fileType) const {
    static const QStringList htmlLike = {"html", "htm", "xml", "tsx", "jsx", "php", "vue"};
    return htmlLike.contains(fileType);
}

void EditorView::wheelEvent(QWheelEvent *event) {
    if (m_infoWidget) m_infoWidget->hide();
    if (m_diagnosticWidget) m_diagnosticWidget->hide();
    
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0)
            zoomIn();
        else
            zoomOut();
        
        event->accept();
    } else {
        QPlainTextEdit::wheelEvent(event);
    }
}

void EditorView::zoomIn(int range) {
    QPlainTextEdit::zoomIn(range);
    updateLineNumberAreaWidth(0);
}

void EditorView::zoomOut(int range) {
    QPlainTextEdit::zoomOut(range);
    updateLineNumberAreaWidth(0);
}

void EditorView::matchBrackets() {
    QList<QTextEdit::ExtraSelection> selections = extraSelections();
    
    QTextCursor cursor = textCursor();
    QChar current = document()->characterAt(cursor.position());
    QChar prev = document()->characterAt(cursor.position() - 1);
    
    auto highlightBracket = [&](int pos) {
        int maxPos = document()->characterCount();
        if (pos < 0 || pos >= maxPos) return;
        QTextEdit::ExtraSelection s;
        s.format.setBackground(Core::ThemeManager::instance().getColor(Core::ThemeManager::BracketMatch));
        QTextCursor c = textCursor();
        c.setPosition(pos);
        if (c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor)) {
            s.cursor = c;
            selections.append(s);
        }
    };

    QChar target;
    int direction = 0;
    int startPos = -1;

    if (current == '{' || current == '(' || current == '[') {
        target = (current == '{') ? '}' : (current == '(' ? ')' : ']');
        direction = 1;
        startPos = cursor.position();
    } else if (prev == '}' || prev == ')' || prev == ']') {
        target = (prev == '}') ? '{' : (prev == ')' ? '(' : '[');
        direction = -1;
        startPos = cursor.position() - 1;
    }

    if (direction != 0) {
        int depth = 1;
        int pos = startPos + direction;
        while (pos >= 0 && pos < document()->characterCount()) {
            QChar c = document()->characterAt(pos);
            if (c == document()->characterAt(startPos)) depth++;
            else if (c == target) depth--;

            if (depth == 0) {
                highlightBracket(startPos);
                highlightBracket(pos);
                break;
            }
            pos += direction;
        }
    }
    
    setExtraSelections(selections);
}

void EditorView::onDiagnosticsReady(const QString &filePath, const QJsonArray &diagnosticsArray) {
    if (filePath != m_filePath) return;
    m_diagnostics.clear();
    for (const QJsonValue &val : diagnosticsArray) {
        if (!val.isObject()) continue;
        QJsonObject diag = val.toObject();
        QJsonObject range = diag["range"].toObject();
        QJsonObject start = range["start"].toObject();
        QJsonObject end = range["end"].toObject();
        
        Diagnostic d;
        d.startLine = start["line"].toInt();
        d.startCol = start["character"].toInt();
        d.endLine = end["line"].toInt();
        d.endCol = end["character"].toInt();
        d.severity = diag.contains("severity") ? diag["severity"].toInt() : 1;
        d.message = diag["message"].toString();
        d.hasFixes = diag.contains("codeActions") || diag.contains("fixes");
        m_diagnostics.append(d);
    }
    highlightCurrentLine();
}

bool EditorView::event(QEvent *event) {
    if (event->type() == QEvent::ToolTip) {
        return true; 
    }
    return QPlainTextEdit::event(event);
}

void EditorView::mouseMoveEvent(QMouseEvent *event) {
    QPlainTextEdit::mouseMoveEvent(event);
    
    QTextCursor cursor = cursorForPosition(event->pos());
    QRect cursorRc = cursorRect(cursor);
    bool overText = false;
    
    if (abs(cursorRc.bottom() - event->pos().y()) <= fontMetrics().height() * 2) {
        if (event->pos().x() <= cursorRc.right() || cursor.positionInBlock() < cursor.block().length() - 1) {
            overText = true;
        }
    }
    
    if (m_lastMousePos != event->pos()) {
        m_lastMousePos = event->pos();
        
        bool isWidgetVisible = (m_infoWidget && m_infoWidget->isVisible()) || 
                               (m_diagnosticWidget && m_diagnosticWidget->isVisible());
                               
        if (isWidgetVisible) {
            int distToTrigger = (event->pos() - m_hoverTriggerPos).manhattanLength();
            
            QPoint gpos = event->globalPosition().toPoint();
            int distToWidget = 10000;
            auto checkDist = [&](QWidget* w) {
                if (w && w->isVisible()) {
                    QRect r = w->geometry();
                    if (r.contains(gpos)) {
                        distToWidget = 0;
                    } else {
                        int dx = std::max({0, r.left() - gpos.x(), gpos.x() - r.right()});
                        int dy = std::max({0, r.top() - gpos.y(), gpos.y() - r.bottom()});
                        distToWidget = std::min(distToWidget, dx + dy);
                    }
                }
            };
            checkDist(m_infoWidget);
            checkDist(m_diagnosticWidget);
            
            if (distToTrigger > 20 && distToWidget > 30) {
                if (m_infoWidget) m_infoWidget->hide();
                if (m_diagnosticWidget) m_diagnosticWidget->hide();
                if (overText) {
                    if (m_hoverTimer) m_hoverTimer->start(400); 
                } else {
                    if (m_hoverTimer) m_hoverTimer->stop();
                }
            }
        } else {
            if (overText) {
                if (m_infoWidget) m_infoWidget->hide();
                if (m_diagnosticWidget) m_diagnosticWidget->hide();
                if (m_hoverTimer) m_hoverTimer->start(400);
            } else {
                if (m_hoverTimer) m_hoverTimer->stop();
                if (m_infoWidget) m_infoWidget->hide();
                if (m_diagnosticWidget) m_diagnosticWidget->hide();
            }
        }
    }
}

void EditorView::leaveEvent(QEvent *event) {
    QPoint globalPos = QCursor::pos();
    bool overWidget = false;
    if (m_infoWidget && m_infoWidget->isVisible() && m_infoWidget->geometry().contains(globalPos)) overWidget = true;
    if (m_diagnosticWidget && m_diagnosticWidget->isVisible() && m_diagnosticWidget->geometry().contains(globalPos)) overWidget = true;
    
    if (!overWidget) {
        m_hoverTimer->stop();
        if (m_infoWidget) m_infoWidget->hide();
        if (m_diagnosticWidget) m_diagnosticWidget->hide();
    }
    QPlainTextEdit::leaveEvent(event);
}

void EditorView::onHoverTimeout() {
    m_hoverTriggerPos = m_lastMousePos;
    QTextCursor cursor = cursorForPosition(m_lastMousePos);
    int line = cursor.blockNumber();
    int col = cursor.positionInBlock();
    
    QString tooltipText;
    for (const Diagnostic &d : m_diagnostics) {
        if (line >= d.startLine && line <= d.endLine) {
            if (line == d.startLine && col < d.startCol) continue;
            if (line == d.endLine && col > d.endCol) continue;
            
            if (!tooltipText.isEmpty()) tooltipText += "\n\n---\n\n";
            
            QString severityStr;
            switch(d.severity) {
                case 1: severityStr = "**Error**"; break;
                case 2: severityStr = "**Warning**"; break;
                case 3: severityStr = "**Info**"; break;
                default: severityStr = "**Hint**"; break;
            }
            
            QString msg = d.message;
            msg.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;");
            msg.replace("\n", "\n\n");
            
            tooltipText += severityStr + ":\n\n" + msg;
        }
    }
    
    if (!tooltipText.isEmpty()) {
        m_diagnosticWidget->setContent(tooltipText);
        m_diagnosticWidget->showAt(viewport()->mapToGlobal(m_lastMousePos) + QPoint(10, 10));
    } else {
        LspClient::LspManager::instance().requestHover(m_filePath, line, col);
    }
}

void EditorView::mousePressEvent(QMouseEvent *event) {
    if (m_infoWidget) m_infoWidget->hide();
    if (m_diagnosticWidget) m_diagnosticWidget->hide();
    QPlainTextEdit::mousePressEvent(event);
}

void EditorView::handleHoverResult(const QString &filePath, const QJsonObject &hover) {
    if (filePath != m_filePath) return;
    
    QString contents;
    QJsonValue val = hover["contents"];
    if (val.isString()) {
        contents = val.toString();
    } else if (val.isArray()) {
        for (const QJsonValue &v : val.toArray()) {
            if (v.isString()) contents += v.toString() + "\n";
            else if (v.isObject()) {
                QJsonObject obj = v.toObject();
                if (obj.contains("value")) contents += obj["value"].toString() + "\n";
            }
        }
    } else if (val.isObject()) {
        contents = val.toObject()["value"].toString();
    }
    
    if (!contents.isEmpty()) {
        m_infoWidget->setContent(contents);
        m_infoWidget->showAt(QCursor::pos() + QPoint(10, -10));
    }
}

void EditorView::handleRenameResult(const QString &, const QJsonObject &workspaceEdit) {
    
    if (workspaceEdit.contains("changes")) {
        QJsonObject changes = workspaceEdit["changes"].toObject();
        for (auto it = changes.begin(); it != changes.end(); ++it) {
            QString fileUri = it.key();
            if (fileUri.startsWith("file://")) fileUri = QUrl(fileUri).toLocalFile();
            
            if (fileUri == m_filePath) {
                applyTextEdits(it.value().toArray());
            }
        }
    }
}

void EditorView::handleFormattingResult(const QString &filePath, const QJsonArray &edits) {
    if (filePath != m_filePath) return;
    applyTextEdits(edits);
}

void EditorView::applyTextEdits(const QJsonArray &edits) {
    if (edits.isEmpty()) return;

    struct Edit {
        int startLine, startCol, endLine, endCol;
        QString text;
    };
    QList<Edit> sortedEdits;
    for (const QJsonValue &v : edits) {
        QJsonObject edit = v.toObject();
        QJsonObject range = edit["range"].toObject();
        QJsonObject start = range["start"].toObject();
        QJsonObject end = range["end"].toObject();
        sortedEdits.append({
            start["line"].toInt(), start["character"].toInt(),
            end["line"].toInt(), end["character"].toInt(),
            edit["newText"].toString()
        });
    }

    std::sort(sortedEdits.begin(), sortedEdits.end(), [](const Edit &a, const Edit &b) {
        if (a.startLine != b.startLine) return a.startLine > b.startLine;
        return a.startCol > b.startCol;
    });

    QTextCursor cursor(document());
    cursor.beginEditBlock();
    for (const Edit &e : sortedEdits) {
        QTextBlock startBlock = document()->findBlockByNumber(e.startLine);
        QTextBlock endBlock = document()->findBlockByNumber(e.endLine);
        
        int startPos = startBlock.position() + e.startCol;
        int endPos = endBlock.position() + e.endCol;
        
        cursor.setPosition(startPos);
        cursor.setPosition(endPos, QTextCursor::KeepAnchor);
        cursor.insertText(e.text);
    }
    cursor.endEditBlock();
}

void EditorView::handleDefinitionResult(const QString &filePath, const QJsonValue &result) {
    if (filePath != m_filePath) return;

    QJsonObject location;
    if (result.isArray()) {
        QJsonArray arr = result.toArray();
        if (arr.isEmpty()) return;
        location = arr[0].toObject();
    } else if (result.isObject()) {
        location = result.toObject();
    } else {
        return;
    }

    QString uri = location["uri"].toString();
    if (uri.startsWith("file://")) uri = QUrl(uri).toLocalFile();
    
    QJsonObject range = location["range"].toObject();
    QJsonObject start = range["start"].toObject();
    int line = start["line"].toInt() + 1;
    int col = start["character"].toInt();

    if (uri == m_filePath) {
        goToLine(line, col);
    } else {
        qDebug() << "LSP Definition in another file:" << uri << "at" << line << ":" << col;
    }
}

void EditorView::requestGoToDefinition() {
    QTextCursor cursor = textCursor();
    LspClient::LspManager::instance().requestDefinition(m_filePath, cursor.blockNumber(), cursor.positionInBlock());
}

void EditorView::requestRename() {
    bool ok;
    QString newName = QInputDialog::getText(this, "Rename Symbol", "New name:", QLineEdit::Normal, textUnderCursor(), &ok);
    if (ok && !newName.isEmpty()) {
        QTextCursor cursor = textCursor();
        LspClient::LspManager::instance().requestRename(m_filePath, cursor.blockNumber(), cursor.positionInBlock(), newName);
    }
}

void EditorView::requestFormatting() {
    LspClient::LspManager::instance().requestFormatting(m_filePath);
}

void EditorView::contextMenuEvent(QContextMenuEvent *event) {
    QTextCursor cursor = cursorForPosition(event->pos());
    setTextCursor(cursor);

    QMenu *menu = createStandardContextMenu();
    menu->addSeparator();
    
    if (LspClient::LspManager::instance().hasActiveClient(m_filePath)) {
        QAction *defAction = menu->addAction("Go to Definition");
        connect(defAction, &QAction::triggered, this, &EditorView::requestGoToDefinition);
        
        QAction *renameAction = menu->addAction("Rename Symbol");
        connect(renameAction, &QAction::triggered, this, &EditorView::requestRename);
        
        QAction *formatAction = menu->addAction("Format Document");
        connect(formatAction, &QAction::triggered, this, &EditorView::requestFormatting);
    }
    
    menu->exec(event->globalPos());
    delete menu;
}

void EditorView::paintEvent(QPaintEvent *event) {
    QPlainTextEdit::paintEvent(event);
    
    if (m_diagnostics.isEmpty()) return;
    
    QPainter painter(viewport());
    
    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            int blockNum = block.blockNumber();
            
            for (const Diagnostic &d : m_diagnostics) {
                if (blockNum >= d.startLine && blockNum <= d.endLine) {
                    QColor color = Qt::red;
                    if (d.severity == 2) color = Qt::yellow;
                    else if (d.severity == 3) color = Qt::blue;
                    else if (d.severity == 4) color = Qt::gray;
                    
                    QPen pen(color);
                    pen.setStyle(Qt::DotLine);
                    pen.setWidth(2);
                    painter.setPen(pen);
                    
                    int startCol = (blockNum == d.startLine) ? d.startCol : 0;
                    int endCol = (blockNum == d.endLine) ? d.endCol : block.length() - 1;
                    if (endCol < startCol) endCol = block.length() - 1;
                    
                    QTextCursor cursor(block);
                    cursor.setPosition(block.position() + startCol);
                    QRect startRect = cursorRect(cursor);
                    
                    cursor.setPosition(block.position() + endCol);
                    QRect endRect;
                    if (endCol > startCol) {
                        endRect = cursorRect(cursor);
                    } else {
                        endRect = startRect;
                        endRect.setWidth(fontMetrics().horizontalAdvance('W'));
                    }
                    
                    int x1 = startRect.left();
                    int x2 = endRect.right();
                    if (x2 < x1) x2 = x1 + fontMetrics().horizontalAdvance('W');
                    
                    int y = startRect.bottom() - 1;
                    
                    QPainterPath path;
                    path.moveTo(x1, y);
                    
                    int waveLen = 4;
                    int waveAmp = 2;
                    bool up = true;
                    for (int x = x1 + waveLen; x < x2; x += waveLen) {
                        path.lineTo(x, y + (up ? waveAmp : 0));
                        up = !up;
                    }
                    path.lineTo(x2, y);
                    painter.drawPath(path);
                }
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
    }
}

bool EditorView::handleCompleterKeys(QKeyEvent *event) {
    if (!m_completer || !m_completer->popup() || !m_completer->popup()->isVisible())
        return false;

    switch (event->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Tab: {
        QModelIndex idx = m_completer->popup()->currentIndex();
        if (idx.isValid()) {
            insertCompletion(idx.data().toString());
            m_completer->popup()->hide();
        }
        return true;
    }
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        return true; 
    case Qt::Key_Escape:
    case Qt::Key_Backtab:
        event->ignore();
        return true;
    default:
        break;
    }
    return false;
}

bool EditorView::handleShortcutKeys(QKeyEvent *) {
    return false;
}

bool EditorView::handleBackspaceKey(QKeyEvent *event) {
    if (event->key() != Qt::Key_Backspace)
        return false;

    QTextCursor cursor = textCursor();
    if (cursor.hasSelection())
        return false;

    QString line = cursor.block().text();
    int posInBlock = cursor.positionInBlock();
    
    if (posInBlock > 0) {
        int indentLen = 0;
        while (indentLen < line.length() && line[indentLen].isSpace()) {
            indentLen++;
        }
        
        if (posInBlock <= indentLen) {
            int tabSize = Core::SettingsManager::instance().tabSize();
            int spacesToDelete = posInBlock % tabSize;
            if (spacesToDelete == 0) spacesToDelete = tabSize;
            
            
            bool allSpaces = true;
            for (int i = posInBlock - spacesToDelete; i < posInBlock; ++i) {
                if (!line[i].isSpace()) {
                    allSpaces = false;
                    break;
                }
            }
            
            if (allSpaces) {
                cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, spacesToDelete);
                cursor.removeSelectedText();
                setTextCursor(cursor);
                return true;
            }
        }
    }
    return false;
}

bool EditorView::handleEnterKey(QKeyEvent *event) {
    if (event->key() != Qt::Key_Enter && event->key() != Qt::Key_Return)
        return false;

    QTextCursor cursor = textCursor();
    QString line = cursor.block().text();
    
    int indentLen = 0;
    while (indentLen < line.length() && line[indentLen].isSpace()) {
        indentLen++;
    }
    QString indent = line.left(indentLen);
    
    QString trimmed = line.trimmed();
    if (trimmed.endsWith('{') || trimmed.endsWith(':') || trimmed.endsWith('[') || trimmed.endsWith('(')) {
        indent += "\t";
    }
    
    cursor.insertText("\n" + indent);
    return true;
}

bool EditorView::handleAutoClosingPairs(QKeyEvent *event) {
    QString text = event->text();
    if (text.isEmpty()) return false;

    QTextCursor cursor = textCursor();
    QChar current = document()->characterAt(cursor.position());

    
    if (text == "(" || text == "[" || text == "{" || text == "<") {
        QChar closing = (text == "(") ? ')' : (text == "[" ? ']' : (text == "{" ? '}' : '>'));
        cursor.insertText(text + closing);
        cursor.movePosition(QTextCursor::PreviousCharacter);
        setTextCursor(cursor);
        return true;
    }

    
    if ((text == ")" || text == "]" || text == "}") && current == text[0]) {
        cursor.movePosition(QTextCursor::NextCharacter);
        setTextCursor(cursor);
        return true;
    }

    
    if (text == "}" || text == "]" || text == ")") {
        QString line = cursor.block().text();
        int posInBlock = cursor.positionInBlock();
        
        bool onlyWhitespaceBefore = true;
        for (int i = 0; i < posInBlock; ++i) {
            if (!line[i].isSpace()) {
                onlyWhitespaceBefore = false;
                break;
            }
        }
        
        if (onlyWhitespaceBefore) {
            QChar closing = text[0];
            QChar opening = (closing == '}') ? '{' : (closing == ']' ? '[' : '(');
            
            int depth = 1;
            int currentPos = cursor.position() - 1;
            while (currentPos >= 0) {
                QChar c = document()->characterAt(currentPos);
                if (c == closing) depth++;
                else if (c == opening) depth--;
                
                if (depth == 0) {
                    QTextBlock matchBlock = document()->findBlock(currentPos);
                    QString matchLine = matchBlock.text();
                    int matchIndent = 0;
                    while (matchIndent < matchLine.length() && matchLine[matchIndent].isSpace()) {
                        matchIndent++;
                    }
                    
                    QString newIndent = matchLine.left(matchIndent);
                    cursor.beginEditBlock();
                    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
                    cursor.removeSelectedText();
                    cursor.insertText(newIndent + closing);
                    cursor.endEditBlock();
                    return true;
                }
                currentPos--;
            }
        }
    }

    return false;
}

bool EditorView::handleTagClosing(QKeyEvent *event) {
    if (event->text() != ">" || !isHtmlLikeLanguage(getFileType()))
        return false;

    QTextCursor cursor = textCursor();
    bool overtyping = (document()->characterAt(cursor.position()) == '>');
    
    int pos = cursor.position();
    QTextBlock block = cursor.block();
    QString blockText = block.text();
    int posInBlock = pos - block.position();

    int start = -1;
    for (int i = posInBlock - 1; i >= 0; --i) {
        if (blockText[i] == '<') {
            start = i;
            break;
        } else if (blockText[i] == '>') {
            break; 
        }
    }

    if (start != -1 && start < posInBlock) {
        QString tagContent = blockText.mid(start + 1, posInBlock - start - 1).trimmed();
        QString tagName = tagContent.split(QRegularExpression("\\s+")).first();
        
        if (!tagName.isEmpty() && !tagName.startsWith("/") && !tagName.startsWith("!") && !tagContent.endsWith("/")) {
            if (overtyping) {
                cursor.movePosition(QTextCursor::NextCharacter);
            } else {
                cursor.insertText(">");
            }
            cursor.insertText("</" + tagName + ">");
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor, tagName.length() + 3);
            setTextCursor(cursor);
            return true;
        }
    }
    return false;
}

void EditorView::updateCompleterPopup(QKeyEvent *event) {
    if (!m_completer) return;

    bool ctrlOrShift = event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
    if (ctrlOrShift && event->text().isEmpty())
        return;

    static QString eow("~!@#$%^&*()_+{}|:\"<>?,/;'[]\\-=");
    bool hasModifier = (event->modifiers() != Qt::NoModifier) && !ctrlOrShift;
    QString completionPrefix = textUnderCursor();
    QString text = event->text();

    bool isTrigger = text == "." || text == ":" || text == ">";

    if ((hasModifier || (text.isEmpty()) || (completionPrefix.length() < 1 && !isTrigger)
                        || eow.contains(text.right(1))) && !isTrigger) {
        m_completer->popup()->hide();
        return;
    }

    if (completionPrefix != m_completer->completionPrefix()) {
        m_completer->setCompletionPrefix(completionPrefix);
        m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));
    }

    QRect cr = cursorRect();
    int prefixLen = m_completer->completionPrefix().length();
    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, prefixLen);
    cr.setLeft(cursorRect(tc).left());
    
    cr.translate(viewportMargins().left(), 0);
    cr.setWidth(m_completer->popup()->sizeHintForColumn(0)
                + m_completer->popup()->verticalScrollBar()->sizeHint().width() + 20);
    m_completer->complete(cr);
}

void EditorView::commentSelection() {
    QTextCursor cursor = textCursor();
    QString prefix = getCommentPrefix();
    if (prefix.isEmpty()) return;

    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();

    QTextBlock startBlock = document()->findBlock(start);
    QTextBlock endBlock = document()->findBlock(end);

    if (end > endBlock.position() && endBlock.next().isValid() && end == endBlock.next().position()) {
        endBlock = endBlock.previous();
    }

    cursor.beginEditBlock();

    bool allCommented = true;
    for (QTextBlock block = startBlock; block.isValid() && block.blockNumber() <= endBlock.blockNumber(); block = block.next()) {
        QString text = block.text().trimmed();
        if (!text.isEmpty() && !text.startsWith(prefix)) {
            allCommented = false;
            break;
        }
    }

    QTextCursor editCursor(document());
    for (QTextBlock block = startBlock; block.isValid() && block.blockNumber() <= endBlock.blockNumber(); block = block.next()) {
        editCursor.setVisualNavigation(false);
        if (allCommented) {
            QString text = block.text();
            int idx = text.indexOf(prefix);
            if (idx != -1) {
                editCursor.setPosition(block.position() + idx);
                editCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, prefix.length());
                editCursor.removeSelectedText();
            }
        } else {
            editCursor.setPosition(block.position());
            editCursor.insertText(prefix);
        }
    }

    cursor.endEditBlock();
}

void EditorView::triggerSuggestion() {
    if (m_completer) {
        m_completer->setCompletionPrefix(textUnderCursor());
    }
    LspClient::LspManager::instance().requestCompletion(m_filePath, textCursor().blockNumber(), textCursor().positionInBlock());
}

QString EditorView::getCommentPrefix() const {
    QString ext = getFileType();
    if (ext == "cpp" || ext == "h" || ext == "c" || ext == "java" || ext == "js" || ext == "ts" || ext == "cs" || ext == "go" || ext == "php") {
        return "//";
    } else if (ext == "py" || ext == "rb" || ext == "sh" || ext == "yaml" || ext == "yml" || ext == "cmake") {
        return "#";
    } else if (ext == "html" || ext == "xml") {
        return "<!--"; 
    } else if (ext == "css") {
        return "/*"; 
    }
    return "//";
}

} 
