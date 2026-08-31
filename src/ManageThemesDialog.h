#pragma once

#include "MarkdownAppearance.h"
#include "Theme.h"

#include <QDialog>

class QListWidget;
class QPushButton;
class ThemeManager;

class ManageThemesDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ManageThemesDialog(ThemeManager *manager, QWidget *parent = nullptr);

signals:
    // Emitted whenever the effective active theme changes (a different theme
    // selected, or the active theme edited/deleted) so the main window re-renders.
    void activeThemeChanged(const MarkdownAppearance &appearance);

private:
    void createLayout();
    void refreshThemeList(const QString &selectName);  // rebuild list, select the row whose theme name == selectName
    void onSelectionChanged();
    void updateButtonStates();
    QString selectedThemeName() const;                 // name stored in the current item's Qt::UserRole; empty if none
    void newTheme();
    void duplicateTheme();
    void editTheme();
    void deleteTheme();
    // Save an edited/new theme, handling rename + name collisions. oldName is empty for New/Duplicate.
    void commitTheme(Theme edited, const QString &oldName);

    ThemeManager *manager {};
    QListWidget *themeList {};
    QPushButton *newButton {};
    QPushButton *duplicateButton {};
    QPushButton *editButton {};
    QPushButton *deleteButton {};
};
