#include "search_widget.h"
#include <QStyledItemDelegate>
#include <QHeaderView>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QAbstractItemView>
#include <QTreeView>
#include <QApplication>
#include "../core/theme_manager.h"
#include "../core/project_manager.h"

namespace Search {

class SearchItemDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        if (index.column() == 1) { 
            painter->save();

            
            QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
            style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

            
            QTextDocument doc;
            doc.setDocumentMargin(4);
            doc.setDefaultFont(opt.font);
            
            QString text = index.data().toString();
            doc.setPlainText(text);
            
            QTextCharFormat format;
            format.setForeground(opt.palette.text());
            QTextCursor cursor(&doc);
            cursor.select(QTextCursor::Document);
            cursor.mergeCharFormat(format);
            
            doc.setTextWidth(opt.rect.width());

            painter->translate(opt.rect.left(), opt.rect.top());
            QRect clip(0, 0, opt.rect.width(), opt.rect.height());
            doc.drawContents(painter, clip);

            painter->restore();
        } else {
            QStyledItemDelegate::paint(painter, option, index);
        }
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        if (index.column() == 1) {
            QStyleOptionViewItem opt = option;
            initStyleOption(&opt, index);

            QTextDocument doc;
            doc.setDocumentMargin(4);
            doc.setDefaultFont(opt.font);
            doc.setPlainText(index.data().toString());
            
            
            
            int width = 200; 
            if (const QTreeView *view = qobject_cast<const QTreeView*>(opt.widget)) {
                width = view->columnWidth(1);
            }
            doc.setTextWidth(width);
            
            return QSize(width, doc.size().height());
        }
        return QStyledItemDelegate::sizeHint(option, index);
    }
};

SearchWidget::SearchWidget(QWidget *parent) : QWidget(parent) {
    m_engine = new SearchEngine(this);
    setupUI();

    connect(m_engine, &SearchEngine::matchFound, this, &SearchWidget::onMatchFound, Qt::QueuedConnection);
    connect(m_engine, &SearchEngine::searchFinished, this, &SearchWidget::onSearchFinished, Qt::QueuedConnection);
    connect(&Core::ThemeManager::instance(), &Core::ThemeManager::themeChanged, this, &SearchWidget::onThemeChanged);
    connect(&Core::ProjectManager::instance(), &Core::ProjectManager::projectRootChanged,
            this, &SearchWidget::setProjectRoot);
    
    setProjectRoot(Core::ProjectManager::instance().projectRoot());
    
    applyTheme();
}

void SearchWidget::setupUI() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto *controlsGrid = new QGridLayout();
    controlsGrid->setSpacing(6);

    
    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText("Search...");
    m_searchButton = new QPushButton("Search", this);
    m_searchButton->setCursor(Qt::PointingHandCursor);
    
    controlsGrid->addWidget(m_searchInput, 0, 0);
    controlsGrid->addWidget(m_searchButton, 0, 1);

    
    m_replaceInput = new QLineEdit(this);
    m_replaceInput->setPlaceholderText("Replace with...");
    m_replaceButton = new QPushButton("Replace", this);
    m_replaceButton->setCursor(Qt::PointingHandCursor);
    m_replaceAllButton = new QPushButton("All", this);
    m_replaceAllButton->setCursor(Qt::PointingHandCursor);

    auto *replaceBtnsLayout = new QHBoxLayout();
    replaceBtnsLayout->setSpacing(4);
    replaceBtnsLayout->addWidget(m_replaceButton);
    replaceBtnsLayout->addWidget(m_replaceAllButton);

    controlsGrid->addWidget(m_replaceInput, 1, 0);
    controlsGrid->addLayout(replaceBtnsLayout, 1, 1);

    layout->addLayout(controlsGrid);

    
    m_caseSensitiveCheck = new QCheckBox("Case Sensitive", this);
    layout->addWidget(m_caseSensitiveCheck);

    
    m_resultsTree = new QTreeWidget(this);
    m_resultsTree->setHeaderLabels({"Location", "Preview"});
    m_resultsTree->setColumnCount(2);
    m_resultsTree->header()->setStretchLastSection(true);
    m_resultsTree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_resultsTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_resultsTree->setAnimated(false);
    m_resultsTree->setUniformRowHeights(true);
    m_resultsTree->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    m_resultsTree->setAlternatingRowColors(true);
    m_resultsTree->setWordWrap(true);
    
    
    m_resultsTree->setItemDelegate(new SearchItemDelegate(m_resultsTree));
    
    layout->addWidget(m_resultsTree);

    
    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);

    connect(m_searchButton, &QPushButton::clicked, this, &SearchWidget::onSearch);
    connect(m_replaceButton, &QPushButton::clicked, this, &SearchWidget::onReplace);
    connect(m_replaceAllButton, &QPushButton::clicked, this, &SearchWidget::onReplaceAll);
    connect(m_resultsTree, &QTreeWidget::itemDoubleClicked, this, &SearchWidget::onItemDoubleClicked);
    connect(m_searchInput, &QLineEdit::returnPressed, this, &SearchWidget::onSearch);
}

