#pragma once

#include "MarkdownAppearance.h"
#include "ThemeManager.h"

#include <QMainWindow>
#include <QUrl>

class QAction;
class QFileSystemWatcher;
class QFrame;
class QLineEdit;
class QMenu;
class QWebEngineView;
class TocPanel;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    bool loadMarkdownFile(const QString &filePath);

protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void loadAppearanceSettings();
    void restoreWindowGeometry();
    void closeFindBar();
    void createActions();
    void createFindBar();
    void createTocOverlay();
    void setTocVisible(bool visible);
    void positionTocOverlay();
    void populateToc();
    void navigateToHeading(const QString &id);
    void createToolBar();
    void createUi();
    void findNext();
    void findPrevious();
    void findTextChanged(const QString &text);
    void openAppearanceOptions();
    void setFindBarVisible(bool visible);
    void setFindFeedback(bool found);
    void positionFindBar();
    void showViewerContextMenu(const QPoint &position);
    void performFind(bool backwards = false);
    void applyAppearance(const MarkdownAppearance &appearance);
    void renderCurrentDocument(bool preserveScroll = false);
    void setHtmlPreservingScroll(const QString &html, int x, int y);
    QString chooseMarkdownFile();
    void openDocument();
    void openDocumentInNewWindow();
    void newWindow();
    MainWindow *createSiblingWindow();
    void setCurrentFile(const QString &filePath);
    void showWelcomeMessage();
    void updateWindowTitle();
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void installDragDropFilter(QObject *object);
    void onFileChanged(const QString &path);
    void addToRecentFiles(const QString &path);
    void buildRecentFilesMenu();

    ThemeManager themeManager;
    MarkdownAppearance appearance {MarkdownAppearance::defaults()};
    QWebEngineView *viewer {};
    QFileSystemWatcher *fileWatcher {};
    QMenu *recentFilesMenu {};
    QFrame *findBar {};
    QLineEdit *findLineEdit {};
    TocPanel *tocPanel {};
    QAction *tocAction {};
    QAction *openAction {};
    QAction *openInNewWindowAction {};
    QAction *newWindowAction {};
    QAction *appearanceAction {};
    QAction *findAction {};
    QAction *findNextAction {};
    QAction *findPreviousAction {};
    QAction *closeFindAction {};
    QAction *zoomInAction {};
    QAction *zoomOutAction {};
    QAction *zoomResetAction {};
    QAction *quitAction {};
    QString currentFile;
    QString currentMarkdown;
    QUrl currentBaseUrl;
};
