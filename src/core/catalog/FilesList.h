#ifndef FILESLIST_H
#define FILESLIST_H

#include <QList>
#include <QObject>
#include <QString>
#include <QThread>
#include <QVector>

#include "core/catalog/CatalogItem.h"

struct CatalogFolderTarget {
  QString title;
  QString path;
  int type = 0;
};

class FilesList : public QThread {
  Q_OBJECT
public:
  explicit FilesList(QObject *parent = nullptr);
  ~FilesList();

  void scanLocal(const QString &path, const QString &search, int mediaType = 0);
  void scanGlobal(const QList<CatalogFolderTarget> &folders, const QString &search);
  void init(const QString &path, const QString &search);

private:
  void run() override;

  bool m_isGlobal = false;
  QString m_path;
  QString m_search;
  int m_mediaType = 0;
  QList<CatalogFolderTarget> m_folders;

signals:
  void finish(const QVector<CatalogItem> &list, const QString &pathFolder, bool isGlobal);
};

#endif // FILESLIST_H
