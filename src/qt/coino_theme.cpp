#include "coino_theme.h"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QColor>
#include <QStyleFactory>

void applyCoinoDarkTheme(QApplication &app)
{
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette p;
    p.setColor(QPalette::Window,          QColor(0x1e, 0x1e, 0x1e));
    p.setColor(QPalette::WindowText,      QColor(0xe8, 0xe8, 0xe8));
    p.setColor(QPalette::Base,            QColor(0x1e, 0x1e, 0x1e));
    p.setColor(QPalette::AlternateBase,   QColor(0x23, 0x23, 0x25));
    p.setColor(QPalette::ToolTipBase,     QColor(0x2c, 0x2c, 0x2e));
    p.setColor(QPalette::ToolTipText,     QColor(0xe8, 0xe8, 0xe8));
    p.setColor(QPalette::Text,            QColor(0xe8, 0xe8, 0xe8));
    p.setColor(QPalette::Button,          QColor(0x2c, 0x2c, 0x2e));
    p.setColor(QPalette::ButtonText,      QColor(0xe8, 0xe8, 0xe8));
    p.setColor(QPalette::BrightText,      QColor(0xff, 0x45, 0x3a));
    p.setColor(QPalette::Link,            QColor(0x0a, 0x84, 0xff));
    p.setColor(QPalette::Highlight,       QColor(0x0a, 0x84, 0xff));
    p.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));

    p.setColor(QPalette::Disabled, QPalette::Text,       QColor(0x5a, 0x5a, 0x5c));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x5a, 0x5a, 0x5c));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x5a, 0x5a, 0x5c));

    app.setPalette(p);

    QFile qss(":/images/coino.qss");
    if (qss.open(QFile::ReadOnly)) {
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));
        qss.close();
    }
}
