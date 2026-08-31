#include "ResourceSchemeHandler.h"

#include <QFile>
#include <QMimeDatabase>
#include <QUrl>
#include <QWebEngineProfile>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlScheme>

namespace
{
constexpr auto kSchemeName = "madqt";
}

ResourceSchemeHandler::ResourceSchemeHandler(QObject *parent)
    : QWebEngineUrlSchemeHandler(parent)
{
}

void ResourceSchemeHandler::requestStarted(QWebEngineUrlRequestJob *job)
{
    const QString resourcePath = QLatin1Char(':') + job->requestUrl().path();

    auto *file = new QFile(resourcePath, job);
    if (!file->open(QIODevice::ReadOnly)) {
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }

    const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(resourcePath);
    job->reply(mimeType.name().toUtf8(), file);
}

void ResourceSchemeHandler::registerScheme()
{
    QWebEngineUrlScheme scheme(kSchemeName);
    scheme.setSyntax(QWebEngineUrlScheme::Syntax::Path);
    scheme.setFlags(QWebEngineUrlScheme::SecureScheme
                    | QWebEngineUrlScheme::LocalScheme
                    | QWebEngineUrlScheme::CorsEnabled);
    QWebEngineUrlScheme::registerScheme(scheme);
}

void ResourceSchemeHandler::install()
{
    static ResourceSchemeHandler handler;
    QWebEngineProfile::defaultProfile()->installUrlSchemeHandler(kSchemeName, &handler);
}
