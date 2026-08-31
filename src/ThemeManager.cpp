#include "ThemeManager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>

namespace
{
QString slug(const QString &name)
{
    QString result = name.toLower();
    QString slugged;
    bool lastWasDash = true;

    for (const auto &ch : result) {
        const bool isAlphaNum = ch.isLower() || (ch >= QLatin1Char('0') && ch <= QLatin1Char('9'));
        if (isAlphaNum) {
            slugged.append(ch);
            lastWasDash = false;
        } else if (!lastWasDash) {
            slugged.append(QLatin1Char('-'));
            lastWasDash = true;
        }
    }

    // Trim leading/trailing dashes
    slugged = slugged.trimmed();
    while (slugged.startsWith(QLatin1Char('-'))) {
        slugged.remove(0, 1);
    }
    while (slugged.endsWith(QLatin1Char('-'))) {
        slugged.chop(1);
    }

    if (slugged.isEmpty()) {
        slugged = QStringLiteral("theme");
    }

    return slugged;
}
}

ThemeManager::ThemeManager()
{
    // Build built-in themes
    auto light = MarkdownAppearance::defaults();
    builtinThemes.append(Theme{QStringLiteral("Light"), true, light});

    auto dark = MarkdownAppearance::defaults();
    dark.bodyTextColor = QColor(QStringLiteral("#d7dde5"));
    dark.pageBackgroundColor = QColor(QStringLiteral("#0d1117"));
    dark.headingColor = QColor(QStringLiteral("#f0f6fc"));
    dark.linkColor = QColor(QStringLiteral("#4493f8"));
    dark.mutedTextColor = QColor(QStringLiteral("#8b949e"));
    dark.codeTextColor = QColor(QStringLiteral("#e6edf3"));
    dark.codeBackgroundColor = QColor(QStringLiteral("#161b22"));
    dark.panelBackgroundColor = QColor(QStringLiteral("#161b22"));
    dark.borderColor = QColor(QStringLiteral("#30363d"));
    dark.blockquoteBorderColor = QColor(QStringLiteral("#3d444d"));
    dark.syntaxCommentColor = QColor(QStringLiteral("#8b949e"));
    dark.syntaxKeywordColor = QColor(QStringLiteral("#ff7b72"));
    dark.syntaxStringColor = QColor(QStringLiteral("#a5d6ff"));
    dark.syntaxNumberColor = QColor(QStringLiteral("#79c0ff"));
    dark.syntaxFunctionColor = QColor(QStringLiteral("#d2a8ff"));
    dark.syntaxTypeColor = QColor(QStringLiteral("#ffa657"));
    builtinThemes.append(Theme{QStringLiteral("Dark"), true, dark});

    auto sepia = MarkdownAppearance::defaults();
    sepia.bodyFontFamily = QStringLiteral("IBM Plex Serif");
    sepia.monospaceFontFamily = QStringLiteral("IBM Plex Mono");
    sepia.bodyFontSize = 14;
    sepia.monospaceFontSize = 11;
    sepia.contentMaxWidth = 720;
    sepia.lineHeight = 1.7;
    sepia.bodyTextColor = QColor(QStringLiteral("#5b4636"));
    sepia.pageBackgroundColor = QColor(QStringLiteral("#f4ecd8"));
    sepia.headingColor = QColor(QStringLiteral("#3a2a1c"));
    sepia.linkColor = QColor(QStringLiteral("#9a5b2e"));
    sepia.mutedTextColor = QColor(QStringLiteral("#7a6a55"));
    sepia.codeTextColor = QColor(QStringLiteral("#4a3c2c"));
    sepia.codeBackgroundColor = QColor(QStringLiteral("#ece0c8"));
    sepia.panelBackgroundColor = QColor(QStringLiteral("#ece0c8"));
    sepia.borderColor = QColor(QStringLiteral("#ddccae"));
    sepia.blockquoteBorderColor = QColor(QStringLiteral("#cbb892"));
    // Warm, lower-saturation hues so highlighting reads as part of the parchment.
    sepia.syntaxCommentColor = QColor(QStringLiteral("#9a8c74"));
    sepia.syntaxKeywordColor = QColor(QStringLiteral("#9a5b2e"));
    sepia.syntaxStringColor = QColor(QStringLiteral("#5a6a3a"));
    sepia.syntaxNumberColor = QColor(QStringLiteral("#7a5230"));
    sepia.syntaxFunctionColor = QColor(QStringLiteral("#6a4a8a"));
    sepia.syntaxTypeColor = QColor(QStringLiteral("#8a5a2a"));
    builtinThemes.append(Theme{QStringLiteral("Sepia"), true, sepia});

    // Load custom themes
    reload();

    // Read active theme name from settings
    QSettings settings;
    if (settings.contains(QStringLiteral("activeTheme"))) {
        activeName = settings.value(QStringLiteral("activeTheme")).toString();
    } else {
        // Migration: check for legacy appearance group
        settings.beginGroup(QStringLiteral("appearance"));
        const bool hasLegacy = !settings.childKeys().isEmpty();
        settings.endGroup();

        if (hasLegacy) {
            Theme t{uniqueName(QStringLiteral("My Theme")), false, MarkdownAppearance::readSettings(settings)};
            saveCustomTheme(t);
            setActiveThemeName(t.name);
        } else {
            setActiveThemeName(QStringLiteral("Light"));
        }
    }
}

