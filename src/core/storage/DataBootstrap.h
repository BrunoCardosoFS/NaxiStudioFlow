#ifndef DATABOOTSTRAP_H
#define DATABOOTSTRAP_H

#include <QByteArray>
#include <QString>
#include <QStringList>

struct DataBootstrapResult {
  bool ok = false;
  QStringList createdDirs;
  QStringList createdFiles;
  QString errorMessage;
};

class DataBootstrap {
public:
  static QByteArray defaultCatalogJson();
  static DataBootstrapResult initialize(const QString &dataDir,
                                        bool createDefaultCatalog = true);
};

#endif // DATABOOTSTRAP_H
