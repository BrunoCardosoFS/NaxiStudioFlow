#ifndef CATALOGITEMDELEGATE_H
#define CATALOGITEMDELEGATE_H

#include <QPainter>
#include <QStyledItemDelegate>

class CatalogItemDelegate : public QStyledItemDelegate {
  Q_OBJECT
public:
  explicit CatalogItemDelegate(QObject *parent = nullptr);
  ~CatalogItemDelegate() override = default;

  QSize sizeHint(const QStyleOptionViewItem &option,
                 const QModelIndex &index) const override;
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;
};

#endif // CATALOGITEMDELEGATE_H
