#include "CatalogItemDelegate.h"

#include <QFontMetrics>
#include <QIcon>

#include "core/catalog/CatalogItem.h"

CatalogItemDelegate::CatalogItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent) {}

QSize CatalogItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const {
  int itemType = index.data(CatalogItemTypeRole).toInt();
  if (itemType == static_cast<int>(CatalogItemType::FolderHeader)) {
    return QSize(option.rect.width(), 34);
  }
  return QSize(option.rect.width(), 42);
}

void CatalogItemDelegate::paint(QPainter *painter,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index) const {
  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, true);

  int itemType = index.data(CatalogItemTypeRole).toInt();
  QString title = index.data(Qt::DisplayRole).toString();

  if (itemType == static_cast<int>(CatalogItemType::FolderHeader)) {
    // Renderiza o cabeçalho de divisão por pasta
    QRect rect = option.rect.adjusted(2, 6, -2, -2);

    painter->setBrush(QColor("#181B22"));
    painter->setPen(QColor("#2C3442"));
    painter->drawRoundedRect(rect, 4, 4);

    // Ícone de pasta
    QIcon folderIcon(":/images/icons/open.svg");
    QRect iconRect(rect.left() + 8, rect.top() + (rect.height() - 16) / 2, 16, 16);
    folderIcon.paint(painter, iconRect);

    // Título da pasta
    QFont font = option.font;
    font.setBold(true);
    font.setPixelSize(11);
    painter->setFont(font);
    painter->setPen(QColor("#8E9EB5"));

    QRect textRect(iconRect.right() + 8, rect.top(), rect.width() - 36, rect.height());
    QFontMetrics fm(font);
    QString elidedTitle = fm.elidedText(title.toUpper(), Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedTitle);
  } else {
    // Renderiza o item de arquivo de áudio
    QRect rect = option.rect.adjusted(2, 2, -2, -2);

    QColor bgColor = QColor("#2D3340");
    if (option.state & QStyle::State_Selected) {
      bgColor = QColor("#3E4A5E");
    } else if (option.state & QStyle::State_MouseOver) {
      bgColor = QColor("#333A48");
    }

    painter->setBrush(bgColor);
    if (option.state & QStyle::State_Selected) {
      painter->setPen(QColor("#4E95FF"));
    } else {
      painter->setPen(Qt::NoPen);
    }
    painter->drawRoundedRect(rect, 6, 6);

    // Ícone do tipo de mídia
    int mediaType = index.data(CatalogMediaTypeRole).toInt();
    QString iconPath;
    if (mediaType == 0) {
      iconPath = ":/images/catalog/jingle.svg";
    } else if (mediaType == 1) {
      iconPath = ":/images/catalog/music.svg";
    } else if (mediaType == 2) {
      iconPath = ":/images/catalog/commercial.svg";
    } else {
      iconPath = ":/images/catalog/other.svg";
    }

    QIcon mediaIcon(iconPath);
    QRect iconRect(rect.left() + 8, rect.top() + (rect.height() - 20) / 2, 20, 20);
    mediaIcon.paint(painter, iconRect);

    // Título do arquivo
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
