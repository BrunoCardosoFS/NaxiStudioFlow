#ifndef FOLDERITEM_H
#define FOLDERITEM_H

#include <QString>
#include <QStringList>
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
  QStringList mirrorPaths;
  QString activePath;
  bool usingMirror = false;
  bool accessible = true;
  MediaType type = MediaType::Other;
  QVector<CatalogItem> items;

  MediaType mediaType() const { return type; }
  void setMediaType(MediaType mt) { type = mt; }

  bool isHeader() const { return itemType == FolderItemType::CategoryHeader; }
  bool isCommandFolder() const { return type == MediaType::Command; }
  bool hasAnyPath() const { return !path.isEmpty() || !mirrorPaths.isEmpty(); }
};

enum FolderRoles {
  FolderItemTypeRole = Qt::UserRole + 200,
  FolderPathRole,
  FolderTypeRole,
  FolderIdRole,
  FolderMirrorPathsRole,
  FolderActivePathRole,
  FolderUsingMirrorRole,
  FolderAccessibleRole
};

#endif // FOLDERITEM_H
