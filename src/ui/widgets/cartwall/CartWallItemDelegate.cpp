#include "CartWallItemDelegate.h"
#include "core/cartwall/CartItem.h"
#include "core/utils/TimeUtils.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>

namespace {
constexpr int kCellMargin = 2;
constexpr int kPadding = 8;
constexpr qreal kRadius = 10.0;

const QColor kColWhite = QColor::fromRgb(0xFFFFFF);
const QColor kColWhiteDim = QColor(255, 255, 255, 215);

QBrush makeVerticalBrush(const QColor &top, const QColor &bottom) {
  QLinearGradient g(0.0, 0.0, 0.0, 1.0);
  g.setCoordinateMode(QGradient::ObjectBoundingMode);
  g.setColorAt(0.0, top);
  g.setColorAt(1.0, bottom);
  return QBrush(g);
}
} // namespace

CartWallItemDelegate::CartWallItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent), m_iconMusic(":/images/catalog/music.svg"),
      m_iconJingle(":/images/catalog/jingle.svg"),
      m_iconCommercial(":/images/catalog/commercial.svg"),
      m_iconOther(":/images/catalog/other.svg"),
      m_brushMusic(makeVerticalBrush(QColor::fromRgb(0x418DB4),
                                     QColor::fromRgb(0x0066A5))),
      m_brushJingle(makeVerticalBrush(QColor::fromRgb(0x68B127),
                                      QColor::fromRgb(0x3C8808))),
      m_brushCommercial(makeVerticalBrush(QColor::fromRgb(0xD97706),
                                          QColor::fromRgb(0xB45309))),
      m_brushOther(makeVerticalBrush(QColor::fromRgb(0x475569),
                                     QColor::fromRgb(0x334155))),
      m_brushProgress(makeVerticalBrush(QColor::fromRgb(0xCF3B6B),
                                        QColor::fromRgb(0xAC073C))) {}

CartWallItemDelegate::~CartWallItemDelegate() = default;

const QIcon &CartWallItemDelegate::iconForType(MediaType type) const {
  switch (type) {
  case MediaType::Music:
    return m_iconMusic;
  case MediaType::Jingle:
    return m_iconJingle;
  case MediaType::Commercial:
    return m_iconCommercial;
  default:
    return m_iconOther;
  }
}

const QBrush &CartWallItemDelegate::brushForType(MediaType type) const {
  switch (type) {
  case MediaType::Music:
    return m_brushMusic;
  case MediaType::Jingle:
    return m_brushJingle;
  case MediaType::Commercial:
    return m_brushCommercial;
  default:
    return m_brushOther;
  }
}

QSize CartWallItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
  Q_UNUSED(index);
  return option.rect.isValid() ? option.rect.size() : QSize(120, 80);
}

void CartWallItemDelegate::paint(QPainter *p,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const {
  if (!index.data(CartUuidRole).isValid())
    return; // Not an item: nothing is drawn for cells past the list end.

  const auto type =
      static_cast<MediaType>(index.data(CartMediaTypeRole).toInt());
  const auto status =
      static_cast<CartItemStatus>(index.data(CartStatusRole).toInt());
  const qint64 duration = index.data(CartDurationRole).toLongLong();
  const qint64 position = index.data(CartPositionRole).toLongLong();
  const QString title = index.data(CartTitleRole).toString();

  const bool selected = option.state & QStyle::State_Selected;
  const bool hovered = option.state & QStyle::State_MouseOver;

  const QRectF card =
      QRectF(option.rect)
          .adjusted(kCellMargin, kCellMargin, -kCellMargin, -kCellMargin);
  if (card.width() <= 0 || card.height() <= 0)
    return;

  p->save();
  p->setRenderHint(QPainter::Antialiasing, true);
  p->setRenderHint(QPainter::TextAntialiasing, true);

  QPainterPath clip;
  clip.addRoundedRect(card, kRadius, kRadius);

  // Background
  p->setPen(Qt::NoPen);
  p->setBrush(brushForType(type));
  p->drawPath(clip);

  if (status == CartItemStatus::Finished) {
    p->fillPath(clip, QColor(0, 0, 0, 90));
  } else if (hovered) {
    p->fillPath(clip, QColor(255, 255, 255, 30));
  }

  // Progress: fills the card from the left, clipped to the rounded shape
  // (same approach used by the playlist media item)
  if (status == CartItemStatus::Playing && duration > 0) {
    const qreal ratio = qBound<qreal>(0.0, qreal(position) / duration, 1.0);
    const qreal progWidth = card.width() * ratio;
    if (progWidth > 0) {
      p->save();
      p->setClipPath(clip);
      p->fillRect(QRectF(card.left(), card.top(), progWidth, card.height()),
                  m_brushProgress);
      p->restore();
    }
  }

  // Icon
  const int iconSize = qBound(14, int(card.height() * 0.22), 28);
  QRect iconRect(int(card.left()) + kPadding, int(card.top()) + kPadding,
                 iconSize, iconSize);
  iconForType(type).paint(p, iconRect);

  // Time (top right): remaining while playing, total otherwise
  QFont timeFont = option.font;
  timeFont.setPointSize(10);
  timeFont.setBold(true);
  p->setFont(timeFont);
  p->setPen(kColWhite);

  const qint64 shown = (status == CartItemStatus::Playing)
                           ? qMax<qint64>(0, duration - position)
                           : duration;
  const QString timeText = formatDuration(shown);
  QRect timeRect(iconRect.right() + kPadding, iconRect.top(),
                 int(card.right()) - kPadding - iconRect.right() - kPadding,
                 iconRect.height());
  p->drawText(timeRect, Qt::AlignRight | Qt::AlignVCenter, timeText);

  // Title (anchored to the bottom, wrapped, ellipsized to the free height)
  QFont titleFont = option.font;
  titleFont.setPointSize(10);
  titleFont.setBold(true);
  p->setFont(titleFont);
  p->setPen(kColWhiteDim);

  QRect titleRect(int(card.left()) + kPadding, iconRect.bottom() + 4,
                  int(card.width()) - 2 * kPadding,
                  int(card.bottom()) - kPadding - iconRect.bottom() - 4);
  if (titleRect.height() > 0) {
    QFontMetrics fm(titleFont);
    const int lineH = fm.lineSpacing();
    const int maxLines = qMax(1, titleRect.height() / lineH);
    // Keep the bottom edge of the text box fixed, so the text hugs the bottom
    QRect bounded = titleRect;
    bounded.setTop(titleRect.bottom() + 1 - maxLines * lineH);
    QString text = title;
    QRect needed = fm.boundingRect(bounded, Qt::TextWordWrap, text);
    if (needed.height() > bounded.height()) {
      // Trim until it fits, then append an ellipsis
      while (!text.isEmpty() &&
             fm.boundingRect(bounded, Qt::TextWordWrap, text + QChar(0x2026))
                     .height() > bounded.height()) {
        text.chop(1);
      }
      text = text.trimmed() + QChar(0x2026);
    }
    p->drawText(bounded, Qt::AlignLeft | Qt::AlignBottom | Qt::TextWordWrap,
                text);
  }

  // Selection outline
  if (selected) {
    p->setBrush(Qt::NoBrush);
    p->setPen(QPen(kColWhite, 2.0));
    p->drawRoundedRect(card.adjusted(1, 1, -1, -1), kRadius, kRadius);
  }

  p->restore();
}
