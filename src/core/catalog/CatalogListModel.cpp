#include "CatalogListModel.h"

#include <QDataStream>
#include <QIODevice>

CatalogListModel::CatalogListModel(QObject *parent)
    : QAbstractListModel(parent) {}

int CatalogListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return m_items.size();
}

QVariant CatalogListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
    return QVariant();
  }

  const CatalogItem &item = m_items.at(index.row());

  switch (role) {
  case Qt::DisplayRole:
  case Qt::ToolTipRole:
    return item.title;
  case CatalogItemTypeRole:
    return static_cast<int>(item.itemType);
  case CatalogPathRole:
    return item.path;
  case CatalogMediaTypeRole:
    return item.mediaType;
  case CatalogDurationRole:
    return item.durationMs;
  case CatalogFolderNameRole:
    return item.folderName;
  default:
    return QVariant();
  }
}

Qt::ItemFlags CatalogListModel::flags(const QModelIndex &index) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
    return Qt::NoItemFlags;
  }

  const CatalogItem &item = m_items.at(index.row());
  if (item.isHeader()) {
    // Cabeçalho de pasta: visível mas não selecionável e não arrastável
    return Qt::ItemIsEnabled;
  }

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QStringList CatalogListModel::mimeTypes() const {
  return {"application/x-catalog-item"};
}

QMimeData *CatalogListModel::mimeData(const QModelIndexList &indexes) const {
  QMimeData *mime = new QMimeData();
  for (const QModelIndex &idx : indexes) {
    if (!idx.isValid() || idx.row() < 0 || idx.row() >= m_items.size()) {
      continue;
    }
    const CatalogItem &item = m_items.at(idx.row());
    if (item.isHeader()) {
      continue;
    }

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << item.title << item.path << item.mediaType;
    mime->setData("application/x-catalog-item", data);
    break; // Arrasto de item único
  }
  return mime;
}

void CatalogListModel::setItems(const QVector<CatalogItem> &items) {
  beginResetModel();
  m_items = items;
  endResetModel();
}

void CatalogListModel::clear() {
  beginResetModel();
  m_items.clear();
  endResetModel();
}

const CatalogItem *CatalogListModel::itemAt(int row) const {
  if (row < 0 || row >= m_items.size()) {
    return nullptr;
  }
  return &m_items.at(row);
}
