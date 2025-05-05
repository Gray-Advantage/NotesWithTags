#include "themeutils.h"
#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>

void ThemeUtils::applyTheme(const QString &theme)
{
    if (theme == "dark") {
        QApplication::setStyle(QStyleFactory::create("Fusion"));
    } else {
        QApplication::setStyle(QStyleFactory::create("WindowsVista"));
    }
}
