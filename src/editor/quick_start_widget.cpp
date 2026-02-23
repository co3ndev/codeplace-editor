#include "quick_start_widget.h"
#include <QPainter>
#include "core/theme_manager.h"
#include "core/default_shortcuts.h"

namespace Editor {

QuickStartWidget::QuickStartWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void QuickStartWidget::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);

    auto *container = new QWidget(this);
    auto *containerLayout = new QVBoxLayout(container);
    containerLayout->setSpacing(20);
    containerLayout->setContentsMargins(40, 40, 40, 40);

    auto *titleLabel = new QLabel("Quick Start", container);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    containerLayout->addWidget(titleLabel);

    auto *shortcutGrid = new QGridLayout();
    shortcutGrid->setSpacing(15);
    shortcutGrid->setContentsMargins(20, 10, 20, 10);

    auto shortcuts = Core::defaultShortcuts();
    int row = 0;
    addShortcutRow(shortcutGrid, row++, "New File", shortcuts.value("New"));
    addShortcutRow(shortcutGrid, row++, "Open File", shortcuts.value("Open"));
    addShortcutRow(shortcutGrid, row++, "Open Folder", shortcuts.value("Open Folder"));
    addShortcutRow(shortcutGrid, row++, "Settings", shortcuts.value("Settings"));

    containerLayout->addLayout(shortcutGrid);
    mainLayout->addWidget(container);

    
    
    connect(&Core::ThemeManager::instance(), &Core::ThemeManager::themeChanged, this, qOverload<>(&QuickStartWidget::update));
}

void QuickStartWidget::addShortcutRow(QGridLayout *layout, int row, const QString &labelStr, const QString &shortcutStr) {
    auto *label = new QLabel(labelStr);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    auto *shortcut = new QLabel(shortcutStr);
    shortcut->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    shortcut->setStyleSheet("font-weight: bold;");
    
    layout->addWidget(label, row, 0);
    layout->addWidget(shortcut, row, 1);
}

void QuickStartWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    auto &tm = Core::ThemeManager::instance();
    painter.fillRect(rect(), tm.getColor(Core::ThemeManager::EditorBackground));
}

} 
