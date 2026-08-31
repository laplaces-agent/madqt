use std::collections::HashMap;
use std::sync::OnceLock;

use pulldown_cmark::{html, CodeBlockKind, CowStr, Event, Options, Parser, Tag, TagEnd};
use syntect::html::{ClassStyle, ClassedHTMLGenerator};
use syntect::parsing::SyntaxSet;
use syntect::util::LinesWithEndings;

#[cxx::bridge]
mod ffi {
    extern "Rust" {
        fn render_markdown_to_html(markdown: &str) -> String;
    }
}

// Class names carry a "syn-" prefix so the generated stylesheet can target
// `.syn-keyword` etc. without colliding with any document class. Loading the
// syntax set deserializes a sizeable binary dump, so do it once and share it
// across re-renders (the viewer reloads on every disk change).
const SYNTAX_CLASS_PREFIX: &str = "syn-";

fn syntax_set() -> &'static SyntaxSet {
    static SYNTAX_SET: OnceLock<SyntaxSet> = OnceLock::new();
    SYNTAX_SET.get_or_init(SyntaxSet::load_defaults_newlines)
}

fn render_markdown_to_html(markdown: &str) -> String {
    let parser = Parser::new_ext(markdown, Options::all());

    // Pull fenced code blocks out of the event stream and replace them with our
    // own highlighted HTML; everything else passes through to pulldown's
    // renderer untouched.
    let mut events: Vec<Event> = Vec::new();
    let mut code = String::new();
    let mut language = String::new();
    let mut in_code_block = false;

    // Give every heading a stable anchor id (unless the author set one via
    // `{#id}`) so the table of contents and in-document links can target it.
    // The id is derived from the heading's rendered text, which only arrives in
    // the events between Start and End, so we buffer the text and patch the
    // already-pushed Start event once the heading closes.
    let mut heading_start: Option<usize> = None;
    let mut heading_text = String::new();
    let mut slug_counts: HashMap<String, u32> = HashMap::new();

    for event in parser {
        match event {
            Event::Start(Tag::Heading { .. }) => {
                heading_start = Some(events.len());
                heading_text.clear();
                events.push(event);
            }
            Event::End(TagEnd::Heading(_)) => {
                if let Some(index) = heading_start.take() {
                    if let Event::Start(Tag::Heading { id, .. }) = &mut events[index] {
                        if id.is_none() {
                            let slug = unique_slug(&heading_text, &mut slug_counts);
                            *id = Some(CowStr::Boxed(slug.into_boxed_str()));
                        }
                    }
                }
                events.push(event);
            }
            Event::Text(ref text) | Event::Code(ref text) if heading_start.is_some() => {
                heading_text.push_str(text);
                events.push(event);
            }
            Event::Start(Tag::CodeBlock(kind)) => {
                in_code_block = true;
                code.clear();
                language = match kind {
                    CodeBlockKind::Fenced(info) => info
                        .split(|c: char| c.is_whitespace() || c == ',')
                        .next()
                        .unwrap_or("")
                        .to_string(),
                    CodeBlockKind::Indented => String::new(),
                };
            }
            Event::End(TagEnd::CodeBlock) if in_code_block => {
                in_code_block = false;
                let block = render_code_block(&code, &language);
                events.push(Event::Html(CowStr::Boxed(block.into_boxed_str())));
            }
            Event::Text(text) if in_code_block => code.push_str(&text),
            other => events.push(other),
        }
    }

    let mut html_out = String::new();
    html::push_html(&mut html_out, events.into_iter());
    html_out
}

fn render_code_block(code: &str, language: &str) -> String {
    // pulldown hands us a trailing newline; <pre> would render it as a blank
    // final line, so drop one.
    let code = code.strip_suffix('\n').unwrap_or(code);

    // Mermaid diagrams render client-side: the page runs in Chromium, so hand
    // the raw source to a <pre class="mermaid"> element and let mermaid.js turn
    // it into SVG. The C++ layer injects that script only when such a block is
    // present. Escaping is safe — mermaid reads textContent, which decodes it.
    if language.eq_ignore_ascii_case("mermaid") {
        return format!("<pre class=\"mermaid\">{}</pre>", escape_html(code));
    }

    let class_attr = if language.is_empty() {
        String::new()
    } else {
        format!(" class=\"language-{}\"", escape_html(language))
    };

    let body = highlight(code, language).unwrap_or_else(|| escape_html(code));
    format!("<pre><code{class_attr}>{body}</code></pre>")
}

// Returns classed `<span>` markup for a recognized language, or None when the
// language is empty/unknown so the caller can fall back to plain escaped text.
fn highlight(code: &str, language: &str) -> Option<String> {
    if language.is_empty() {
        return None;
    }

    let syntax_set = syntax_set();
    let syntax = syntax_set.find_syntax_by_token(language)?;
    let mut generator = ClassedHTMLGenerator::new_with_class_style(
        syntax,
        syntax_set,
        ClassStyle::SpacedPrefixed {
            prefix: SYNTAX_CLASS_PREFIX,
        },
    );

    for line in LinesWithEndings::from(code) {
        generator
            .parse_html_for_line_which_includes_newline(line)
            .ok()?;
    }

    Some(generator.finalize())
}

