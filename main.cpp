#include "frontend/flow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QStyleFactory>
#include <QDirIterator>
#include <QFontDatabase>

#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/images/icons/logo.ico"));

    QFontDatabase::addApplicationFont(":/fonts/fonts/Roboto-VariableFont_wdth,wght.ttf");
    QFontDatabase::addApplicationFont(":/fonts/fonts/Roboto-Italic-VariableFont_wdth,wght.ttf");

    QFont globalFont("Roboto", 10);
    globalFont.setFeature("tnum", 1);
    a.setFont(globalFont);


    QSettings settings("NaxiStudio", "NaxiStudio Flow");

    QString locale;

    if(settings.contains("lang")){
        locale = settings.value("lang").toString();
    }else{
        locale = QLocale::system().name();
    }

    QTranslator translator;
    if(translator.load(":/i18n/" + locale + ".qm")){
        a.installTranslator(&translator);
    }

    Flow w;
    w.showMaximized();

    return a.exec();
}
