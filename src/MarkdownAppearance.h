#pragma once

#include <QColor>
#include <QFont>
#include <QString>

class QJsonObject;
class QSettings;

struct MarkdownAppearance
{
    QString bodyFontFamily {QStringLiteral("IBM Plex Sans")};
    QString monospaceFontFamily {QStringLiteral("IBM Plex Mono")};
    int bodyFontSize {13};
    int monospaceFontSize {11};
    int contentPadding {32};
    int contentMaxWidth {760};
    double lineHeight {1.6};
    QColor bodyTextColor {QStringLiteral("#1d2433")};
    QColor pageBackgroundColor {QStringLiteral("#ffffff")};
    QColor headingColor {QStringLiteral("#0f172a")};
    QColor linkColor {QStringLiteral("#0057b8")};
    QColor mutedTextColor {QStringLiteral("#4a5568")};
    QColor codeTextColor {QStringLiteral("#152132")};
    QColor codeBackgroundColor {QStringLiteral("#eef2f7")};
    QColor panelBackgroundColor {QStringLiteral("#f1f5f9")};
    QColor borderColor {QStringLiteral("#d8e0ea")};
    QColor blockquoteBorderColor {QStringLiteral("#b6c2d2")};

    // Syntax-highlighting palette for fenced code blocks. The Rust renderer
    // emits prefixed CSS classes (.syn-keyword, …); these drive the --syn-*
    // custom properties those classes resolve against. Defaults suit a light
    // theme (GitHub-light flavored); dark/sepia override them.
    QColor syntaxCommentColor {QStringLiteral("#6e7781")};
    QColor syntaxKeywordColor {QStringLiteral("#cf222e")};
    QColor syntaxStringColor {QStringLiteral("#0a3069")};
    QColor syntaxNumberColor {QStringLiteral("#0550ae")};
    QColor syntaxFunctionColor {QStringLiteral("#8250df")};
    QColor syntaxTypeColor {QStringLiteral("#953800")};

    [[nodiscard]] QString documentStyleSheet() const;
    [[nodiscard]] QString renderMarkdownToHtml(const QString &markdown) const;
    [[nodiscard]] QFont bodyFont() const;
    [[nodiscard]] QFont monospaceFont() const;
    static MarkdownAppearance readSettings(QSettings &settings);
    [[nodiscard]] QJsonObject toJson() const;
    static MarkdownAppearance fromJson(const QJsonObject &object);

    static MarkdownAppearance defaults();
};