QList<Theme> ThemeManager::themes() const
{
    return builtinThemes + customThemes;
}

Theme ThemeManager::themeByName(const QString &name) const
{
    for (const auto &theme : builtinThemes) {
        if (theme.name == name) {
            return theme;
        }
    }

    for (const auto &theme : customThemes) {
        if (theme.name == name) {
            return theme;
        }
    }

    // Return first built-in (Light) if not found
    if (!builtinThemes.isEmpty()) {
        return builtinThemes.first();
    }

    return Theme{QStringLiteral("Light"), true, MarkdownAppearance::defaults()};
}

Theme ThemeManager::activeTheme() const
{
    return themeByName(activeName);
}

QString ThemeManager::activeThemeName() const
{
    return activeName;
}

void ThemeManager::setActiveThemeName(const QString &name)
{
    activeName = name;
    QSettings settings;
    settings.setValue(QStringLiteral("activeTheme"), name);
}

bool ThemeManager::isBuiltIn(const QString &name) const
{
    for (const auto &theme : builtinThemes) {
        if (theme.name == name) {
            return true;
        }
    }
    return false;
}

bool ThemeManager::nameExists(const QString &name) const
{
    // Case-insensitive check
    const auto lower = name.toLower();
    for (const auto &theme : builtinThemes) {
        if (theme.name.toLower() == lower) {
            return true;
        }
    }
    for (const auto &theme : customThemes) {
        if (theme.name.toLower() == lower) {
            return true;
        }
    }
    return false;
}

QString ThemeManager::uniqueName(const QString &base) const
{
    if (!nameExists(base)) {
        return base;
    }

    for (int i = 2; i <= 10000; ++i) {
        const auto candidate = base + QStringLiteral(" ") + QString::number(i);
        if (!nameExists(candidate)) {
            return candidate;
        }
    }

    return base + QStringLiteral(" copy");
}

bool ThemeManager::saveCustomTheme(const Theme &theme)
{
    const auto dir = themesDir();
    if (!QDir().mkpath(dir)) {
        return false;
    }

    QJsonObject obj = theme.appearance.toJson();
    obj.insert(QStringLiteral("name"), theme.name);

    const auto filePath = dir + QStringLiteral("/") + slug(theme.name) + QStringLiteral(".json");
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(QJsonDocument(obj).toJson());
    file.close();

    reload();
    return true;
}

bool ThemeManager::deleteCustomTheme(const QString &name)
{
    const auto filePath = themesDir() + QStringLiteral("/") + slug(name) + QStringLiteral(".json");
    const bool success = QFile::remove(filePath);
    if (success) {
        reload();
    }
    return success;
}

void ThemeManager::reload()
{
    customThemes.clear();

    const auto dir = themesDir();
    const QDir directory(dir);
    if (!directory.exists()) {
        return;
    }

    const auto jsonFiles = directory.entryList(QStringList() << QStringLiteral("*.json"), QDir::Files);
    for (const auto &filename : jsonFiles) {
        const auto filePath = dir + QStringLiteral("/") + filename;
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }

        const auto doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        if (!doc.isObject()) {
            continue;
        }

        const auto obj = doc.object();
        const auto themeName = obj.value(QStringLiteral("name")).toString();

        // Skip if name is empty or collides with built-in
        if (themeName.isEmpty() || isBuiltIn(themeName)) {
            continue;
        }

        Theme theme{themeName, false, MarkdownAppearance::fromJson(obj)};
        customThemes.append(theme);
    }

    // Sort custom themes by name (case-insensitive)
    std::sort(customThemes.begin(), customThemes.end(),
              [](const Theme &a, const Theme &b) {
                  return a.name.toLower() < b.name.toLower();
              });
}

QString ThemeManager::themesDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/themes");
}
