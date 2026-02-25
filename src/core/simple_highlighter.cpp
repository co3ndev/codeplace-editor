#include "simple_highlighter.h"
#include <QTextDocument>
#include <QDebug>

namespace Core {
 
std::optional<SimpleHighlighter::Language> SimpleHighlighter::languageForExtension(const QString &ext) {
    static const QMap<QString, Language> map = {
        {"c",    Lang_Cpp}, {"h",    Lang_Cpp}, {"cpp",  Lang_Cpp}, {"cc",   Lang_Cpp}, {"cxx",  Lang_Cpp},
        {"hpp",  Lang_Cpp}, {"hxx",  Lang_Cpp}, {"hh",   Lang_Cpp}, {"ipp",  Lang_Cpp},
        {"py",   Lang_Python}, {"pyw",  Lang_Python}, {"pyi",  Lang_Python},
        {"rs",   Lang_Rust}, {"go",   Lang_Go},
        {"html", Lang_HTML}, {"htm",  Lang_HTML},
        {"css",  Lang_CSS}, {"scss", Lang_CSS}, {"less", Lang_CSS}, {"sass", Lang_CSS},
        {"js",   Lang_JS}, {"jsx",  Lang_JS}, {"mjs",  Lang_JS}, {"cjs",  Lang_JS},
        {"ts",   Lang_TypeScript}, {"tsx",  Lang_TypeScript}, {"mts",  Lang_TypeScript}, {"cts",  Lang_TypeScript},
        {"java", Lang_Java}, {"cs",   Lang_CSharp}, {"csx",  Lang_CSharp},
        {"rb",   Lang_Ruby}, {"rake", Lang_Ruby}, {"gemspec", Lang_Ruby},
        {"php",  Lang_PHP}, {"phtml",Lang_PHP},
        {"sh",   Lang_Shell}, {"bash", Lang_Shell}, {"zsh",  Lang_Shell}, {"fish", Lang_Shell},
        {"yml",  Lang_YAML}, {"yaml", Lang_YAML},
        {"json", Lang_JSON}, {"jsonc",Lang_JSON}, {"jsonl",Lang_JSON},
        {"xml",  Lang_XML}, {"xsl",  Lang_XML}, {"xsd",  Lang_XML}, {"svg",  Lang_XML}, {"xhtml",Lang_XML}, {"qrc", Lang_XML},
        {"sql",  Lang_SQL}, {"md",   Lang_Markdown}, {"markdown", Lang_Markdown},
        {"lua",  Lang_Lua}, {"kt",   Lang_Kotlin}, {"kts",  Lang_Kotlin},
        {"swift",Lang_Swift}, {"dart", Lang_Dart}, {"pl",   Lang_Perl}, {"pm",   Lang_Perl}, {"t",    Lang_Perl},
    };
 
    auto it = map.find(ext.toLower());
    if (it != map.end()) {
        return it.value();
    }
    return std::nullopt;
}
 
std::optional<SimpleHighlighter::Language> SimpleHighlighter::languageForName(const QString &name) {
    QString n = name.toLower().trimmed();
    if (n.isEmpty()) return std::nullopt;

    static const QMap<QString, Language> nameMap = {
        {"cpp", Lang_Cpp}, {"c++", Lang_Cpp}, {"c", Lang_Cpp},
        {"python", Lang_Python}, {"py", Lang_Python},
        {"rust", Lang_Rust}, {"rs", Lang_Rust},
        {"go", Lang_Go}, {"golang", Lang_Go},
        {"html", Lang_HTML}, {"htm", Lang_HTML},
        {"css", Lang_CSS},
        {"javascript", Lang_JS}, {"js", Lang_JS}, {"node", Lang_JS},
        {"typescript", Lang_TypeScript}, {"ts", Lang_TypeScript},
        {"java", Lang_Java},
        {"csharp", Lang_CSharp}, {"cs", Lang_CSharp}, {"c#", Lang_CSharp},
        {"ruby", Lang_Ruby}, {"rb", Lang_Ruby},
        {"php", Lang_PHP},
        {"bash", Lang_Shell}, {"sh", Lang_Shell}, {"shell", Lang_Shell}, {"zsh", Lang_Shell},
        {"yaml", Lang_YAML}, {"yml", Lang_YAML},
        {"json", Lang_JSON},
        {"xml", Lang_XML},
        {"sql", Lang_SQL},
        {"markdown", Lang_Markdown}, {"md", Lang_Markdown},
        {"lua", Lang_Lua},
        {"kotlin", Lang_Kotlin}, {"kt", Lang_Kotlin},
        {"swift", Lang_Swift},
        {"dart", Lang_Dart},
        {"perl", Lang_Perl}, {"pl", Lang_Perl}
    };

    auto it = nameMap.find(n);
    if (it != nameMap.end()) return it.value();
    
    // Fallback to extension check if name doesn't match
    return languageForExtension(n);
}

SimpleHighlighter::SimpleHighlighter(QTextDocument *parent, Language lang)
    : QSyntaxHighlighter(parent), m_lang(lang) {
    setupRules();
    updateFormatsFromTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &SimpleHighlighter::onThemeChanged);
}

void SimpleHighlighter::onThemeChanged() {
    updateFormatsFromTheme();
    rehighlight();
}

static void addKeywordRule(QVector<SimpleHighlighter::Rule> &rules, const QStringList &words, SimpleHighlighter::TokenType role) {
    if (words.isEmpty()) return;
    QStringList sortedWords = words;
    std::sort(sortedWords.begin(), sortedWords.end(), [](const QString &a, const QString &b) {
        return a.length() > b.length();
    });
    SimpleHighlighter::Rule r;
    r.pattern = QRegularExpression("\\b(" + sortedWords.join('|') + ")\\b");
    r.role = role;
    rules.append(r);
}

