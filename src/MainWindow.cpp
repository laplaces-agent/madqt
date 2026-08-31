#include "LinkPolicyWebPage.h"
#include "ManageThemesDialog.h"
#include "MainWindow.h"
#include "TocPanel.h"

#include <QAction>
#include <QApplication>
#include <QChildEvent>
#include <QCloseEvent>
#include <QDialog>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFrame>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QResizeEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QWebEngineContextMenuRequest>
#include <QWebEnginePage>
#include <QWebEngineView>

#include <QJsonArray>
#include <QJsonDocument>

#include <memory>

namespace
{
constexpr auto kApplicationTitle = "madqt";
constexpr auto kWindowGeometryKey = "window/geometry";
constexpr auto kRecentFilesKey = "recentFiles";
constexpr auto kLastOpenDirKey = "lastOpenDirectory";
constexpr int kMaxRecentFiles = 10;

QString firstLocalMarkdownPath(const QMimeData *mimeData)
{
    if (mimeData == nullptr || !mimeData->hasUrls()) {
        return {};
    }

    for (const QUrl &url : mimeData->urls()) {
        if (!url.isLocalFile()) {
            continue;
        }

        const QFileInfo fileInfo(url.toLocalFile());
        if (fileInfo.isFile()) {
            return fileInfo.absoluteFilePath();
        }
    }

    return {};
}

QString jsStringLiteral(const QString &value)
{
    const QJsonDocument json(QJsonArray {value});
    const QString encoded = QString::fromUtf8(json.toJson(QJsonDocument::Compact));
    return encoded.mid(1, encoded.size() - 2);
}

QWidget *expandingSpacer(QWidget *parent)
{
    auto spacer = new QWidget(parent);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    return spacer;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    loadAppearanceSettings();
    createActions();
    createToolBar();
    createUi();
    createFindBar();
    createTocOverlay();
    updateWindowTitle();

    fileWatcher = new QFileSystemWatcher(this);
    connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::onFileChanged);

    setAcceptDrops(true);
    resize(960, 720);
    restoreWindowGeometry();
}

bool MainWindow::loadMarkdownFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(
            this,
            tr("Unable to Open File"),
            tr("Could not open:\n%1").arg(QDir::toNativeSeparators(filePath)));
        return false;
    }

    const QByteArray rawContent = file.readAll();
    const QString markdown = QString::fromUtf8(rawContent);
    const QFileInfo fileInfo(file);

    if (!currentFile.isEmpty()) {
        fileWatcher->removePath(currentFile);
    }

    currentBaseUrl = QUrl::fromLocalFile(fileInfo.absolutePath() + '/');
    currentMarkdown = markdown;
    renderCurrentDocument();

    const QString absolutePath = fileInfo.absoluteFilePath();
    setCurrentFile(absolutePath);
    fileWatcher->addPath(absolutePath);
    addToRecentFiles(absolutePath);
    return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue(QLatin1String(kWindowGeometryKey), saveGeometry());
    QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (findBar != nullptr && findBar->isVisible()) {
        positionFindBar();
    }
    if (tocPanel != nullptr && tocPanel->isVisible()) {
        positionTocOverlay();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (firstLocalMarkdownPath(event->mimeData()).isEmpty()) {
        event->ignore();
        return;
    }

    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QString filePath = firstLocalMarkdownPath(event->mimeData());
    if (filePath.isEmpty()) {
        event->ignore();
        return;
    }

    if (loadMarkdownFile(filePath)) {
        event->acceptProposedAction();
        return;
    }

    event->ignore();
}

