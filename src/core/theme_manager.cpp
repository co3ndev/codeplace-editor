#include "theme_manager.h"
#include <QApplication>
#include <QStyle>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QWidget>
#include <QMetaEnum>
#include <QRegularExpression>
#include "settings_manager.h"

namespace Core {

static const int kThemeVersion = 9;

ThemeManager& ThemeManager::instance() {
    static ThemeManager inst;
    return inst;
}

ThemeManager::ThemeManager() {
    ensureThemeFolderExists();
    loadAvailableThemes();
}

QStringList ThemeManager::availableThemes() const {
    const_cast<ThemeManager*>(this)->refreshThemes();
    return m_availableThemes;
}

bool ThemeManager::applyTheme(const QString &themeName) {
    m_currentTheme = themeName;
    if (!extractColors()) {
        return false;
    }

    QString path = themePath(themeName);
    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << "Could not open theme file:" << path;
        return false;
    }

    QString styleSheet = QLatin1String(file.readAll());
    styleSheet += getSharedStyleSheet(themeName);
    qApp->setStyleSheet(styleSheet);
    
    SettingsManager::instance().setTheme(themeName);
    emit themeChanged();
    return true;
}

QString ThemeManager::currentTheme() const {
    return m_currentTheme;
}

QColor ThemeManager::getColor(EditorColor color) const {
    return m_colors.value(color, Qt::black);
}

void ThemeManager::refreshThemes() {
    loadAvailableThemes();
}

