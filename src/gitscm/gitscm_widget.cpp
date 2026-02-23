#include "gitscm_widget.h"
#include "create_branch_dialog.h"
#include "git_manager.h"
#include "../core/theme_manager.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QMessageBox>
#include <QMenu>
#include <functional>
#include "../core/project_manager.h"

namespace GitScm {

GitScmWidget::GitScmWidget(QWidget *parent) : QWidget(parent) {
    setupUI();
    
    auto &manager = GitManager::instance();
    connect(&manager, &GitManager::statusUpdated, this, &GitScmWidget::onGitStatusUpdated);
    connect(&manager, &GitManager::branchesUpdated, this, &GitScmWidget::onGitBranchesUpdated);
    connect(&manager, &GitManager::actionFinished, this, &GitScmWidget::onGitActionFinished);

    connect(&Core::ProjectManager::instance(), &Core::ProjectManager::projectRootChanged,
            this, &GitScmWidget::setProjectRoot);
            
    connect(&Core::ThemeManager::instance(), &Core::ThemeManager::themeChanged, this, &GitScmWidget::onThemeChanged);
            
    setProjectRoot(Core::ProjectManager::instance().projectRoot());
}

void GitScmWidget::setupUI() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    
    m_branchComboBox = new QComboBox(this);
    connect(m_branchComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GitScmWidget::onBranchChanged);
    layout->addWidget(m_branchComboBox);

    m_statusLabel = new QLabel("No Git Repository Found", this);
    layout->addWidget(m_statusLabel);

    m_scmTree = new QTreeWidget(this);
    m_scmTree->setHeaderHidden(true);
    m_scmTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_scmTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_scmTree->setAnimated(false);
    m_scmTree->setIndentation(12);
    m_scmTree->setUniformRowHeights(true);
    m_scmTree->setAlternatingRowColors(true);
    connect(m_scmTree, &QTreeWidget::customContextMenuRequested, this, &GitScmWidget::showContextMenu);
    layout->addWidget(m_scmTree);
    connect(m_scmTree, &QTreeWidget::itemDoubleClicked, this, &GitScmWidget::onItemDoubleClicked);

    m_controlsWidget = new QWidget(this);
    m_controlsWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *controlsLayout = new QVBoxLayout(m_controlsWidget);
    controlsLayout->setContentsMargins(0, 0, 0, 0);

    m_commitMessageEdit = new QTextEdit(this);
    m_commitMessageEdit->setPlaceholderText("Commit message...");
    m_commitMessageEdit->setMaximumHeight(60);
    controlsLayout->addWidget(m_commitMessageEdit);

    auto *buttonsLayout = new QHBoxLayout();
    m_stageButton = new QPushButton("Stage", this);
    m_unstageButton = new QPushButton("Unstage", this);
    m_commitButton = new QPushButton("Commit", this);
    
    connect(m_stageButton, &QPushButton::clicked, this, &GitScmWidget::onStageSelected);
    connect(m_unstageButton, &QPushButton::clicked, this, &GitScmWidget::onUnstageSelected);
    connect(m_commitButton, &QPushButton::clicked, this, &GitScmWidget::onCommitClicked);
    
    buttonsLayout->addWidget(m_stageButton);
    buttonsLayout->addWidget(m_unstageButton);
    buttonsLayout->addWidget(m_commitButton);
    controlsLayout->addLayout(buttonsLayout);
    
    layout->addWidget(m_controlsWidget);
    
    m_branchComboBox->setEnabled(false);
    m_controlsWidget->setEnabled(false);
}

void GitScmWidget::setProjectRoot(const QString &root) {
    m_projectRoot = root;
    m_scmTree->clear();
    m_currentFileStatuses.clear();
    m_branchComboBox->clear();
}

QMap<QString, QString> GitScmWidget::getFileStatuses() const {
    return m_currentFileStatuses;
}

