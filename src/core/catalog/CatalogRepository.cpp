#include "CatalogRepository.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

QJsonArray CatalogRepository::getFoldersJson(const QString &dbPath) {
  QFile catalogFile(dbPath + "/catalog.json");
  if (!catalogFile.open(QFile::ReadOnly)) {
    return QJsonArray();
  }

  const QByteArray data = catalogFile.readAll();
  catalogFile.close();

  QJsonParseError parseError;
  QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &parseError);
  if (parseError.error != QJsonParseError::NoError || !jsonDocument.isArray()) {
    return QJsonArray();
  }

  return jsonDocument.array();
}

QVector<FolderItem> CatalogRepository::loadFolders(const QString &dbPath) {
  const QJsonArray foldersArray = getFoldersJson(dbPath);
  QVector<FolderItem> items;
  items.reserve(foldersArray.size());

  for (const QJsonValue &val : foldersArray) {
    if (!val.isObject()) {
      continue;
    }
    const QJsonObject obj = val.toObject();
    FolderItem item;
    item.itemType = FolderItemType::Folder;
    item.id = obj.value("id").toString();
    item.title = obj.value("title").toString();
    item.path = obj.value("path").toString();
    item.type = obj.value("type").toInt();
    items.append(std::move(item));
  }

  return items;
}