void SearchWidget::onThemeChanged() {
	applyTheme();
}

void SearchWidget::applyTheme() {
	m_resultsTree->update();
}

void SearchWidget::setProjectRoot(const QString &root) {
    m_projectRoot = root;
}

void SearchWidget::onSearch() {
    m_resultsTree->clear();
    m_currentResults.clear();
    m_fileItemCache.clear();
    QString query = m_searchInput->text();
    if (query.isEmpty()) return;

    m_statusLabel->setText("Searching...");
    m_searchButton->setEnabled(false);
    m_engine->cancel(); 
    m_engine->search(m_projectRoot, query, m_caseSensitiveCheck->isChecked());
}

void SearchWidget::onReplace() {
    QTreeWidgetItem *current = m_resultsTree->currentItem();
    if (!current || current->childCount() > 0) {
        QMessageBox::information(this, "Replace", "Please select a specific match to replace.");
        return;
    }

    int resultIndex = current->data(0, Qt::UserRole).toInt();
    if (resultIndex < 0 || resultIndex >= m_currentResults.size()) return;

    const SearchResult &res = m_currentResults[resultIndex];
    QString replacement = m_replaceInput->text();
    
    if (m_engine->replaceInFile(res.filePath, res.line, res.column, res.matchLength, replacement)) {
        
        delete current;
        m_statusLabel->setText("Replaced check mark.");
    }
}

void SearchWidget::onReplaceAll() {
    if (m_currentResults.isEmpty()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Replace All",
        QString("Are you sure you want to replace all %1 matches?").arg(m_currentResults.size()),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        int count = m_engine->replaceAll(m_currentResults, m_replaceInput->text());
        m_resultsTree->clear();
        m_currentResults.clear();
        m_statusLabel->setText(QString("Replaced %1 matches. Press Search to refresh.").arg(count));
    }
}

void SearchWidget::onMatchFound(const SearchResult &result) {
    m_currentResults.append(result);
    
    QTreeWidgetItem *fileItem = m_fileItemCache.value(result.filePath);

    if (!fileItem) {
        fileItem = new QTreeWidgetItem(m_resultsTree);
        fileItem->setText(0, QFileInfo(result.filePath).fileName());
        fileItem->setToolTip(0, result.filePath);
        m_fileItemCache.insert(result.filePath, fileItem);
    }

    auto *matchItem = new QTreeWidgetItem(fileItem);
    matchItem->setText(0, QString("Line %1, Col %2").arg(result.line).arg(result.column + 1));
    matchItem->setText(1, result.lineContent);
    matchItem->setData(0, Qt::UserRole, m_currentResults.size() - 1);
    
    fileItem->setExpanded(true);
}

void SearchWidget::onSearchFinished(int totalMatches) {
    if (totalMatches >= SearchEngine::MAX_TOTAL_MATCHES) {
        m_statusLabel->setText(QString("Found %1+ matches (truncated).").arg(totalMatches));
    } else {
        m_statusLabel->setText(QString("Found %1 matches.").arg(totalMatches));
    }
    m_searchButton->setEnabled(true);
}

void SearchWidget::onItemDoubleClicked(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column);
    if (item->childCount() == 0) {
        int index = item->data(0, Qt::UserRole).toInt();
        if (index >= 0 && index < m_currentResults.size()) {
            const SearchResult &res = m_currentResults[index];
            emit fileSelected(res.filePath, res.line, res.column);
        }
    }
}

} 
