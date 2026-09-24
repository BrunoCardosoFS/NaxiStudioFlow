#include "FilesList.h"

#include <QDir>
#include <QFileInfo>

FilesList::FilesList(QObject *parent) : QThread{parent} {}

FilesList::~FilesList() {
  if (isRunning()) {
    requestInterruption();
    wait();
  }
}

void FilesList::init(const QString &path, const QString &search) {
  scanLocal(path, search, 0);
}

void FilesList::scanLocal(const QString &path, const QString &search, int mediaType) {
  if (isRunning()) {
    requestInterruption();
    wait();
  }

  m_isGlobal = false;
  m_path = path;
  m_search = search.trimmed();
  m_mediaType = mediaType;
  m_folders.clear();

  start();
}

void FilesList::scanGlobal(const QList<CatalogFolderTarget> &folders, const QString &search) {
  if (isRunning()) {
    requestInterruption();
    wait();
  }

  m_isGlobal = true;
  m_path.clear();
  m_search = search.trimmed();
  m_folders = folders;

  start();
}

void FilesList::run() {
  QVector<CatalogItem> result;

  auto isAudio = [](const QString &ext) {
    return ext == "mp3" || ext == "wav" || ext == "opus" ||
           ext == "aac" || ext == "flac" || ext == "webm" || ext == "m4a";
  };

  if (!m_isGlobal) {
    // Busca Local (pasta aberta)
    QDir dir(m_path);
    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    for (const QFileInfo &qfi : entries) {
      if (isInterruptionRequested()) {
        return;
      }

      QString suffix = qfi.completeSuffix().toLower();
      QString fileName = qfi.fileName();

      if (isAudio(suffix) &&
          (m_search.isEmpty() || fileName.contains(m_search, Qt::CaseInsensitive))) {
        CatalogItem item;
        item.itemType = CatalogItemType::File;
        item.title = fileName;
        item.path = qfi.absoluteFilePath();
        item.mediaType = static_cast<qint8>(m_mediaType);
        result.append(item);
      }
    }

    emit finish(result, m_path, false);
  } else {
    // Busca Global (agrupada por pasta com divisão/cabeçalho)
    for (const CatalogFolderTarget &folder : m_folders) {
      if (isInterruptionRequested()) {
        return;
      }

      QDir dir(folder.path);
      const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
      QVector<CatalogItem> folderFiles;

      for (const QFileInfo &qfi : entries) {
        if (isInterruptionRequested()) {
          return;
        }

        QString suffix = qfi.completeSuffix().toLower();
        QString fileName = qfi.fileName();

        if (isAudio(suffix) &&
            (m_search.isEmpty() || fileName.contains(m_search, Qt::CaseInsensitive))) {
          CatalogItem item;
          item.itemType = CatalogItemType::File;
          item.title = fileName;
          item.path = qfi.absoluteFilePath();
          item.mediaType = static_cast<qint8>(folder.type);
          item.folderName = folder.title;
          folderFiles.append(item);
        }
      }

      if (!folderFiles.isEmpty()) {
        // Cabeçalho de divisão da pasta
        CatalogItem header;
        header.itemType = CatalogItemType::FolderHeader;
        header.title = folder.title;
        header.folderName = folder.title;
        header.mediaType = static_cast<qint8>(folder.type);
        result.append(header);

        // Itens de arquivo pertencentes à pasta
        result.append(folderFiles);
      }
    }

    emit finish(result, "", true);
  }
}
