#ifndef CATALOGSCANNER_H
#define CATALOGSCANNER_H

#include <QAtomicInt>
#include <QList>
#include <QObject>
#include <QString>
#include <QThread>
#include <QVector>

#include "core/catalog/CatalogItem.h"
#include "core/models.h"

struct CatalogFolderTarget {
  QString title;
  QString path;
  int type = 0;

  MediaType mediaType() const { return static_cast<MediaType>(type); }
  void setMediaType(MediaType mt) { type = static_cast<int>(mt); }
};

class CatalogScannerWorker : public QObject {
  Q_OBJECT
public:
  explicit CatalogScannerWorker(QObject *parent = nullptr);

public slots:
  void scanLocal(int requestId, const QString &path, const QString &search,
                 MediaType mediaType);
  void scanGlobal(int requestId, const QList<CatalogFolderTarget> &folders,
                  const QString &search);
  void cancel(int requestId);

signals:
  void scanFinished(int requestId, const QVector<CatalogItem> &list,
                    const QString &pathFolder, bool isGlobal);

private:
  QAtomicInt m_cancelledRequestId{0};
};

class CatalogScanner : public QObject {
  Q_OBJECT
public:
  explicit CatalogScanner(QObject *parent = nullptr);
  ~CatalogScanner() override;

  void scanLocal(const QString &path, const QString &search,
                 MediaType mediaType = MediaType::Jingle);
  void scanLocal(const QString &path, const QString &search, int mediaType);
  void scanGlobal(const QList<CatalogFolderTarget> &folders, const QString &search);
  void cancelCurrentScan();

signals:
  void requestScanLocal(int requestId, const QString &path, const QString &search,
                        MediaType mediaType);
  void requestScanGlobal(int requestId, const QList<CatalogFolderTarget> &folders,
                         const QString &search);
  void requestCancel(int requestId);

  void finish(const QVector<CatalogItem> &list, const QString &pathFolder,
              bool isGlobal);

private slots:
  void onScanFinished(int requestId, const QVector<CatalogItem> &list,
                      const QString &pathFolder, bool isGlobal);

private:
  QThread m_workerThread;
  CatalogScannerWorker *m_worker = nullptr;
  QAtomicInt m_currentRequestId{0};
};

#endif // CATALOGSCANNER_H