void MainWindow::installDragDropFilter(QObject *object)
{
    object->installEventFilter(this);
    for (QObject *child : object->children()) {
        installDragDropFilter(child);
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::ChildAdded:
        // Follow the render delegate (and anything nested under it) as it is
        // created, so the filter covers the widget that actually receives drops.
        installDragDropFilter(static_cast<QChildEvent *>(event)->child());
        break;
    case QEvent::DragEnter:
    case QEvent::DragMove: {
        auto *dragEvent = static_cast<QDragMoveEvent *>(event);
        if (!firstLocalMarkdownPath(dragEvent->mimeData()).isEmpty()) {
            dragEvent->acceptProposedAction();
            return true;
        }
        break;
    }
    case QEvent::Drop: {
        auto *dropEvent = static_cast<QDropEvent *>(event);
        const QString filePath = firstLocalMarkdownPath(dropEvent->mimeData());
        if (!filePath.isEmpty() && loadMarkdownFile(filePath)) {
            dropEvent->acceptProposedAction();
            return true;
        }
        break;
    }
    default:
        break;
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::createActions()
{
    openAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpen), tr("Open…"), this);
    openAction->setShortcut(QKeySequence::Open);
    openAction->setToolTip(
        tr("Open a Markdown file (%1)").arg(QKeySequence(QKeySequence::Open).toString(QKeySequence::NativeText)));
    connect(openAction, &QAction::triggered, this, &MainWindow::openDocument);

    newWindowAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::WindowNew), tr("New Window"), this);
    newWindowAction->setShortcut(QKeySequence::New);
    newWindowAction->setToolTip(
        tr("Open a new window (%1)").arg(QKeySequence(QKeySequence::New).toString(QKeySequence::NativeText)));
    connect(newWindowAction, &QAction::triggered, this, &MainWindow::newWindow);

    const QKeySequence openInNewWindowShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_O);
    openInNewWindowAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpen), tr("Open in New Window…"), this);
    openInNewWindowAction->setShortcut(openInNewWindowShortcut);
    openInNewWindowAction->setToolTip(
        tr("Open a Markdown file in a new window (%1)").arg(openInNewWindowShortcut.toString(QKeySequence::NativeText)));
    connect(openInNewWindowAction, &QAction::triggered, this, &MainWindow::openDocumentInNewWindow);

    findAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::EditFind), tr("Find"), this);
    findAction->setCheckable(true);
    findAction->setShortcut(QKeySequence::Find);
    findAction->setToolTip(
        tr("Find in document (%1)").arg(QKeySequence(QKeySequence::Find).toString(QKeySequence::NativeText)));
    connect(findAction, &QAction::toggled, this, &MainWindow::setFindBarVisible);

    findNextAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::GoDown), tr("Find Next"), this);
    findNextAction->setShortcut(QKeySequence::FindNext);
    connect(findNextAction, &QAction::triggered, this, &MainWindow::findNext);

    findPreviousAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::GoUp), tr("Find Previous"), this);
    findPreviousAction->setShortcut(QKeySequence::FindPrevious);
    connect(findPreviousAction, &QAction::triggered, this, &MainWindow::findPrevious);

    closeFindAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::WindowClose), tr("Close Find Bar"), this);
    closeFindAction->setShortcut(QKeySequence::Cancel);
    closeFindAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(closeFindAction, &QAction::triggered, this, &MainWindow::closeFindBar);

    tocAction = new QAction(
        QIcon::fromTheme(QStringLiteral("view-list-tree"),
                         QIcon::fromTheme(QStringLiteral("format-list-unordered"))),
        tr("Table of Contents"),
        this);
    tocAction->setCheckable(true);
    const QKeySequence tocShortcut(Qt::CTRL | Qt::Key_T);
    tocAction->setShortcut(tocShortcut);
    tocAction->setToolTip(
        tr("Show the table of contents (%1)").arg(tocShortcut.toString(QKeySequence::NativeText)));
    connect(tocAction, &QAction::toggled, this, &MainWindow::setTocVisible);

    appearanceAction = new QAction(
        QIcon::fromTheme(QStringLiteral("configure"), QIcon::fromTheme(QIcon::ThemeIcon::DocumentProperties)),
        tr("Themes…"),
        this);
    connect(appearanceAction, &QAction::triggered, this, &MainWindow::openAppearanceOptions);

    zoomInAction = new QAction(tr("Zoom In"), this);
    zoomInAction->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_Equal), QKeySequence(Qt::CTRL | Qt::Key_Plus)});
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);

    zoomOutAction = new QAction(tr("Zoom Out"), this);
    zoomOutAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus));
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);

    zoomResetAction = new QAction(tr("Reset Zoom"), this);
    zoomResetAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    connect(zoomResetAction, &QAction::triggered, this, &MainWindow::zoomReset);

    quitAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::ApplicationExit), tr("Quit"), this);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    // Menu-only and hidden-bar actions still need window-level shortcut routing.
    // closeFindAction is intentionally excluded: it is scoped to the find bar via
    // Qt::WidgetWithChildrenShortcut (see createFindBar). Adding it here too would
    // make its Esc shortcut fire for every descendant of the window — including the
    // TOC panel's search field — stealing Esc before that panel can handle it.
    addActions({openAction, openInNewWindowAction, newWindowAction, findAction, findNextAction,
                findPreviousAction, tocAction, appearanceAction, zoomInAction,
                zoomOutAction, zoomResetAction, quitAction});
}

