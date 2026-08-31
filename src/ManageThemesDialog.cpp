#include "ManageThemesDialog.h"

#include "AppearanceDialog.h"
#include "ThemeManager.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

ManageThemesDialog::ManageThemesDialog(ThemeManager *manager, QWidget *parent)
    : QDialog(parent)
    , manager(manager)
{
    setWindowTitle(tr("Manage Themes"));
    resize(420, 360);

    createLayout();
    refreshThemeList(manager->activeThemeName());
}

void ManageThemesDialog::createLayout()
{
    auto layout = new QHBoxLayout;

    // Left: theme list
    themeList = new QListWidget;
    layout->addWidget(themeList, 1);

    // Right: buttons
    auto buttonLayout = new QVBoxLayout;
    newButton = new QPushButton(tr("New"));
    duplicateButton = new QPushButton(tr("Duplicate"));
    editButton = new QPushButton(tr("Edit"));
    deleteButton = new QPushButton(tr("Delete"));

    buttonLayout->addWidget(newButton);
    buttonLayout->addWidget(duplicateButton);
    buttonLayout->addWidget(editButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch();

    layout->addLayout(buttonLayout);

    // Bottom: close button
    auto dialogButtons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(dialogButtons, &QDialogButtonBox::rejected, this, &QDialog::accept);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(layout, 1);
    mainLayout->addWidget(dialogButtons);

    setLayout(mainLayout);

    // Connect signals
    connect(themeList, &QListWidget::currentItemChanged, this, &ManageThemesDialog::onSelectionChanged);
    connect(newButton, &QPushButton::clicked, this, &ManageThemesDialog::newTheme);
    connect(duplicateButton, &QPushButton::clicked, this, &ManageThemesDialog::duplicateTheme);
    connect(editButton, &QPushButton::clicked, this, &ManageThemesDialog::editTheme);
    connect(deleteButton, &QPushButton::clicked, this, &ManageThemesDialog::deleteTheme);
}

void ManageThemesDialog::refreshThemeList(const QString &selectName)
{
    QSignalBlocker blocker(themeList);

    themeList->clear();
    for (const Theme &t : manager->themes()) {
        auto item = new QListWidgetItem;
        QString displayName = t.name;
        if (t.builtIn) {
            displayName += tr(" (built-in)");
        }
        item->setText(displayName);
        item->setData(Qt::UserRole, t.name);
        themeList->addItem(item);
    }

    // Select the requested theme, fall back to active, then to row 0
    int selectRow = -1;
    if (!selectName.isEmpty()) {
        for (int i = 0; i < themeList->count(); ++i) {
            if (themeList->item(i)->data(Qt::UserRole).toString() == selectName) {
                selectRow = i;
                break;
            }
        }
    }
    if (selectRow < 0) {
        const QString activeName = manager->activeThemeName();
        for (int i = 0; i < themeList->count(); ++i) {
            if (themeList->item(i)->data(Qt::UserRole).toString() == activeName) {
                selectRow = i;
                break;
            }
        }
    }
    if (selectRow < 0 && themeList->count() > 0) {
        selectRow = 0;
    }

    if (selectRow >= 0) {
        themeList->setCurrentRow(selectRow);
    }

    updateButtonStates();
}

void ManageThemesDialog::onSelectionChanged()
{
    const QString name = selectedThemeName();
    if (!name.isEmpty() && name != manager->activeThemeName()) {
        manager->setActiveThemeName(name);
        emit activeThemeChanged(manager->themeByName(name).appearance);
    }
    updateButtonStates();
}

void ManageThemesDialog::updateButtonStates()
{
    const QString name = selectedThemeName();
    newButton->setEnabled(true);
    duplicateButton->setEnabled(!name.isEmpty());
    editButton->setEnabled(!name.isEmpty() && !manager->isBuiltIn(name));
    deleteButton->setEnabled(!name.isEmpty() && !manager->isBuiltIn(name));
}

QString ManageThemesDialog::selectedThemeName() const
{
    const QListWidgetItem *item = themeList->currentItem();
    if (!item) {
        return QString();
    }
    return item->data(Qt::UserRole).toString();
}

void ManageThemesDialog::newTheme()
{
    Theme t{manager->uniqueName(tr("New Theme")), false, MarkdownAppearance::defaults()};
    AppearanceDialog dlg(t, this);
    if (dlg.exec() == QDialog::Accepted) {
        commitTheme(dlg.editedTheme(), QString());
    }
}

void ManageThemesDialog::duplicateTheme()
{
    const QString name = selectedThemeName();
    if (name.isEmpty()) {
        return;
    }
    Theme src = manager->themeByName(name);
    Theme copy{manager->uniqueName(src.name + tr(" copy")), false, src.appearance};
    AppearanceDialog dlg(copy, this);
    if (dlg.exec() == QDialog::Accepted) {
        commitTheme(dlg.editedTheme(), QString());
    }
}

void ManageThemesDialog::editTheme()
{
    const QString name = selectedThemeName();
    if (name.isEmpty() || manager->isBuiltIn(name)) {
        return;
    }
    Theme t = manager->themeByName(name);
    AppearanceDialog dlg(t, this);
    if (dlg.exec() == QDialog::Accepted) {
        commitTheme(dlg.editedTheme(), name);
    }
}

void ManageThemesDialog::deleteTheme()
{
    const QString name = selectedThemeName();
    if (name.isEmpty() || manager->isBuiltIn(name)) {
        return;
    }
    if (QMessageBox::question(this, tr("Delete Theme"), tr("Delete theme \"%1\"?").arg(name))
        != QMessageBox::Yes) {
        return;
    }
    const bool wasActive = manager->activeThemeName() == name;
    manager->deleteCustomTheme(name);
    if (wasActive) {
        manager->setActiveThemeName(QStringLiteral("Light"));
        emit activeThemeChanged(manager->themeByName(QStringLiteral("Light")).appearance);
    }
    refreshThemeList(manager->activeThemeName());
}

void ManageThemesDialog::commitTheme(Theme edited, const QString &oldName)
{
    QString newName = edited.name.trimmed();
    if (newName.isEmpty()) {
        newName = oldName.isEmpty() ? manager->uniqueName(tr("Untitled")) : oldName;
    }
    const bool wasActive = !oldName.isEmpty() && manager->activeThemeName() == oldName;
    if (!oldName.isEmpty() && newName != oldName) {
        manager->deleteCustomTheme(oldName);
    }
    if (newName != oldName && manager->nameExists(newName)) {
        newName = manager->uniqueName(newName);
    }
    edited.name = newName;
    edited.builtIn = false;
    manager->saveCustomTheme(edited);
    if (oldName.isEmpty() || wasActive) {
        manager->setActiveThemeName(newName);
        emit activeThemeChanged(edited.appearance);
    }
    refreshThemeList(newName);
}
