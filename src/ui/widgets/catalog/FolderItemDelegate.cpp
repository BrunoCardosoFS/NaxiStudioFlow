#include "FolderItemDelegate.h"

#include <QFontMetrics>
#include <QIcon>

#include "core/catalog/FolderItem.h"

FolderItemDelegate::FolderItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent), m_iconJingle(":/images/catalog/jingle.svg"),
      m_iconMusic(":/images/catalog/music.svg"),
      m_iconCommercial(":/images/catalog/commercial.svg"),
      m_iconOther(":/images/catalog/other.svg") {}

const QIcon &FolderItemDelegate::iconForType(int type) const {
  switch (static_cast<MediaType>(type)) {
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

QSize FolderItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const {
  int itemType = index.data(FolderItemTypeRole).toInt();
  if (itemType == static_cast<int>(FolderItemType::CategoryHeader)) {
    return QSize(option.rect.width(), 28);
  }
  return QSize(option.rect.width(), 34);
}

void FolderItemDelegate::paint(QPainter *painter,
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index) const {
  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, true);

  int itemType = index.data(FolderItemTypeRole).toInt();
  QString title = index.data(Qt::DisplayRole).toString();

  if (itemType == static_cast<int>(FolderItemType::CategoryHeader)) {
    // Renders the category header
    QRect rect = option.rect;

    QFont font = option.font;
    font.setBold(true);
    font.setPixelSize(10);
    painter->setFont(font);
    painter->setPen(QColor("#7E91A8"));

    QFontMetrics fm(font);
    QString upperTitle = title.toUpper();
    int textWidth = fm.horizontalAdvance(upperTitle);

    QRect textRect(rect.left() + 6,
                   rect.top() + (rect.height() - fm.height()) / 2, textWidth,
                   fm.height());
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignHCenter,
                      upperTitle);
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

    // Ícone da pasta em cache baseado no tipo
    int folderType = index.data(FolderTypeRole).toInt();
    const QIcon &folderIcon = iconForType(folderType);
    QRect iconRect(rect.left() + 6, rect.top() + (rect.height() - 16) / 2, 16,
                   16);
    folderIcon.paint(painter, iconRect);

    // Título da pasta
    QFont font = option.font;
    font.setPixelSize(11);
    font.setBold(false);
    painter->setFont(font);
    painter->setPen(QColor("#FFFFFF"));

    int textLeft = iconRect.right() + 6;
    int textWidth = rect.right() - textLeft - 6;
    QRect textRect(textLeft, rect.top(), textWidth, rect.height());

    QFontMetrics fm(font);
    QString elidedTitle = fm.elidedText(title, Qt::ElideRight, textWidth);
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedTitle);
  }

  painter->restore();
}
