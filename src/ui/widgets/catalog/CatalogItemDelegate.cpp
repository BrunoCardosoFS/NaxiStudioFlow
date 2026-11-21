#include "CatalogItemDelegate.h"

#include <QFontMetrics>
#include <QIcon>

#include "core/catalog/CatalogItem.h"

CatalogItemDelegate::CatalogItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent), m_iconFolderOpen(":/images/icons/open.svg"),
      m_iconJingle(":/images/catalog/jingle.svg"),
      m_iconMusic(":/images/catalog/music.svg"),
      m_iconCommercial(":/images/catalog/commercial.svg"),
      m_iconOther(":/images/catalog/other.svg") {}

const QIcon &CatalogItemDelegate::iconForType(int mediaType) const {
  switch (static_cast<MediaType>(mediaType)) {
  case MediaType::Jingle:
    return m_iconJingle;
  case MediaType::Music:
    return m_iconMusic;
  case MediaType::Commercial:
    return m_iconCommercial;
  default:
    return m_iconOther;
  }
}

QSize CatalogItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const {
  int itemType = index.data(CatalogItemTypeRole).toInt();
  if (itemType == static_cast<int>(CatalogItemType::Loading)) {
    return QSize(option.rect.width(), 44);
  }
  if (itemType == static_cast<int>(CatalogItemType::FolderHeader)) {
    return QSize(option.rect.width(), 34);
  }
  return QSize(option.rect.width(), 35);
}

void CatalogItemDelegate::paint(QPainter *painter,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index) const {
  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, true);

  int itemType = index.data(CatalogItemTypeRole).toInt();
  QString title = index.data(Qt::DisplayRole).toString();

  if (itemType == static_cast<int>(CatalogItemType::Loading)) {
    QRect rect = option.rect.adjusted(2, 2, -2, -2);

    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(rect, 6, 6);

    QFont font = option.font;
    font.setPixelSize(11);
    font.setBold(false);
    painter->setFont(font);
    painter->setPen(QColor("#8E9EB5"));

    int textLeft = 10;
    int textWidth = rect.right() - textLeft - 8;
    QRect textRect(textLeft, rect.top(), textWidth, rect.height());
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignHCenter,
                      title.isEmpty() ? "Carregando..." : title);

  } else if (itemType == static_cast<int>(CatalogItemType::FolderHeader)) {
    QRect rect = option.rect.adjusted(2, 6, -2, -2);

    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(rect, 4, 4);

    QFont font = option.font;
    font.setBold(true);
    font.setPixelSize(11);
    painter->setFont(font);
    painter->setPen(QColor("#8E9EB5"));

    QRect textRect(8, rect.top(), rect.width() - 36, rect.height());
    QFontMetrics fm(font);
    QString elidedTitle =
        fm.elidedText(title.toUpper(), Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedTitle);
  } else {
    QRect rect = option.rect.adjusted(2, 2, -2, -2);

    QColor bgColor = QColor("#2D3340");
    if (option.state & QStyle::State_Selected) {
      bgColor = QColor("#3E4A5E");
    } else if (option.state & QStyle::State_MouseOver) {
      bgColor = QColor("#333A48");
    }

    painter->setBrush(bgColor);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(rect, 6, 6);

    int mediaType = index.data(CatalogMediaTypeRole).toInt();
    const QIcon &mediaIcon = iconForType(mediaType);
    QRect iconRect(rect.left() + 8, rect.top() + (rect.height() - 20) / 2, 20,
                   20);
    mediaIcon.paint(painter, iconRect);

    QFont font = option.font;
    font.setPixelSize(12);
    font.setBold(false);
    painter->setFont(font);
    painter->setPen(QColor("#FFFFFF"));

    int textLeft = iconRect.right() + 8;
    int textWidth = rect.right() - textLeft - 8;
    QRect textRect(textLeft, rect.top(), textWidth, rect.height());

    QFontMetrics fm(font);
    QString elidedTitle = fm.elidedText(title, Qt::ElideRight, textWidth);
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedTitle);
  }

  painter->restore();
}
