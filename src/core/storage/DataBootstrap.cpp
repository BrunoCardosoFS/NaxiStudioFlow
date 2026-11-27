#include "core/storage/DataBootstrap.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "core/storage/AppPaths.h"

QByteArray DataBootstrap::defaultCatalogJson() {
  QJsonObject pauseItem;
  pauseItem.insert(QStringLiteral("id"), QStringLiteral("pause"));
  pauseItem.insert(QStringLiteral("title"), QStringLiteral("Pausa"));
  pauseItem.insert(QStringLiteral("command"), QStringLiteral("pause"));

  QJsonArray items;
  items.append(pauseItem);

  QJsonObject commandFolder;
  commandFolder.insert(QStringLiteral("id"),
                       QStringLiteral("comandos-playlist"));
  commandFolder.insert(QStringLiteral("title"), QStringLiteral("Comandos"));
  commandFolder.insert(QStringLiteral("type"), QStringLiteral("command"));
  commandFolder.insert(QStringLiteral("items"), items);

  QJsonArray folders;
  folders.append(commandFolder);

  QJsonObject root;
  root.insert(QStringLiteral("version"), 1);
  root.insert(QStringLiteral("folders"), folders);

  return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

DataBootstrapResult DataBootstrap::initialize(const QString &dataDir,
                                              bool createDefaultCatalog) {
  DataBootstrapResult result;
  const QString trimmed = dataDir.trimmed();
  if (trimmed.isEmpty()) {
    result.errorMessage = QStringLiteral("Data directory path is empty.");
    return result;
  }

  QDir dir;
  const QStringList requiredDirs = {
      trimmed,
      AppPaths::blocksDir(trimmed),
      AppPaths::profilesDir(trimmed),
      AppPaths::logsDir(trimmed),
      AppPaths::configDir(trimmed),
  };

  for (const QString &subDir : requiredDirs) {
    const QFileInfo fi(subDir);
    if (fi.exists()) {
      if (!fi.isDir()) {
        result.errorMessage =
            QStringLiteral("Expected directory but found file: %1").arg(subDir);
        return result;
      }
      continue;
    }

    if (!dir.mkpath(subDir)) {
      result.errorMessage =
          QStringLiteral("Failed to create directory: %1").arg(subDir);
      return result;
    }
    result.createdDirs.append(QDir::cleanPath(subDir));
  }

  if (createDefaultCatalog) {
    const QString catalogPath = AppPaths::catalogFilePath(trimmed);
    if (!QFileInfo::exists(catalogPath)) {
      QFile file(catalogPath);
      if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        result.errorMessage =
            QStringLiteral("Failed to create catalog file: %1")
                .arg(catalogPath);
        return result;
      }
      const QByteArray json = defaultCatalogJson();
      if (file.write(json) != json.size()) {
        file.close();
        result.errorMessage =
            QStringLiteral("Failed to write catalog file: %1")
                .arg(catalogPath);
        return result;
      }
      file.close();
      result.createdFiles.append(QDir::cleanPath(catalogPath));
    }
  }

  result.ok = true;
  return result;
}
