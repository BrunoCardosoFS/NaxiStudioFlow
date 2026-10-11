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

enum class MediaType : qint8 {
  Jingle = 0,
  Music = 1,
  Commercial = 2,
  Other = 3,
};

enum class BlockType {
  Music,
  Commercial,
};

enum class PlaylistItemStatus {
  Default,
  Next,
  Playing,
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

  BlockTypeRole,
  TypeRole,
  MediaTypeRole,

  EstimatedTimeRole,
  TotalDurationRole,
  ItemsCountRole,
  MarkerTextRole
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

  BlockType blockType = BlockType::Music;
  ItemType type = ItemType::Media;
  MediaType mediaType = MediaType::Music;

  QString estimatedTime;
  qint64 totalDurationMs = 0;
  int itemsCount = 0;
  QString markerText;
};

#endif // MODELS_H