static void addPatternRule(QVector<SimpleHighlighter::Rule> &rules, const QString &pattern, SimpleHighlighter::TokenType role, QRegularExpression::PatternOptions opts = QRegularExpression::NoPatternOption) {
    SimpleHighlighter::Rule r;
    r.pattern = QRegularExpression(pattern, opts);
    if (!r.pattern.isValid()) {
        qWarning() << "Invalid regex pattern:" << pattern << "Error:" << r.pattern.errorString();
        return;
    }
    r.role = role;
    rules.append(r);
}

static void addMultiLineRule(QVector<SimpleHighlighter::Rule> &rules, const QString &startPattern, const QString &endPattern, SimpleHighlighter::TokenType role) {
    SimpleHighlighter::Rule r;
    r.pattern = QRegularExpression(startPattern);
    r.multiLineStart = true;
    r.endPattern = QRegularExpression(endPattern);
    r.role = role;
    rules.append(r);
}

static void addCStyleComments(QVector<SimpleHighlighter::Rule> &rules) {
    addPatternRule(rules, "//.*$", SimpleHighlighter::Token_Comment, QRegularExpression::MultilineOption);
    addMultiLineRule(rules, "/\\*", "\\*/", SimpleHighlighter::Token_Comment);
    addPatternRule(rules, "\\b(TODO|FIXME|NOTE|XXX|BUG|HACK|OPTIMIZE|CRITICAL)\\b", SimpleHighlighter::Token_Annotation);
}

static void addHashComments(QVector<SimpleHighlighter::Rule> &rules) {
    addPatternRule(rules, "#.*$", SimpleHighlighter::Token_Comment, QRegularExpression::MultilineOption);
    addPatternRule(rules, "\\b(TODO|FIXME|NOTE|XXX|BUG|HACK|OPTIMIZE|CRITICAL)\\b", SimpleHighlighter::Token_Annotation);
}

static void addCommonNumbers(QVector<SimpleHighlighter::Rule> &rules) {
    addPatternRule(rules, "\\b0[xX][0-9a-fA-F_]+\\b", SimpleHighlighter::Token_Number);
    addPatternRule(rules, "\\b0[bB][01_]+\\b", SimpleHighlighter::Token_Number);
    addPatternRule(rules, "\\b0[oO][0-7_]+\\b", SimpleHighlighter::Token_Number);
    addPatternRule(rules, "\\b\\d[0-9_]*(\\.[0-9_]*)?([eE][+-]?[0-9_]+)?[fFLuU]*\\b", SimpleHighlighter::Token_Number);
}

