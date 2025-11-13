#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include <QLatin1String>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setStyle(QStyleFactory::create("Fusion"));

    // 加载自定义字体
    int fontId = QFontDatabase::addApplicationFont(":/styles/OPPOSans4.0.ttf");
    if (fontId != -1) {
        QString fontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);

        
        // 设置应用程序全局字体
        QFont font(fontFamily);
        font.setPointSize(10);  // 设置字体大小
        a.setFont(font);
    }
    
    QPalette palette = a.palette();
    palette.setColor(QPalette::Window, QColor(245, 245, 247));
    palette.setColor(QPalette::WindowText, QColor(0, 0, 0));
    palette.setColor(QPalette::Base, QColor(255, 255, 255));
    palette.setColor(QPalette::AlternateBase, QColor(242, 242, 247));
    palette.setColor(QPalette::Text, QColor(0, 0, 0));
    palette.setColor(QPalette::Button, QColor(242, 242, 247));
    palette.setColor(QPalette::ButtonText, QColor(0, 0, 0));
    palette.setColor(QPalette::Highlight, QColor(0, 122, 255));
    palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    a.setPalette(palette);

    /*加载和应用自定义主题样式表*/
    QFile qssFile(":/styles/styles.css");
    qssFile.open(QFile::ReadOnly);
    if(qssFile.isOpen()){
        qApp->setStyleSheet(QLatin1String(qssFile.readAll()));
        qssFile.close();
    }

    MainWindow w;
    w.show();
    return a.exec();
}