bool ThemeManager::extractColors() {
    bool isDark = !m_currentTheme.contains("Light", Qt::CaseInsensitive);
    
    // Default fallback colors (Dark/Tokyo Night default)
    m_colors[EditorBackground] = isDark ? QColor(26, 27, 38) : QColor(243, 244, 246);
    m_colors[EditorForeground] = isDark ? QColor(169, 177, 214) : QColor(36, 41, 46);
    m_colors[LineNumberBackground] = m_colors[EditorBackground];
    m_colors[LineNumberForeground] = isDark ? QColor(65, 72, 104) : QColor(88, 96, 105);
    m_colors[LineHighlight] = isDark ? QColor(36, 40, 59) : QColor(235, 237, 239);
    m_colors[SelectionBackground] = isDark ? QColor(51, 70, 124) : QColor(204, 227, 255);
    
    
    m_colors[SyntaxKeyword] = isDark ? QColor(122, 162, 247) : QColor(215, 58, 73);
    m_colors[SyntaxType] = isDark ? QColor(125, 207, 255) : QColor(215, 58, 73);
    m_colors[SyntaxFunction] = isDark ? QColor(143, 179, 255) : QColor(111, 66, 193);
    m_colors[SyntaxVariable] = isDark ? QColor(192, 202, 245) : QColor(227, 98, 9);
    m_colors[SyntaxString] = isDark ? QColor(247, 118, 142) : QColor(34, 134, 58);
    m_colors[SyntaxComment] = isDark ? QColor(106, 115, 148) : QColor(106, 115, 125);
    m_colors[SyntaxNumber] = isDark ? QColor(255, 158, 100) : QColor(0, 92, 197);
    m_colors[SyntaxOperator] = isDark ? QColor(137, 221, 255) : QColor(0, 92, 197);
    m_colors[SyntaxPreprocessor] = isDark ? QColor(200, 160, 255) : QColor(0, 92, 197);
    m_colors[SyntaxTag] = isDark ? m_colors[SyntaxType] : QColor(34, 134, 58);
    m_colors[SyntaxAttribute] = isDark ? QColor(255, 213, 128) : QColor(111, 66, 193);
    m_colors[SyntaxCssProperty] = m_colors[SyntaxFunction];
    m_colors[SyntaxCssValue] = m_colors[SyntaxString];
    m_colors[SyntaxBuiltin] = isDark ? QColor(126, 203, 255) : QColor(0, 92, 197);
    m_colors[SyntaxAnnotation] = isDark ? QColor(154, 165, 198) : QColor(106, 115, 125);
    
    m_colors[StatusBarBackground] = isDark ? QColor(26, 27, 38) : QColor(250, 251, 252);
    m_colors[StatusBarForeground] = isDark ? QColor(86, 95, 137) : QColor(88, 96, 105);
    m_colors[FindBarBackground] = isDark ? QColor(36, 40, 59) : QColor(235, 237, 239);
    m_colors[FindBarForeground] = isDark ? QColor(169, 177, 214) : QColor(36, 41, 46);
    m_colors[FindBarBorder] = isDark ? QColor(65, 72, 104) : QColor(209, 213, 218);
    m_colors[BracketMatch] = isDark ? QColor(59, 66, 97) : QColor(192, 192, 192);
    m_colors[ModifiedIndicator] = isDark ? QColor(122, 162, 247) : QColor(3, 102, 214);
    m_colors[GitModifiedIndicator] = isDark ? QColor(224, 175, 104) : QColor(240, 173, 0);
    m_colors[GitUntrackedIndicator] = isDark ? QColor(158, 206, 106) : QColor(34, 134, 58);
    
    m_colors[DiffAddedBackground] = QColor(0, 255, 0, 40);
    m_colors[DiffRemovedBackground] = QColor(255, 0, 0, 40);
    m_colors[DiagnosticError] = isDark ? QColor(247, 118, 142) : QColor(203, 36, 49);
    m_colors[DiagnosticWarning] = isDark ? QColor(224, 175, 104) : QColor(240, 173, 0);
    m_colors[DiagnosticInfo] = isDark ? QColor(122, 162, 247) : QColor(3, 102, 214);
    m_colors[DiagnosticHint] = isDark ? QColor(106, 115, 148) : QColor(106, 115, 125);
    m_colors[DiagnosticHoverBackground] = isDark ? QColor(36, 40, 59) : QColor(243, 244, 246);
    m_colors[DiagnosticHoverForeground] = m_colors[EditorForeground];

    
    m_colors[TerminalBlack] = isDark ? QColor(65, 72, 104) : QColor(36, 41, 46);
    m_colors[TerminalRed] = isDark ? QColor(247, 118, 142) : QColor(203, 36, 49);
    m_colors[TerminalGreen] = isDark ? QColor(158, 206, 106) : QColor(34, 134, 58);
    m_colors[TerminalYellow] = isDark ? QColor(224, 175, 104) : QColor(240, 173, 0);
    m_colors[TerminalBlue] = isDark ? QColor(122, 162, 247) : QColor(3, 102, 214);
    m_colors[TerminalMagenta] = isDark ? QColor(187, 154, 247) : QColor(111, 66, 193);
    m_colors[TerminalCyan] = isDark ? QColor(125, 207, 255) : QColor(5, 122, 141);
    m_colors[TerminalWhite] = isDark ? QColor(169, 177, 214) : QColor(36, 41, 46);
    
    m_colors[TerminalBrightBlack] = isDark ? QColor(86, 95, 137) : QColor(106, 115, 125);
    m_colors[TerminalBrightRed] = isDark ? QColor(255, 128, 149) : QColor(203, 36, 49);
    m_colors[TerminalBrightGreen] = isDark ? QColor(185, 242, 124) : QColor(34, 134, 58);
    m_colors[TerminalBrightYellow] = isDark ? QColor(255, 158, 100) : QColor(240, 173, 0);
    m_colors[TerminalBrightBlue] = isDark ? QColor(137, 221, 255) : QColor(3, 102, 214);
    m_colors[TerminalBrightMagenta] = isDark ? QColor(192, 153, 255) : QColor(111, 66, 193);
    m_colors[TerminalBrightCyan] = isDark ? QColor(13, 185, 215) : QColor(5, 122, 141);
    m_colors[TerminalBrightWhite] = isDark ? QColor(192, 202, 245) : QColor(36, 41, 46);

    QString path = themePath(m_currentTheme);
    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) return true; 
    QString content = file.readAll();
    file.close();

    QMetaEnum metaEnum = QMetaEnum::fromType<EditorColor>();
    QMap<QString, EditorColor> nameToColor;
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        QString key = metaEnum.key(i);
        nameToColor[key.toLower()] = static_cast<EditorColor>(metaEnum.value(i));
        
        if (key.startsWith("Editor")) {
            nameToColor[key.mid(6).toLower()] = static_cast<EditorColor>(metaEnum.value(i));
        }
    }

    QRegularExpression colorRegex(R"((?:qproperty-)?(\w+):\s*([^;]+);)");
    QRegularExpressionMatchIterator it = colorRegex.globalMatch(content);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString propertyName = match.captured(1).toLower();
        QString colorValue = match.captured(2).trimmed();

        if (nameToColor.contains(propertyName)) {
            QColor color(colorValue);
            if (color.isValid()) {
                m_colors[nameToColor[propertyName]] = color;
            }
        }
    }
    return true;
}