void MainWindow::createToolBar()
{
    QToolBar *toolBar = addToolBar(tr("Main Toolbar"));
    toolBar->setMovable(false);
    toolBar->setFloatable(false);
    toolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    toolBar->addAction(openAction);
    toolBar->addAction(tocAction);
    toolBar->addAction(findAction);
    toolBar->addWidget(expandingSpacer(toolBar));

    auto menu = new QMenu(this);
    menu->addAction(newWindowAction);
    menu->addAction(openInNewWindowAction);
    menu->addSeparator();
    recentFilesMenu = new QMenu(tr("Recent Files"), menu);
    connect(recentFilesMenu, &QMenu::aboutToShow, this, &MainWindow::buildRecentFilesMenu);
    menu->addMenu(recentFilesMenu);
    menu->addAction(appearanceAction);
    menu->addSeparator();
    menu->addAction(quitAction);

    auto menuButton = new QToolButton(toolBar);
    menuButton->setIcon(
        QIcon::fromTheme(QStringLiteral("open-menu-symbolic"), QIcon::fromTheme(QStringLiteral("application-menu"))));
    menuButton->setText(tr("Menu"));
    menuButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    menuButton->setToolTip(tr("Menu (F10)"));
    menuButton->setShortcut(Qt::Key_F10);
    menuButton->setPopupMode(QToolButton::InstantPopup);
    menuButton->setStyleSheet(QStringLiteral("QToolButton::menu-indicator { image: none; }"));
    menuButton->setMenu(menu);
    toolBar->addWidget(menuButton);
}

void MainWindow::createUi()
{
    viewer = new QWebEngineView(this);
    viewer->setPage(new LinkPolicyWebPage(LinkPolicy::OpenExternally, viewer));
    viewer->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(viewer, &QWebEngineView::customContextMenuRequested, this, &MainWindow::showViewerContextMenu);
    setCentralWidget(viewer);

    viewer->pageAction(QWebEnginePage::Copy)->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditCopy));
    viewer->pageAction(QWebEnginePage::SelectAll)->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditSelectAll));

    // QWebEngineView spawns an internal render-delegate child that grabs drag
    // events before they can bubble to the window, defeating MainWindow's own
    // drag handlers. Filter the view and its whole child subtree so drops onto
    // the document area still open files. The delegate is created lazily on
    // first content, so ChildAdded keeps coverage current.
    //
    // Note: accepting DragMove here is the standard way to get the copy cursor,
    // but under native Wayland QtWebEngine does not propagate it to the
    // compositor, so the "no-drop" cursor lingers during the drag (cosmetic;
    // the drop itself works). It shows correctly under XWayland.
    installDragDropFilter(viewer);

    showWelcomeMessage();
}

