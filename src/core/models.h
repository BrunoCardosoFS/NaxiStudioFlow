#ifndef MODELS_H
#define MODELS_H

#include <QList>
#include <QString>
#include <QUuid>

enum class PlaylistStatus {
  Playing,
  Stopped,
  Paused,
};

enum class ItemType {
  Media,
  Command,

  BlockMarker,
  StartMarker,
  EndMarker,
};

enum class MediaType : qint8 {
  Jingle = 0,
  Music = 1,
  Commercial = 2,
  Other = 3,
  Command = 4,
};

inline MediaType mediaTypeFromString(const QString &name,
                                     MediaType fallback = MediaType::Other) {
  const QString normalized = name.trimmed().toLower();
  if (normalized == QStringLiteral("jingle")) {
    return MediaType::Jingle;
  }
  if (normalized == QStringLiteral("music")) {
    return MediaType::Music;
  }
  if (normalized == QStringLiteral("commercial")) {
    return MediaType::Commercial;
  }
  if (normalized == QStringLiteral("other")) {
    return MediaType::Other;
  }
  if (normalized == QStringLiteral("command")) {
    return MediaType::Command;
  }
  return fallback;
}

inline QString mediaTypeToString(MediaType type) {
  switch (type) {
  case MediaType::Jingle:
    return QStringLiteral("jingle");
  case MediaType::Music:
    return QStringLiteral("music");
  case MediaType::Commercial:
    return QStringLiteral("commercial");
  case MediaType::Command:
    return QStringLiteral("command");
  case MediaType::Other:
  default:
    return QStringLiteral("other");
  }
}

enum class CommandType : qint8 {
  None = 0,
  Pause = 1,
};

inline CommandType commandTypeFromString(const QString &name) {
  const QString normalized = name.trimmed().toLower();
  if (normalized == QStringLiteral("pause")) {
    return CommandType::Pause;
  }
  return CommandType::None;
}

inline QString commandTypeToString(CommandType type) {
  switch (type) {
  case CommandType::Pause:
    return QStringLiteral("pause");
  case CommandType::None:
  default:
    return QString();
  }
}

enum class BlockType {
  Music,
  Commercial,
};

enum class PlaylistItemStatus {
  Default,
  Next,
  Playing,
  Mixing,
  Finished,
};

enum PlaylistItemRoles {
  UuidRole = Qt::UserRole + 1,
  StatusRole,

  TitleRole,
  BlockTitleRole,
  PathRole,

  DurationRole,
  PositionRole,

  MixStartRole,
  MixEndRole,

  StartPointRole,
  EndPointRole,

  FadeInEndPointRole,
  FadeOutStartPointRole,

  StartVolumeRole,
  EndVolumeRole,
  VolumeUpStartPointRole,
  VolumeUpEndPointRole,
  VolumeDownStartPointRole,
  VolumeDownEndPointRole,

  BlockTypeRole,
  TypeRole,
  MediaTypeRole,

  EstimatedTimeRole,
  TotalDurationRole,
  ItemsCountRole,
  MarkerTextRole,

  CommandTypeRole,
  CommandIdRole
};

struct PlaylistItem {
  QUuid uuid;
  PlaylistItemStatus status = PlaylistItemStatus::Default;
  int playerId = -1;

  QString title;
  QString blockTitle;
  QString path;

  qint64 durationMs = 0;
  qint64 positionMs = 0;

  qint64 mixStart = 0;
  qint64 mixEnd = 0;

  qint64 startPoint = 0;
  qint64 endPoint = 0;

  qint64 fadeInEndPoint = 0;
  qint64 fadeOutStartPoint = 0;

  float startVolume = 1.0f;
  float endVolume = 1.0f;
  qint64 volumeUpStartPoint = 0;
  qint64 volumeUpEndPoint = 0;
  qint64 volumeDownStartPoint = 0;
  qint64 volumeDownEndPoint = 0;

  BlockType blockType = BlockType::Music;
  ItemType type = ItemType::Media;
  MediaType mediaType = MediaType::Music;
  CommandType commandType = CommandType::None;
  QString commandId;

  QString estimatedTime;
  qint64 estimatedTimeMs = -1;
  qint64 totalDurationMs = 0;
  int itemsCount = 0;
  QString markerText;

  bool isEntry() const {
    return type == ItemType::Media || type == ItemType::Command;
  }
};

#endif // MODELS_H