#ifndef CATALOGITEM_H
#define CATALOGITEM_H

#include <QString>
#include <QtGlobal>

#include "core/models.h"

enum class CatalogItemType {
  File,
  FolderHeader,
  Loading
};

struct CatalogItem {
  CatalogItemType itemType = CatalogItemType::File;
  QString title;
  QString path;
  MediaType mediaType = MediaType::Jingle;
  qint64 durationMs = 0;
  QString folderName;

  qint8 rawMediaType() const { return static_cast<qint8>(mediaType); }
  bool isHeader() const { return itemType == CatalogItemType::FolderHeader; }
  bool isLoading() const { return itemType == CatalogItemType::Loading; }
};

enum CatalogRoles {
  CatalogItemTypeRole = Qt::UserRole + 100,
  CatalogPathRole,
  CatalogMediaTypeRole,
  CatalogDurationRole,
  CatalogFolderNameRole
};

#endif // CATALOGITEM_H
