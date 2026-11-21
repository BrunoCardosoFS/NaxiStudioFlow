#ifndef TIMEUTILS_H
#define TIMEUTILS_H

#include <QString>

QString msec2string(qint64 msec);
QString formatDuration(qint64 msec, bool forceHours = false);

#endif // TIMEUTILS_H


