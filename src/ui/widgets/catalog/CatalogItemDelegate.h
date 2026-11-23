#ifndef CATALOGITEMDELEGATE_H
#define CATALOGITEMDELEGATE_H

#include <QPainter>
#include <QStyledItemDelegate>

enum class MediaType : qint8;

class CatalogItemDelegate : public QStyledItemDelegate {
  Q_OBJECT
public:
  explicit CatalogItemDelegate(QObject *parent = nullptr);
  ~CatalogItemDelegate() override = default;

  QSize sizeHint(const QStyleOptionViewItem &option,
                 const QModelIndex &index) const override;
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;

private:
  QIcon m_iconFolderOpen;
  QIcon m_iconJingle;
  QIcon m_iconMusic;
  QIcon m_iconCommercial;
  QIcon m_iconOther;
  QIcon m_iconCommand;
  QIcon m_iconCommandPause;

  int m_spinnerAngle = 0;
  QWidget *m_viewport = nullptr;

  const QIcon &iconForType(MediaType mediaType) const;
  const QIcon &iconForIndex(const QModelIndex &index) const;
};

#endif // CATALOGITEMDELEGATE_H
