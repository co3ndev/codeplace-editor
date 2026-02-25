#ifndef SIMPLE_HIGHLIGHTER_H
#define SIMPLE_HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextBlockUserData>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QVector>
#include <optional>
#include "theme_manager.h"

namespace Core {

class BlockUserData : public QTextBlockUserData {
public:
    int foldingLevel = 0;
    bool foldStart = false;
    bool folded = false;
};

class SimpleHighlighter : public QSyntaxHighlighter {
public:
    enum Language {
        Lang_Cpp,
        Lang_Python,
        Lang_Rust,
        Lang_Go,
        Lang_HTML,
        Lang_CSS,
        Lang_JS,
        Lang_TypeScript,
        Lang_Java,
        Lang_CSharp,
        Lang_Ruby,
        Lang_PHP,
        Lang_Shell,
        Lang_YAML,
        Lang_JSON,
        Lang_XML,
        Lang_SQL,
        Lang_Markdown,
        Lang_Lua,
        Lang_Kotlin,
        Lang_Swift,
        Lang_Dart,
        Lang_Perl
    };

    enum TokenType {
        Token_Default,
        Token_Keyword,
        Token_Type,
        Token_Function,
        Token_Variable,
        Token_String,
        Token_Comment,
        Token_Number,
        Token_Operator,
        Token_Preprocessor,
        Token_Tag,
        Token_Attribute,
        Token_CssProperty,
        Token_CssValue,
        Token_Builtin,
        Token_Annotation
    };

    struct Rule { QRegularExpression pattern; QTextCharFormat format; bool multiLineStart=false; QRegularExpression endPattern; TokenType role = Token_Default; };

    struct SymbolRule {
        QRegularExpression pattern;
        QString type;
        int nameGroup = 1;
    };

    static std::optional<Language> languageForExtension(const QString &ext);
    static std::optional<Language> languageForName(const QString &name);
    static QVector<SymbolRule> getSymbolRules(Language lang);

    SimpleHighlighter(QTextDocument *parent, Language lang);
    ~SimpleHighlighter() override = default;

protected:
    void highlightBlock(const QString &text) override;

private:
    void onThemeChanged();

    QVector<Rule> m_rules;
    Language m_lang;

    void setupRules();
    void updateFormatsFromTheme();
};

} 

#endif 