// Derives a URL-friendly anchor slug from heading text and guarantees it is
// unique within the document by suffixing repeats (`intro`, `intro-1`, …), the
// same scheme GitHub uses. The C++ layer reads the resulting ids back from the
// DOM, so the only requirements are validity and stability, not a specific format.
fn unique_slug(text: &str, counts: &mut HashMap<String, u32>) -> String {
    let base = slugify(text);
    let base = if base.is_empty() {
        String::from("section")
    } else {
        base
    };

    let count = counts.entry(base.clone()).or_insert(0);
    let slug = if *count == 0 {
        base.clone()
    } else {
        format!("{base}-{count}")
    };
    *count += 1;
    slug
}

fn slugify(text: &str) -> String {
    let mut slug = String::with_capacity(text.len());
    let mut pending_dash = false;
    for ch in text.chars() {
        if ch.is_alphanumeric() {
            if pending_dash && !slug.is_empty() {
                slug.push('-');
            }
            pending_dash = false;
            slug.extend(ch.to_lowercase());
        } else {
            // Collapse any run of spaces/punctuation into a single separator,
            // emitted lazily so trailing separators never appear.
            pending_dash = true;
        }
    }
    slug
}

fn escape_html(input: &str) -> String {
    let mut out = String::with_capacity(input.len());
    for ch in input.chars() {
        match ch {
            '&' => out.push_str("&amp;"),
            '<' => out.push_str("&lt;"),
            '>' => out.push_str("&gt;"),
            '"' => out.push_str("&quot;"),
            _ => out.push(ch),
        }
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn fenced_block_with_language_gets_classed_spans() {
        let html = render_markdown_to_html("```rust\nfn main() {}\n```");
        assert!(html.contains("<pre><code class=\"language-rust\">"));
        // `fn` -> storage.type.function, the function name -> entity.name.function;
        // both must come through as prefixed, scope-derived classes.
        assert!(html.contains("syn-storage"));
        assert!(html.contains("syn-entity"));
    }

    #[test]
    fn unknown_language_falls_back_to_escaped_text() {
        let html = render_markdown_to_html("```nonsense-lang\n<a> & b\n```");
        assert!(html.contains("&lt;a&gt; &amp; b"));
        assert!(!html.contains("syn-"));
    }

    #[test]
    fn indented_block_has_no_language_class_and_is_escaped() {
        let html = render_markdown_to_html("    <tag> & co\n");
        assert!(html.contains("<pre><code>"));
        assert!(html.contains("&lt;tag&gt; &amp; co"));
    }

    #[test]
    fn mermaid_block_becomes_mermaid_pre() {
        let html = render_markdown_to_html("```mermaid\ngraph TD; A--> B;\n```");
        assert!(html.contains("<pre class=\"mermaid\">"));
        assert!(html.contains("graph TD; A--&gt; B;")); // raw source, escaped
        assert!(!html.contains("syn-")); // not syntax-highlighted
    }

    #[test]
    fn gfm_alert_is_tagged_and_marker_stripped() {
        let html = render_markdown_to_html("> [!WARNING]\n> Be careful here.");
        assert!(html.contains("markdown-alert-warning"));
        assert!(html.contains("Be careful here."));
        assert!(!html.contains("[!WARNING]")); // marker consumed, not shown
    }

    #[test]
    fn math_spans_pass_through() {
        let html = render_markdown_to_html("Inline $a^2$ and block:\n\n$$E=mc^2$$");
        assert!(html.contains(r#"<span class="math math-inline">a^2</span>"#));
        assert!(html.contains(r#"<span class="math math-display">E=mc^2</span>"#));
    }

    #[test]
    fn demo_languages_all_resolve() {
        for lang in ["rust", "cpp", "python", "javascript", "bash", "json"] {
            let src = format!("```{lang}\nlet x = 1\n```");
            let html = render_markdown_to_html(&src);
            assert!(html.contains("class=\"syn-"), "{lang} did not highlight");
        }
    }

    #[test]
    fn prose_still_renders() {
        let html = render_markdown_to_html("# Title\n\nSome **bold** text.");
        assert!(html.contains("<h1 id=\"title\">Title</h1>"));
        assert!(html.contains("<strong>bold</strong>"));
    }

    #[test]
    fn headings_get_slugged_anchor_ids() {
        let html = render_markdown_to_html("## Getting Started!\n\n### API & Usage");
        assert!(html.contains("<h2 id=\"getting-started\">"));
        // Punctuation is dropped, the ampersand collapses to a single separator.
        assert!(html.contains("<h3 id=\"api-usage\">"));
    }

    #[test]
    fn heading_text_includes_inline_code_and_emphasis() {
        let html = render_markdown_to_html("# The `render()` *call*");
        assert!(html.contains("id=\"the-render-call\""));
    }

    #[test]
    fn duplicate_headings_get_unique_ids() {
        let html = render_markdown_to_html("# Intro\n\n## Intro\n\n## Intro");
        assert!(html.contains("<h1 id=\"intro\">"));
        assert!(html.contains("<h2 id=\"intro-1\">"));
        assert!(html.contains("<h2 id=\"intro-2\">"));
    }

    #[test]
    fn explicit_heading_id_is_respected() {
        let html = render_markdown_to_html("# Custom Title {#my-anchor}");
        assert!(html.contains("id=\"my-anchor\""));
        assert!(!html.contains("id=\"custom-title\""));
    }

    #[test]
    fn symbol_only_heading_falls_back_to_section() {
        let html = render_markdown_to_html("# !!!");
        assert!(html.contains("id=\"section\""));
    }
}
