#include "MainWindow.h"
#include "ResourceSchemeHandler.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDirIterator>
#include <QFileInfo>
#include <QFontDatabase>
#include <QIcon>
#include <QStringList>

namespace
{
void registerBundledFonts()
{
    QDirIterator fontIterator(QStringLiteral(":/fonts"), {QStringLiteral("*.ttf")});
    while (fontIterator.hasNext()) {
        QFontDatabase::addApplicationFont(fontIterator.next());
    }
}
}

int main(int argc, char *argv[])
{
    ResourceSchemeHandler::registerScheme();

    QApplication app(argc, argv);
    registerBundledFonts();
    ResourceSchemeHandler::install();
    QApplication::setApplicationName(QStringLiteral("madqt"));
    QApplication::setApplicationDisplayName(QStringLiteral("madqt"));
    QApplication::setOrganizationName(QStringLiteral("madqt"));
    QApplication::setWindowIcon(
        QIcon::fromTheme(QStringLiteral("madqt"), QIcon(QStringLiteral(":/icons/madqt.svg"))));
    QApplication::setDesktopFileName(QStringLiteral("madqt"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("A simple markdown viewer built with Qt Widgets."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("Markdown file to open."));
    parser.process(app);

    MainWindow window;
    window.show();

    const QStringList positionalArguments = parser.positionalArguments();
    if (!positionalArguments.isEmpty()) {
        const QString requestedPath = QFileInfo(positionalArguments.constFirst()).absoluteFilePath();
        window.loadMarkdownFile(requestedPath);
    }

    return app.exec();
}
