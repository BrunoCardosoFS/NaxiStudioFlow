#include "FolderListModel.h"
#include "CatalogRepository.h"

#include <QHash>
#include <QList>

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
  case Qt::ToolTipRole:
    return item.isHeader() ? item.title : item.title;
  case FolderItemTypeRole:
    return static_cast<int>(item.itemType);
  case FolderPathRole:
    return item.path;
  case FolderTypeRole:
    return item.type;
  case FolderIdRole:
    return item.id;
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
  QHash<int, QVector<FolderItem>> groupedFolders;
  for (const FolderItem &item : loadedFolders) {
    groupedFolders[item.type].append(item);
  }

  // Ordem padrão de exibição das categorias
  QList<int> categoryOrder = {0, 1, 2};
  for (auto it = groupedFolders.constBegin(); it != groupedFolders.constEnd(); ++it) {
    if (!categoryOrder.contains(it.key())) {
      categoryOrder.append(it.key());
    }
  }

  auto categoryTitle = [](int type) -> QString {
    switch (static_cast<MediaType>(type)) {
    case MediaType::Jingle:
      return "Vinhetas";
    case MediaType::Music:
      return "Músicas";
    case MediaType::Commercial:
      return "Comerciais";
    default:
      return "Outros";
    }
  };

  for (int cat : categoryOrder) {
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

const FolderItem *FolderListModel::itemAt(int row) const {
  if (row < 0 || row >= m_items.size()) {
    return nullptr;
  }
  return &m_items.at(row);
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