void GitScmWidget::onGitStatusUpdated(const QMap<QString, QString> &fileStatuses) {
    QSet<QString> selectedFiles;
    for (auto *item : m_scmTree->selectedItems()) {
        QString relPath = item->data(0, Qt::UserRole).toString();
        if (!relPath.isEmpty()) selectedFiles.insert(relPath);
    }
    
    m_scmTree->clear();
    
    if (m_projectRoot.isEmpty()) {
        m_statusLabel->setText("No Git Repository Found");
        m_controlsWidget->setEnabled(false);
        m_branchComboBox->setEnabled(false);
        if (m_currentFileStatuses != fileStatuses) {
            m_currentFileStatuses = fileStatuses;
            emit statusUpdated(m_currentFileStatuses);
        }
        return;
    }

    auto createHeader = [&](const QString &text) {
        auto *item = new QTreeWidgetItem(m_scmTree);
        item->setText(0, text);
        item->setFirstColumnSpanned(true);
        item->setExpanded(true);
        QFont font = item->font(0);
        font.setBold(true);
        item->setFont(0, font);
        return item;
    };

    auto addPathToTree = [&](QTreeWidgetItem *parent, const QString &fullPath, const QString &state, bool isUntracked) {
        QDir rootDir(m_projectRoot);
        QString relPath = rootDir.relativeFilePath(fullPath);
        QStringList parts = relPath.split("/", Qt::SkipEmptyParts);
        
        QTreeWidgetItem *current = parent;
        for (int i = 0; i < parts.size(); ++i) {
            QString part = parts[i];
            bool isFile = (i == parts.size() - 1);
            
            QTreeWidgetItem *child = nullptr;
            for (int k = 0; k < current->childCount(); ++k) {
                QTreeWidgetItem *c = current->child(k);
                if (c->text(0) == part || (isFile && c->data(0, Qt::UserRole).toString() == relPath)) {
                    child = c;
                    break;
                }
            }
            
            if (!child) {
                child = new QTreeWidgetItem();
                child->setText(0, part);
                if (isFile) {
                    QString displayState = isUntracked ? "?" : QString("[%1]").arg(state.trimmed());
                    child->setText(0, displayState + " " + part);
                    child->setData(0, Qt::UserRole, relPath);
                    child->setData(0, Qt::UserRole + 1, false); 
                } else {
                    child->setData(0, Qt::UserRole + 1, true); 
                }
                
                
                int insertPos = 0;
                for (; insertPos < current->childCount(); ++insertPos) {
                    QTreeWidgetItem *exist = current->child(insertPos);
                    bool existIsFolder = exist->data(0, Qt::UserRole + 1).toBool();
                    bool currentIsFolder = !isFile;
                    
                    if (currentIsFolder && !existIsFolder) break;
                    if (currentIsFolder == existIsFolder) {
                        if (part.compare(exist->text(0), Qt::CaseInsensitive) < 0) break;
                    }
                }
                current->insertChild(insertPos, child);
            }
            current = child;
        }
        
        
        current->setExpanded(true);
    };

    QTreeWidgetItem *stagedHeader = createHeader("STAGED");
    QTreeWidgetItem *changesHeader = createHeader("CHANGES");
    QTreeWidgetItem *untrackedHeader = createHeader("UNTRACKED");

    int stagedCount = 0, changesCount = 0, untrackedCount = 0;

    QStringList sortedPaths = fileStatuses.keys();
    std::sort(sortedPaths.begin(), sortedPaths.end());

    for (const QString &path : sortedPaths) {
        QString state = fileStatuses[path];
        if (state.length() < 2) continue;
        
        char x = state[0].toLatin1();
        char y = state[1].toLatin1();

        if (x != ' ' && x != '?') {
            addPathToTree(stagedHeader, path, QString(x), false);
            stagedCount++;
        }
        if (y != ' ' && y != '?') {
            addPathToTree(changesHeader, path, QString(y), false);
            changesCount++;
        }
        if (x == '?' && y == '?') {
            addPathToTree(untrackedHeader, path, "?", true);
            untrackedCount++;
        }
    }

    if (stagedCount == 0) delete stagedHeader;
    if (changesCount == 0) delete changesHeader;
    if (untrackedCount == 0) delete untrackedHeader;

    
    
    
    m_statusLabel->setText(QString("Changes: %1").arg(fileStatuses.size()));
    m_controlsWidget->setEnabled(true);
    
    if (m_currentFileStatuses != fileStatuses) {
        m_currentFileStatuses = fileStatuses;
        emit statusUpdated(m_currentFileStatuses);
    }
}

void GitScmWidget::onGitBranchesUpdated(const QStringList &branches, const QString &currentBranch) {
    m_updatingBranches = true;
    m_branchComboBox->clear();
    m_branchComboBox->addItems(branches);
    m_branchComboBox->insertSeparator(m_branchComboBox->count());
    m_branchComboBox->addItem("Create branch...");
    
    int currentIndex = branches.indexOf(currentBranch);
    if (currentIndex != -1) {
        m_branchComboBox->setCurrentIndex(currentIndex);
    }
    m_updatingBranches = false;
    m_branchComboBox->setEnabled(true);
}

void GitScmWidget::onGitActionFinished(bool success, const QString &error) {
    if (success) {
        m_commitMessageEdit->clear();
    } else {
        QMessageBox::warning(this, "Git Action Failed", "Git command failed:\n" + error);
    }
}

void GitScmWidget::onItemDoubleClicked(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column);
    if (!item) return;
    QString fileRelPath = item->data(0, Qt::UserRole).toString();
    if (fileRelPath.isEmpty()) return;
    QString fullPath = QDir(m_projectRoot).filePath(fileRelPath);
    emit fileSelected(fullPath);
}

