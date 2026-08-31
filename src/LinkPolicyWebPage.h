#pragma once

#include <QWebEnginePage>

enum class LinkPolicy
{
    OpenExternally,
    IgnoreClicks,
};

class LinkPolicyWebPage final : public QWebEnginePage
{
    Q_OBJECT

public:
    explicit LinkPolicyWebPage(LinkPolicy linkPolicy, QObject *parent = nullptr);

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType navigationType, bool isMainFrame) override;

private:
    LinkPolicy linkPolicy;
};
