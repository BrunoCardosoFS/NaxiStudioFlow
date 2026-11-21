#ifndef CATALOGREPOSITORY_H
#define CATALOGREPOSITORY_H

#include <QJsonArray>
#include <QString>
#include <QVector>

#include "core/catalog/FolderItem.h"

class CatalogRepository {
public:
  static QJsonArray getFoldersJson(const QString &dbPath);
  static QVector<FolderItem> loadFolders(const QString &dbPath);
};

#endif // CATALOGREPOSITORY_H