void SimpleHighlighter::setupRules() {
    m_rules.clear();

    
    
    

    if (m_lang == Lang_Cpp) {
        
        addCommonNumbers(m_rules);
        addPatternRule(m_rules, "[\\+\\-\\*/%&\\|!<>\\^~=]+", Token_Operator);
        addPatternRule(m_rules, "\\b[a-zA-Z_][a-zA-Z0-9_]*::", Token_Type);
        addPatternRule(m_rules, "\\b[A-Z][a-zA-Z0-9_]*\\b", Token_Type);
        addPatternRule(m_rules, "\\b([A-Za-z_][A-Za-z0-9_]*)\\s*(?=\\()", Token_Function);

        
        QStringList types = {
            "int", "long", "short", "char", "void", "float", "double", "bool",
            "size_t", "ssize_t", "off_t", "ptrdiff_t", "intptr_t", "uintptr_t",
            "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
            "char8_t", "char16_t", "char32_t", "wchar_t", "auto"
        };
        addKeywordRule(m_rules, types, Token_Type);

        QStringList keywords = {
            "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit", "atomic_noexcept",
            "bitand", "bitor", "break", "case", "catch", "class", "compl", "concept", "const", "consteval",
            "constexpr", "constinit", "const_cast", "continue", "co_await", "co_return", "co_yield", "decltype",
            "default", "delete", "do", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false",
            "final", "for", "friend", "goto", "if", "inline", "mutable", "namespace", "new", "noexcept", "not",
            "not_eq", "nullptr", "operator", "or", "or_eq", "override", "private", "protected", "public", "reflexpr",
            "register", "reinterpret_cast", "requires", "return", "signed", "sizeof", "static", "static_assert",
            "static_cast", "struct", "switch", "synchronized", "template", "this", "thread_local", "throw", "true",
            "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "volatile", "while",
            "xor", "xor_eq"
        };
        addKeywordRule(m_rules, keywords, Token_Keyword);

        QStringList builtins = {
            "std", "string", "vector", "map", "set", "list", "deque", "stack", "queue", "priority_queue",
            "unordered_map", "unordered_set", "unique_ptr", "shared_ptr", "weak_ptr", "make_unique", "make_shared",
            "cout", "cin", "cerr", "clog", "endl", "printf", "scanf", "malloc", "free", "memcpy", "memset"
        };
        addKeywordRule(m_rules, builtins, Token_Builtin);

        
        addPatternRule(m_rules, "\\b([A-Z_][A-Z0-9_]{2,})\\b", Token_Variable); 
        addPatternRule(m_rules, "\"([^\"\\\\]|\\\\.)*\"", Token_String);
        addPatternRule(m_rules, "'([^'\\\\]|\\\\.)*'", Token_String);
        addMultiLineRule(m_rules, "(?:u8|u|U|L)?R\\\"[^\\(]{0,16}\\(", "\\)\\\"[^\\)]{0,16}", Token_String); 
        addPatternRule(m_rules, "^\\s*#\\s*[a-zA-Z]+", Token_Preprocessor, QRegularExpression::MultilineOption);
        addCStyleComments(m_rules);

    } else if (m_lang == Lang_Python) {
        addCommonNumbers(m_rules);
        addPatternRule(m_rules, "[\\+\\-\\*/%&\\|!<>\\^~=]+", Token_Operator);
        addPatternRule(m_rules, "\\b([A-Za-z_][A-Za-z0-9_]*)\\s*(?=\\()", Token_Function);

        QStringList keywords = {
            "and", "as", "assert", "async", "await", "break", "class", "continue", "def", "del", "elif",
            "else", "except", "False", "finally", "for", "from", "global", "if", "import", "in", "is",
            "lambda", "None", "nonlocal", "not", "or", "pass", "raise", "return", "True", "try", "while",
            "with", "yield", "match", "case"
        };
        addKeywordRule(m_rules, keywords, Token_Keyword);

        QStringList builtins = {
            "abs", "all", "any", "ascii", "bin", "bool", "breakpoint", "bytearray", "bytes", "callable",
            "chr", "classmethod", "compile", "complex", "delattr", "dict", "dir", "divmod", "enumerate",
            "eval", "exec", "filter", "float", "format", "frozenset", "getattr", "globals", "hasattr",
            "hash", "help", "hex", "id", "input", "int", "isinstance", "issubclass", "iter", "len",
            "list", "locals", "map", "max", "memoryview", "min", "next", "object", "oct", "open",
            "ord", "pow", "print", "property", "range", "repr", "reversed", "round", "set", "setattr",
            "slice", "sorted", "staticmethod", "str", "sum", "super", "tuple", "type", "vars", "zip",
            "self", "cls"
        };
        addKeywordRule(m_rules, builtins, Token_Builtin);

        addPatternRule(m_rules, "@[A-Za-z_][A-Za-z0-9_.]*", Token_Annotation);
        addPatternRule(m_rules, "\"[^\"]*\"", Token_String);
        addPatternRule(m_rules, "'[^']*'", Token_String);
        addMultiLineRule(m_rules, "'''", "'''", Token_String);
        addMultiLineRule(m_rules, "\"\"\"", "\"\"\"", Token_String);
        addHashComments(m_rules);

    } else if (m_lang == Lang_Rust) {
        addCommonNumbers(m_rules);
        addPatternRule(m_rules, "[\\+\\-\\*/%&\\|!<>\\^~=]+", Token_Operator);
        addPatternRule(m_rules, "\\b([A-Za-z_][A-Za-z0-9_]*)\\s*(?=\\()", Token_Function);
        addPatternRule(m_rules, "\\b[A-Z][a-zA-Z0-9_]*\\b", Token_Type);

        QStringList keywords = {
            "as", "break", "const", "continue", "crate", "else", "enum", "extern", "false", "fn", "for",
            "if", "impl", "in", "let", "loop", "match", "mod", "move", "mut", "pub", "ref", "return",
            "self", "Self", "static", "struct", "super", "trait", "true", "type", "unsafe", "use",
            "where", "while", "async", "await", "dyn", "abstract", "become", "box", "do", "final",
            "macro", "override", "priv", "typeof", "unsized", "virtual", "yield", "try"
        };
        addKeywordRule(m_rules, keywords, Token_Keyword);

        QStringList types = {
            "i8", "i16", "i32", "i64", "i128", "isize", "u8", "u16", "u32", "u64", "u128", "usize",
            "f32", "f64", "bool", "char", "str", "String", "Option", "Result", "Box", "Vec", "Rc", "Arc"
        };
        addKeywordRule(m_rules, types, Token_Type);

        addPatternRule(m_rules, "\\b[a-z_][a-z0-9_]*!", Token_Builtin); 
        addPatternRule(m_rules, "'[a-zA-Z_][a-zA-Z0-9_]*", Token_Annotation); 
        addPatternRule(m_rules, "#\\!?\\[[^\\]]*\\]", Token_Annotation); 
        addPatternRule(m_rules, "\"([^\"\\\\]|\\\\.)*\"", Token_String);
        addMultiLineRule(m_rules, "b?r#*\\\"", "\\\"#*", Token_String);
        addCStyleComments(m_rules);

    } else if (m_lang == Lang_JS || m_lang == Lang_TypeScript) {
        addCommonNumbers(m_rules);
        addPatternRule(m_rules, "[\\+\\-\\*/%&\\|!<>\\^~=]+", Token_Operator);
        addPatternRule(m_rules, "\\b([A-Za-z_][A-Za-z0-9_]*)\\s*(?=\\()", Token_Function);

        QStringList keywords = {
            "break", "case", "catch", "class", "const", "continue", "debugger", "default", "delete",
            "do", "else", "export", "extends", "finally", "for", "function", "if", "import", "in",
            "instanceof", "new", "return", "super", "switch", "this", "throw", "try", "typeof",
            "var", "void", "while", "with", "yield", "async", "await", "let", "static", "of", "from"
        };
        if (m_lang == Lang_TypeScript) {
            keywords << "as" << "implements" << "interface" << "package" << "private" << "protected" 
                     << "public" << "type" << "enum" << "declare" << "readonly" << "abstract" 
                     << "override" << "namespace" << "module" << "keyof" << "infer" << "is" << "asserts";
        }
        addKeywordRule(m_rules, keywords, Token_Keyword);

        QStringList builtins = {
            "console", "window", "document", "Math", "JSON", "Promise", "Object", "Array", "String",
            "Number", "Boolean", "Date", "RegExp", "Error", "Map", "Set", "WeakMap", "WeakSet",
            "fetch", "setTimeout", "setInterval", "clearTimeout", "clearInterval"
        };
        addKeywordRule(m_rules, builtins, Token_Builtin);

        addPatternRule(m_rules, "@[A-Za-z_][A-Za-z0-9_.]*", Token_Annotation);
        addPatternRule(m_rules, "\"([^\"\\\\]|\\\\.)*\"", Token_String);
        addPatternRule(m_rules, "'([^'\\\\]|\\\\.)*'", Token_String);
        addMultiLineRule(m_rules, "`", "`", Token_String); 
        addCStyleComments(m_rules);

    } else if (m_lang == Lang_HTML || m_lang == Lang_XML) {
        addPatternRule(m_rules, "<!--[\\s\\S]*?-->", Token_Comment);
        addPatternRule(m_rules, "<!DOCTYPE[^>]*>", Token_Preprocessor, QRegularExpression::CaseInsensitiveOption);
        addPatternRule(m_rules, "<\\?[^?]*\\?>", Token_Preprocessor);
        addPatternRule(m_rules, "<\\/?[A-Za-z0-9\\-.:]+", Token_Tag);
        addPatternRule(m_rules, "\\/?>", Token_Tag);
        addPatternRule(m_rules, "\\b[a-zA-Z0-9\\-.:]+(?=\\s*=)", Token_Attribute);
        addPatternRule(m_rules, "\"([^\"\\\\]|\\\\.)*\"", Token_String);
        addPatternRule(m_rules, "'([^'\\\\]|\\\\.)*'", Token_String);
        addPatternRule(m_rules, "&[#A-Za-z0-9]+;", Token_Attribute);

    } else if (m_lang == Lang_CSS) {
        addPatternRule(m_rules, "/\\*[\\s\\S]*?\\*/", Token_Comment);
        addPatternRule(m_rules, "[.#][A-Za-z_][A-Za-z0-9_-]*", Token_Tag);
        addPatternRule(m_rules, "@[a-z-]+", Token_Keyword);
        addPatternRule(m_rules, "\\b[a-z-]+(?=\\s*:)", Token_CssProperty);
        addPatternRule(m_rules, ":\\s*([^;]+)", Token_CssValue);
        addPatternRule(m_rules, "\\b\\d+(\\.\\d+)?(px|em|rem|%|vh|vw|s|ms|deg)?\\b", Token_Number);
        addPatternRule(m_rules, "--[A-Za-z0-9_-]+", Token_Variable);

    } else if (m_lang == Lang_Markdown) {
        
        addPatternRule(m_rules, "^\\[[ xX]\\] ", Token_Operator, QRegularExpression::MultilineOption);
        addPatternRule(m_rules, "^#{1,6}\\s+.*$", Token_Keyword, QRegularExpression::MultilineOption);
        addPatternRule(m_rules, "(\\b|\\s)(\\*\\*|__)[^*_]+(\\*\\*|__)", Token_Keyword); 
        addPatternRule(m_rules, "(\\b|\\s)(\\*|_)[^*_]+(\\*|_)", Token_Attribute); 
        addPatternRule(m_rules, "!\\[[^\\]]*\\]\\([^)]+\\)", Token_Function); 
        addPatternRule(m_rules, "\\[[^\\]]+\\]\\([^)]+\\)", Token_Function); 
        addPatternRule(m_rules, "`[^`]+`", Token_String); 
        addMultiLineRule(m_rules, "```", "```", Token_String); 
        addPatternRule(m_rules, "^\\s*>.*$", Token_Comment, QRegularExpression::MultilineOption); 
        addPatternRule(m_rules, "^\\s*[-*+]\\s", Token_Operator, QRegularExpression::MultilineOption); 
        addPatternRule(m_rules, "^\\s*\\d+\\.\\s", Token_Operator, QRegularExpression::MultilineOption); 
        addPatternRule(m_rules, "^\\s*(---+|\\*\\*\\*+|___+)\\s*$", Token_Operator, QRegularExpression::MultilineOption); 

    } else if (m_lang == Lang_Go) {
        addCommonNumbers(m_rules);
        addPatternRule(m_rules, "[\\+\\-\\*/%&\\|!<>\\^~=]+", Token_Operator);
        addPatternRule(m_rules, "\\b([A-Za-z_][A-Za-z0-9_]*)\\s*(?=\\()", Token_Function);

        QStringList keywords = {
            "break", "default", "func", "interface", "select", "case", "defer", "go", "map", "struct",
            "chan", "else", "goto", "package", "switch", "const", "fallthrough", "if", "range", "type",
            "continue", "for", "import", "return", "var"
        };
        addKeywordRule(m_rules, keywords, Token_Keyword);

        QStringList types = {
            "bool", "byte", "complex64", "complex128", "error", "float32", "float64", "int", "int8",
            "int16", "int32", "int64", "rune", "string", "uint", "uint8", "uint16", "uint32", "uint64",
            "uintptr", "any", "comparable"
        };
        addKeywordRule(m_rules, types, Token_Type);

        addPatternRule(m_rules, "\"([^\"\\\\]|\\\\.)*\"", Token_String);
        addMultiLineRule(m_rules, "`", "`", Token_String);
        addCStyleComments(m_rules);

    } else if (m_lang == Lang_Java || m_lang == Lang_Kotlin) {
        addCommonNumbers(m_rules);
        addPatternRule(m_rules, "[\\+\\-\\*/%&\\|!<>\\^~=]+", Token_Operator);
        addPatternRule(m_rules, "\\b([A-Za-z_][A-Za-z0-9_]*)\\s*(?=\\()", Token_Function);
        addPatternRule(m_rules, "\\b[A-Z][a-zA-Z0-9_]*\\b", Token_Type);

        QStringList keywords = {
            "abstract", "assert", "break", "case", "catch", "class", "const", "continue", "default",
            "do", "else", "enum", "extends", "final", "finally", "for", "goto", "if", "implements",
            "import", "instanceof", "interface", "native", "new", "package", "private", "protected",
            "public", "return", "static", "strictfp", "super", "switch", "synchronized", "this",
            "throw", "throws", "transient", "try", "volatile", "while", "yield", "record", "var"
        };
        if (m_lang == Lang_Kotlin) {
            keywords << "fun" << "val" << "when" << "is" << "in" << "it" << "suspend" << "object" << "companion";
        }
        addKeywordRule(m_rules, keywords, Token_Keyword);

        addPatternRule(m_rules, "@[A-Za-z_][A-Za-z0-9_.]*", Token_Annotation);
        addPatternRule(m_rules, "\"([^\"\\\\]|\\\\.)*\"", Token_String);
        addCStyleComments(m_rules);

    } else if (m_lang == Lang_CSharp) {
        addCommonNumbers(m_rules);
        addPatternRule(m_rules, "[\\+\\-\\*/%&\\|!<>\\^~=]+", Token_Operator);
        addPatternRule(m_rules, "\\b([A-Za-z_][A-Za-z0-9_]*)\\s*(?=\\()", Token_Function);
        addPatternRule(m_rules, "\\b[A-Z][a-zA-Z0-9_]*\\b", Token_Type);

        QStringList keywords = {
            "abstract", "as", "base", "bool", "break", "byte", "case", "catch", "char", "checked",
            "class", "const", "continue", "decimal", "default", "delegate", "do", "double", "else",
            "enum", "event", "explicit", "extern", "false", "finally", "fixed", "float", "for",
            "foreach", "goto", "if", "implicit", "in", "int", "interface", "internal", "is", "lock",
            "long", "namespace", "new", "null", "object", "operator", "out", "override", "params",
            "private", "protected", "public", "readonly", "ref", "return", "sbyte", "sealed", "short",
            "sizeof", "stackalloc", "static", "string", "struct", "switch", "this", "throw", "true",
            "try", "typeof", "uint", "ulong", "unchecked", "unsafe", "ushort", "using", "virtual",
            "void", "volatile", "while", "add", "alias", "ascending", "async", "await", "by",
            "descending", "dynamic", "from", "get", "global", "group", "into", "join", "let",
            "nameof", "on", "orderby", "partial", "remove", "select", "set", "value", "var", "when",
            "where", "yield", "record"
        };
        addKeywordRule(m_rules, keywords, Token_Keyword);

        addPatternRule(m_rules, "\\[[A-Za-z_][A-Za-z0-9_.]*\\]", Token_Annotation);
        addPatternRule(m_rules, "\"([^\"\\\\]|\\\\.)*\"", Token_String);
        addPatternRule(m_rules, "@\"[^\"]*\"", Token_String); 
        addCStyleComments(m_rules);

    } else if (m_lang == Lang_YAML || m_lang == Lang_JSON) {
        if (m_lang == Lang_JSON) {
            addPatternRule(m_rules, "-?\\d+(\\.\\d+)?([eE][+-]?\\d+)?", Token_Number);
            addPatternRule(m_rules, "\\b(true|false|null)\\b", Token_Builtin);
            addPatternRule(m_rules, "\"([^\"\\\\]|\\\\.)*\"", Token_String);
            addPatternRule(m_rules, "\"[^\"]*\"(?=\\s*:)", Token_Keyword);
        } else {
            addHashComments(m_rules);
            addPatternRule(m_rules, "\\b\\d+(\\.\\d+)?\\b", Token_Number);
            addPatternRule(m_rules, "\\b(true|false|yes|no|on|off|null|~)\\b", Token_Builtin);
            addPatternRule(m_rules, "^\\s*[a-zA-Z0-9_-]+(?=\\s*:)", Token_Keyword);
            addPatternRule(m_rules, "[&*][a-zA-Z0-9_-]+", Token_Annotation);
            addPatternRule(m_rules, "\"([^\"\\\\]|\\\\.)*\"", Token_String);
            addPatternRule(m_rules, "'([^'\\\\]|\\\\.)*'", Token_String);
        }

    } else if (m_lang == Lang_SQL) {
        QStringList keywords = {
             "SELECT", "FROM", "WHERE", "AND", "OR", "NOT", "INSERT", "INTO", "VALUES", "UPDATE", "SET",
             "DELETE", "CREATE", "ALTER", "DROP", "TABLE", "DATABASE", "INDEX", "VIEW", "JOIN", "ON",
             "GROUP", "BY", "ORDER", "HAVING", "LIMIT", "OFFSET", "UNION", "ALL", "DISTINCT", "AS"
        };
        addKeywordRule(m_rules, keywords, Token_Keyword);
        addPatternRule(m_rules, "--.*$", Token_Comment, QRegularExpression::MultilineOption);
        addPatternRule(m_rules, "'([^'\\\\]|\\\\.)*'", Token_String);
        addCommonNumbers(m_rules);

    } else if (m_lang == Lang_Ruby) {
        addCommonNumbers(m_rules);
        addPatternRule(m_rules, "[\\+\\-\\*/%&\\|!<>\\^~=]+", Token_Operator);
        addPatternRule(m_rules, "\\b([A-Za-z_][A-Za-z0-9_]*)\\s*(?=\\()", Token_Function);

        QStringList keywords = {
            "alias", "and", "begin", "break", "case", "class", "def", "defined?", "do", "else", "elsif",
            "end", "ensure", "false", "for", "if", "in", "module", "next", "nil", "not", "or", "redo",
            "rescue", "retry", "return", "self", "super", "then", "true", "undef", "unless", "until",
            "when", "while", "yield", "raise", "require", "require_relative", "include", "extend"
        };
        addKeywordRule(m_rules, keywords, Token_Keyword);

        addPatternRule(m_rules, ":[a-zA-Z_][a-zA-Z0-9_]*", Token_Annotation); 
        addPatternRule(m_rules, "@@?[a-zA-Z_][a-zA-Z0-9_]*", Token_Variable); 
        addPatternRule(m_rules, "\\$[a-zA-Z_][a-zA-Z0-9_]*", Token_Variable); 
        addPatternRule(m_rules, "\"([^\"\\\\]|\\\\.)*\"", Token_String);
        addPatternRule(m_rules, "'([^'\\\\]|\\\\.)*'", Token_String);
        addMultiLineRule(m_rules, "^=begin", "^=end", Token_Comment);
        addHashComments(m_rules);

    } else if (m_lang == Lang_PHP) {
        addCommonNumbers(m_rules);
        addPatternRule(m_rules, "[\\+\\-\\*/%&\\|!<>\\^~=]+", Token_Operator);
        addPatternRule(m_rules, "\\b([A-Za-z_][A-Za-z0-9_]*)\\s*(?=\\()", Token_Function);

        QStringList keywords = {
            "abstract", "and", "as", "break", "callable", "case", "catch", "class", "clone", "const",
            "continue", "declare", "default", "do", "echo", "else", "elseif", "empty", "endfor",
            "endforeach", "endif", "endswitch", "endwhile", "eval", "exit", "extends", "final",
            "finally", "for", "foreach", "function", "global", "goto", "if", "implements", "include",
            "include_once", "instanceof", "insteadof", "interface", "isset", "list", "match",
            "namespace", "new", "or", "print", "private", "protected", "public", "require",
            "require_once", "return", "static", "switch", "throw", "trait", "try", "unset", "use",
            "var", "while", "xor", "yield"
        };
        addKeywordRule(m_rules, keywords, Token_Keyword);

        addPatternRule(m_rules, "\\$[a-zA-Z_][a-zA-Z0-9_]*", Token_Variable);
        addPatternRule(m_rules, "<\\?php|\\?>", Token_Preprocessor);
        addPatternRule(m_rules, "\"([^\"\\\\]|\\\\.)*\"", Token_String);
        addPatternRule(m_rules, "'([^'\\\\]|\\\\.)*'", Token_String);
        addCStyleComments(m_rules);
        addHashComments(m_rules);

    } else if (m_lang == Lang_Shell) {
        addPatternRule(m_rules, "\\b\\d+\\b", Token_Number);
        addPatternRule(m_rules, "[\\+\\-\\*/%&\\|!<>\\^~=]+", Token_Operator);

        QStringList keywords = {
            "if", "then", "else", "elif", "fi", "for", "while", "until", "do", "done", "case", "esac",
            "in", "function", "select", "time", "return", "exit", "break", "continue"
        };
        addKeywordRule(m_rules, keywords, Token_Keyword);

        QStringList builtins = {
            "echo", "printf", "read", "cd", "pwd", "ls", "cp", "mv", "rm", "mkdir", "rmdir", "touch",
            "cat", "grep", "sed", "awk", "export", "local", "alias", "source", "test"
        };
        addKeywordRule(m_rules, builtins, Token_Builtin);

        addPatternRule(m_rules, "\\$[a-zA-Z_][a-zA-Z0-9_]*", Token_Variable);
        addPatternRule(m_rules, "\\$\\{[^\\}]+\\}", Token_Variable);
        addPatternRule(m_rules, "^#!.*$", Token_Preprocessor, QRegularExpression::MultilineOption);
        addPatternRule(m_rules, "\"([^\"\\\\]|\\\\.)*\"", Token_String);
        addPatternRule(m_rules, "'[^']*'", Token_String);
        addHashComments(m_rules);

    } else if (m_lang == Lang_Lua) {
        addCommonNumbers(m_rules);
        addPatternRule(m_rules, "[\\+\\-\\*/%&\\|!<>\\^~=]+", Token_Operator);
        addPatternRule(m_rules, "\\b([A-Za-z_][A-Za-z0-9_]*)\\s*(?=\\()", Token_Function);

        QStringList keywords = {
            "and", "break", "do", "else", "elseif", "end", "false", "for", "function", "goto", "if",
            "in", "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while"
        };
        addKeywordRule(m_rules, keywords, Token_Keyword);

        addPatternRule(m_rules, "\"([^\"\\\\]|\\\\.)*\"", Token_String);
        addPatternRule(m_rules, "'([^'\\\\]|\\\\.)*'", Token_String);
        addMultiLineRule(m_rules, "\\[\\[", "\\]\\]", Token_String);
        addPatternRule(m_rules, "--(?!\\[\\[).*$", Token_Comment, QRegularExpression::MultilineOption);
        addMultiLineRule(m_rules, "--\\[\\[", "\\]\\]", Token_Comment);

    } else if (m_lang == Lang_Perl) {
        QStringList keywords = {
            "if", "elsif", "else", "unless", "while", "until", "for", "foreach", "do", "last", "next",
            "redo", "return", "my", "our", "local", "use", "require", "no", "package", "sub"
        };
        addKeywordRule(m_rules, keywords, Token_Keyword);

        addPatternRule(m_rules, "[\\$@%][a-zA-Z_][a-zA-Z0-9_]*", Token_Variable);
        addPatternRule(m_rules, "\"([^\"\\\\]|\\\\.)*\"", Token_String);
        addPatternRule(m_rules, "'([^'\\\\]|\\\\.)*'", Token_String);
        addHashComments(m_rules);

    } else {
        
        addCommonNumbers(m_rules);
        addPatternRule(m_rules, "[\\+\\-\\*/%&\\|!<>\\^~=]+", Token_Operator);
        addPatternRule(m_rules, "\\b[A-Za-z_][a-zA-Z0-9_]*\\b", Token_Default);
        addPatternRule(m_rules, "\\b([A-Za-z_][a-zA-Z0-9_]*)\\s*(?=\\()", Token_Function);
        addPatternRule(m_rules, "\"([^\"\\\\]|\\\\.)*\"", Token_String);
        addPatternRule(m_rules, "'([^'\\\\]|\\\\.)*'", Token_String);
        addCStyleComments(m_rules);
        addHashComments(m_rules);
    }
}

