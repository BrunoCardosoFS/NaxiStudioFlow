#ifndef CARTWALLITEMDELEGATE_H
#define CARTWALLITEMDELEGATE_H

#pragma once

#include <QBrush>
#include <QFont>
#include <QIcon>
#include <QStyledItemDelegate>

enum class MediaType : qint8;

// Paint-only delegate. Interaction (click, drag) is handled by CartWallView.
class CartWallItemDelegate : public QStyledItemDelegate {
  Q_OBJECT
public:
  explicit CartWallItemDelegate(QObject *parent = nullptr);
  ~CartWallItemDelegate() override;

  void paint(QPainter *p, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;
  QSize sizeHint(const QStyleOptionViewItem &option,
                 const QModelIndex &index) const override;

private:
  const QIcon &iconForType(MediaType type) const;
  const QBrush &brushForType(MediaType type) const;

  QIcon m_iconMusic;
  QIcon m_iconJingle;
  QIcon m_iconCommercial;
  QIcon m_iconOther;

  // ObjectBoundingMode gradients: independent of the item rect.
  QBrush m_brushMusic;
  QBrush m_brushJingle;
  QBrush m_brushCommercial;
  QBrush m_brushOther;
  QBrush m_brushProgress;
};

#endif // CARTWALLITEMDELEGATE_H
