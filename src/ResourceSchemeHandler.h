#pragma once

#include <QWebEngineUrlSchemeHandler>

// Serves embedded Qt resources (bundled fonts) to web content as
// madqt:/<resource-path>. A dedicated scheme is required because Chromium
// fetches @font-face URLs with CORS, which the built-in qrc: scheme does not
// support.
class ResourceSchemeHandler final : public QWebEngineUrlSchemeHandler
{
    Q_OBJECT

public:
    explicit ResourceSchemeHandler(QObject *parent = nullptr);

    void requestStarted(QWebEngineUrlRequestJob *job) override;

    static void registerScheme();
    static void install();
};
