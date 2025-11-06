#include "frontend/flow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QStyleFactory>

#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/images/icons/logo.ico"));
    // a.setStyle(QStyleFactory::create("Fusion"));

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
