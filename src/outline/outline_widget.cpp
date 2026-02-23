#include "outline_widget.h"
#include "../editor/editor_view.h"
#include "../core/theme_manager.h"
#include <QAbstractItemView>
#include <QHeaderView>
#include <QRegularExpression>
#include <QTextBlock>
#include "../lspclient/lsp_manager.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QFileInfo>
#include <QMenu>
#include <QClipboard>
#include <QApplication>

namespace Outline {

OutlineWidget::OutlineWidget(QWidget *parent) : QWidget(parent) {
    setupUI();

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    m_refreshTimer->setInterval(500); 

    connect(m_refreshTimer, &QTimer::timeout, this, &OutlineWidget::parseSymbols);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, &OutlineWidget::onItemDoubleClicked);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, this, &OutlineWidget::onContextMenuRequested);
    connect(&LspClient::LspManager::instance(), &LspClient::LspManager::documentSymbolsReady, this, &OutlineWidget::onDocumentSymbolsReady);
}

void OutlineWidget::setupUI() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
	
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setIndentation(15);
    m_treeWidget->setUniformRowHeights(true);
    m_treeWidget->setAnimated(false);
    m_treeWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    m_treeWidget->setAlternatingRowColors(false);
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    layout->addWidget(m_treeWidget);
}

void OutlineWidget::setEditor(Editor::EditorView *editor) {
    if (m_currentEditor) {
        disconnect(m_currentEditor->document(), &QTextDocument::contentsChanged, m_refreshTimer, qOverload<>(&QTimer::start));
    }

    m_currentEditor = editor;

    if (m_currentEditor) {
        connect(m_currentEditor->document(), &QTextDocument::contentsChanged, m_refreshTimer, qOverload<>(&QTimer::start));
        m_refreshTimer->start(); 
    } else {
        m_refreshTimer->stop();
        m_treeWidget->clear();
        m_currentSymbols.clear();
    }
}

void OutlineWidget::refreshOutline() {
    if (m_refreshTimer->isActive()) return;
    m_refreshTimer->start();
}

void OutlineWidget::parseSymbols() {
    m_treeWidget->clear();
    m_currentSymbols.clear();

    if (!m_currentEditor) return;

    QTextDocument *doc = m_currentEditor->document();
    if (!doc) return;

    QString filePath = m_currentEditor->currentFilePath();
    QString suffix = !filePath.isEmpty() ? QFileInfo(filePath).suffix() : "";
    if (!filePath.isEmpty() && LspClient::LspManager::instance().isLanguageSupported(suffix)) {
        LspClient::LspManager::instance().requestDocumentSymbols(filePath);
    }

    m_currentSymbols = SymbolParser::parse(doc, suffix);

    for (int i = 0; i < m_currentSymbols.size(); ++i) {
        const OutlineSymbol &sym = m_currentSymbols[i];
        
        QTreeWidgetItem *item = new QTreeWidgetItem(m_treeWidget);
        
        if (sym.type == "class") {
            item->setText(0, "{} " + sym.name);
        } else {
            item->setText(0, "() " + sym.name);
        }

        item->setData(0, Qt::UserRole, i);
        item->setToolTip(0, QString("Line %1").arg(sym.line));
    }
    
    m_treeWidget->expandAll();
}

void OutlineWidget::onDocumentSymbolsReady(const QString &uri, const QJsonArray &symbols) {
    if (!m_currentEditor || m_currentEditor->currentFilePath() != uri) return;

    m_treeWidget->clear();
    m_currentSymbols.clear();

    auto addSymbol = [&](auto self, const QJsonObject &sym, QTreeWidgetItem *parent) -> void {
        QString name = sym["name"].toString();
        int kind = sym["kind"].toInt();
        
        QJsonObject selectionRange = sym["selectionRange"].toObject();
        QJsonObject range = sym["range"].toObject();
        
        QJsonObject targetRange = selectionRange.isEmpty() ? range : selectionRange;
        
        if (targetRange.isEmpty() && sym.contains("location")) {
            targetRange = sym["location"].toObject()["range"].toObject();
        }
        
        QJsonObject start = targetRange["start"].toObject();
        
        OutlineSymbol s;
        s.name = name;
        s.line = start["line"].toInt() + 1;
        s.column = start["character"].toInt();
        s.type = (kind == 5 || kind == 10 || kind == 11) ? "class" : "function";
        
        m_currentSymbols.append(s);
        int symIdx = m_currentSymbols.size() - 1;

        QTreeWidgetItem *item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(m_treeWidget);
        
        QString prefix = (s.type == "class") ? "{} " : "() ";
        item->setText(0, prefix + name);
        item->setData(0, Qt::UserRole, symIdx);
        item->setToolTip(0, QString("Line %1").arg(s.line));

        if (sym.contains("children")) {
            QJsonArray children = sym["children"].toArray();
            for (const QJsonValue &child : children) {
                self(self, child.toObject(), item);
            }
        }
    };

    for (const QJsonValue &val : symbols) {
        addSymbol(addSymbol, val.toObject(), nullptr);
    }
    
    m_treeWidget->expandAll();
}

void OutlineWidget::onItemDoubleClicked(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column);
    if (!item) return;

    int idx = getSymbolIndexForItem(item);
    if (idx >= 0 && idx < m_currentSymbols.size()) {
        const OutlineSymbol &sym = m_currentSymbols[idx];
        emit symbolSelected(sym.line, sym.column);
    }
}

void OutlineWidget::onContextMenuRequested(const QPoint &pos) {
    QTreeWidgetItem *item = m_treeWidget->itemAt(pos);
    if (!item) return;

    int idx = getSymbolIndexForItem(item);
    if (idx < 0 || idx >= m_currentSymbols.size()) return;

    const OutlineSymbol &sym = m_currentSymbols[idx];
    QMenu menu(this);
    
    QAction *goToAction = menu.addAction("Go to Symbol");
    menu.addSeparator();
    QAction *copyNameAction = menu.addAction("Copy Name");
    QAction *copyLineAction = menu.addAction("Copy Line Number");

    QAction *selectedAction = menu.exec(m_treeWidget->viewport()->mapToGlobal(pos));

    if (selectedAction == goToAction) {
        emit symbolSelected(sym.line, sym.column);
    } else if (selectedAction == copyNameAction) {
        QApplication::clipboard()->setText(sym.name);
    } else if (selectedAction == copyLineAction) {
        QApplication::clipboard()->setText(QString::number(sym.line));
    }
}

int OutlineWidget::getSymbolIndexForItem(QTreeWidgetItem *item) const {
    if (!item) return -1;
    bool ok;
    int idx = item->data(0, Qt::UserRole).toInt(&ok);
    return ok ? idx : -1;
}

} 
