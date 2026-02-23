#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <QObject>
#include <QStringList>
#include <QColor>
#include <QMap>

namespace Core {

class ThemeManager : public QObject {
    Q_OBJECT

public:
    enum EditorColor {
        EditorBackground,
        EditorForeground,
        LineNumberBackground,
        LineNumberForeground,
        LineHighlight,
        SelectionBackground,
        SyntaxKeyword,
        SyntaxType,
        SyntaxFunction,
        SyntaxVariable,
        SyntaxString,
        SyntaxComment,
        SyntaxNumber,
        SyntaxOperator,
        SyntaxPreprocessor,
        SyntaxTag,
        SyntaxAttribute,
        SyntaxCssProperty,
        SyntaxCssValue,
        SyntaxBuiltin,
        SyntaxAnnotation,
        StatusBarBackground,
        StatusBarForeground,
        FindBarBackground,
        FindBarForeground,
        FindBarBorder,
        BracketMatch,
        ModifiedIndicator,
        GitModifiedIndicator,
        GitUntrackedIndicator,
        DiffAddedBackground,
        DiffRemovedBackground,
        DiagnosticError,
        DiagnosticWarning,
        DiagnosticInfo,
        DiagnosticHint,
        DiagnosticHoverBackground,
        DiagnosticHoverForeground,
        TerminalBlack, TerminalRed, TerminalGreen, TerminalYellow, TerminalBlue, TerminalMagenta, TerminalCyan, TerminalWhite,
        TerminalBrightBlack, TerminalBrightRed, TerminalBrightGreen, TerminalBrightYellow, TerminalBrightBlue, TerminalBrightMagenta, TerminalBrightCyan, TerminalBrightWhite
    };
    Q_ENUM(EditorColor)

    static ThemeManager& instance();

    QStringList availableThemes() const;
    bool applyTheme(const QString &themeName);
    QString currentTheme() const;

    QColor getColor(EditorColor color) const;

    void refreshThemes();
    
    QString getSharedStyleSheet(const QString &themeName) const;

signals:
    void themeChanged();

private:
    ThemeManager();
    QString m_currentTheme;
    QStringList m_availableThemes;
    QMap<QString, QString> m_themeFiles;
    QMap<EditorColor, QColor> m_colors;

    void ensureThemeFolderExists();
    void loadAvailableThemes();
    QString themePath(const QString &themeName) const;
    bool extractColors();
    
    
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
};

} 

#endif 
