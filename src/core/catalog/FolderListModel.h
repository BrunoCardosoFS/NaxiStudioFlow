#ifndef FOLDERLISTMODEL_H
#define FOLDERLISTMODEL_H

#include <QAbstractListModel>
#include <QVector>

#include "core/catalog/FolderItem.h"

class FolderListModel : public QAbstractListModel {
  Q_OBJECT
public:
  explicit FolderListModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;

  void loadFromDatabase(const QString &pathDB);
  void clear();

  const QVector<FolderItem> &items() const { return m_items; }
  const FolderItem *itemAt(int row) const;
  QModelIndex indexForPath(const QString &path) const;
  QModelIndex firstFolderIndex() const;

private:
  QVector<FolderItem> m_items;
};

#endif // FOLDERLISTMODEL_H