void GitScmWidget::onBranchChanged(int index) {
    if (m_updatingBranches || index < 0) return;
    
    QString branch = m_branchComboBox->itemText(index);
    if (branch.isEmpty()) return;
    
    if (branch == "Create branch...") {
        showCreateBranchDialog();
        return;
    }
    
    GitManager::instance().runAction(QStringList() << "checkout" << branch);
}

void GitScmWidget::showCreateBranchDialog() {
    CreateBranchDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString newBranch = dialog.branchName();
        if (!newBranch.isEmpty()) {
            GitManager::instance().runAction(QStringList() << "checkout" << "-b" << newBranch);
        }
    } else {
        GitManager::instance().refreshBranches();
    }
}

void GitScmWidget::showContextMenu(const QPoint &pos) {
    auto *item = m_scmTree->itemAt(pos);
    if (!item) return;

    QList<QTreeWidgetItem*> selectedItems = m_scmTree->selectedItems();
    if (!selectedItems.contains(item)) {
        m_scmTree->setCurrentItem(item);
        selectedItems = m_scmTree->selectedItems();
        if (selectedItems.isEmpty()) selectedItems.append(item);
    }
    
    QSet<QString> paths;
    std::function<void(QTreeWidgetItem*)> collectPaths = [&](QTreeWidgetItem *i) {
        QString path = i->data(0, Qt::UserRole).toString();
        if (!path.isEmpty()) {
            paths.insert(path);
        } else {
            for (int k = 0; k < i->childCount(); ++k) {
                collectPaths(i->child(k));
            }
        }
    };
    for (auto *si : selectedItems) {
        collectPaths(si);
    }

    QMenu menu(this);
    if (paths.size() == 1) {
        QString fullPath = QDir(m_projectRoot).filePath(*paths.begin());
        menu.addAction("Open Diff", [this, fullPath]() { emit diffRequested(fullPath); });
    }
    
    if (!paths.isEmpty()) {
        menu.addAction("Discard Changes", [this, paths]() {
            QMessageBox::StandardButton ret = QMessageBox::warning(this, "Discard Changes",
                QString("Are you sure you want to discard changes for %1 items?").arg(paths.size()),
                QMessageBox::Yes | QMessageBox::No);
                
            if (ret == QMessageBox::Yes) {
                QStringList args;
                args << "restore" << QStringList(paths.values());
                GitManager::instance().runAction(args);
            }
        });
        menu.addSeparator();
        menu.addAction("Stage", [this, paths]() {
            QStringList args;
            args << "add" << QStringList(paths.values());
            GitManager::instance().runAction(args);
        });
        menu.addAction("Unstage", [this, paths]() {
            QStringList args;
            args << "restore" << "--staged" << QStringList(paths.values());
            GitManager::instance().runAction(args);
        });
    }
    
    menu.exec(m_scmTree->mapToGlobal(pos));
}

void GitScmWidget::onStageSelected() {
    QStringList args;
    args << "add";
    
    QSet<QString> paths;
    std::function<void(QTreeWidgetItem*)> collectPaths = [&](QTreeWidgetItem *item) {
        QString path = item->data(0, Qt::UserRole).toString();
        if (!path.isEmpty()) {
            paths.insert(path);
        } else {
            for (int i = 0; i < item->childCount(); ++i) {
                collectPaths(item->child(i));
            }
        }
    };

    for (auto *item : m_scmTree->selectedItems()) {
        collectPaths(item);
    }

    if (!paths.isEmpty()) {
        args << paths.values();
        GitManager::instance().runAction(args);
    }
}

void GitScmWidget::onUnstageSelected() {
    QStringList args;
    args << "restore" << "--staged";
    
    QSet<QString> paths;
    std::function<void(QTreeWidgetItem*)> collectPaths = [&](QTreeWidgetItem *item) {
        QString path = item->data(0, Qt::UserRole).toString();
        if (!path.isEmpty()) {
            paths.insert(path);
        } else {
            for (int i = 0; i < item->childCount(); ++i) {
                collectPaths(item->child(i));
            }
        }
    };

    for (auto *item : m_scmTree->selectedItems()) {
        collectPaths(item);
    }

    if (!paths.isEmpty()) {
        args << paths.values();
        GitManager::instance().runAction(args);
    }
}

void GitScmWidget::onCommitClicked() {
    QString message = m_commitMessageEdit->toPlainText().trimmed();
    if (message.isEmpty()) {
        QMessageBox::warning(this, "Commit", "Please enter a commit message.");
        return;
    }
    GitManager::instance().runAction(QStringList() << "commit" << "-m" << message);
}

void GitScmWidget::onThemeChanged() {
    m_scmTree->viewport()->update();
}

} 
