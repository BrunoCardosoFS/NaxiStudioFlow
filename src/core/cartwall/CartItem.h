#ifndef CARTITEM_H
#define CARTITEM_H

#include <QString>
#include <QUuid>
#include <Qt>

#include "core/models.h"

enum class CartItemStatus {
  Idle,
  Playing,
  Finished,
};

enum CartWallRoles {
  CartUuidRole = Qt::UserRole + 1,
  CartStatusRole,
  CartTitleRole,
  CartPathRole,
  CartDurationRole,
  CartPositionRole,
  CartMediaTypeRole,
};

struct CartItem {
  QUuid uuid;
  CartItemStatus status = CartItemStatus::Idle;
  int playerId = -1; // reserved for the future per-slot player

  QString title;
  QString path;

  qint64 durationMs = 0;
  qint64 positionMs = 0;

  MediaType mediaType = MediaType::Music;
};

#endif // CARTITEM_H
