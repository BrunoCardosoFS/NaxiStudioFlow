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

  BlockMarker,
  StartMarker,
  EndMarker,
};

enum class MediaType {
  Music,
  Commercial,
  Jingle,
};

enum class BlockType {
  Music,
  Commercial,
};

enum PlaylistItemStatus {
  Default,
  Loaded,
  Playing,
  Finished,
};

enum PlaylistItemRoles {
  UuidRole = Qt::UserRole + 1,
  StatusRole,

  TitleRole,
  PathRole,

  DurationRole,
  PositionRole,

  MixStartRole,
  MixEndRole,

  BlockTypeRole,
  TypeRole,
  MediaTypeRole
};

struct PlaylistItem {
  QUuid uuid;
  PlaylistItemStatus status = PlaylistItemStatus::Default;
  int playerId = -1;

  QString title;
  QString path;

  qint64 durationMs;
  qint64 positionMs = 0;

  qint64 mixStart;
  qint64 mixEnd;

  BlockType blockType;
  ItemType type = ItemType::Media;
  MediaType mediaType = MediaType::Music;
};

#endif // MODELS_H