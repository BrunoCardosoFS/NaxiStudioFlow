#include "CatalogScanner.h"

#include <QDirIterator>
#include <QFileInfo>
#include <algorithm>

namespace {
const QStringList &audioExtensions() {
  static const QStringList filters = {
      "*.mp3", "*.wav", "*.opus", "*.aac", "*.flac", "*.webm", "*.m4a", "*.ogg", "*.wma"
  };
  return filters;
}
} // namespace

CatalogScannerWorker::CatalogScannerWorker(QObject *parent)
    : QObject(parent) {}

void CatalogScannerWorker::cancel(int requestId) {
  m_cancelledRequestId.storeRelease(requestId);
}

void CatalogScannerWorker::scanLocal(int requestId, const QString &path,
                                     const QString &search, MediaType mediaType) {
  QVector<CatalogItem> result;
  result.reserve(1024);

  const QString trimmedSearch = search.trimmed();
  QDirIterator it(path, audioExtensions(), QDir::Files | QDir::NoDotAndDotDot);

  while (it.hasNext()) {
    if (m_cancelledRequestId.loadAcquire() >= requestId) {
      return;
    }
    const QString filePath = it.next();
    const QString fileName = it.fileName();

    if (trimmedSearch.isEmpty() ||
        fileName.contains(trimmedSearch, Qt::CaseInsensitive)) {
      CatalogItem item;
      item.itemType = CatalogItemType::File;
      item.title = fileName;
      item.path = filePath;
      item.mediaType = mediaType;
      result.append(std::move(item));
    }
  }

  if (m_cancelledRequestId.loadAcquire() >= requestId) {
    return;
  }

  std::sort(result.begin(), result.end(),
            [](const CatalogItem &a, const CatalogItem &b) {
              return QString::compare(a.title, b.title, Qt::CaseInsensitive) < 0;
            });

  emit scanFinished(requestId, result, path, false);
}

void CatalogScannerWorker::scanGlobal(int requestId,
                                      const QList<CatalogFolderTarget> &folders,
                                      const QString &search) {
  QVector<CatalogItem> result;
  const QString trimmedSearch = search.trimmed();

  for (const CatalogFolderTarget &folder : folders) {
    if (m_cancelledRequestId.loadAcquire() >= requestId) {
      return;
    }

    QDirIterator it(folder.path, audioExtensions(), QDir::Files | QDir::NoDotAndDotDot);
    QVector<CatalogItem> folderFiles;
    folderFiles.reserve(64);

    while (it.hasNext()) {
      if (m_cancelledRequestId.loadAcquire() >= requestId) {
        return;
      }
      const QString filePath = it.next();
      const QString fileName = it.fileName();

      if (trimmedSearch.isEmpty() ||
          fileName.contains(trimmedSearch, Qt::CaseInsensitive)) {
        CatalogItem item;
        item.itemType = CatalogItemType::File;
        item.title = fileName;
        item.path = filePath;
        item.mediaType = folder.mediaType();
        item.folderName = folder.title;
        folderFiles.append(std::move(item));
      }
    }

    if (!folderFiles.isEmpty()) {
      std::sort(folderFiles.begin(), folderFiles.end(),
                [](const CatalogItem &a, const CatalogItem &b) {
                  return QString::compare(a.title, b.title, Qt::CaseInsensitive) < 0;
                });

      CatalogItem header;
      header.itemType = CatalogItemType::FolderHeader;
      header.title = folder.title;
      header.folderName = folder.title;
      header.mediaType = folder.mediaType();

      result.append(std::move(header));
      result.append(std::move(folderFiles));
    }
  }

  if (m_cancelledRequestId.loadAcquire() >= requestId) {
    return;
  }

  emit scanFinished(requestId, result, QString(), true);
}

CatalogScanner::CatalogScanner(QObject *parent)
    : QObject(parent), m_worker(new CatalogScannerWorker()) {
  qRegisterMetaType<QList<CatalogFolderTarget>>("QList<CatalogFolderTarget>");
  qRegisterMetaType<QVector<CatalogItem>>("QVector<CatalogItem>");
  qRegisterMetaType<MediaType>("MediaType");

  m_worker->moveToThread(&m_workerThread);

  connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

  connect(this, &CatalogScanner::requestScanLocal, m_worker,
          &CatalogScannerWorker::scanLocal, Qt::QueuedConnection);
  connect(this, &CatalogScanner::requestScanGlobal, m_worker,
          &CatalogScannerWorker::scanGlobal, Qt::QueuedConnection);
  connect(this, &CatalogScanner::requestCancel, m_worker,
          &CatalogScannerWorker::cancel, Qt::DirectConnection);

  connect(m_worker, &CatalogScannerWorker::scanFinished, this,
          &CatalogScanner::onScanFinished, Qt::QueuedConnection);

  m_workerThread.start();
}

CatalogScanner::~CatalogScanner() {
  cancelCurrentScan();
  m_workerThread.quit();
  m_workerThread.wait();
}

void CatalogScanner::cancelCurrentScan() {
  const int current = m_currentRequestId.loadRelaxed();
  if (current > 0) {
    emit requestCancel(current);
  }
}

void CatalogScanner::scanLocal(const QString &path, const QString &search,
                               MediaType mediaType) {
  cancelCurrentScan();
  const int reqId = m_currentRequestId.fetchAndAddRelaxed(1) + 1;
  emit requestScanLocal(reqId, path, search, mediaType);
}

void CatalogScanner::scanLocal(const QString &path, const QString &search,
                               int mediaType) {
  scanLocal(path, search, static_cast<MediaType>(mediaType));
}

void CatalogScanner::scanGlobal(const QList<CatalogFolderTarget> &folders,
                                const QString &search) {
  cancelCurrentScan();
  const int reqId = m_currentRequestId.fetchAndAddRelaxed(1) + 1;
  emit requestScanGlobal(reqId, folders, search);
}

void CatalogScanner::onScanFinished(int requestId, const QVector<CatalogItem> &list,
                                    const QString &pathFolder, bool isGlobal) {
  if (requestId != m_currentRequestId.loadAcquire()) {
    return; // Descarte de resultados de buscas canceladas/desatualizadas
  }
  emit finish(list, pathFolder, isGlobal);
}