void MainWindow::showViewerContextMenu(const QPoint &position)
{
    QMenu menu(viewer);
    menu.addAction(viewer->pageAction(QWebEnginePage::Copy));

    const QWebEngineContextMenuRequest *request = viewer->lastContextMenuRequest();
    if (request != nullptr && !request->linkUrl().isEmpty()) {
        menu.addAction(viewer->pageAction(QWebEnginePage::CopyLinkToClipboard));
    }

    menu.addSeparator();
    menu.addAction(viewer->pageAction(QWebEnginePage::SelectAll));
    menu.exec(viewer->mapToGlobal(position));
}

void MainWindow::createFindBar()
{
    // An overlay on top of the viewer, outside any layout, so showing it never reflows the document.
    findBar = new QFrame(this);
    findBar->setObjectName(QStringLiteral("findBar"));
    findBar->setAutoFillBackground(true);
    findBar->setStyleSheet(QStringLiteral(
        "QFrame#findBar {"
        " background-color: palette(window);"
        " border: 1px solid palette(mid);"
        " border-radius: 6px;"
        "}"));

    findLineEdit = new QLineEdit(findBar);
    findLineEdit->setClearButtonEnabled(true);
    findLineEdit->setPlaceholderText(tr("Find in document…"));
    findLineEdit->setMinimumWidth(240);
    connect(findLineEdit, &QLineEdit::returnPressed, this, &MainWindow::findNext);
    connect(findLineEdit, &QLineEdit::textChanged, this, &MainWindow::findTextChanged);

    auto toolButton = [this](QAction *action) {
        auto button = new QToolButton(findBar);
        button->setDefaultAction(action);
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::NoFocus);
        return button;
    };

    auto layout = new QHBoxLayout(findBar);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(4);
    layout->addWidget(findLineEdit);
    layout->addWidget(toolButton(findNextAction));
    layout->addWidget(toolButton(findPreviousAction));
    layout->addWidget(toolButton(closeFindAction));

    // Scope the Esc shortcut to the find bar so it doesn't intercept Esc in
    // other overlays (e.g. the TOC panel) when the find bar is hidden.
    findBar->addAction(closeFindAction);
    findBar->hide();
}

void MainWindow::positionFindBar()
{
    if (centralWidget() == nullptr) {
        return;
    }

    constexpr int kMargin = 12;
    findBar->adjustSize();
    const QRect area = centralWidget()->geometry();
    findBar->move(area.right() - findBar->width() - kMargin, area.top() + kMargin);
    findBar->raise();
}

void MainWindow::createTocOverlay()
{
    tocPanel = new TocPanel(this);
    connect(tocPanel, &TocPanel::headingActivated, this, &MainWindow::navigateToHeading);
    connect(tocPanel, &TocPanel::closeRequested, this, [this]() { tocAction->setChecked(false); });
    tocPanel->hide();
}

void MainWindow::setTocVisible(bool visible)
{
    if (visible) {
        // The two overlays share the top-right corner, so only one shows at a time.
        if (findAction->isChecked()) {
            findAction->setChecked(false);
        }
        populateToc();
        positionTocOverlay();
        tocPanel->show();
        tocPanel->setFocus();
    } else {
        tocPanel->hide();
        viewer->setFocus();
    }
}

void MainWindow::positionTocOverlay()
{
    if (centralWidget() == nullptr) {
        return;
    }

    constexpr int kMargin = 12;
    const QRect area = centralWidget()->geometry();
    tocPanel->setFixedHeight(qMin(area.height() - 2 * kMargin, area.height() * 3 / 5));
    tocPanel->move(area.right() - tocPanel->width() - kMargin, area.top() + kMargin);
    tocPanel->raise();
}

