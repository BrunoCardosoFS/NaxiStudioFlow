#ifndef FOLDERITEM_H
#define FOLDERITEM_H

#include <QString>
#include <QVector>
#include <QtGlobal>

#include "core/catalog/CatalogItem.h"
#include "core/models.h"

enum class FolderItemType {
  CategoryHeader,
  Folder
};

struct FolderItem {
  FolderItemType itemType = FolderItemType::Folder;
  QString id;
  QString title;
  QString path;
  MediaType type = MediaType::Other;
  QVector<CatalogItem> items;

  MediaType mediaType() const { return type; }
  void setMediaType(MediaType mt) { type = mt; }

  bool isHeader() const { return itemType == FolderItemType::CategoryHeader; }
  bool isCommandFolder() const { return type == MediaType::Command; }
};

enum FolderRoles {
  FolderItemTypeRole = Qt::UserRole + 200,
  FolderPathRole,
  FolderTypeRole,
  FolderIdRole
};

#endif // FOLDERITEM_H