void ThemeManager::ensureThemeFolderExists() {
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configPath + "/themes");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    auto needsUpdate = [](const QString &path) {
        if (!QFile::exists(path)) return true;
        QFile file(path);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QString content = file.readAll();
            QRegularExpression versionRx(R"(/\*\s*theme-version:\s*(\d+)\s*\*/)");
            QRegularExpressionMatch m = versionRx.match(content);
            if (m.hasMatch()) {
                return m.captured(1).toInt() < kThemeVersion;
            }
            return true; 
        }
        return true;
    };

    
    QString tokyoNightPath = dir.absoluteFilePath("Tokyo_Night.qss");
    if (needsUpdate(tokyoNightPath)) {
        QFile file(tokyoNightPath);
        if (file.open(QFile::WriteOnly | QFile::Text)) {
            file.write("/* Tokyo Night Theme */\n"
                       "/* theme-version: 9 */\n\n"
                       "ThemeDefinition {\n"
                       "    qproperty-editorBackground: #1a1b26;\n"
                       "    qproperty-editorForeground: #a9b1d6;\n"
                       "    qproperty-lineNumberBackground: #1a1b26;\n"
                       "    qproperty-lineNumberForeground: #414868;\n"
                       "    qproperty-lineHighlight: #24283b;\n"
                       "    qproperty-selectionBackground: #33467c;\n"
                       "    qproperty-syntaxKeyword: #7aa2f7;\n"
                       "    qproperty-syntaxType: #7dcfff;\n"
                       "    qproperty-syntaxFunction: #8fb3ff;\n"
                       "    qproperty-syntaxVariable: #c0caf5;\n"
                       "    qproperty-syntaxString: #f7768e;\n"
                       "    qproperty-syntaxComment: #6a7394;\n"
                       "    qproperty-syntaxNumber: #ff9e64;\n"
                       "    qproperty-syntaxOperator: #89ddff;\n"
                       "    qproperty-syntaxPreprocessor: #c8a0ff;\n"
                       "    qproperty-syntaxTag: #7dcfff;\n"
                       "    qproperty-syntaxAttribute: #ffd580;\n"
                       "    qproperty-syntaxCssProperty: #8fb3ff;\n"
                       "    qproperty-syntaxCssValue: #f7768e;\n"
                       "    qproperty-syntaxBuiltin: #7ecbff;\n"
                       "    qproperty-syntaxAnnotation: #9aa5c6;\n"
                       "    qproperty-statusBarBackground: #1a1b26;\n"
                       "    qproperty-statusBarForeground: #565f89;\n"
                       "    qproperty-findBarBackground: #24283b;\n"
                       "    qproperty-findBarForeground: #a9b1d6;\n"
                       "    qproperty-findBarBorder: #414868;\n"
                       "    qproperty-bracketMatch: #3b4261;\n"
                       "    qproperty-modifiedIndicator: #7aa2f7;\n"
                       "    qproperty-gitModifiedIndicator: #e0af68;\n"
                       "    qproperty-gitUntrackedIndicator: #9ece6a;\n"
                       "    qproperty-diffAddedBackground: rgba(0, 255, 0, 40);\n"
                       "    qproperty-diffRemovedBackground: rgba(255, 0, 0, 40);\n"
                       "    qproperty-diagnosticError: #f7768e;\n"
                       "    qproperty-diagnosticWarning: #e0af68;\n"
                       "    qproperty-diagnosticInfo: #7aa2f7;\n"
                       "    qproperty-diagnosticHint: #6a7394;\n"
                       "    qproperty-diagnosticHoverBackground: #24283b;\n"
                       "    qproperty-diagnosticHoverForeground: #a9b1d6;\n"
                       "    qproperty-terminalBlack: #414868;\n"
                       "    qproperty-terminalRed: #f7768e;\n"
                       "    qproperty-terminalGreen: #9ece6a;\n"
                       "    qproperty-terminalYellow: #e0af68;\n"
                       "    qproperty-terminalBlue: #7aa2f7;\n"
                       "    qproperty-terminalMagenta: #bb9af7;\n"
                       "    qproperty-terminalCyan: #7dcfff;\n"
                       "    qproperty-terminalWhite: #a9b1d6;\n"
                       "    qproperty-terminalBrightBlack: #565f89;\n"
                       "    qproperty-terminalBrightRed: #ff8095;\n"
                       "    qproperty-terminalBrightGreen: #b9f27c;\n"
                       "    qproperty-terminalBrightYellow: #ff9e64;\n"
                       "    qproperty-terminalBrightBlue: #89ddff;\n"
                       "    qproperty-terminalBrightMagenta: #c099ff;\n"
                       "    qproperty-terminalBrightCyan: #0db9d7;\n"
                       "    qproperty-terminalBrightWhite: #c0caf5;\n"
                       "}\n\n"
                       "/* Shared widget styles are centrally applied in ThemeManager::getSharedStyleSheet */\n");
        }
    }

    QString slateDarkPath = dir.absoluteFilePath("Slate_Dark.qss");
    if (needsUpdate(slateDarkPath)) {
        QFile file(slateDarkPath);
        if (file.open(QFile::WriteOnly | QFile::Text)) {
            file.write("/* Slate Dark Theme */\n"
                       "/* theme-version: 9 */\n\n"
                       "ThemeDefinition {\n"
                       "    qproperty-editorBackground: #0f172a;\n"
                       "    qproperty-editorForeground: #f8fafc;\n"
                       "    qproperty-lineNumberBackground: #0f172a;\n"
                       "    qproperty-lineNumberForeground: #475569;\n"
                       "    qproperty-lineHighlight: #1e293b;\n"
                       "    qproperty-selectionBackground: #334155;\n"
                       "    qproperty-syntaxKeyword: #f472b6;\n"
                       "    qproperty-syntaxType: #38bdf8;\n"
                       "    qproperty-syntaxFunction: #38bdf8;\n"
                       "    qproperty-syntaxVariable: #f8fafc;\n"
                       "    qproperty-syntaxString: #4ade80;\n"
                       "    qproperty-syntaxComment: #64748b;\n"
                       "    qproperty-syntaxNumber: #fbbf24;\n"
                       "    qproperty-syntaxOperator: #94a3b8;\n"
                       "    qproperty-syntaxPreprocessor: #c084fc;\n"
                       "    qproperty-syntaxTag: #38bdf8;\n"
                       "    qproperty-syntaxAttribute: #fbbf24;\n"
                       "    qproperty-syntaxCssProperty: #38bdf8;\n"
                       "    qproperty-syntaxCssValue: #4ade80;\n"
                       "    qproperty-syntaxBuiltin: #38bdf8;\n"
                       "    qproperty-syntaxAnnotation: #64748b;\n"
                       "    qproperty-statusBarBackground: #0f172a;\n"
                       "    qproperty-statusBarForeground: #94a3b8;\n"
                       "    qproperty-findBarBackground: #1e293b;\n"
                       "    qproperty-findBarForeground: #f8fafc;\n"
                       "    qproperty-findBarBorder: #334155;\n"
                       "    qproperty-bracketMatch: #1e293b;\n"
                       "    qproperty-modifiedIndicator: #38bdf8;\n"
                       "    qproperty-gitModifiedIndicator: #fbbf24;\n"
                       "    qproperty-gitUntrackedIndicator: #4ade80;\n"
                       "    qproperty-diffAddedBackground: rgba(74, 222, 128, 40);\n"
                       "    qproperty-diffRemovedBackground: rgba(244, 114, 182, 40);\n"
                       "    qproperty-diagnosticError: #f472b6;\n"
                       "    qproperty-diagnosticWarning: #fbbf24;\n"
                       "    qproperty-diagnosticInfo: #38bdf8;\n"
                       "    qproperty-diagnosticHint: #64748b;\n"
                       "    qproperty-diagnosticHoverBackground: #1e293b;\n"
                       "    qproperty-diagnosticHoverForeground: #f8fafc;\n"
                       "    qproperty-terminalBlack: #0f172a;\n"
                       "    qproperty-terminalRed: #f472b6;\n"
                       "    qproperty-terminalGreen: #4ade80;\n"
                       "    qproperty-terminalYellow: #fbbf24;\n"
                       "    qproperty-terminalBlue: #38bdf8;\n"
                       "    qproperty-terminalMagenta: #c084fc;\n"
                       "    qproperty-terminalCyan: #22d3ee;\n"
                       "    qproperty-terminalWhite: #f8fafc;\n"
                       "    qproperty-terminalBrightBlack: #475569;\n"
                       "    qproperty-terminalBrightRed: #fb7185;\n"
                       "    qproperty-terminalBrightGreen: #86efac;\n"
                       "    qproperty-terminalBrightYellow: #fcd34d;\n"
                       "    qproperty-terminalBrightBlue: #7dd3fc;\n"
                       "    qproperty-terminalBrightMagenta: #d8b4fe;\n"
                       "    qproperty-terminalBrightCyan: #67e8f9;\n"
                       "    qproperty-terminalBrightWhite: #ffffff;\n"
                       "}\n\n"
                       "/* Shared widget styles are centrally applied in ThemeManager::getSharedStyleSheet */\n");
        }
    }
}

