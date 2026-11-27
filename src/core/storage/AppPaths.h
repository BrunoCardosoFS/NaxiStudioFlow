#ifndef APPPATHS_H
#define APPPATHS_H

#include <QByteArray>
#include <QString>

class QSettings;

class AppPaths {
public:
  static constexpr const char *kDataDirEnvVar = "NAXFLOW_DATA_DIR";
  static constexpr const char *kDataDirSettingsKey = "dataDir";

  static QString defaultDataDir(const QString &applicationDirPath = QString());
  static QString resolveDataDir(const QSettings *settings = nullptr,
                                const QString &applicationDirPath = QString(),
                                const QString &envOverride = QString());

  static QString catalogFilePath(const QString &dataDir);
  static QString blocksDir(const QString &dataDir);
  static QString profilesDir(const QString &dataDir);
  static QString logsDir(const QString &dataDir);
  static QString configDir(const QString &dataDir);

  static QByteArray defaultCatalogJson();
  static bool ensureDataStructure(const QString &dataDir,
                                  bool createDefaultCatalog = true);
};

#endif // APPPATHS_H