void MainWindow::populateToc()
{
    // Read the headings straight from the rendered DOM so the list always
    // reflects what is on screen, including the anchor ids the Rust renderer
    // generated. Returning the array lets QtWebEngine marshal it to a
    // QVariantList of QVariantMaps — no JSON parsing needed.
    const QString script = QStringLiteral(
        "Array.from(document.querySelectorAll('h1,h2,h3,h4,h5,h6')).map(function(h){"
        "return {level: parseInt(h.tagName.substring(1)), text: h.textContent.trim(), id: h.id};"
        "});");
    viewer->page()->runJavaScript(script, [this](const QVariant &result) {
        QList<TocPanel::Heading> headings;
        const QVariantList entries = result.toList();
        headings.reserve(entries.size());
        for (const QVariant &entry : entries) {
            const QVariantMap heading = entry.toMap();
            headings.append({heading.value(QStringLiteral("level")).toInt(),
                             heading.value(QStringLiteral("text")).toString(),
                             heading.value(QStringLiteral("id")).toString()});
        }
        tocPanel->setHeadings(headings);
    });
}

void MainWindow::navigateToHeading(const QString &id)
{
    if (id.isEmpty()) {
        return;
    }
    const QString script = QStringLiteral(
        "(function(){var el=document.getElementById(%1);"
        "if(el){el.scrollIntoView({behavior:'smooth',block:'start'});}})();")
        .arg(jsStringLiteral(id));
    viewer->page()->runJavaScript(script);
}

void MainWindow::openAppearanceOptions()
{
    // Shown application-modal rather than exec()'d so the manager runs in the
    // main event loop. The per-theme editor it opens (a QWebEngineView-bearing
    // dialog) then sits at a single nested-modal level, matching the original
    // appearance dialog. Nesting a web-view dialog's exec() inside another
    // exec() deadlocks QtWebEngine on teardown.
    auto *dialog = new ManageThemesDialog(&themeManager, this);
    dialog->setWindowModality(Qt::ApplicationModal);
    connect(dialog, &ManageThemesDialog::activeThemeChanged, this, &MainWindow::applyAppearance);
    connect(dialog, &QDialog::finished, dialog, &QObject::deleteLater);
    dialog->show();
}

void MainWindow::setFindBarVisible(bool visible)
{
    if (visible && tocAction->isChecked()) {
        tocAction->setChecked(false);
    }
    findBar->setVisible(visible);
    if (visible) {
        positionFindBar();
        findLineEdit->setFocus();
        findLineEdit->selectAll();
    } else {
        setFindFeedback(true);
        viewer->setFocus();
    }
}

void MainWindow::findNext()
{
    performFind(false);
}

void MainWindow::findPrevious()
{
    performFind(true);
}

void MainWindow::findTextChanged(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        setFindFeedback(true);
        viewer->page()->runJavaScript(QStringLiteral("window.getSelection().removeAllRanges();"));
        return;
    }

    // Restart from the top so typing refines the match instead of walking away from it.
    const QString script = QStringLiteral(
                               "window.getSelection().removeAllRanges();"
                               "window.find(%1, false, false, true, false, false, false);")
                               .arg(jsStringLiteral(text.trimmed()));
    viewer->page()->runJavaScript(script, [this](const QVariant &result) {
        setFindFeedback(result.toBool());
    });
}

void MainWindow::performFind(bool backwards)
{
    const QString needle = findLineEdit->text().trimmed();
    if (needle.isEmpty()) {
        return;
    }

    const QString script = QStringLiteral("window.find(%1, false, %2, true, false, false, false);")
                               .arg(jsStringLiteral(needle), backwards ? QStringLiteral("true") : QStringLiteral("false"));
    viewer->page()->runJavaScript(script, [this](const QVariant &result) {
        setFindFeedback(result.toBool());
    });
}

void MainWindow::setFindFeedback(bool found)
{
    if (found) {
        findLineEdit->setPalette(QPalette());
        return;
    }

    // Blend the theme's base color toward Breeze negative red so it reads in light and dark themes.
    const QColor negative(218, 68, 83);
    const QColor base = findLineEdit->palette().color(QPalette::Base);
    const QColor blended(
        (base.red() * 7 + negative.red() * 3) / 10,
        (base.green() * 7 + negative.green() * 3) / 10,
        (base.blue() * 7 + negative.blue() * 3) / 10);

    QPalette palette = findLineEdit->palette();
    palette.setColor(QPalette::Base, blended);
    findLineEdit->setPalette(palette);
}

