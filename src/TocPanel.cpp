#include "TocPanel.h"

#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QShowEvent>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

TocPanel::TocPanel(QWidget *parent)
    : QFrame(parent)
{
    // An overlay outside any layout so toggling it never reflows the document.
    setObjectName(QStringLiteral("tocOverlay"));
    setAutoFillBackground(true);
    setStyleSheet(QStringLiteral(
        "QFrame#tocOverlay {"
        " background-color: palette(window);"
        " border: 1px solid palette(mid);"
        " border-radius: 6px;"
        "}"));
    setFixedWidth(300);

    auto *header = new QLabel(tr("Contents"), this);
    QFont headerFont = header->font();
    headerFont.setBold(true);
    header->setFont(headerFont);

    auto *closeButton = new QToolButton(this);
    closeButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::WindowClose));
    closeButton->setAutoRaise(true);
    closeButton->setFocusPolicy(Qt::NoFocus);
    closeButton->setToolTip(tr("Close"));
    connect(closeButton, &QToolButton::clicked, this, &TocPanel::closeRequested);

    auto *headerRow = new QHBoxLayout;
    headerRow->setContentsMargins(0, 0, 0, 0);
    headerRow->addWidget(header);
    headerRow->addStretch();
    headerRow->addWidget(closeButton);

    searchField = new QLineEdit(this);
    searchField->setPlaceholderText(tr("Filter…"));
    searchField->setClearButtonEnabled(true);
    searchField->installEventFilter(this);
    connect(searchField, &QLineEdit::textChanged, this, &TocPanel::applyFilter);
    setFocusProxy(searchField);

    tree = new QTreeWidget(this);
    tree->setHeaderHidden(true);
    tree->setRootIsDecorated(false);
    tree->setIndentation(14);
    tree->setUniformRowHeights(true);
    tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // Keep keyboard focus on the search field at all times so that Esc, arrows,
    // and Enter are always handled there. Clicks still fire itemClicked.
    tree->setFocusPolicy(Qt::NoFocus);
    connect(tree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int) { activateItem(item); });
    connect(tree, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *item, int) { activateItem(item); });

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 10);
    layout->setSpacing(6);
    layout->addLayout(headerRow);
    layout->addWidget(searchField);
    layout->addWidget(tree);
}

void TocPanel::setHeadings(const QList<Heading> &headings)
{
    // The search field is cleared on show(); don't touch it here. setHeadings is
    // fed asynchronously from a runJavaScript callback that can land after the
    // panel is already visible and focused — clearing it then would wipe whatever
    // the user has started typing into the filter.
    tree->clear();

    if (headings.isEmpty()) {
        auto *placeholder = new QTreeWidgetItem(tree);
        placeholder->setText(0, tr("No headings"));
        placeholder->setFlags(Qt::NoItemFlags);
        return;
    }

    // Build the hierarchy by heading level: each item nests under the most
    // recent shallower heading.
    QList<QPair<int, QTreeWidgetItem *>> stack;
    for (const Heading &heading : headings) {
        while (!stack.isEmpty() && stack.last().first >= heading.level) {
            stack.removeLast();
        }

        auto *item = stack.isEmpty()
            ? new QTreeWidgetItem(tree)
            : new QTreeWidgetItem(stack.last().second);
        item->setText(0, heading.text);
        item->setToolTip(0, heading.text);
        item->setData(0, Qt::UserRole, heading.id);
        stack.append({heading.level, item});
    }
    tree->expandAll();
}

void TocPanel::showEvent(QShowEvent *event)
{
    QFrame::showEvent(event);
    searchField->clear();
    searchField->setFocus();
}

bool TocPanel::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() != QEvent::KeyPress)
        return QFrame::eventFilter(obj, event);

    auto *key = static_cast<QKeyEvent *>(event);

    if (obj != searchField)
        return QFrame::eventFilter(obj, event);

    if (key->key() == Qt::Key_Escape) {
        if (!searchField->text().isEmpty()) {
            searchField->clear();
            return true;
        }
        emit closeRequested();
        return true;
    }

    switch (key->key()) {
    case Qt::Key_Down:
        navigateVisible(1);
        return true;
    case Qt::Key_Up:
        navigateVisible(-1);
        return true;
    case Qt::Key_Return:
    case Qt::Key_Enter: {
        QTreeWidgetItem *item = tree->currentItem();
        if (!item) {
            const auto visible = visibleItems();
            if (!visible.isEmpty())
                item = visible.first();
        }
        if (item)
            activateItem(item);
        return true;
    }
    default:
        break;
    }
    return QFrame::eventFilter(obj, event);
}

static bool filterItem(QTreeWidgetItem *item, const QString &text)
{
    const bool selfMatches = item->text(0).contains(text, Qt::CaseInsensitive);
    bool anyChildMatches = false;
    for (int i = 0; i < item->childCount(); i++) {
        if (filterItem(item->child(i), text))
            anyChildMatches = true;
    }
    const bool visible = selfMatches || anyChildMatches;
    item->setHidden(!visible);
    return visible;
}

// Returns the first item whose text actually matches (depth-first), skipping
// parents that are only visible because a child matched.
static QTreeWidgetItem *firstMatchingItem(QTreeWidgetItem *item, const QString &text)
{
    if (item->isHidden())
        return nullptr;
    if (item->text(0).contains(text, Qt::CaseInsensitive))
        return item;
    for (int i = 0; i < item->childCount(); i++) {
        if (auto *found = firstMatchingItem(item->child(i), text))
            return found;
    }
    return nullptr;
}

void TocPanel::applyFilter(const QString &text)
{
    for (int i = 0; i < tree->topLevelItemCount(); i++)
        filterItem(tree->topLevelItem(i), text);

    QTreeWidgetItem *toSelect = nullptr;
    if (!text.isEmpty()) {
        for (int i = 0; i < tree->topLevelItemCount() && !toSelect; i++)
            toSelect = firstMatchingItem(tree->topLevelItem(i), text);
    }
    tree->setCurrentItem(toSelect);
}

static void collectVisible(QTreeWidgetItem *item, QList<QTreeWidgetItem *> &out)
{
    if (item->isHidden())
        return;
    out.append(item);
    for (int i = 0; i < item->childCount(); i++)
        collectVisible(item->child(i), out);
}

QList<QTreeWidgetItem *> TocPanel::visibleItems() const
{
    QList<QTreeWidgetItem *> result;
    for (int i = 0; i < tree->topLevelItemCount(); i++)
        collectVisible(tree->topLevelItem(i), result);
    return result;
}

void TocPanel::activateItem(QTreeWidgetItem *item)
{
    const QString id = item->data(0, Qt::UserRole).toString();
    if (!id.isEmpty())
        emit headingActivated(id);
}

void TocPanel::navigateVisible(int delta)
{
    const auto visible = visibleItems();
    if (visible.isEmpty())
        return;

    QTreeWidgetItem *current = tree->currentItem();
    const int idx = visible.indexOf(current);

    if (idx == -1) {
        tree->setCurrentItem(delta > 0 ? visible.first() : visible.last());
    } else {
        tree->setCurrentItem(visible[qBound(0, idx + delta, visible.size() - 1)]);
    }
}
