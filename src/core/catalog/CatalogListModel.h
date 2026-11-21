#ifndef CATALOGLISTMODEL_H
#define CATALOGLISTMODEL_H

#include <QAbstractListModel>
#include <QMimeData>
#include <QVector>

#include "core/catalog/CatalogItem.h"

class CatalogListModel : public QAbstractListModel {
  Q_OBJECT
public:
  explicit CatalogListModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;

  QStringList mimeTypes() const override;
  QMimeData *mimeData(const QModelIndexList &indexes) const override;

  void setItems(const QVector<CatalogItem> &items);
  void showLoading(const QString &message = "Carregando...");
  void clear();

  bool isLoading() const;
  const QVector<CatalogItem> &items() const { return m_items; }
  const CatalogItem *itemAt(int row) const;

private:
  QVector<CatalogItem> m_items;
};

#endif // CATALOGLISTMODEL_H
