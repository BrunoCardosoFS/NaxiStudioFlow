#include "TimeUtils.h"

#include <QTime>

QString msec2string(qint64 msec) {
  if (msec <= 0) {
    return QStringLiteral("00:00:000");
  }
  QTime time = QTime(0, 0).addMSecs(msec);

  QString format = "mm:ss:zzz";
  if (msec >= 3600000) {
    format = "hh:mm:ss:zzz";
  }

  return time.toString(format);
}

QString formatDuration(qint64 msec, bool forceHours) {
  if (msec <= 0) {
    return forceHours ? QStringLiteral("00:00:00") : QStringLiteral("00:00");
  }
  qint64 totalSec = msec / 1000;
  qint64 minutes = totalSec / 60;
  qint64 seconds = totalSec % 60;
  qint64 hours = minutes / 60;
  minutes = minutes % 60;

  if (hours > 0 || forceHours) {
    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
  }
  return QString("%1:%2")
      .arg(minutes, 2, 10, QChar('0'))
      .arg(seconds, 2, 10, QChar('0'));
}

