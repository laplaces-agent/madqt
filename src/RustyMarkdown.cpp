#include "RustyMarkdown.h"

#include <cstddef>

#include <QByteArray>

#include <madqt_rs/lib.h>

namespace madqt::rs {
QString renderMarkdownToHtml(const QString &markdown) {
  const QByteArray utf8 = markdown.toUtf8();
  const auto html = render_markdown_to_html(
      {utf8.constData(), static_cast<std::size_t>(utf8.size())});
  return QString::fromUtf8(html.data(), static_cast<qsizetype>(html.size()));
}
} // namespace madqt::rs
