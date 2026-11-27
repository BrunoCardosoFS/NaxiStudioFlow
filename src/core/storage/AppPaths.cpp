#include "core/storage/AppPaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

QString AppPaths::defaultDataDir(const QString &applicationDirPath) {
  const QString baseDir = applicationDirPath.trimmed().isEmpty()
                              ? QCoreApplication::applicationDirPath()
                              : applicationDirPath.trimmed();
  if (baseDir.isEmpty()) {
    return QDir::cleanPath(QStringLiteral("../data"));
  }
  return QDir::cleanPath(QDir::fromNativeSeparators(baseDir) +
                         QStringLiteral("/../data"));
}

QString AppPaths::resolveDataDir(const QSettings *settings,
                                 const QString &applicationDirPath,
                                 const QString &envOverride) {
  const QString envValue = !envOverride.isNull()
                               ? envOverride.trimmed()
                               : qEnvironmentVariable(kDataDirEnvVar).trimmed();
  if (!envValue.isEmpty()) {
    return QDir::cleanPath(QDir::fromNativeSeparators(envValue));
  }

  if (settings) {
    const QString configured =
        settings->value(QString::fromLatin1(kDataDirSettingsKey))
            .toString()
            .trimmed();
    if (!configured.isEmpty()) {
      return QDir::cleanPath(QDir::fromNativeSeparators(configured));
    }
  }

  return defaultDataDir(applicationDirPath);
}

QString AppPaths::catalogFilePath(const QString &dataDir) {
  return QDir(dataDir).filePath(QStringLiteral("catalog.json"));
}

QString AppPaths::blocksDir(const QString &dataDir) {
  return QDir(dataDir).filePath(QStringLiteral("blocks"));
}

QString AppPaths::profilesDir(const QString &dataDir) {
  return QDir(dataDir).filePath(QStringLiteral("profiles"));
}

QString AppPaths::logsDir(const QString &dataDir) {
  return QDir(dataDir).filePath(QStringLiteral("logs"));
}

QString AppPaths::configDir(const QString &dataDir) {
  return QDir(dataDir).filePath(QStringLiteral("config"));
}

QByteArray AppPaths::defaultCatalogJson() {
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

bool AppPaths::ensureDataStructure(const QString &dataDir,
                                   bool createDefaultCatalog) {
  const QString trimmed = dataDir.trimmed();
  if (trimmed.isEmpty()) {
    return false;
  }

  QDir dir;
  const QStringList requiredDirs = {
      trimmed,
      blocksDir(trimmed),
      profilesDir(trimmed),
      logsDir(trimmed),
      configDir(trimmed),
  };

  for (const QString &subDir : requiredDirs) {
    if (!dir.mkpath(subDir)) {
      return false;
    }
  }

  if (createDefaultCatalog) {
    const QString catalogPath = catalogFilePath(trimmed);
    if (!QFileInfo::exists(catalogPath)) {
      QFile file(catalogPath);
      if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
      }
      const QByteArray json = defaultCatalogJson();
      if (file.write(json) != json.size()) {
        file.close();
        return false;
      }
      file.close();
    }
  }

  return true;
}
