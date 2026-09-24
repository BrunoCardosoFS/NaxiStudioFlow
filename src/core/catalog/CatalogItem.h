#ifndef CATALOGITEM_H
#define CATALOGITEM_H

#include <QString>
#include <QtGlobal>

enum class CatalogItemType {
  File,
  FolderHeader
};

struct CatalogItem {
  CatalogItemType itemType = CatalogItemType::File;
  QString title;
  QString path;
  qint8 mediaType = 0; // 0: Jingle, 1: Music, 2: Commercial, etc.
  qint64 durationMs = 0;
  QString folderName;

  bool isHeader() const { return itemType == CatalogItemType::FolderHeader; }
};

enum CatalogRoles {
  CatalogItemTypeRole = Qt::UserRole + 100,
  CatalogPathRole,
  CatalogMediaTypeRole,
  CatalogDurationRole,
  CatalogFolderNameRole
};

#endif // CATALOGITEM_H