void SimpleHighlighter::updateFormatsFromTheme() {
    
    auto &tm = ThemeManager::instance();
    QColor keyword = tm.getColor(ThemeManager::SyntaxKeyword);
    QColor type = tm.getColor(ThemeManager::SyntaxType);
    QColor function = tm.getColor(ThemeManager::SyntaxFunction);
    QColor variable = tm.getColor(ThemeManager::SyntaxVariable);
    QColor str = tm.getColor(ThemeManager::SyntaxString);
    QColor comment = tm.getColor(ThemeManager::SyntaxComment);
    QColor number = tm.getColor(ThemeManager::SyntaxNumber);
    QColor op = tm.getColor(ThemeManager::SyntaxOperator);
    QColor pre = tm.getColor(ThemeManager::SyntaxPreprocessor);
    QColor tag = tm.getColor(ThemeManager::SyntaxTag);
    QColor attr = tm.getColor(ThemeManager::SyntaxAttribute);
    QColor cssProp = tm.getColor(ThemeManager::SyntaxCssProperty);
    QColor cssVal = tm.getColor(ThemeManager::SyntaxCssValue);
    QColor builtin = tm.getColor(ThemeManager::SyntaxBuiltin);
    QColor annot = tm.getColor(ThemeManager::SyntaxAnnotation);

    for (Rule &r : m_rules) {
        QTextCharFormat fmt;
        switch (r.role) {
            case Token_Keyword: fmt.setForeground(keyword); break;
            case Token_Type: fmt.setForeground(type); break;
            case Token_Function: fmt.setForeground(function); break;
            case Token_Variable: fmt.setForeground(variable); break;
            case Token_String: fmt.setForeground(str); break;
            case Token_Comment: fmt.setForeground(comment); fmt.setFontItalic(true); break;
            case Token_Number: fmt.setForeground(number); break;
            case Token_Operator: fmt.setForeground(op); break;
            case Token_Preprocessor: fmt.setForeground(pre); break;
            case Token_Tag: fmt.setForeground(tag); break;
            case Token_Attribute: fmt.setForeground(attr); break;
            case Token_CssProperty: fmt.setForeground(cssProp); break;
            case Token_CssValue: fmt.setForeground(cssVal); break;
            case Token_Builtin: fmt.setForeground(builtin); break;
            case Token_Annotation: fmt.setForeground(annot); break;
            default: fmt.setForeground(tm.getColor(ThemeManager::EditorForeground)); break;
        }
        r.format = fmt;
    }
}

