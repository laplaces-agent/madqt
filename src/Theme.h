#pragma once

#include "MarkdownAppearance.h"
#include <QString>

struct Theme
{
    QString name;
    bool builtIn = false;
    MarkdownAppearance appearance;
};
