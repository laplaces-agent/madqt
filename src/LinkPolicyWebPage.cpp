#include "LinkPolicyWebPage.h"

#include <QDesktopServices>

LinkPolicyWebPage::LinkPolicyWebPage(LinkPolicy linkPolicy, QObject *parent)
    : QWebEnginePage(parent)
    , linkPolicy(linkPolicy)
{
}

bool LinkPolicyWebPage::acceptNavigationRequest(const QUrl &url, NavigationType navigationType, bool isMainFrame)
{
    if (navigationType != NavigationTypeLinkClicked) {
        return QWebEnginePage::acceptNavigationRequest(url, navigationType, isMainFrame);
    }

    if (linkPolicy == LinkPolicy::IgnoreClicks) {
        return false;
    }

    // Anchor links within the current document still scroll in place.
    if (url.hasFragment() && url.matches(this->url(), QUrl::RemoveFragment)) {
        return true;
    }

    QDesktopServices::openUrl(url);
    return false;
}
