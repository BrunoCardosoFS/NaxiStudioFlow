#ifndef FOLDERITEMDELEGATE_H
#define FOLDERITEMDELEGATE_H

#include <QPainter>
#include <QStyledItemDelegate>

class FolderItemDelegate : public QStyledItemDelegate {
  Q_OBJECT
public:
  explicit FolderItemDelegate(QObject *parent = nullptr);
  ~FolderItemDelegate() override = default;

  QSize sizeHint(const QStyleOptionViewItem &option,
                 const QModelIndex &index) const override;
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;

private:
  QIcon m_iconJingle;
  QIcon m_iconMusic;
  QIcon m_iconCommercial;
  QIcon m_iconOther;

  const QIcon &iconForType(int type) const;
};

#endif // FOLDERITEMDELEGATE_H
