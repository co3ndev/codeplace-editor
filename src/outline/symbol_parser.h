#ifndef SYMBOL_PARSER_H
#define SYMBOL_PARSER_H

#include <QString>
#include <QVector>
#include <QTextDocument>

#include "../core/simple_highlighter.h"

namespace Outline {

struct OutlineSymbol {
    QString name;
    QString type;
    int line;
    int column;
    int length;
};

class SymbolParser {
public:
    static QVector<OutlineSymbol> parse(QTextDocument *doc, const QString &extension = "");
};

} 

#endif 
