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
  if (parseError.error != QJsonParseError::NoError || !jsonDocument.isObject()) {
    return QJsonArray();
  }

  const QJsonValue foldersVal =
      jsonDocument.object().value(QStringLiteral("folders"));
  return foldersVal.isArray() ? foldersVal.toArray() : QJsonArray();
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
    item.id = obj.value(QStringLiteral("id")).toString();
    item.title = obj.value(QStringLiteral("title")).toString();
    item.path = obj.value(QStringLiteral("path")).toString();
    item.type = mediaTypeFromString(obj.value(QStringLiteral("type")).toString(),
                                    MediaType::Other);

    if (item.isCommandFolder() && obj.value("items").isArray()) {
      const QJsonArray inlineItems = obj.value("items").toArray();
      item.items.reserve(inlineItems.size());
      for (const QJsonValue &itemVal : inlineItems) {
        if (!itemVal.isObject()) {
          continue;
        }
        const QJsonObject itemObj = itemVal.toObject();
        const QString cmdStr =
            itemObj.value("command").toString().trimmed().toLower();
        const CommandType cmdType = commandTypeFromString(cmdStr);
        if (cmdType == CommandType::None) {
          continue;
        }

        CatalogItem ci;
        ci.itemType = CatalogItemType::Command;
        ci.mediaType = MediaType::Command;
        ci.commandType = cmdType;
        ci.commandId = cmdStr;
        ci.title = itemObj.value("title").toString().trimmed();
        if (ci.title.isEmpty()) {
          ci.title = (cmdType == CommandType::Pause)
                         ? QStringLiteral("Pausa")
                         : cmdStr;
        }
        ci.folderName = item.title;
        item.items.append(std::move(ci));
      }
    }

    items.append(std::move(item));
  }

  return items;
}
