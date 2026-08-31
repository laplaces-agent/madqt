#include "MarkdownAppearance.h"
#include "RustyMarkdown.h"

#include <QJsonObject>
#include <QSettings>

namespace
{
QString colorToCss(const QColor &color)
{
    return color.name(QColor::HexRgb);
}

// Fonts shipped in resources/fonts.qrc. Qt WebEngine cannot see fonts registered
// through QFontDatabase, so the page loads them itself through the madqt:
// resource scheme (see ResourceSchemeHandler).
QString bundledFontFaces()
{
    return QStringLiteral(R"(
        @font-face { font-family: "Source Sans 3"; src: url("madqt:/fonts/SourceSans3-Regular.ttf"); font-weight: 400; font-style: normal; }
        @font-face { font-family: "Source Sans 3"; src: url("madqt:/fonts/SourceSans3-It.ttf"); font-weight: 400; font-style: italic; }
        @font-face { font-family: "Source Sans 3"; src: url("madqt:/fonts/SourceSans3-Semibold.ttf"); font-weight: 600; font-style: normal; }
        @font-face { font-family: "Source Sans 3"; src: url("madqt:/fonts/SourceSans3-Bold.ttf"); font-weight: 700; font-style: normal; }
        @font-face { font-family: "Source Sans 3"; src: url("madqt:/fonts/SourceSans3-BoldIt.ttf"); font-weight: 700; font-style: italic; }
        @font-face { font-family: "Source Serif 4"; src: url("madqt:/fonts/SourceSerif4-Regular.ttf"); font-weight: 400; font-style: normal; }
        @font-face { font-family: "Source Serif 4"; src: url("madqt:/fonts/SourceSerif4-It.ttf"); font-weight: 400; font-style: italic; }
        @font-face { font-family: "Source Serif 4"; src: url("madqt:/fonts/SourceSerif4-Semibold.ttf"); font-weight: 600; font-style: normal; }
        @font-face { font-family: "Source Serif 4"; src: url("madqt:/fonts/SourceSerif4-Bold.ttf"); font-weight: 700; font-style: normal; }
        @font-face { font-family: "Source Serif 4"; src: url("madqt:/fonts/SourceSerif4-BoldIt.ttf"); font-weight: 700; font-style: italic; }
        @font-face { font-family: "JetBrains Mono"; src: url("madqt:/fonts/JetBrainsMono-Variable.ttf") format("truetype-variations"); font-weight: 100 800; font-style: normal; }
        @font-face { font-family: "JetBrains Mono"; src: url("madqt:/fonts/JetBrainsMono-Italic-Variable.ttf") format("truetype-variations"); font-weight: 100 800; font-style: italic; }
        @font-face { font-family: "IBM Plex Sans"; src: url("madqt:/fonts/IBMPlexSans-Regular.ttf"); font-weight: 400; font-style: normal; }
        @font-face { font-family: "IBM Plex Sans"; src: url("madqt:/fonts/IBMPlexSans-Italic.ttf"); font-weight: 400; font-style: italic; }
        @font-face { font-family: "IBM Plex Sans"; src: url("madqt:/fonts/IBMPlexSans-SemiBold.ttf"); font-weight: 600; font-style: normal; }
        @font-face { font-family: "IBM Plex Sans"; src: url("madqt:/fonts/IBMPlexSans-Bold.ttf"); font-weight: 700; font-style: normal; }
        @font-face { font-family: "IBM Plex Sans"; src: url("madqt:/fonts/IBMPlexSans-BoldItalic.ttf"); font-weight: 700; font-style: italic; }
        @font-face { font-family: "IBM Plex Serif"; src: url("madqt:/fonts/IBMPlexSerif-Regular.ttf"); font-weight: 400; font-style: normal; }
        @font-face { font-family: "IBM Plex Serif"; src: url("madqt:/fonts/IBMPlexSerif-Italic.ttf"); font-weight: 400; font-style: italic; }
        @font-face { font-family: "IBM Plex Serif"; src: url("madqt:/fonts/IBMPlexSerif-SemiBold.ttf"); font-weight: 600; font-style: normal; }
        @font-face { font-family: "IBM Plex Serif"; src: url("madqt:/fonts/IBMPlexSerif-Bold.ttf"); font-weight: 700; font-style: normal; }
        @font-face { font-family: "IBM Plex Serif"; src: url("madqt:/fonts/IBMPlexSerif-BoldItalic.ttf"); font-weight: 700; font-style: italic; }
        @font-face { font-family: "IBM Plex Mono"; src: url("madqt:/fonts/IBMPlexMono-Regular.ttf"); font-weight: 400; font-style: normal; }
        @font-face { font-family: "IBM Plex Mono"; src: url("madqt:/fonts/IBMPlexMono-Italic.ttf"); font-weight: 400; font-style: italic; }
        @font-face { font-family: "IBM Plex Mono"; src: url("madqt:/fonts/IBMPlexMono-SemiBold.ttf"); font-weight: 600; font-style: normal; }
        @font-face { font-family: "IBM Plex Mono"; src: url("madqt:/fonts/IBMPlexMono-Bold.ttf"); font-weight: 700; font-style: normal; }
    )");
}

// Everything user-tunable enters the stylesheet through these custom properties;
// the rest of the CSS below is static.
QString cssTokens(const MarkdownAppearance &a)
{
    const QString metrics = QStringLiteral(R"(
        :root {
            --body-font: "%1";
            --mono-font: "%2";
            --body-size: %3pt;
            --mono-size: %4pt;
            --content-padding: %5px;
            --measure: %6px;
            --line-height: %7;
        }
    )")
        .arg(a.bodyFontFamily,
             a.monospaceFontFamily,
             QString::number(a.bodyFontSize),
             QString::number(a.monospaceFontSize),
             QString::number(a.contentPadding),
             QString::number(a.contentMaxWidth),
             QString::number(a.lineHeight, 'f', 2));

    const QString palette = QStringLiteral(R"(
        :root {
            --fg: %1;
            --page-bg: %2;
            --heading: %3;
            --accent: %4;
            --muted: %5;
            --code-fg: %6;
            --code-bg: %7;
            --panel-bg: %8;
            --border: %9;
            --quote-border: %10;
        }
    )")
        .arg(colorToCss(a.bodyTextColor),
             colorToCss(a.pageBackgroundColor),
             colorToCss(a.headingColor),
             colorToCss(a.linkColor),
             colorToCss(a.mutedTextColor),
             colorToCss(a.codeTextColor),
             colorToCss(a.codeBackgroundColor),
             colorToCss(a.panelBackgroundColor),
             colorToCss(a.borderColor))
        .arg(colorToCss(a.blockquoteBorderColor));

    const QString syntax = QStringLiteral(R"(
        :root {
            --syn-comment: %1;
            --syn-keyword: %2;
            --syn-string: %3;
            --syn-number: %4;
            --syn-function: %5;
            --syn-type: %6;
        }
    )")
        .arg(colorToCss(a.syntaxCommentColor),
             colorToCss(a.syntaxKeywordColor),
             colorToCss(a.syntaxStringColor),
             colorToCss(a.syntaxNumberColor),
             colorToCss(a.syntaxFunctionColor),
             colorToCss(a.syntaxTypeColor));

    return metrics + palette + syntax;
}

QString staticStyleSheet()
{
    return QStringLiteral(R"(
        * { box-sizing: border-box; }

        html {
            background-color: var(--page-bg);
            scroll-behavior: smooth;
        }

        body {
            font-family: var(--body-font), "IBM Plex Sans", "Source Sans 3", "Noto Sans", system-ui, sans-serif;
            font-size: var(--body-size);
            line-height: var(--line-height);
            color: var(--fg);
            background-color: var(--page-bg);
            max-width: calc(var(--measure) + 2 * var(--content-padding));
            margin: 0 auto;
            padding: var(--content-padding);
            font-kerning: normal;
            font-feature-settings: "kern", "liga", "calt";
            text-rendering: optimizeLegibility;
            -webkit-font-smoothing: antialiased;
            overflow-wrap: break-word;
        }
        body > :first-child { margin-top: 0; }
        body > :last-child { margin-bottom: 0; }

        p, ul, ol, dl, table, blockquote, pre, figure {
            margin: 0.9em 0;
        }

        h1, h2, h3, h4, h5, h6 {
            color: var(--heading);
            font-weight: 600;
            line-height: 1.25;
            letter-spacing: -0.01em;
            margin: 1.6em 0 0.6em;
        }
        h1 {
            font-size: 2em;
            font-weight: 700;
            letter-spacing: -0.02em;
            line-height: 1.15;
            margin-top: 1.1em;
            padding-bottom: 0.3em;
            border-bottom: 1px solid var(--border);
        }
        h2 {
            font-size: 1.5em;
            letter-spacing: -0.015em;
            padding-bottom: 0.25em;
            border-bottom: 1px solid var(--border);
        }
        h3 { font-size: 1.25em; }
        h4 { font-size: 1.05em; }
        h5 { font-size: 1em; }
        h6 {
            font-size: 0.85em;
            color: var(--muted);
            text-transform: uppercase;
            letter-spacing: 0.06em;
        }

        a {
            color: var(--accent);
            text-decoration: underline;
            text-decoration-thickness: 1px;
            text-decoration-color: color-mix(in srgb, var(--accent) 30%, transparent);
            text-underline-offset: 0.15em;
        }
        a:hover { text-decoration-color: var(--accent); }

        code, kbd, samp, pre {
            font-family: var(--mono-font), "IBM Plex Mono", "JetBrains Mono", "Cascadia Code", "DejaVu Sans Mono", monospace;
            font-feature-settings: "calt";
        }
        code {
            font-size: 0.92em;
            color: var(--code-fg);
            background-color: var(--code-bg);
            padding: 0.08em 0.35em;
            border-radius: 4px;
        }
        pre {
            font-size: var(--mono-size);
            line-height: 1.55;
            color: var(--code-fg);
            background-color: var(--panel-bg);
            border: 1px solid var(--border);
            border-radius: 8px;
            padding: 0.9em 1.1em;
            overflow-x: auto;
        }
        pre code {
            font-size: 100%;
            color: inherit;
            background: none;
            padding: 0;
            border-radius: 0;
        }

        /* Mermaid blocks are diagrams, not code panels: drop the panel chrome
           and center the rendered SVG. mermaid.js replaces the source text in
           place once it runs (see the injected script). */
        pre.mermaid {
            background: none;
            border: none;
            padding: 0;
            text-align: center;
            line-height: normal;
            font-size: var(--body-size);
        }

        /* Math. pulldown (ENABLE_MATH) wraps $…$ / $$…$$ as
           <span class="math math-inline|math-display"> holding the raw TeX;
           KaTeX renders it in place (see the injected script). KaTeX inherits
           the surrounding color, so math is theme-aware for free. */
        .math-display {
            display: block;
            overflow-x: auto;
            overflow-y: hidden;
            margin: 0.9em 0;
        }

        /* Syntax highlighting. The Rust renderer tags tokens with prefixed,
           scope-derived classes (e.g. keyword.control -> "syn-keyword
           syn-control"); these map the top-level scopes onto the theme palette.
           Function/type/tag rules require the entity/support component so they
           hit the actual name token, not the broad meta.function / meta.class
           wrappers (which also carry "function"/"class" and would otherwise
           bleed color onto nested punctuation via inheritance). Order matters
           for the rest: a token carrying several classes takes the last matching
           color, so keyword/storage sit last to win cases like storage.type. */
        pre code .syn-comment { color: var(--syn-comment); font-style: italic; }
        pre code .syn-string { color: var(--syn-string); }
        pre code .syn-constant,
        pre code .syn-numeric { color: var(--syn-number); }
        pre code .syn-entity.syn-function,
        pre code .syn-support.syn-function { color: var(--syn-function); }
        pre code .syn-entity.syn-type,
        pre code .syn-entity.syn-class,
        pre code .syn-support.syn-type,
        pre code .syn-support.syn-class { color: var(--syn-type); }
        pre code .syn-entity.syn-tag { color: var(--syn-keyword); }
        pre code .syn-keyword,
        pre code .syn-storage { color: var(--syn-keyword); }

        blockquote {
            padding: 0.1em 1.1em;
            border-left: 3px solid var(--quote-border);
            color: var(--muted);
        }
        blockquote > blockquote { margin: 0.5em 0; }

        /* GitHub-style alerts: pulldown (GFM) tags `> [!NOTE]` blockquotes with
           a markdown-alert-* class. Draw a colored rule, a faint tint, and a
           titled icon. Hues are fixed semantic colors (per-theme tuning is a
           follow-up); the tint is transparent so it sits on any page color. */
        blockquote[class*="markdown-alert-"] {
            border-left: 3px solid var(--alert-color, var(--quote-border));
            background-color: color-mix(in srgb, var(--alert-color) 7%, transparent);
            border-radius: 0 6px 6px 0;
            padding: 0.6em 1.1em;
            color: var(--fg);
        }
        blockquote[class*="markdown-alert-"]::before {
            display: block;
            font-weight: 600;
            color: var(--alert-color);
            margin-bottom: 0.3em;
            padding-left: 1.55em;
            background-repeat: no-repeat;
            background-position: 0 0.12em;
            background-size: 1.1em 1.1em;
            line-height: 1.35;
        }
        .markdown-alert-note { --alert-color: #0969da; }
        .markdown-alert-note::before { content: "Note";
            background-image: url("data:image/svg+xml,%3Csvg fill='%230969da' xmlns=%22http://www.w3.org/2000/svg%22 width=%2216%22 height=%2216%22 viewBox=%220 0 16 16%22%3E%3Cpath d=%22M0 8a8 8 0 1 1 16 0A8 8 0 0 1 0 8Zm8-6.5a6.5 6.5 0 1 0 0 13 6.5 6.5 0 0 0 0-13ZM6.5 7.75A.75.75 0 0 1 7.25 7h1a.75.75 0 0 1 .75.75v2.75h.25a.75.75 0 0 1 0 1.5h-2a.75.75 0 0 1 0-1.5h.25v-2h-.25a.75.75 0 0 1-.75-.75ZM8 6a1 1 0 1 1 0-2 1 1 0 0 1 0 2Z%22/%3E%3C/svg%3E"); }
        .markdown-alert-tip { --alert-color: #1a7f37; }
        .markdown-alert-tip::before { content: "Tip";
            background-image: url("data:image/svg+xml,%3Csvg fill='%231a7f37' xmlns=%22http://www.w3.org/2000/svg%22 width=%2216%22 height=%2216%22 viewBox=%220 0 16 16%22%3E%3Cpath d=%22M8 1.5c-2.363 0-4 1.69-4 3.75 0 .984.424 1.625.984 2.304l.214.253c.223.264.47.556.673.848.284.411.537.896.621 1.49a.75.75 0 0 1-1.484.211c-.04-.282-.163-.547-.37-.847a8.456 8.456 0 0 0-.542-.68c-.084-.1-.173-.205-.268-.32C3.201 7.75 2.5 6.766 2.5 5.25 2.5 2.31 4.863 0 8 0s5.5 2.31 5.5 5.25c0 1.516-.701 2.5-1.328 3.259-.095.115-.184.22-.268.319-.207.245-.383.453-.541.681-.208.3-.33.565-.37.847a.751.751 0 0 1-1.485-.212c.084-.593.337-1.078.621-1.489.203-.292.45-.584.673-.848.075-.088.147-.173.213-.253.561-.679.985-1.32.985-2.304 0-2.06-1.637-3.75-4-3.75ZM5.75 12h4.5a.75.75 0 0 1 0 1.5h-4.5a.75.75 0 0 1 0-1.5ZM6 15.25a.75.75 0 0 1 .75-.75h2.5a.75.75 0 0 1 0 1.5h-2.5a.75.75 0 0 1-.75-.75Z%22/%3E%3C/svg%3E"); }
        .markdown-alert-important { --alert-color: #8250df; }
        .markdown-alert-important::before { content: "Important";
            background-image: url("data:image/svg+xml,%3Csvg fill='%238250df' xmlns=%22http://www.w3.org/2000/svg%22 width=%2216%22 height=%2216%22 viewBox=%220 0 16 16%22%3E%3Cpath d=%22M0 1.75C0 .784.784 0 1.75 0h12.5C15.216 0 16 .784 16 1.75v9.5A1.75 1.75 0 0 1 14.25 13H8.06l-2.573 2.573A1.458 1.458 0 0 1 3 14.543V13H1.75A1.75 1.75 0 0 1 0 11.25Zm1.75-.25a.25.25 0 0 0-.25.25v9.5c0 .138.112.25.25.25h2a.75.75 0 0 1 .75.75v2.19l2.72-2.72a.749.749 0 0 1 .53-.22h6.5a.25.25 0 0 0 .25-.25v-9.5a.25.25 0 0 0-.25-.25Zm7 2.25v2.5a.75.75 0 0 1-1.5 0v-2.5a.75.75 0 0 1 1.5 0ZM9 9a1 1 0 1 1-2 0 1 1 0 0 1 2 0Z%22/%3E%3C/svg%3E"); }
        .markdown-alert-warning { --alert-color: #9a6700; }
        .markdown-alert-warning::before { content: "Warning";
            background-image: url("data:image/svg+xml,%3Csvg fill='%239a6700' xmlns=%22http://www.w3.org/2000/svg%22 width=%2216%22 height=%2216%22 viewBox=%220 0 16 16%22%3E%3Cpath d=%22M6.457 1.047c.659-1.234 2.427-1.234 3.086 0l6.082 11.378A1.75 1.75 0 0 1 14.082 15H1.918a1.75 1.75 0 0 1-1.543-2.575Zm1.763.707a.25.25 0 0 0-.44 0L1.698 13.132a.25.25 0 0 0 .22.368h12.164a.25.25 0 0 0 .22-.368Zm.53 3.996v2.5a.75.75 0 0 1-1.5 0v-2.5a.75.75 0 0 1 1.5 0ZM9 11a1 1 0 1 1-2 0 1 1 0 0 1 2 0Z%22/%3E%3C/svg%3E"); }
        .markdown-alert-caution { --alert-color: #cf222e; }
        .markdown-alert-caution::before { content: "Caution";
            background-image: url("data:image/svg+xml,%3Csvg fill='%23cf222e' xmlns=%22http://www.w3.org/2000/svg%22 width=%2216%22 height=%2216%22 viewBox=%220 0 16 16%22%3E%3Cpath d=%22M4.47.22A.749.749 0 0 1 5 0h6c.199 0 .389.079.53.22l4.25 4.25c.141.14.22.331.22.53v6a.749.749 0 0 1-.22.53l-4.25 4.25A.749.749 0 0 1 11 16H5a.749.749 0 0 1-.53-.22L.22 11.53A.749.749 0 0 1 0 11V5c0-.199.079-.389.22-.53Zm.84 1.28L1.5 5.31v5.38l3.81 3.81h5.38l3.81-3.81V5.31L10.69 1.5ZM8 4a.75.75 0 0 1 .75.75v3.5a.75.75 0 0 1-1.5 0v-3.5A.75.75 0 0 1 8 4Zm0 8a1 1 0 1 1 0-2 1 1 0 0 1 0 2Z%22/%3E%3C/svg%3E"); }

        ul, ol { padding-left: 1.7em; }
        li { margin: 0.25em 0; }
        li > ul, li > ol, li > p { margin: 0.25em 0; }
        li::marker { color: var(--muted); }

        /* Definition lists (pulldown ENABLE_DEFINITION_LIST emits dl/dt/dd). */
        dt { font-weight: 600; color: var(--heading); margin-top: 0.6em; }
        dd { margin: 0.2em 0 0.2em 1.6em; }
        dd + dd { margin-top: 0; }

        li:has(> input[type="checkbox"]:first-child),
        li:has(> p:first-child > input[type="checkbox"]:first-child) {
            list-style: none;
            position: relative;
        }
        li input[type="checkbox"]:first-child {
            accent-color: var(--accent);
            font-size: inherit;
            position: absolute;
            left: -1.55em;
            width: 0.95em;
            height: 0.95em;
            top: calc((var(--line-height) * 1em - 0.95em) / 2);
            margin: 0;
        }

        table {
            display: block;
            width: max-content;
            max-width: 100%;
            overflow-x: auto;
            border-collapse: collapse;
            font-variant-numeric: tabular-nums;
        }
        th, td {
            border: 1px solid var(--border);
            padding: 0.45em 0.9em;
        }
        th {
            background-color: var(--panel-bg);
            font-weight: 600;
            text-align: left;
        }
        tbody tr:nth-child(even) {
            background-color: color-mix(in srgb, var(--panel-bg) 45%, transparent);
        }

        hr {
            border: none;
            border-top: 1px solid var(--border);
            margin: 2.2em 0;
        }

        img {
            max-width: 100%;
            height: auto;
            border-radius: 4px;
        }

        kbd {
            font-size: 0.85em;
            color: var(--fg);
            background-color: var(--panel-bg);
            border: 1px solid var(--border);
            border-bottom-width: 2px;
            border-radius: 4px;
            padding: 0.1em 0.45em;
        }

        del { color: var(--muted); }

        /* Opt-in brand wordmark: an oversized hero with a light-to-deep sweep
           of the theme's own accent, so it pops in every theme. Only active
           where the markup is present (the welcome page) — ordinary documents
           that mention the name are untouched. */
        .madqt-internal-splash-wordmark-7e3b91 {
            font-size: 3.2rem;
            font-weight: 800;
            letter-spacing: -0.03em;
            background-image: linear-gradient(
                100deg,
                color-mix(in srgb, var(--accent) 55%, white) 0%,
                var(--accent) 45%,
                color-mix(in srgb, var(--accent) 65%, black) 100%);
            -webkit-background-clip: text;
            background-clip: text;
            -webkit-text-fill-color: transparent;
        }

        mark {
            background-color: color-mix(in srgb, var(--accent) 18%, transparent);
            color: inherit;
            border-radius: 2px;
            padding: 0 0.15em;
        }

        .footnote-reference {
            font-size: 0.8em;
            vertical-align: super;
            line-height: 0;
        }
        .footnote-definition {
            font-size: 0.9em;
            color: var(--muted);
            margin: 0.4em 0;
        }
        .footnote-definition p { display: inline; margin: 0; }
        .footnote-definition-label {
            font-size: 0.85em;
            vertical-align: super;
            line-height: 0;
            margin-right: 0.4em;
        }

        ::selection {
            background-color: color-mix(in srgb, var(--accent) 22%, transparent);
        }

        ::-webkit-scrollbar { width: 12px; height: 10px; }
        ::-webkit-scrollbar-track { background: transparent; }
        ::-webkit-scrollbar-thumb {
            background-color: color-mix(in srgb, var(--muted) 35%, transparent);
            border-radius: 6px;
            border: 3px solid transparent;
            background-clip: content-box;
        }
        ::-webkit-scrollbar-thumb:hover {
            background-color: color-mix(in srgb, var(--muted) 55%, transparent);
        }
    )");
}

// Injected only when a document actually contains a mermaid block, so ordinary
// documents never pay the ~3 MB script cost. mermaid wants literal colors (it
// darkens/derives them with its own color library and can't parse var()), so
// the theme palette is baked in here rather than referenced through tokens.
QString mermaidScript(const MarkdownAppearance &a)
{
    return QStringLiteral(R"(
        <script src="madqt:/scripts/mermaid.min.js"></script>
        <script>
            mermaid.initialize({
                startOnLoad: false,
                securityLevel: 'strict',
                theme: 'base',
                fontFamily: '"%1", system-ui, sans-serif',
                themeVariables: {
                    background: '%2',
                    primaryColor: '%3',
                    primaryTextColor: '%4',
                    primaryBorderColor: '%5',
                    secondaryColor: '%6',
                    tertiaryColor: '%2',
                    lineColor: '%7',
                    textColor: '%4',
                    titleColor: '%8'
                }
            });
            mermaid.run({ querySelector: 'pre.mermaid' });
        </script>
    )")
        .arg(a.bodyFontFamily,
             colorToCss(a.pageBackgroundColor),
             colorToCss(a.panelBackgroundColor),
             colorToCss(a.bodyTextColor),
             colorToCss(a.borderColor),
             colorToCss(a.codeBackgroundColor),
             colorToCss(a.mutedTextColor),
             colorToCss(a.headingColor));
}

// Injected only when a document contains math. pulldown already wrapped each
// $…$ / $$…$$ as <span class="math math-inline|math-display"> holding the raw
// TeX; KaTeX renders each span in place. No theme values needed — KaTeX
// inherits the surrounding text color. The stylesheet is served from the same
// madqt: dir so its relative font url()s resolve against madqt:/scripts/katex/.
QString katexScript()
{
    return QStringLiteral(R"(
        <link rel="stylesheet" href="madqt:/scripts/katex/katex.min.css">
        <script src="madqt:/scripts/katex/katex.min.js"></script>
        <script>
            document.querySelectorAll('span.math').forEach(function(el) {
                katex.render(el.textContent, el, {
                    displayMode: el.classList.contains('math-display'),
                    throwOnError: false
                });
            });
        </script>
    )");
}
}

QString MarkdownAppearance::documentStyleSheet() const
{
    return bundledFontFaces() + cssTokens(*this) + staticStyleSheet();
}

QString MarkdownAppearance::renderMarkdownToHtml(const QString &markdown) const
{
    const QString renderedHtml = madqt::rs::renderMarkdownToHtml(markdown);

    // Inject renderer scripts only for the features a document actually uses, so
    // ordinary documents pull in neither KaTeX nor mermaid.
    QString scripts;
    if (renderedHtml.contains(QStringLiteral("class=\"math"))) {
        scripts += katexScript();
    }
    if (renderedHtml.contains(QStringLiteral("class=\"mermaid\""))) {
        scripts += mermaidScript(*this);
    }

    return QStringLiteral(R"(
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="utf-8">
            <style>
%1
            </style>
        </head>
        <body>
%2
%3
        </body>
        </html>
    )")
        .arg(documentStyleSheet(), renderedHtml, scripts);
}

QFont MarkdownAppearance::bodyFont() const
{
    QFont font(bodyFontFamily, bodyFontSize);
    font.setStyleHint(QFont::SansSerif);
    return font;
}

QFont MarkdownAppearance::monospaceFont() const
{
    QFont font(monospaceFontFamily, monospaceFontSize);
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    return font;
}

MarkdownAppearance MarkdownAppearance::readSettings(QSettings &settings)
{
    auto appearance = MarkdownAppearance::defaults();

    settings.beginGroup(QStringLiteral("appearance"));
    appearance.bodyFontFamily = settings.value(QStringLiteral("bodyFontFamily"), appearance.bodyFontFamily).toString();
    appearance.monospaceFontFamily = settings.value(QStringLiteral("monospaceFontFamily"), appearance.monospaceFontFamily).toString();
    appearance.bodyFontSize = settings.value(QStringLiteral("bodyFontSize"), appearance.bodyFontSize).toInt();
    appearance.monospaceFontSize = settings.value(QStringLiteral("monospaceFontSize"), appearance.monospaceFontSize).toInt();
    appearance.contentPadding = settings.value(QStringLiteral("contentPadding"), appearance.contentPadding).toInt();
    appearance.contentMaxWidth = settings.value(QStringLiteral("contentMaxWidth"), appearance.contentMaxWidth).toInt();
    appearance.lineHeight = settings.value(QStringLiteral("lineHeight"), appearance.lineHeight).toDouble();
    appearance.bodyTextColor = settings.value(QStringLiteral("bodyTextColor"), appearance.bodyTextColor).value<QColor>();
    appearance.pageBackgroundColor = settings.value(QStringLiteral("pageBackgroundColor"), appearance.pageBackgroundColor).value<QColor>();
    appearance.headingColor = settings.value(QStringLiteral("headingColor"), appearance.headingColor).value<QColor>();
    appearance.linkColor = settings.value(QStringLiteral("linkColor"), appearance.linkColor).value<QColor>();
    appearance.mutedTextColor = settings.value(QStringLiteral("mutedTextColor"), appearance.mutedTextColor).value<QColor>();
    appearance.codeTextColor = settings.value(QStringLiteral("codeTextColor"), appearance.codeTextColor).value<QColor>();
    appearance.codeBackgroundColor = settings.value(QStringLiteral("codeBackgroundColor"), appearance.codeBackgroundColor).value<QColor>();
    appearance.panelBackgroundColor = settings.value(QStringLiteral("panelBackgroundColor"), appearance.panelBackgroundColor).value<QColor>();
    appearance.borderColor = settings.value(QStringLiteral("borderColor"), appearance.borderColor).value<QColor>();
    appearance.blockquoteBorderColor = settings.value(QStringLiteral("blockquoteBorderColor"), appearance.blockquoteBorderColor).value<QColor>();
    appearance.syntaxCommentColor = settings.value(QStringLiteral("syntaxCommentColor"), appearance.syntaxCommentColor).value<QColor>();
    appearance.syntaxKeywordColor = settings.value(QStringLiteral("syntaxKeywordColor"), appearance.syntaxKeywordColor).value<QColor>();
    appearance.syntaxStringColor = settings.value(QStringLiteral("syntaxStringColor"), appearance.syntaxStringColor).value<QColor>();
    appearance.syntaxNumberColor = settings.value(QStringLiteral("syntaxNumberColor"), appearance.syntaxNumberColor).value<QColor>();
    appearance.syntaxFunctionColor = settings.value(QStringLiteral("syntaxFunctionColor"), appearance.syntaxFunctionColor).value<QColor>();
    appearance.syntaxTypeColor = settings.value(QStringLiteral("syntaxTypeColor"), appearance.syntaxTypeColor).value<QColor>();
    settings.endGroup();

    if (appearance.bodyFontFamily.trimmed().isEmpty()) {
        appearance.bodyFontFamily = MarkdownAppearance::defaults().bodyFontFamily;
    }

    if (appearance.monospaceFontFamily.trimmed().isEmpty()) {
        appearance.monospaceFontFamily = MarkdownAppearance::defaults().monospaceFontFamily;
    }

    return appearance;
}

MarkdownAppearance MarkdownAppearance::defaults()
{
    return {};
}

QJsonObject MarkdownAppearance::toJson() const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("bodyFontFamily"), bodyFontFamily);
    obj.insert(QStringLiteral("monospaceFontFamily"), monospaceFontFamily);
    obj.insert(QStringLiteral("bodyFontSize"), bodyFontSize);
    obj.insert(QStringLiteral("monospaceFontSize"), monospaceFontSize);
    obj.insert(QStringLiteral("contentPadding"), contentPadding);
    obj.insert(QStringLiteral("contentMaxWidth"), contentMaxWidth);
    obj.insert(QStringLiteral("lineHeight"), lineHeight);
    obj.insert(QStringLiteral("bodyTextColor"), bodyTextColor.name(QColor::HexRgb));
    obj.insert(QStringLiteral("pageBackgroundColor"), pageBackgroundColor.name(QColor::HexRgb));
    obj.insert(QStringLiteral("headingColor"), headingColor.name(QColor::HexRgb));
    obj.insert(QStringLiteral("linkColor"), linkColor.name(QColor::HexRgb));
    obj.insert(QStringLiteral("mutedTextColor"), mutedTextColor.name(QColor::HexRgb));
    obj.insert(QStringLiteral("codeTextColor"), codeTextColor.name(QColor::HexRgb));
    obj.insert(QStringLiteral("codeBackgroundColor"), codeBackgroundColor.name(QColor::HexRgb));
    obj.insert(QStringLiteral("panelBackgroundColor"), panelBackgroundColor.name(QColor::HexRgb));
    obj.insert(QStringLiteral("borderColor"), borderColor.name(QColor::HexRgb));
    obj.insert(QStringLiteral("blockquoteBorderColor"), blockquoteBorderColor.name(QColor::HexRgb));
    obj.insert(QStringLiteral("syntaxCommentColor"), syntaxCommentColor.name(QColor::HexRgb));
    obj.insert(QStringLiteral("syntaxKeywordColor"), syntaxKeywordColor.name(QColor::HexRgb));
    obj.insert(QStringLiteral("syntaxStringColor"), syntaxStringColor.name(QColor::HexRgb));
    obj.insert(QStringLiteral("syntaxNumberColor"), syntaxNumberColor.name(QColor::HexRgb));
    obj.insert(QStringLiteral("syntaxFunctionColor"), syntaxFunctionColor.name(QColor::HexRgb));
    obj.insert(QStringLiteral("syntaxTypeColor"), syntaxTypeColor.name(QColor::HexRgb));
    return obj;
}

MarkdownAppearance MarkdownAppearance::fromJson(const QJsonObject &object)
{
    auto appearance = MarkdownAppearance::defaults();

    if (object.contains(QStringLiteral("bodyFontFamily"))) {
        const auto val = object.value(QStringLiteral("bodyFontFamily")).toString();
        if (!val.trimmed().isEmpty()) {
            appearance.bodyFontFamily = val;
        }
    }

    if (object.contains(QStringLiteral("monospaceFontFamily"))) {
        const auto val = object.value(QStringLiteral("monospaceFontFamily")).toString();
        if (!val.trimmed().isEmpty()) {
            appearance.monospaceFontFamily = val;
        }
    }

    if (object.contains(QStringLiteral("bodyFontSize"))) {
        appearance.bodyFontSize = object.value(QStringLiteral("bodyFontSize")).toInt(appearance.bodyFontSize);
    }

    if (object.contains(QStringLiteral("monospaceFontSize"))) {
        appearance.monospaceFontSize = object.value(QStringLiteral("monospaceFontSize")).toInt(appearance.monospaceFontSize);
    }

    if (object.contains(QStringLiteral("contentPadding"))) {
        appearance.contentPadding = object.value(QStringLiteral("contentPadding")).toInt(appearance.contentPadding);
    }

    if (object.contains(QStringLiteral("contentMaxWidth"))) {
        appearance.contentMaxWidth = object.value(QStringLiteral("contentMaxWidth")).toInt(appearance.contentMaxWidth);
    }

    if (object.contains(QStringLiteral("lineHeight"))) {
        appearance.lineHeight = object.value(QStringLiteral("lineHeight")).toDouble(appearance.lineHeight);
    }

    if (object.contains(QStringLiteral("bodyTextColor"))) {
        const auto colorStr = object.value(QStringLiteral("bodyTextColor")).toString();
        if (!colorStr.isEmpty()) {
            appearance.bodyTextColor = QColor(colorStr);
        }
    }

    if (object.contains(QStringLiteral("pageBackgroundColor"))) {
        const auto colorStr = object.value(QStringLiteral("pageBackgroundColor")).toString();
        if (!colorStr.isEmpty()) {
            appearance.pageBackgroundColor = QColor(colorStr);
        }
    }

    if (object.contains(QStringLiteral("headingColor"))) {
        const auto colorStr = object.value(QStringLiteral("headingColor")).toString();
        if (!colorStr.isEmpty()) {
            appearance.headingColor = QColor(colorStr);
        }
    }

    if (object.contains(QStringLiteral("linkColor"))) {
        const auto colorStr = object.value(QStringLiteral("linkColor")).toString();
        if (!colorStr.isEmpty()) {
            appearance.linkColor = QColor(colorStr);
        }
    }

    if (object.contains(QStringLiteral("mutedTextColor"))) {
        const auto colorStr = object.value(QStringLiteral("mutedTextColor")).toString();
        if (!colorStr.isEmpty()) {
            appearance.mutedTextColor = QColor(colorStr);
        }
    }

    if (object.contains(QStringLiteral("codeTextColor"))) {
        const auto colorStr = object.value(QStringLiteral("codeTextColor")).toString();
        if (!colorStr.isEmpty()) {
            appearance.codeTextColor = QColor(colorStr);
        }
    }

    if (object.contains(QStringLiteral("codeBackgroundColor"))) {
        const auto colorStr = object.value(QStringLiteral("codeBackgroundColor")).toString();
        if (!colorStr.isEmpty()) {
            appearance.codeBackgroundColor = QColor(colorStr);
        }
    }

    if (object.contains(QStringLiteral("panelBackgroundColor"))) {
        const auto colorStr = object.value(QStringLiteral("panelBackgroundColor")).toString();
        if (!colorStr.isEmpty()) {
            appearance.panelBackgroundColor = QColor(colorStr);
        }
    }

    if (object.contains(QStringLiteral("borderColor"))) {
        const auto colorStr = object.value(QStringLiteral("borderColor")).toString();
        if (!colorStr.isEmpty()) {
            appearance.borderColor = QColor(colorStr);
        }
    }

    if (object.contains(QStringLiteral("blockquoteBorderColor"))) {
        const auto colorStr = object.value(QStringLiteral("blockquoteBorderColor")).toString();
        if (!colorStr.isEmpty()) {
            appearance.blockquoteBorderColor = QColor(colorStr);
        }
    }

    const auto readColor = [&object](const QString &key, QColor &target) {
        if (object.contains(key)) {
            const auto colorStr = object.value(key).toString();
            if (!colorStr.isEmpty()) {
                target = QColor(colorStr);
            }
        }
    };
    readColor(QStringLiteral("syntaxCommentColor"), appearance.syntaxCommentColor);
    readColor(QStringLiteral("syntaxKeywordColor"), appearance.syntaxKeywordColor);
    readColor(QStringLiteral("syntaxStringColor"), appearance.syntaxStringColor);
    readColor(QStringLiteral("syntaxNumberColor"), appearance.syntaxNumberColor);
    readColor(QStringLiteral("syntaxFunctionColor"), appearance.syntaxFunctionColor);
    readColor(QStringLiteral("syntaxTypeColor"), appearance.syntaxTypeColor);

    return appearance;
}
