#ifndef PLAYLISTITEMDELEGATE_H
#define PLAYLISTITEMDELEGATE_H

#pragma once

#include <QPainter>
#include <QStyledItemDelegate>


class PlaylistItemDelegate : public QStyledItemDelegate {
  Q_OBJECT
public:
  PlaylistItemDelegate(QObject *parent = nullptr);
  ~PlaylistItemDelegate();

  QSize sizeHint(const QStyleOptionViewItem &option,
                 const QModelIndex &index) const override;
  void paint(QPainter *p, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;

signals:
};

#endif // PLAYLISTITEMDELEGATE_H
