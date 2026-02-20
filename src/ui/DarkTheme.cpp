#include "DarkTheme.h"
#include <QPalette>
#include <QStyleFactory>

void DarkTheme::apply(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(45, 45, 48));
    darkPalette.setColor(QPalette::WindowText, QColor(212, 212, 212));
    darkPalette.setColor(QPalette::Base, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::AlternateBase, QColor(45, 45, 48));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(45, 45, 48));
    darkPalette.setColor(QPalette::ToolTipText, QColor(212, 212, 212));
    darkPalette.setColor(QPalette::Text, QColor(212, 212, 212));
    darkPalette.setColor(QPalette::Button, QColor(51, 51, 55));
    darkPalette.setColor(QPalette::ButtonText, QColor(212, 212, 212));
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);

    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(128, 128, 128));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));

    app.setPalette(darkPalette);
}
