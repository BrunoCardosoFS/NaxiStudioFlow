#include "core/storage/AppPaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

#include "core/storage/DataBootstrap.h"

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
  return DataBootstrap::defaultCatalogJson();
}

bool AppPaths::ensureDataStructure(const QString &dataDir,
                                   bool createDefaultCatalog) {
  return DataBootstrap::initialize(dataDir, createDefaultCatalog).ok;
}
