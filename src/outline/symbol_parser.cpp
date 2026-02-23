#include "symbol_parser.h"
#include <QRegularExpression>
#include <QTextBlock>

namespace Outline {

QVector<OutlineSymbol> SymbolParser::parse(QTextDocument *doc, const QString &extension) {
    QVector<OutlineSymbol> symbols;
    if (!doc) return symbols;

    Core::SimpleHighlighter::Language lang = Core::SimpleHighlighter::Lang_Cpp; 
    if (!extension.isEmpty()) {
        auto detected = Core::SimpleHighlighter::languageForExtension(extension);
        if (detected) lang = *detected;
    }

    QVector<Core::SimpleHighlighter::SymbolRule> rules = Core::SimpleHighlighter::getSymbolRules(lang);

    for (int i = 0; i < doc->blockCount(); ++i) {
        QTextBlock block = doc->findBlockByNumber(i);
        QString text = block.text();

        for (const auto &rule : rules) {
            QRegularExpressionMatch match = rule.pattern.match(text);
            if (match.hasMatch()) {
                OutlineSymbol sym;
                sym.name = match.captured(rule.nameGroup);
                sym.type = rule.type;
                sym.line = i + 1;
                sym.column = match.capturedStart(rule.nameGroup);
                sym.length = match.capturedLength(rule.nameGroup);
                
                if (!sym.name.isEmpty()) {
                    
                    static const QStringList filters = {"if", "for", "while", "switch", "catch", "else", "try", "return"};
                    if (!filters.contains(sym.name)) {
                        symbols.append(sym);
                        break; 
                    }
                }
            }
        }
    }

    return symbols;
}

} 
