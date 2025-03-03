#ifndef TIMEFUNCTIONS_H
#define TIMEFUNCTIONS_H

#include <QCoreApplication>
#include <QTime>

QString msec2string(qint64 msec){
    QTime time = QTime(0, 0).addMSecs(msec);

    QString format = "mm:ss:zzz";
    if(msec >= 3600000){
        format = "hh:mm:ss";
    }

    return time.toString(format);
}

#endif // TIMEFUNCTIONS_H
