#include "FolderListModel.h"
#include "CatalogRepository.h"

#include <QHash>
#include <QList>

#include "core/storage/MediaPathResolver.h"

FolderListModel::FolderListModel(QObject *parent)
    : QAbstractListModel(parent) {}

int FolderListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return m_items.size();
}

QVariant FolderListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
    return QVariant();
  }

  const FolderItem &item = m_items.at(index.row());

  switch (role) {
  case Qt::DisplayRole:
    return item.title;
  case Qt::ToolTipRole: {
    if (item.isHeader() || item.isCommandFolder()) {
      return item.title;
    }
    if (!item.accessible) {
      return QStringLiteral("%1\n%2").arg(item.title,
                                          tr("Caminho indisponível"));
    }
    if (item.usingMirror) {
      return QStringLiteral("%1\n%2 (%3)").arg(item.title, item.activePath,
                                               tr("Espelho local"));
    }
    return QStringLiteral("%1\n%2").arg(item.title, item.activePath);
  }
  case FolderItemTypeRole:
    return static_cast<int>(item.itemType);
  case FolderPathRole:
    return item.path;
  case FolderTypeRole:
    return static_cast<int>(item.type);
  case FolderIdRole:
    return item.id;
  case FolderMirrorPathsRole:
    return item.mirrorPaths;
  case FolderActivePathRole:
    return item.activePath;
  case FolderUsingMirrorRole:
    return item.usingMirror;
  case FolderAccessibleRole:
    return item.accessible;
  default:
    return QVariant();
  }
}

Qt::ItemFlags FolderListModel::flags(const QModelIndex &index) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
    return Qt::NoItemFlags;
  }

  const FolderItem &item = m_items.at(index.row());
  if (item.isHeader()) {
    return Qt::ItemIsEnabled; // Cabeçalho não selecionável
  }

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void FolderListModel::loadFromDatabase(const QString &pathDB) {
  beginResetModel();
  m_items.clear();

  const QVector<FolderItem> loadedFolders = CatalogRepository::loadFolders(pathDB);

  // Agrupa as pastas por categoria (tipo)
  QHash<MediaType, QVector<FolderItem>> groupedFolders;
  for (FolderItem item : loadedFolders) {
    if (item.isCommandFolder()) {
      item.activePath.clear();
      item.usingMirror = false;
      item.accessible = true;
    } else {
      const FolderDirectoryResolution res =
          MediaPathResolver::resolveFolderDirectory(item);
      item.activePath = res.resolvedDir;
      item.usingMirror = res.usedMirror;
      item.accessible = res.found;
    }
    groupedFolders[item.type].append(std::move(item));
  }

  // Ordem padrão de exibição das categorias
  QList<MediaType> categoryOrder = {
      MediaType::Command,
      MediaType::Jingle,
      MediaType::Music,
      MediaType::Commercial,
      MediaType::Other,
  };
  for (auto it = groupedFolders.constBegin(); it != groupedFolders.constEnd(); ++it) {
    if (!categoryOrder.contains(it.key())) {
      categoryOrder.append(it.key());
    }
  }

  auto categoryTitle = [](MediaType type) -> QString {
    switch (type) {
    case MediaType::Jingle:
      return tr("Vinhetas");
    case MediaType::Music:
      return tr("Músicas");
    case MediaType::Commercial:
      return tr("Comerciais");
    case MediaType::Command:
      return tr("Comandos");
    default:
      return tr("Outros");
    }
  };

  for (MediaType cat : categoryOrder) {
    if (!groupedFolders.contains(cat) || groupedFolders[cat].isEmpty()) {
      continue;
    }

    // Cabeçalho de Categoria
    FolderItem header;
    header.itemType = FolderItemType::CategoryHeader;
    header.title = categoryTitle(cat);
    header.type = cat;
    m_items.append(header);

    // Pastas pertencentes à categoria
    m_items.append(groupedFolders[cat]);
  }

  endResetModel();
}

void FolderListModel::clear() {
  beginResetModel();
  m_items.clear();
  endResetModel();
}

QVector<FolderItem> FolderListModel::mediaFolders() const {
  QVector<FolderItem> result;
  result.reserve(m_items.size());
  for (const FolderItem &item : std::as_const(m_items)) {
    if (!item.isHeader() && !item.isCommandFolder()) {
      result.append(item);
    }
  }
  return result;
}

const FolderItem *FolderListModel::itemAt(int row) const {
  if (row < 0 || row >= m_items.size()) {
    return nullptr;
  }
  return &m_items.at(row);
}

QModelIndex FolderListModel::indexForId(const QString &id) const {
  if (id.isEmpty()) {
    return QModelIndex();
  }
  for (int i = 0; i < m_items.size(); ++i) {
    if (!m_items[i].isHeader() && m_items[i].id == id) {
      return index(i, 0);
    }
  }
  return QModelIndex();
}

QModelIndex FolderListModel::indexForPath(const QString &path) const {
  for (int i = 0; i < m_items.size(); ++i) {
    if (!m_items[i].isHeader() && m_items[i].path == path) {
      return index(i, 0);
    }
  }
  return QModelIndex();
}

QModelIndex FolderListModel::firstFolderIndex() const {
  for (int i = 0; i < m_items.size(); ++i) {
    if (!m_items[i].isHeader()) {
      return index(i, 0);
    }
  }
  return QModelIndex();
}