void ThemeManager::loadAvailableThemes() {
    m_availableThemes.clear();
    m_themeFiles.clear();
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configPath + "/themes");
    
    QStringList filters;
    filters << "*.qss";
    QStringList files = dir.entryList(filters, QDir::Files);
    
    for (const QString &file : files) {
        QString fileName = QFileInfo(file).baseName();
        QString displayName = fileName;
        displayName.replace('_', ' ');
        if (!displayName.isEmpty()) displayName[0] = displayName[0].toUpper();
        
        m_availableThemes.append(displayName);
        m_themeFiles[displayName] = fileName;
    }
}

QString ThemeManager::getSharedStyleSheet(const QString &) const {
    QString shared;
    
    QColor bg = getColor(EditorBackground);
    QColor fg = getColor(EditorForeground);
    QColor selBg = getColor(SelectionBackground);
    
    QString editorBg = bg.name();
    QString editorFg = fg.name();
    QString selectionBg = selBg.name();
    QString selectionFg = (selBg.lightness() > 140) ? "#000000" : "#ffffff";
    
    QString statusBarBg = getColor(StatusBarBackground).name();
    QString statusBarFg = getColor(StatusBarForeground).name();
    
    QString lineHighlight = getColor(LineHighlight).name();
    QString findBarBg = getColor(FindBarBackground).name();
    QString findBarBorder = getColor(FindBarBorder).name();

    QColor alternateCol = bg.darker(bg.lightness() > 128 ? 105 : 115);
    if (alternateCol == bg) {
        alternateCol = bg.lighter(110);
    }
    QString alternateBg = alternateCol.name();

    
    shared += QString("QWidget { background-color: %1; color: %2; }\n").arg(editorBg, editorFg);
    shared += "QLabel { background: transparent; }\n";
    
    
    shared += QString(
        "QPlainTextEdit, QTextEdit { background-color: %1; color: %2; selection-background-color: %3; selection-color: %4; border: none; }\n"
        "QLineEdit { background-color: %5; color: %2; border: 1px solid %6; padding: 4px; border-radius: 4px; }\n"
    ).arg(editorBg, editorFg, selectionBg, selectionFg, alternateBg, findBarBorder);

    
    shared += QString(
        "QDockWidget { color: %2; titlebar-close-icon: none; titlebar-normal-icon: none; }\n"
        "QDockWidget::title { background-color: %1; color: %2; padding: 6px; font-weight: bold; }\n"
        "QStatusBar { background-color: %1; color: %2; border-top: 1px solid %3; }\n"
        "QStatusBar::item { border: none; }\n"
        "QStatusBar QLabel { color: %2; background: transparent; padding: 0 4px; }\n"
    ).arg(statusBarBg, statusBarFg, findBarBorder);

    
    shared += QString(
        "QTreeWidget, QTreeView, QListView { background-color: %1; color: %2; border: none; outline: none; alternate-background-color: %5; }\n"
        "QTreeWidget::item, QTreeView::item, QListView::item { padding: 4px; border: none; }\n"
        "QTreeWidget::item:selected, QTreeView::item:selected, QListView::item:selected { background-color: %3; color: %4; }\n"
        "QTreeWidget::item:hover, QTreeView::item:hover, QListView::item:hover { background-color: %6; }\n"
    ).arg(editorBg, editorFg, selectionBg, selectionFg, alternateBg, lineHighlight);

    
    shared += QString(
        "QHeaderView::section { background-color: %1; color: %2; border: none; padding: 4px; font-weight: bold; }\n"
    ).arg(statusBarBg, statusBarFg);

    
    shared += QString(
        "#fileBrowserHeader { background-color: %1; color: %2; border-bottom: 1px solid %3; }\n"
    ).arg(editorBg, statusBarFg, statusBarBg);

    
    shared += QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; padding: 5px 12px; border-radius: 4px; min-width: 60px; }\n"
        "QPushButton:hover { background-color: %4; border-color: %5; }\n"
        "QPushButton:pressed { background-color: %6; }\n"
        "QPushButton:disabled { color: %7; background-color: %1; }\n"
    ).arg(statusBarBg, statusBarFg, findBarBorder, lineHighlight, selectionBg, selectionBg, alternateBg);

    
    shared += QString(
        "QTabBar::tab { background: %1; color: %2; padding: 6px 12px; margin: 1px; border-radius: 4px; border: 1px solid transparent; }\n"
        "QTabBar::tab:selected { background: %3; color: %4; font-weight: bold; border-color: %5; }\n"
        "QTabBar::tab:hover:!selected { background: %6; }\n"
        "QTabBar[shape=\"RoundedWest\"]::tab { margin-right: 0; border-top-right-radius: 0; border-bottom-right-radius: 0; }\n"
        "QTabBar[shape=\"RoundedEast\"]::tab { margin-left: 0; border-top-left-radius: 0; border-bottom-left-radius: 0; }\n"
    ).arg(statusBarBg, editorFg, selectionBg, selectionFg, findBarBorder, lineHighlight);

    
    shared += QString(
        "QMenuBar { background-color: %1; color: %2; padding: 2px; }\n"
        "QMenuBar::item { background: transparent; padding: 4px 8px; }\n"
        "QMenuBar::item:selected { background-color: %3; border-radius: 4px; }\n"
        "QMenu { background-color: %4; color: %2; border: 1px solid %5; padding: 4px; }\n"
        "QMenu::item { padding: 6px 24px; border-radius: 3px; }\n"
        "QMenu::item:selected { background-color: %6; color: %7; }\n"
        "QMenu::separator { height: 1px; background: %5; margin: 4px 8px; }\n"
    ).arg(statusBarBg, editorFg, lineHighlight, editorBg, findBarBorder, selectionBg, selectionFg);

    
    shared += QString(
        "QScrollBar:vertical { background: %1; width: 12px; margin: 0; }\n"
        "QScrollBar::handle:vertical { background: %2; min-height: 20px; border-radius: 6px; margin: 2px; }\n"
        "QScrollBar::handle:vertical:hover { background: %3; }\n"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }\n"
        "QScrollBar:horizontal { background: %1; height: 12px; margin: 0; }\n"
        "QScrollBar::handle:horizontal { background: %2; min-width: 20px; border-radius: 6px; margin: 2px; }\n"
        "QScrollBar::handle:horizontal:hover { background: %3; }\n"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }\n"
    ).arg(editorBg, findBarBorder, statusBarFg);

    return shared;
}

QString ThemeManager::themePath(const QString &themeName) const {
    QString fileName = m_themeFiles.value(themeName, themeName);
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return configPath + "/themes/" + fileName + ".qss";
}

} 
