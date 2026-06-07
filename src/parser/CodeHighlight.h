#pragma once
#include <QString>
#include <unordered_map>
#include <functional>

namespace CodeHighlight {
    using Highlighter = std::function<QString(const QString&)>;
    QString highlightCode(const QString &lang, const QString &code);
}