void SimpleHighlighter::highlightBlock(const QString &text) {
    
    for (const Rule &rule : std::as_const(m_rules)) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            int start = match.capturedStart();
            int len = match.capturedLength();
            setFormat(start, len, rule.format);
        }
    }

    
    int prevState = previousBlockState();
    int ruleCount = m_rules.size();
    setCurrentBlockState(0);

    
    if (prevState > 0) {
        int contIdx = prevState - 1;
        if (contIdx >= 0 && contIdx < ruleCount && m_rules[contIdx].multiLineStart) {
            const Rule &rule = m_rules[contIdx];
            int startIndex = 0;
            QRegularExpressionMatch endMatch = rule.endPattern.match(text, startIndex);
            int endIndex = endMatch.hasMatch() ? endMatch.capturedEnd() : -1;
            if (endIndex == -1) {
                setFormat(startIndex, text.length(), rule.format);
                setCurrentBlockState(contIdx + 1);
            } else {
                setFormat(startIndex, endIndex - startIndex, rule.format);
                
                int scanPos = endIndex;
                while (true) {
                    QRegularExpressionMatch startMatch = rule.pattern.match(text, scanPos);
                    if (!startMatch.hasMatch()) break;
                    int s = startMatch.capturedStart();
                    QRegularExpressionMatch eMatch = rule.endPattern.match(text, s);
                    int eIdx = eMatch.hasMatch() ? eMatch.capturedEnd() : -1;
                    if (eIdx == -1) {
                        setFormat(s, text.length() - s, rule.format);
                        setCurrentBlockState(contIdx + 1);
                        break;
                    } else {
                        setFormat(s, eIdx - s, rule.format);
                        scanPos = eIdx;
                    }
                }
            }
        }
    }

    
    if (prevState == 0) {
        for (int i = 0; i < ruleCount; ++i) {
            const Rule &rule = m_rules[i];
            if (!rule.multiLineStart) continue;
            QRegularExpressionMatch startMatch = rule.pattern.match(text);
            int startIndex = startMatch.hasMatch() ? startMatch.capturedStart() : -1;
            while (startIndex >= 0) {
                QRegularExpressionMatch endMatch = rule.endPattern.match(text, startIndex);
                int endIndex = endMatch.hasMatch() ? endMatch.capturedEnd() : -1;
                int length;
                if (endIndex == -1) {
                    setCurrentBlockState(i + 1);
                    length = text.length() - startIndex;
                } else {
                    length = endIndex - startIndex;
                }
                setFormat(startIndex, length, rule.format);
                if (endIndex == -1) break;
                startMatch = rule.pattern.match(text, endIndex);
                startIndex = startMatch.hasMatch() ? startMatch.capturedStart() : -1;
            }
            if (previousBlockState() != 0) break;
        }
    }

    
    BlockUserData *data = static_cast<BlockUserData*>(currentBlockUserData());
    if (!data) {
        data = new BlockUserData();
        setCurrentBlockUserData(data);
    }

    int prevLevel = 0;
    QTextBlock prev = currentBlock().previous();
    if (prev.isValid()) {
        BlockUserData *prevData = static_cast<BlockUserData*>(prev.userData());
        if (prevData) {
            prevLevel = prevData->foldingLevel;
        }
    }

    int currentLevel = prevLevel;
    bool hasStart = false;
    
    
    for (int i = 0; i < text.length(); ++i) {
        if (text[i] == '{' || text[i] == '[' || (m_lang == Lang_Python && text[i] == ':')) {
            currentLevel++;
            hasStart = true;
        } else if (text[i] == '}' || text[i] == ']') {
            currentLevel--;
        }
    }
    
    data->foldingLevel = currentLevel;
    data->foldStart = hasStart;
}

