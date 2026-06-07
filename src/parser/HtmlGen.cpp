#include "HtmlGen.h"

namespace HtmlGen {

QString wrapDocument(const QString &bodyHtml) {
    const QString css = R"(
        <style>
            .k { color: #800080; font-weight: bold; }
            .kt { color: #0000ff; font-weight: bold; }
            .s { color: #a31515; }
            .c { color: #008000; font-style: italic; }
            .cp { color: #800080; }
            .n { color: #098658; }
            .nf { color: #795E26; }
            .nc { color: #2B91AF; }
            .nt { color: #800000; }
            .na { color: #ff0000; }
            .kp { color: #795E26; }
            .nb { color: #1750EB; }
            code { background-color: #f4f4f4; padding: 2px 4px; border-radius: 3px; }
            pre code { display: block; padding: 10px; }
            .fold { margin: 0.5em 0; }
            .fold-content { padding-left: 1em; border-left: 2px solid #ddd; margin-left: 0.5em; margin-top: 0.2em; }
            table { border-collapse: collapse; width: 100%; margin: 0.5em 0; }
            th, td { border: 1px solid #ddd; padding: 6px 8px; }
            th { background-color: #f2f2f2; }
        </style>
    )";
    return QStringLiteral("<!DOCTYPE html><html><head>") + css + QStringLiteral("</head><body>") + bodyHtml + QStringLiteral("</body></html>");
}

} // namespace HtmlGen