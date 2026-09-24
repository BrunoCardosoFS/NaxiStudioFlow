#include "PlaylistItemDelegate.h"

PlaylistItemDelegate::PlaylistItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent) {}

PlaylistItemDelegate::~PlaylistItemDelegate() = default;

QSize PlaylistItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
  return QStyledItemDelegate::sizeHint(option, index);
}

void PlaylistItemDelegate::paint(QPainter *p,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const {
  QStyledItemDelegate::paint(p, option, index);
}
