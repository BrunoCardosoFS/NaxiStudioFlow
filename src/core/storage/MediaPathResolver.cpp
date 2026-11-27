#include "core/storage/MediaPathResolver.h"

#include <QDir>
#include <QFileInfo>

namespace {
Qt::CaseSensitivity pathCaseSensitivity() {
#ifdef Q_OS_WIN
  return Qt::CaseInsensitive;
#else
  return Qt::CaseSensitive;
#endif
}

bool isRootPath(const QString &cleaned) {
  if (cleaned == QStringLiteral("/")) {
    return true;
  }
#ifdef Q_OS_WIN
  if (cleaned.size() == 3 && cleaned.at(1) == QLatin1Char(':') &&
      cleaned.at(2) == QLatin1Char('/')) {
    return true;
  }
#endif
  return false;
}

bool extractRelativeSubPath(const QString &normalizedFilePath,
                            const QString &baseDir, QString &outRelative) {
  const QString normBase = MediaPathResolver::normalizePath(baseDir);
  if (normBase.isEmpty()) {
    return false;
  }
  const QString prefixWithSlash = normBase.endsWith(QLatin1Char('/'))
                                      ? normBase
                                      : (normBase + QLatin1Char('/'));
  if (!normalizedFilePath.startsWith(prefixWithSlash, pathCaseSensitivity())) {
    return false;
  }
  outRelative = normalizedFilePath.mid(prefixWithSlash.size());
  return !outRelative.isEmpty();
}

MediaFileResolution tryResolveInFolder(const FolderItem &folder,
                                       const QString &relativeSubPath) {
  MediaFileResolution result;
  if (relativeSubPath.isEmpty()) {
    return result;
  }

  const QString normPrimary = MediaPathResolver::normalizePath(folder.path);
  const QStringList candidates =
      MediaPathResolver::collectCandidateDirs(folder.path, folder.mirrorPaths);

  for (const QString &candidateDir : candidates) {
    const QString candidateFile = QDir(candidateDir).filePath(relativeSubPath);
    const QFileInfo fi(candidateFile);
    if (fi.exists() && fi.isFile()) {
      result.resolvedPath = QDir::cleanPath(fi.filePath());
      result.found = true;
      result.usedMirror =
          normPrimary.isEmpty() ||
          !MediaPathResolver::pathsEqual(candidateDir, normPrimary);
      result.folderId = folder.id;
      return result;
    }
  }

  return result;
}

MediaFileResolution resolveAgainstFolder(const FolderItem &folder,
                                         const QString &normalizedOrigPath,
                                         bool allowFileNameFallback) {
  const QStringList candidates =
      MediaPathResolver::collectCandidateDirs(folder.path, folder.mirrorPaths);
  for (const QString &candidateDir : candidates) {
    QString relativeSubPath;
    if (extractRelativeSubPath(normalizedOrigPath, candidateDir,
                               relativeSubPath)) {
      const MediaFileResolution res =
          tryResolveInFolder(folder, relativeSubPath);
      if (res.found) {
        return res;
      }
    }
  }

  if (allowFileNameFallback) {
    const QString fileName = QFileInfo(normalizedOrigPath).fileName();
    const MediaFileResolution res = tryResolveInFolder(folder, fileName);
    if (res.found) {
      return res;
    }
  }

  return MediaFileResolution{};
}
} // namespace

QString MediaPathResolver::normalizePath(const QString &path) {
  const QString trimmed = path.trimmed();
  if (trimmed.isEmpty()) {
    return QString();
  }

  QString cleaned = QDir::cleanPath(QDir::fromNativeSeparators(trimmed));
  while (cleaned.size() > 1 && cleaned.endsWith(QLatin1Char('/')) &&
         !isRootPath(cleaned)) {
    cleaned.chop(1);
  }
  return cleaned;
}

bool MediaPathResolver::pathsEqual(const QString &a, const QString &b) {
  return QString::compare(normalizePath(a), normalizePath(b),
                          pathCaseSensitivity()) == 0;
}

QStringList MediaPathResolver::collectCandidateDirs(const QString &primaryPath,
                                                    const QStringList &mirrorPaths) {
  QStringList result;
  auto appendUnique = [&result](const QString &raw) {
    const QString norm = normalizePath(raw);
    if (norm.isEmpty()) {
      return;
    }
    for (const QString &existing : std::as_const(result)) {
      if (pathsEqual(existing, norm)) {
        return;
      }
    }
    result.append(norm);
  };

  appendUnique(primaryPath);
  for (const QString &mirror : mirrorPaths) {
    appendUnique(mirror);
  }
  return result;
}

FolderDirectoryResolution MediaPathResolver::resolveFolderDirectory(
    const QString &primaryPath, const QStringList &mirrorPaths) {
  FolderDirectoryResolution res;

  const QString normPrimary = normalizePath(primaryPath);
  if (!normPrimary.isEmpty()) {
    const QFileInfo fi(normPrimary);
    if (fi.exists() && fi.isDir()) {
      res.resolvedDir = normPrimary;
      res.found = true;
      res.usedMirror = false;
      res.matchedMirrorIndex = -1;
      return res;
    }
  }

  for (int i = 0; i < mirrorPaths.size(); ++i) {
    const QString normMirror = normalizePath(mirrorPaths.at(i));
    if (normMirror.isEmpty()) {
      continue;
    }
    if (!normPrimary.isEmpty() && pathsEqual(normMirror, normPrimary)) {
      continue;
    }
    const QFileInfo fi(normMirror);
    if (fi.exists() && fi.isDir()) {
      res.resolvedDir = normMirror;
      res.found = true;
      res.usedMirror = true;
      res.matchedMirrorIndex = i;
      return res;
    }
  }

  return res;
}

FolderDirectoryResolution
MediaPathResolver::resolveFolderDirectory(const FolderItem &folder) {
  return resolveFolderDirectory(folder.path, folder.mirrorPaths);
}

MediaFileResolution MediaPathResolver::resolveMediaFile(
    const QString &originalPath, const QVector<FolderItem> &folders,
    const QString &preferredFolderId) {
  MediaFileResolution result;
  result.resolvedPath = originalPath;
  result.folderId = preferredFolderId;

  const QString trimmed = originalPath.trimmed();
  if (trimmed.isEmpty()) {
    return result;
  }

  const QFileInfo directFi(trimmed);
  if (directFi.exists() && directFi.isFile()) {
    result.resolvedPath = originalPath;
    result.found = true;
    result.usedMirror = false;
    return result;
  }

  const QString normalizedOrig = normalizePath(trimmed);

  if (!preferredFolderId.isEmpty()) {
    for (const FolderItem &folder : folders) {
      if (folder.isHeader() || folder.isCommandFolder() ||
          folder.id != preferredFolderId) {
        continue;
      }
      const MediaFileResolution res =
          resolveAgainstFolder(folder, normalizedOrig, true);
      if (res.found) {
        return res;
      }
    }
  }

  for (const FolderItem &folder : folders) {
    if (folder.isHeader() || folder.isCommandFolder()) {
      continue;
    }
    if (!preferredFolderId.isEmpty() && folder.id == preferredFolderId) {
      continue;
    }
    const MediaFileResolution res =
        resolveAgainstFolder(folder, normalizedOrig, false);
    if (res.found) {
      return res;
    }
  }

  return result;
}
