#include "TimeUtils.h"

QString msec2string(qint64 msec) {
  if (msec <= 0) {
    return QStringLiteral("00:00:000");
  }
  const qint64 ms = msec % 1000;
  const qint64 totalSec = msec / 1000;
  const qint64 sec = totalSec % 60;
  const qint64 totalMin = totalSec / 60;
  const qint64 min = totalMin % 60;
  const qint64 hours = totalMin / 60;

  if (hours > 0) {
    return QString::asprintf("%02lld:%02lld:%02lld:%03lld",
                             static_cast<long long>(hours),
                             static_cast<long long>(min),
                             static_cast<long long>(sec),
                             static_cast<long long>(ms));
  }
  return QString::asprintf("%02lld:%02lld:%03lld",
                           static_cast<long long>(min),
                           static_cast<long long>(sec),
                           static_cast<long long>(ms));
}

QString formatDuration(qint64 msec, bool forceHours) {
  if (msec <= 0) {
    return forceHours ? QStringLiteral("00:00:00") : QStringLiteral("00:00");
  }
  const qint64 totalSec = msec / 1000;
  const qint64 seconds = totalSec % 60;
  const qint64 totalMin = totalSec / 60;
  const qint64 minutes = totalMin % 60;
  const qint64 hours = totalMin / 60;

  if (hours > 0 || forceHours) {
    return QString::asprintf("%02lld:%02lld:%02lld",
                             static_cast<long long>(hours),
                             static_cast<long long>(minutes),
                             static_cast<long long>(seconds));
  }
  return QString::asprintf("%02lld:%02lld", static_cast<long long>(minutes),
                           static_cast<long long>(seconds));
}

