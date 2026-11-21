#ifndef FOLDERITEM_H
#define FOLDERITEM_H

#include <QString>
#include <QtGlobal>

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
  int type = 0; // 0: Jingle, 1: Music, 2: Commercial, etc.

  MediaType mediaType() const { return static_cast<MediaType>(type); }
  void setMediaType(MediaType mt) { type = static_cast<int>(mt); }

  bool isHeader() const { return itemType == FolderItemType::CategoryHeader; }
};

enum FolderRoles {
  FolderItemTypeRole = Qt::UserRole + 200,
  FolderPathRole,
  FolderTypeRole,
  FolderIdRole
};

#endif // FOLDERITEM_H
