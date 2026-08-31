#pragma once

#include <QFrame>
#include <QList>
#include <QString>

class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

// An overlay panel listing the document's headings as a navigable outline.
// It is a pure view: the owner feeds it headings and reacts to its signals,
// keeping all web-engine interaction out of the widget.
class TocPanel final : public QFrame
{
    Q_OBJECT

public:
    struct Heading
    {
        int level {};
        QString text;
        QString id;
    };

    explicit TocPanel(QWidget *parent = nullptr);

    void setHeadings(const QList<Heading> &headings);

signals:
    void headingActivated(const QString &id);
    void closeRequested();

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void applyFilter(const QString &text);
    QList<QTreeWidgetItem *> visibleItems() const;
    void activateItem(QTreeWidgetItem *item);
    void navigateVisible(int delta);

    QLineEdit *searchField {};
    QTreeWidget *tree {};
};