void MainWindow::onFileChanged(const QString &path)
{
    if (!QFile::exists(path)) {
        return;
    }
    // Re-add if the watcher dropped it (editors that do atomic save via rename remove the inode)
    if (!fileWatcher->files().contains(path)) {
        fileWatcher->addPath(path);
    }
    if (path != currentFile) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    currentMarkdown = QString::fromUtf8(file.readAll());
    renderCurrentDocument(/*preserveScroll=*/true);
}

void MainWindow::addToRecentFiles(const QString &path)
{
    QSettings settings;
    QStringList recent = settings.value(QLatin1String(kRecentFilesKey)).toStringList();
    recent.removeAll(path);
    recent.prepend(path);
    while (recent.size() > kMaxRecentFiles) {
        recent.removeLast();
    }
    settings.setValue(QLatin1String(kRecentFilesKey), recent);
}

void MainWindow::buildRecentFilesMenu()
{
    recentFilesMenu->clear();

    QSettings settings;
    const QStringList recent = settings.value(QLatin1String(kRecentFilesKey)).toStringList();

    for (const QString &path : recent) {
        if (!QFile::exists(path)) {
            continue;
        }
        QAction *action = recentFilesMenu->addAction(QFileInfo(path).fileName());
        action->setToolTip(path);
        action->setStatusTip(path);
        connect(action, &QAction::triggered, this, [this, path]() {
            loadMarkdownFile(path);
        });
    }

    if (recentFilesMenu->isEmpty()) {
        QAction *empty = recentFilesMenu->addAction(tr("No recent files"));
        empty->setEnabled(false);
    }
}

void MainWindow::zoomIn()
{
    viewer->setZoomFactor(qBound(0.25, viewer->zoomFactor() + 0.1, 5.0));
}

void MainWindow::zoomOut()
{
    viewer->setZoomFactor(qBound(0.25, viewer->zoomFactor() - 0.1, 5.0));
}

void MainWindow::zoomReset()
{
    viewer->setZoomFactor(1.0);
}

void MainWindow::applyAppearance(const MarkdownAppearance &appearance)
{
    this->appearance = appearance;
    renderCurrentDocument(/*preserveScroll=*/true);
}

void MainWindow::renderCurrentDocument(bool preserveScroll)
{
    const QString html = this->appearance.renderMarkdownToHtml(currentMarkdown);
    if (!preserveScroll) {
        viewer->setContent(html.toUtf8(), QStringLiteral("text/html;charset=UTF-8"), currentBaseUrl);
        return;
    }

    // Capture the current scroll offset, then restore it once the new page has
    // loaded. Keeps the reader in place across appearance changes and the
    // auto-reload that fires while an agent is rewriting the file.
    viewer->page()->runJavaScript(
        QStringLiteral("[Math.round(window.scrollX), Math.round(window.scrollY)]"),
        [this, html](const QVariant &result) {
            const QVariantList offset = result.toList();
            setHtmlPreservingScroll(html, offset.value(0).toInt(), offset.value(1).toInt());
        });
}

void MainWindow::setHtmlPreservingScroll(const QString &html, int x, int y)
{
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = connect(viewer, &QWebEngineView::loadFinished, this,
        [this, x, y, connection](bool ok) {
            QObject::disconnect(*connection);
            if (!ok || (x == 0 && y == 0)) {
                return;
            }
            // Scroll immediately, then again once web fonts settle: late font
            // layout grows the document, and an early scrollTo would otherwise
            // be clamped short of the intended offset.
            const QString restore = QStringLiteral(
                "(function(){var x=%1,y=%2;window.scrollTo(x,y);"
                "if(document.fonts&&document.fonts.ready){"
                "document.fonts.ready.then(function(){window.scrollTo(x,y);});}})();")
                .arg(x).arg(y);
            viewer->page()->runJavaScript(restore);
        });
    viewer->setContent(html.toUtf8(), QStringLiteral("text/html;charset=UTF-8"), currentBaseUrl);
}

