#include "CodeHighlight.h"
#include "Utils.h"
#include <QRegularExpression>
#include <QSet>
#include <vector>
#include <functional>

namespace CodeHighlight {

using Rule = std::pair<QRegularExpression, std::function<QString(const QRegularExpressionMatch &)>>;

static QString applyRulesToLine(const QString &escapedLine, const std::vector<Rule> &rules) {
    QString processed = escapedLine;
    for (const auto &[re, callback] : rules) {
        processed = ParserUtils::safeReplaceOutsideSpans(processed, re, callback);
    }
    return processed;
}

static QString highlightLines(const QString &code,
                               const std::vector<Rule> &rules,
                               const std::function<QString(const QString&)> &preprocessLine = {}) {
    const QStringList lines = code.split('\n');
    QString result;
    for (const QString &line : lines) {
        QString processed = ParserUtils::escapeHtml(line);
        if (preprocessLine) {
            processed = preprocessLine(processed);
        }
        processed = applyRulesToLine(processed, rules);
        result += processed + '\n';
    }
    return result.trimmed();
}

static Rule makeRule(const QString &pattern, std::function<QString(const QRegularExpressionMatch &)> callback) {
    return {QRegularExpression(pattern), std::move(callback)};
}

static Rule makeKeywordRule(const QString &pattern, const QSet<QString> &typeKeywords = {}) {
    return makeRule(pattern, [typeKeywords](const QRegularExpressionMatch &m) {
        QString word = m.captured();
        if (typeKeywords.contains(word)) {
            return QStringLiteral("<span class=\"kt\">%1</span>").arg(word);
        }
        return QStringLiteral("<span class=\"k\">%1</span>").arg(word);
    });
}

static Rule makeStringRule(const QString &pattern) {
    return makeRule(pattern, [](const QRegularExpressionMatch &m) {
        return QStringLiteral("<span class=\"s\">%1</span>").arg(m.captured(1));
    });
}

static Rule makeCommentRule(const QString &pattern) {
    return makeRule(pattern, [](const QRegularExpressionMatch &m) {
        return QStringLiteral("<span class=\"c\">%1</span>").arg(m.captured(1));
    });
}

static Rule makeNumberRule() {
    return makeRule(R"(\b\d+(\.\d+)?\b)", [](const QRegularExpressionMatch &m) {
        return QStringLiteral("<span class=\"n\">%1</span>").arg(m.captured(0));
    });
}

static Rule makeFuncCallRule() {
    return makeRule(R"(\b([a-zA-Z_]\w*)\s*(?=\())", [](const QRegularExpressionMatch &m) {
        return QStringLiteral("<span class=\"nf\">%1</span>").arg(m.captured(1));
    });
}

static Rule makeClassNameRule() {
    return makeRule(R"(\b([A-Z][a-zA-Z0-9_]*)\b)", [](const QRegularExpressionMatch &m) {
        return QStringLiteral("<span class=\"nc\">%1</span>").arg(m.captured(1));
    });
}

static const QSet<QString> cppTypeKeywords {
    "int", "float", "double", "char", "bool", "void",
    "wchar_t", "char8_t", "char16_t", "char32_t",
    "short", "long", "signed", "unsigned", "auto"
};

static Highlighter createCppHighlighter() {
    return [](const QString &code) {
        std::vector<Rule> rules = {
            makeCommentRule(R"((//[^\n]*))"),
            makeRule(R"("([^\"\\]|\\.)*")", [](const QRegularExpressionMatch &m) {
                return QStringLiteral("<span class=\"s\">%1</span>").arg(m.captured(0));
            }),
            makeKeywordRule(
                "\\b(alignas|alignof|and|and_eq|asm|auto|bitand|bitor|bool|break|case|catch|char|char8_t|"
                "char16_t|char32_t|class|compl|concept|const|consteval|constexpr|constinit|const_cast|continue|"
                "co_await|co_return|co_yield|decltype|default|delete|do|double|dynamic_cast|else|enum|explicit|"
                "export|extern|false|float|for|friend|goto|if|inline|int|long|mutable|namespace|new|noexcept|"
                "not|not_eq|nullptr|operator|or|or_eq|private|protected|public|register|reinterpret_cast|"
                "requires|return|short|signed|sizeof|static|static_assert|static_cast|struct|switch|template|"
                "this|thread_local|throw|true|try|typedef|typeid|typename|union|unsigned|using|virtual|void|"
                "volatile|wchar_t|while|xor|xor_eq)\\b", cppTypeKeywords),
            makeFuncCallRule(),
            makeClassNameRule(),
            makeNumberRule()
        };
        auto preprocess = [](const QString &line) -> QString {
            QString l = line;
            QRegularExpressionMatch m = QRegularExpression(R"(^\s*#.*)").match(l);
            if (m.hasMatch()) {
                l.replace(m.capturedStart(), m.capturedLength(),
                          QStringLiteral("<span class=\"cp\">%1</span>").arg(m.captured()));
            }
            return l;
        };
        return highlightLines(code, rules, preprocess);
    };
}

static Highlighter createPythonHighlighter() {
    return [](const QString &code) {
        std::vector<Rule> rules = {
            makeCommentRule(R"((#[^\n]*))"),
            makeStringRule(R"(("[^"]*"|'[^']*'))"),
            makeKeywordRule(R"(\b(False|None|True|and|as|assert|async|await|break|class|continue|"
                            "def|del|elif|else|except|finally|for|from|global|if|import|in|is|"
                            "lambda|nonlocal|not|or|pass|raise|return|try|while|with|yield)\b)"),
            makeNumberRule()
        };
        return highlightLines(code, rules);
    };
}

static Highlighter createJavaScriptHighlighter() {
    return [](const QString &code) {
        std::vector<Rule> rules = {
            makeCommentRule(R"((//[^\n]*|/\*[^*]*\*+(?:[^/*][^*]*\*+)*/))"),
            makeStringRule(R"(("[^"]*"|'[^']*'|`[^`]*`))"),
            makeKeywordRule(R"(\b(break|case|catch|continue|debugger|default|delete|do|else|finally|"
                            "for|function|if|in|instanceof|new|return|switch|this|throw|try|typeof|"
                            "var|void|while|with|class|const|enum|export|extends|import|super|"
                            "implements|interface|let|package|private|protected|public|static|yield|"
                            "async|await|of)\b)"),
            makeFuncCallRule(),
            makeNumberRule()
        };
        return highlightLines(code, rules);
    };
}

static Highlighter createJavaLikeHighlighter(const QString &keywordPattern) {
    return [keywordPattern](const QString &code) {
        std::vector<Rule> rules = {
            makeCommentRule(R"((//[^\n]*|/\*[^*]*\*+(?:[^/*][^*]*\*+)*/))"),
            makeRule(R"("([^\"\\]|\\.)*")", [](const QRegularExpressionMatch &m) {
                return QStringLiteral("<span class=\"s\">%1</span>").arg(m.captured(0));
            }),
            makeKeywordRule(keywordPattern),
            makeFuncCallRule(),
            makeClassNameRule(),
            makeNumberRule()
        };
        return highlightLines(code, rules);
    };
}

static Highlighter createJavaHighlighter() {
    return createJavaLikeHighlighter(
        R"(\b(abstract|assert|boolean|break|byte|case|catch|char|class|const|continue|default|do|double|else|enum|extends|final|finally|float|for|goto|if|implements|import|instanceof|int|interface|long|native|new|package|private|protected|public|return|short|static|strictfp|super|switch|synchronized|this|throw|throws|transient|try|void|volatile|while|var)\b)"
    );
}

static Highlighter createKotlinHighlighter() {
    return createJavaLikeHighlighter(
        R"(\b(abstract|actual|annotation|as|break|by|catch|class|companion|const|constructor|continue|crossinline|data|delegate|do|dynamic|else|enum|expect|external|false|field|file|final|finally|for|fun|get|if|import|in|infix|init|inline|inner|interface|internal|is|it|lateinit|noinline|null|object|open|operator|out|override|package|param|private|property|protected|public|receiver|reified|return|sealed|set|super|suspend|tailrec|this|throw|true|try|typealias|val|var|vararg|when|where|while)\b)"
    );
}

static Highlighter createCSharpHighlighter() {
    return createJavaLikeHighlighter(
        R"(\b(abstract|as|base|bool|break|byte|case|catch|char|checked|class|const|continue|decimal|default|delegate|do|double|else|enum|event|explicit|extern|false|finally|fixed|float|for|foreach|goto|if|implicit|in|int|interface|internal|is|lock|long|namespace|new|null|object|operator|out|override|params|private|protected|public|readonly|ref|return|sbyte|sealed|short|sizeof|stackalloc|static|string|struct|switch|this|throw|true|try|typeof|uint|ulong|unchecked|unsafe|ushort|using|var|virtual|void|volatile|while)\b)"
    );
}

static Highlighter createShellHighlighter() {
    return [](const QString &code) {
        std::vector<Rule> rules = {
            makeCommentRule(R"((#[^\n]*))"),
            makeStringRule(R"(("[^"]*"|'[^']*'))"),
            makeKeywordRule(R"(\b(if|then|else|elif|fi|for|while|do|done|case|esac|function|return|in|break|continue|export|local|readonly|unset|eval|exec|exit|trap|set|shift)\b)"),
            makeRule(R"(\$\{?[a-zA-Z_]\w*}?)", [](const QRegularExpressionMatch &m) {
                return QStringLiteral("<span class=\"nb\">%1</span>").arg(m.captured());
            })
        };
        return highlightLines(code, rules);
    };
}

static Highlighter createJsonHighlighter() {
    return [](const QString &code) {
        std::vector<Rule> rules = {
            makeRule(R"("([^\"\\]|\\.)*")", [](const QRegularExpressionMatch &m) {
                return QStringLiteral("<span class=\"s\">%1</span>").arg(m.captured(0));
            }),
            makeRule(R"(\b-?\d+(\.\d+)?([eE][+-]?\d+)?\b)", [](const QRegularExpressionMatch &m) {
                return QStringLiteral("<span class=\"n\">%1</span>").arg(m.captured(0));
            }),
            makeKeywordRule(R"(\b(true|false|null)\b)")
        };
        return highlightLines(code, rules);
    };
}

static Highlighter createCssHighlighter() {
    return [](const QString &code) {
        std::vector<Rule> rules = {
            makeRule(R"(/\*[^*]*\*+(?:[^/*][^*]*\*+)*/)", [](const QRegularExpressionMatch &m) {
                return QStringLiteral("<span class=\"c\">%1</span>").arg(m.captured(0));
            }),
            makeRule(R"(([^{]+)(?=\{))", [](const QRegularExpressionMatch &m) {
                return QStringLiteral("<span class=\"nt\">%1</span>").arg(m.captured(1));
            }),
            makeRule(R"(([a-zA-Z-]+)(?=\s*:))", [](const QRegularExpressionMatch &m) {
                return QStringLiteral("<span class=\"kp\">%1</span>").arg(m.captured(1));
            }),
            makeRule(R"(:\s*([^;]+))", [](const QRegularExpressionMatch &m) {
                return QStringLiteral(": <span class=\"s\">%1</span>").arg(m.captured(1));
            })
        };
        return highlightLines(code, rules);
    };
}

static Highlighter createHtmlHighlighter() {
    return [](const QString &code) {
        std::vector<Rule> rules = {
            makeRule(R"(<!--[\s\S]*?-->)", [](const QRegularExpressionMatch &m) {
                return QStringLiteral("<span class=\"c\">%1</span>").arg(m.captured(0));
            }),
            makeRule(R"(</?[a-zA-Z][\w-]*(?:\s[^>]*)?>)", [](const QRegularExpressionMatch &m) {
                return QStringLiteral("<span class=\"nt\">%1</span>").arg(m.captured(0));
            }),
            makeRule(R"((\s[a-zA-Z-]+)=\"([^\"]*)\")", [](const QRegularExpressionMatch &m) {
                return QStringLiteral(" <span class=\"na\">%1</span>=\"<span class=\"s\">%2</span>\"")
                    .arg(m.captured(1), m.captured(2));
            })
        };
        return highlightLines(code, rules);
    };
}

static std::unordered_map<QString, Highlighter> highlighters;
static bool aliasesInit = false;

static void initAll() {
    if (aliasesInit) return;
    highlighters["cpp"] = createCppHighlighter();
    highlighters["c"] = highlighters["cpp"];
    highlighters["python"] = createPythonHighlighter();
    highlighters["py"] = highlighters["python"];
    highlighters["javascript"] = createJavaScriptHighlighter();
    highlighters["js"] = highlighters["javascript"];
    highlighters["java"] = createJavaHighlighter();
    highlighters["kotlin"] = createKotlinHighlighter();
    highlighters["kt"] = highlighters["kotlin"];
    highlighters["shell"] = createShellHighlighter();
    highlighters["sh"] = highlighters["shell"];
    highlighters["bash"] = highlighters["shell"];
    highlighters["csharp"] = createCSharpHighlighter();
    highlighters["cs"] = highlighters["csharp"];
    highlighters["json"] = createJsonHighlighter();
    highlighters["css"] = createCssHighlighter();
    highlighters["html"] = createHtmlHighlighter();
    aliasesInit = true;
}

QString highlightCode(const QString &lang, const QString &code) {
    initAll();
    if (lang.isEmpty()) {
        return ParserUtils::escapeHtml(code);
    }
    auto it = highlighters.find(lang);
    if (it != highlighters.end()) {
        return it->second(code);
    }
    return highlighters["cpp"](code);
}

} // namespace CodeHighlight