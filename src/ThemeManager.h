#pragma once

#include "Theme.h"
#include <QList>
#include <QString>

class ThemeManager
{
public:
    ThemeManager();

    [[nodiscard]] QList<Theme> themes() const;
    [[nodiscard]] Theme themeByName(const QString &name) const;
    [[nodiscard]] Theme activeTheme() const;
    [[nodiscard]] QString activeThemeName() const;
    void setActiveThemeName(const QString &name);

    [[nodiscard]] bool isBuiltIn(const QString &name) const;
    [[nodiscard]] bool nameExists(const QString &name) const;
    [[nodiscard]] QString uniqueName(const QString &base) const;

    bool saveCustomTheme(const Theme &theme);
    bool deleteCustomTheme(const QString &name);
    void reload();

    [[nodiscard]] static QString themesDir();

private:
    QList<Theme> builtinThemes;
    QList<Theme> customThemes;
    QString activeName;
};
