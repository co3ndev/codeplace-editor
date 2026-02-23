#include "problems_widget.h"
#include "core/theme_manager.h"
#include <QAbstractItemView>
#include <QHeaderView>
#include <QFileInfo>
#include <QJsonObject>
#include <QIcon>
#include <QMenu>
#include <QApplication>
#include <QClipboard>

namespace LspClient {

ProblemsWidget::ProblemsWidget(QWidget *parent) : QTreeWidget(parent) {
    setHeaderLabels({"Resource", "Message", "Line", "Column"});
    setIndentation(15);
    setUniformRowHeights(true);
    setAnimated(false);
    setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    setSortingEnabled(true);
    
    header()->setStretchLastSection(true);
    header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    connect(this, &QTreeWidget::itemDoubleClicked, this, &ProblemsWidget::onItemDoubleClicked);
    
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeWidget::customContextMenuRequested, this, &ProblemsWidget::onContextMenuRequested);

    connect(&Core::ThemeManager::instance(), &Core::ThemeManager::themeChanged, this, &ProblemsWidget::updateTheme);
    
    updateTheme();
}

void ProblemsWidget::onDiagnosticsReady(const QString &filePath, const QJsonArray &diagnostics) {
    if (diagnostics.isEmpty()) {
        m_diagnostics.remove(filePath);
    } else {
        m_diagnostics[filePath] = diagnostics;
    }
    
    updateFileItem(filePath);
}

void ProblemsWidget::onOpenFilesChanged(const QStringList &openFiles) {
    m_openFiles = QSet<QString>(openFiles.begin(), openFiles.end());
    
    for (auto it = m_diagnostics.begin(); it != m_diagnostics.end(); ++it) {
        updateFileItem(it.key());
    }
}

void ProblemsWidget::updateFileItem(const QString &filePath) {
    if (m_fileItems.contains(filePath)) {
        delete m_fileItems.take(filePath);
    }
    
    if (!m_diagnostics.contains(filePath) || !m_openFiles.contains(filePath)) return;
    
    const QJsonArray &diagnostics = m_diagnostics[filePath];
    QFileInfo fi(filePath);
    
    auto *fileItem = new QTreeWidgetItem(this);
    fileItem->setText(0, fi.fileName());
    fileItem->setToolTip(0, filePath);
    fileItem->setData(0, Qt::UserRole, filePath);
    
    QFont font = fileItem->font(0);
    font.setBold(true);
    fileItem->setFont(0, font);
    
    for (const QJsonValue &val : diagnostics) {
        QJsonObject diag = val.toObject();
        QJsonObject range = diag["range"].toObject();
        QJsonObject start = range["start"].toObject();
        
        int line = start["line"].toInt();
        int col = start["character"].toInt();
        QString message = diag["message"].toString();
        int severity = diag.contains("severity") ? diag["severity"].toInt() : 1;
        
        auto *item = new QTreeWidgetItem(fileItem);
        item->setText(1, message);
        item->setText(2, QString::number(line + 1));
        item->setText(3, QString::number(col + 1));
        
        auto &tm = Core::ThemeManager::instance();
        QColor color;
        if (severity == 1) color = tm.getColor(Core::ThemeManager::DiagnosticError);
        else if (severity == 2) color = tm.getColor(Core::ThemeManager::DiagnosticWarning);
        else if (severity == 3) color = tm.getColor(Core::ThemeManager::DiagnosticInfo);
        else if (severity == 4) color = tm.getColor(Core::ThemeManager::DiagnosticHint);
        else color = tm.getColor(Core::ThemeManager::EditorForeground);
        
        item->setForeground(1, color);
        
        item->setData(0, Qt::UserRole, filePath);
        item->setData(2, Qt::UserRole, line + 1);
        item->setData(3, Qt::UserRole, col + 1);
    }
    
    m_fileItems[filePath] = fileItem;
    fileItem->setExpanded(true);
}

void ProblemsWidget::onItemDoubleClicked(QTreeWidgetItem *item, int ) {
    if (!item->parent()) return;
    
    QString filePath = item->data(0, Qt::UserRole).toString();
    int line = item->data(2, Qt::UserRole).toInt();
    int col = item->data(3, Qt::UserRole).toInt();
    
    if (!filePath.isEmpty()) {
        emit problemSelected(filePath, line, col);
    }
}

void ProblemsWidget::onContextMenuRequested(const QPoint &pos) {
    QTreeWidgetItem *item = itemAt(pos);
    if (!item) return;

    QMenu menu(this);
    if (item->parent()) {
        menu.addAction("Copy Message", [item]() {
            QApplication::clipboard()->setText(item->text(1));
        });
        menu.addAction("Go to Problem", [this, item]() {
            onItemDoubleClicked(item, 0);
        });
    } else {
        menu.addAction("Copy Path", [item]() {
            QApplication::clipboard()->setText(item->data(0, Qt::UserRole).toString());
        });
    }
    menu.exec(viewport()->mapToGlobal(pos));
}

void ProblemsWidget::updateTheme() {
    
    
    for (auto it = m_diagnostics.begin(); it != m_diagnostics.end(); ++it) {
        updateFileItem(it.key());
    }
}

} 