QVector<SimpleHighlighter::SymbolRule> SimpleHighlighter::getSymbolRules(Language lang) {
    QVector<SymbolRule> rules;

    switch (lang) {
        case Lang_Cpp:
        case Lang_Java:
        case Lang_Kotlin:
        case Lang_CSharp:
            rules.append({QRegularExpression(R"(^\s*(?:class|struct|interface|enum|trait|namespace)\s+([A-Za-z_][A-Za-z0-9_]*))"), "class"});
            rules.append({QRegularExpression(R"(^\s*(?:\[[^\]]*\]\s*)*(?:public|private|protected|static|virtual|inline|async|fun)?\s*(?:[A-Za-z_][A-Za-z0-9_<>,*:& \t\[\]]+\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*\([^)]*\))"), "function"});
            break;
        case Lang_Python:
            rules.append({QRegularExpression(R"(^\s*class\s+([A-Za-z_][A-Za-z0-9_]*))"), "class"});
            rules.append({QRegularExpression(R"(^\s*def\s+([A-Za-z_][A-Za-z0-9_]*))"), "function"});
            break;
        case Lang_Rust:
            rules.append({QRegularExpression(R"(^\s*(?:pub\s+)?(?:struct|enum|trait|union|mod|type)\s+([A-Za-z_][A-Za-z0-9_]*))"), "class"});
            rules.append({QRegularExpression(R"(^\s*(?:pub\s+)?(?:async\s+)?fn\s+([A-Za-z_][A-Za-z0-9_]*))"), "function"});
            break;
        case Lang_Go:
            rules.append({QRegularExpression(R"(^\s*type\s+([A-Za-z_][A-Za-z0-9_]*)\s+(?:struct|interface))"), "class"});
            rules.append({QRegularExpression(R"(^\s*func\s+(?:\([^)]*\)\s+)?([A-Za-z_][A-Za-z0-9_]*))"), "function"});
            break;
        case Lang_JS:
        case Lang_TypeScript:
            rules.append({QRegularExpression(R"(^\s*(?:export\s+)?(?:abstract\s+)?class\s+([A-Za-z_][A-Za-z0-9_]*))"), "class"});
            rules.append({QRegularExpression(R"(^\s*(?:export\s+)?(?:async\s+)?function\s+([A-Za-z_][A-Za-z0-9_]*))"), "function"});
            rules.append({QRegularExpression(R"(^\s*(?:async\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*\([^)]*\)\s*\{)"), "method"});
            break;
        case Lang_Ruby:
            rules.append({QRegularExpression(R"(^\s*(?:class|module)\s+([A-Z][A-Za-z0-9_]*))"), "class"});
            rules.append({QRegularExpression(R"(^\s*def\s+([A-Za-z0-9_!?.]+))"), "function"});
            break;
        case Lang_PHP:
            rules.append({QRegularExpression(R"(^\s*(?:abstract\s+|final\s+)?(?:class|interface|trait|enum)\s+([A-Za-z_][A-Za-z0-9_]*))"), "class"});
            rules.append({QRegularExpression(R"(^\s*(?:public|private|protected|static\s+)*function\s+([A-Za-z_][A-Za-z0-9_]*))"), "function"});
            break;
        case Lang_Lua:
            rules.append({QRegularExpression(R"(^\s*(?:local\s+)?function\s+([A-Za-z_][A-Za-z0-9_.:]*))"), "function"});
            break;
        case Lang_Markdown:
            rules.append({QRegularExpression(R"(^#{1,6}\s+(.*)$)", QRegularExpression::MultilineOption), "section"});
            break;
        case Lang_CSS:
            rules.append({QRegularExpression(R"(^([.#A-Za-z][A-Za-z0-9_-]*)\s*\{)"), "selector"});
            break;
        default:
            
            rules.append({QRegularExpression(R"(^\s*(?:class|struct|interface|enum|trait|namespace|def|func|function|fn|sub)\s+([A-Za-z_][A-Za-z0-9_]*))"), "item"});
            break;
    }

    return rules;
}

} 
