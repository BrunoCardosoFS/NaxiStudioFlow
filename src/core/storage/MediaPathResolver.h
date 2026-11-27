#ifndef MEDIAPATHRESOLVER_H
#define MEDIAPATHRESOLVER_H

#include <QString>
#include <QStringList>
#include <QVector>

#include "core/catalog/FolderItem.h"

struct FolderDirectoryResolution {
  QString resolvedDir;
  bool found = false;
  bool usedMirror = false;
  int matchedMirrorIndex = -1;
};

struct MediaFileResolution {
  QString resolvedPath;
  bool found = false;
  bool usedMirror = false;
  QString folderId;
};

class MediaPathResolver {
public:
  static QString normalizePath(const QString &path);
  static bool pathsEqual(const QString &a, const QString &b);
  static QStringList collectCandidateDirs(const QString &primaryPath,
                                          const QStringList &mirrorPaths);

  static FolderDirectoryResolution
  resolveFolderDirectory(const QString &primaryPath,
                         const QStringList &mirrorPaths);
  static FolderDirectoryResolution
  resolveFolderDirectory(const FolderItem &folder);

  // Local cascade (primary -> mirrorPaths). Returns found=false when absent so
  // M18 can later plug NaxServer network streaming as step 3.
  static MediaFileResolution
  resolveMediaFile(const QString &originalPath,
                   const QVector<FolderItem> &folders,
                   const QString &preferredFolderId = QString());
};

#endif // MEDIAPATHRESOLVER_H