void MainWindow::loadAppearanceSettings()
{
    appearance = themeManager.activeTheme().appearance;
}

void MainWindow::restoreWindowGeometry()
{
    QSettings settings;
    const QByteArray geometry = settings.value(QLatin1String(kWindowGeometryKey)).toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
}

void MainWindow::closeFindBar()
{
    findAction->setChecked(false);
}

QString MainWindow::chooseMarkdownFile()
{
    QSettings settings;
    const QString startDirectory = !currentFile.isEmpty()
        ? QFileInfo(currentFile).absolutePath()
        : settings.value(QLatin1String(kLastOpenDirKey)).toString();

    const QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Open Markdown File"),
        startDirectory,
        tr("Markdown Files (*.md *.markdown *.mdown *.mkd *.mkdn);;All Files (*)"));

    if (!filePath.isEmpty()) {
        settings.setValue(QLatin1String(kLastOpenDirKey), QFileInfo(filePath).absolutePath());
    }
    return filePath;
}

void MainWindow::openDocument()
{
    const QString filePath = chooseMarkdownFile();
    if (!filePath.isEmpty()) {
        loadMarkdownFile(filePath);
    }
}

void MainWindow::openDocumentInNewWindow()
{
    const QString filePath = chooseMarkdownFile();
    if (filePath.isEmpty()) {
        return;
    }
    createSiblingWindow()->loadMarkdownFile(filePath);
}

void MainWindow::newWindow()
{
    createSiblingWindow();
}

MainWindow *MainWindow::createSiblingWindow()
{
    // Heap-allocated and self-owning: unlike the initial window (a stack object
    // in main()), spawned windows must clean themselves up when closed.
    auto *window = new MainWindow();
    window->setAttribute(Qt::WA_DeleteOnClose);
    // Cascade slightly so a freshly spawned window doesn't perfectly cover this
    // one (ignored by some Wayland compositors, which place windows themselves).
    window->move(pos() + QPoint(30, 30));
    window->show();
    return window;
}

void MainWindow::setCurrentFile(const QString &filePath)
{
    currentFile = filePath;
    updateWindowTitle();
}

void MainWindow::showWelcomeMessage()
{
    currentMarkdown = QStringLiteral(R"(
# <span class="madqt-internal-splash-wordmark-7e3b91">madqt</span>

*A read-only markdown viewer for Linux.*

You're looking at a markdown document right now — this welcome page is itself
rendered by madqt.

## Get Started

- Click **Open** in the toolbar, or press `Ctrl+O`
- Or just **drag a `.md` file** into this window
- Pass a file on the command line: `madqt path/to/file.md`
- Open a fresh **New Window** with `Ctrl+N`, or a file in its own window
  with `Ctrl+Shift+O`

## Good to Know

- **Find** as you type with `Ctrl+F`; the bar floats over the page without
  reflowing it
- **Table of contents** with `Ctrl+T` — jump between headings, or filter them
  as you type
- **Themes** live in the menu (`F10`) — pick Light, Dark, or Sepia, or craft
  your own
- **Links** open in your browser; anchors scroll in place
- **Zoom** with `Ctrl++` / `Ctrl+-`, reset with `Ctrl+0`
- Documents reload on disk changes, so edits show up live
)");

    currentBaseUrl = QUrl();
    renderCurrentDocument();
}

void MainWindow::updateWindowTitle()
{
    if (currentFile.isEmpty()) {
        setWindowTitle(QStringLiteral("%1").arg(kApplicationTitle));
        return;
    }

    const QFileInfo fileInfo(currentFile);
    setWindowTitle(QStringLiteral("%1 — %2").arg(fileInfo.fileName(), kApplicationTitle));
}
