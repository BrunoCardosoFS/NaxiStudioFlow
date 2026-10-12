#include "PlaylistItemDelegate.h"
#include "core/models.h"
#include "core/utils/TimeUtils.h"

#include <QLinearGradient>
#include <QPainterPath>

namespace {
constexpr int kBlockPaddingSide = 12;
constexpr int kBlockMargin = 14;
constexpr int kBlockInnerBottomPad = 8;
constexpr qreal kBlockRadius = 14.0;

const QColor kColListBg = QColor::fromRgb(0x2D3340);
const QColor kColBlockBg = QColor::fromRgb(0x1F232C);
const QColor kColWhite = QColor::fromRgb(0xFFFFFF);
const QColor kColWhiteDim = QColor(255, 255, 255, 210);
const QColor kColMuted = QColor::fromRgb(0x8E9EB5);
const QColor kColFinishedSel = QColor::fromRgb(0x283042);
const QColor kColBadgePlayingBg = QColor::fromRgb(0xE50914);
const QColor kColBadgeNextBg = QColor::fromRgb(0xF59E0B);
const QColor kColBlack = QColor::fromRgb(0x000000);

const QPen kPenLine(kColListBg, 2);
const QPen kPenSelection(kColWhite, 1.5);
const QPen kPenClock(kColWhite, 1.2);

QBrush makeVerticalBrush(const QColor &top, const QColor &bottom) {
  QLinearGradient g(0.0, 0.0, 0.0, 1.0);
  g.setCoordinateMode(QGradient::ObjectBoundingMode);
  g.setColorAt(0.0, top);
  g.setColorAt(1.0, bottom);
  return QBrush(g);
}

inline ItemType typeOf(const QModelIndex &idx) {
  return static_cast<ItemType>(idx.data(TypeRole).toInt());
}
} // namespace

PlaylistItemDelegate::FontSet::FontSet(const QFont &base, int pointSize,
                                       bool bold)
    : font([&] {
        QFont f = base;
        f.setPointSize(pointSize);
        f.setBold(bold);
        return f;
      }()),
      fm(font) {}

PlaylistItemDelegate::Fonts::Fonts(const QFont &base)
    : blockTime(base, 13, true), blockName(base, 11, false),
      marker(base, 9, true), mediaFinished(base, 10, false),
      mediaTitle(base, 10, true), mediaPos(base, 10, true),
      mediaDur(base, 9, false), badge(base, 7, true), est(base, 9, false) {}

const PlaylistItemDelegate::Fonts &
PlaylistItemDelegate::fonts(const QFont &base) const {
  if (!m_fonts || m_cachedBaseFont != base) {
    m_cachedBaseFont = base;
    m_fonts = std::make_unique<Fonts>(base);
  }
  return *m_fonts;
}

PlaylistItemDelegate::PlaylistItemDelegate(QObject *parent)
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

PlaylistItemDelegate::~PlaylistItemDelegate() = default;

const QIcon &PlaylistItemDelegate::iconForType(MediaType type) const {
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

const QBrush &PlaylistItemDelegate::brushForType(MediaType type) const {
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

void PlaylistItemDelegate::drawTypeIcon(QPainter *p, const QRect &rect,
                                        MediaType type) const {
  int size = qMin(rect.width(), rect.height());
  if (size <= 0)
    return;

  QRect iconRect(rect.left() + (rect.width() - size) / 2,
                 rect.top() + (rect.height() - size) / 2, size, size);
  iconForType(type).paint(p, iconRect);
}

bool PlaylistItemDelegate::isFirstInBlock(const QModelIndex &index,
                                          bool isBlockMarker) const {
  return index.row() == 0 || isBlockMarker;
}

bool PlaylistItemDelegate::isLastInBlock(const QModelIndex &index) const {
  const QAbstractItemModel *m = index.model();
  if (!m)
    return true;
  const int nextRow = index.row() + 1;
  if (nextRow >= m->rowCount())
    return true;
  return typeOf(m->index(nextRow, 0)) == ItemType::BlockMarker;
}

void PlaylistItemDelegate::drawBlockBackground(QPainter *p,
                                               const QRect &blockRect,
                                               bool isFirst,
                                               bool isLast) const {
  if (!isFirst && !isLast) {
    p->fillRect(blockRect, kColBlockBg);
    return;
  }

  const QRectF rf(blockRect);
  const qreal r = qMin(kBlockRadius, qMin(rf.width() / 2.0, rf.height()));
  QPainterPath path;

  if (isFirst && isLast) {
    path.addRoundedRect(rf, kBlockRadius, kBlockRadius);
  } else if (isFirst) {
    path.moveTo(rf.left(), rf.bottom());
    path.lineTo(rf.left(), rf.top() + r);
    path.arcTo(rf.left(), rf.top(), 2 * r, 2 * r, 180, -90);
    path.lineTo(rf.right() - r, rf.top());
    path.arcTo(rf.right() - 2 * r, rf.top(), 2 * r, 2 * r, 90, -90);
    path.lineTo(rf.right(), rf.bottom());
    path.closeSubpath();
  } else {
    path.moveTo(rf.left(), rf.top());
    path.lineTo(rf.right(), rf.top());
    path.lineTo(rf.right(), rf.bottom() - r);
    path.arcTo(rf.right() - 2 * r, rf.bottom() - 2 * r, 2 * r, 2 * r, 0, -90);
    path.lineTo(rf.left() + r, rf.bottom());
    path.arcTo(rf.left(), rf.bottom() - 2 * r, 2 * r, 2 * r, 270, -90);
    path.closeSubpath();
  }
  p->fillPath(path, kColBlockBg);
}

QSize PlaylistItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
  if (!index.isValid())
    return QStyledItemDelegate::sizeHint(option, index);

  const ItemType type = typeOf(index);

  int h = 48;
  switch (type) {
  case ItemType::BlockMarker:
    h = 44;
    break;
  case ItemType::StartMarker: {
    const BlockType bType =
        static_cast<BlockType>(index.data(BlockTypeRole).toInt());
    h = (bType == BlockType::Commercial) ? 24 : 6;
    break;
  }
  case ItemType::EndMarker: {
    const BlockType bType =
        static_cast<BlockType>(index.data(BlockTypeRole).toInt());
    h = (bType == BlockType::Commercial) ? 26 : 24;
    break;
  }
  case ItemType::Media: {
    const PlaylistItemStatus status =
        static_cast<PlaylistItemStatus>(index.data(StatusRole).toInt());
    if (status == PlaylistItemStatus::Finished)
      h = 32;
    else if (status == PlaylistItemStatus::Playing)
      h = 50;
    break;
  }
  }

  if (isFirstInBlock(index, type == ItemType::BlockMarker))
    h += kBlockMargin;

  if (isLastInBlock(index)) {
    h += kBlockInnerBottomPad;
    const QAbstractItemModel *m = index.model();
    if (m && index.row() + 1 >= m->rowCount())
      h += kBlockMargin;
  }

  return QSize(option.rect.width(), h);
}

void PlaylistItemDelegate::paint(QPainter *p,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const {
  if (!index.isValid())
    return;

  const Fonts &f = fonts(option.font);
  const ItemType type = typeOf(index);

  p->save();

  p->fillRect(option.rect, kColListBg);

  const bool first = isFirstInBlock(index, type == ItemType::BlockMarker);
  const bool last = isLastInBlock(index);
  const QAbstractItemModel *m = index.model();
  const bool isEndOfList = last && m && index.row() + 1 >= m->rowCount();

  const QRect r = option.rect;
  const int topOffset = first ? kBlockMargin : 0;
  const int bottomOffset = isEndOfList ? kBlockMargin : 0;

  const QRect blockRect(r.left() + kBlockPaddingSide, r.top() + topOffset,
                        r.width() - 2 * kBlockPaddingSide,
                        r.height() - topOffset - bottomOffset);

  p->setRenderHint(QPainter::Antialiasing, first || last);
  drawBlockBackground(p, blockRect, first, last);

  QRect contentRect = blockRect;
  if (last)
    contentRect.setHeight(contentRect.height() - kBlockInnerBottomPad);

  QStyleOptionViewItem itemOpt = option;
  itemOpt.rect = contentRect;

  p->setRenderHint(QPainter::Antialiasing, false);
  switch (type) {
  case ItemType::BlockMarker:
    paintBlockMarker(p, itemOpt, index, f);
    break;
  case ItemType::StartMarker:
    paintStartMarker(p, itemOpt, index, f);
    break;
  case ItemType::EndMarker:
    paintEndMarker(p, itemOpt, index, f);
    break;
  case ItemType::Media:
    paintMediaItem(p, itemOpt, index, f);
    break;
  }

  p->restore();
}

void PlaylistItemDelegate::paintBlockMarker(QPainter *p,
                                            const QStyleOptionViewItem &opt,
                                            const QModelIndex &index,
                                            const Fonts &f) const {
  const QRect r = opt.rect;

  const QString timeStr = index.data(TitleRole).toString();
  p->setFont(f.blockTime.font);
  p->setPen(kColWhite);

  const int timeW = f.blockTime.fm.horizontalAdvance(timeStr);
  const QRect timeRect(r.left() + 16, r.top(), timeW, r.height());
  p->drawText(timeRect, Qt::AlignVCenter | Qt::AlignLeft, timeStr);

  const QString blockName = index.data(BlockTitleRole).toString();
  if (!blockName.isEmpty()) {
    const int nameLeft = timeRect.right() + 12;
    const int nameWidth = r.right() - 16 - nameLeft;
    if (nameWidth > 0) {
      p->setFont(f.blockName.font);
      p->drawText(QRect(nameLeft, r.top(), nameWidth, r.height()),
                  Qt::AlignVCenter | Qt::AlignLeft, blockName);
    }
  }
}

void PlaylistItemDelegate::paintStartMarker(QPainter *p,
                                            const QStyleOptionViewItem &opt,
                                            const QModelIndex &index,
                                            const Fonts &f) const {
  const QRect r = opt.rect;
  const BlockType bType =
      static_cast<BlockType>(index.data(BlockTypeRole).toInt());

  p->setPen(kColListBg);
  p->drawLine(r.left(), r.top(), r.right(), r.top());

  if (bType == BlockType::Commercial) {
    const int lineX = r.left() + 7;
    const int centerY = r.center().y();
    const int tickLength = 14;

    p->setPen(kPenLine);
    p->drawLine(lineX, centerY, lineX, r.bottom());
    p->drawLine(lineX, centerY, lineX + tickLength, centerY);

    p->setFont(f.marker.font);
    p->setPen(kColMuted);

    const int textLeft = lineX + tickLength + 8;
    p->drawText(QRect(textLeft, r.top(), r.right() - 16 - textLeft, r.height()),
                Qt::AlignVCenter | Qt::AlignLeft,
                QStringLiteral("BLOCO COMERCIAL"));
  }
}

void PlaylistItemDelegate::paintEndMarker(QPainter *p,
                                          const QStyleOptionViewItem &opt,
                                          const QModelIndex &index,
                                          const Fonts &f) const {
  const QRect r = opt.rect;
  const BlockType bType =
      static_cast<BlockType>(index.data(BlockTypeRole).toInt());
  const int itemsCount = index.data(ItemsCountRole).toInt();
  const qint64 totalDurationMs = index.data(TotalDurationRole).toLongLong();

  p->setFont(f.marker.font);

  if (bType == BlockType::Commercial) {
    const int lineX = r.left() + 7;
    const int centerY = r.center().y();
    const int tickLength = 14;

    p->setPen(kPenLine);
    p->drawLine(lineX, r.top(), lineX, centerY);
    p->drawLine(lineX, centerY, lineX + tickLength, centerY);

    p->setPen(kColMuted);
    const int textLeft = lineX + tickLength + 8;
    p->drawText(QRect(textLeft, r.top(), 220, r.height()),
                Qt::AlignVCenter | Qt::AlignLeft,
                QStringLiteral("BLOCO COMERCIAL"));
  }

  const QString durFormatted = (totalDurationMs >= 3600000)
                                   ? formatDuration(totalDurationMs, true)
                                   : msec2string(totalDurationMs);
  const QString summary = QString::number(itemsCount) +
                          (itemsCount == 1 ? QStringLiteral(" item - ")
                                           : QStringLiteral(" itens - ")) +
                          durFormatted;

  const int sumW = f.marker.fm.horizontalAdvance(summary);
  p->setPen(kColMuted);
  p->drawText(QRect(r.right() - sumW - 16, r.top(), sumW, r.height()),
              Qt::AlignVCenter | Qt::AlignRight, summary);
}

void PlaylistItemDelegate::paintMediaItem(QPainter *p,
                                          const QStyleOptionViewItem &opt,
                                          const QModelIndex &index,
                                          const Fonts &f) const {
  const QRect r = opt.rect;

  const PlaylistItemStatus status =
      static_cast<PlaylistItemStatus>(index.data(StatusRole).toInt());
  const MediaType mType =
      static_cast<MediaType>(index.data(MediaTypeRole).toInt());
  const BlockType bType =
      static_cast<BlockType>(index.data(BlockTypeRole).toInt());
  const QString title = index.data(TitleRole).toString();

  if (bType == BlockType::Commercial) {
    const int lineX = r.left() + 7;
    p->setPen(kPenLine);
    p->drawLine(lineX, r.top(), lineX, r.bottom());
  }

  const bool isPlaying = (status == PlaylistItemStatus::Playing);
  const bool isNext = (status == PlaylistItemStatus::Next);
  const bool isSelected = opt.state & QStyle::State_Selected;

  if (status == PlaylistItemStatus::Finished) {
    const int iconLeft = r.left() + 22;

    if (isSelected) {
      p->save();
      p->setRenderHint(QPainter::Antialiasing, true);
      p->setPen(Qt::NoPen);
      p->setBrush(kColFinishedSel);
      p->drawRoundedRect(QRectF(r.adjusted(14, 0, -14, 0)), 7, 7);
      p->restore();
    }

    const QRect iconRect(iconLeft, r.top() + (r.height() - 20) / 2, 20, 20);
    drawTypeIcon(p, iconRect, mType);

    const int textLeft = iconRect.right() + 10;
    const int textWidth = r.right() - 16 - textLeft;
    if (textWidth > 20) {
      p->setFont(f.mediaFinished.font);
      p->setPen(kColWhite);
      p->drawText(
          QRect(textLeft, r.top(), textWidth, r.height()),
          Qt::AlignVCenter | Qt::AlignLeft,
          f.mediaFinished.fm.elidedText(title, Qt::ElideRight, textWidth));
    }
    return;
  }

  const QString estTime = index.data(EstimatedTimeRole).toString();
  const qint64 durationMs = index.data(DurationRole).toLongLong();
  const qint64 positionMs =
      isPlaying ? index.data(PositionRole).toLongLong() : 0;

  const QRect cardRect = r.adjusted(14, 1, -14, -1);

  p->setRenderHint(QPainter::Antialiasing, true);
  QPainterPath cardPath;
  cardPath.addRoundedRect(QRectF(cardRect), 7, 7);

  p->fillPath(cardPath, brushForType(mType));

  if (isPlaying && durationMs > 0) {
    const qreal ratio =
        qBound(0.0, static_cast<qreal>(positionMs) / durationMs, 1.0);
    const int progWidth = static_cast<int>(cardRect.width() * ratio);
    if (progWidth > 0) {
      // Clip the progress fill to the rounded card shape.
      p->save();
      p->setClipPath(cardPath);
      p->fillRect(
          QRect(cardRect.left(), cardRect.top(), progWidth, cardRect.height()),
          m_brushProgress);
      p->restore();
    }
  }

  if (isSelected) {
    p->setPen(kPenSelection);
    p->setBrush(Qt::NoBrush);
    p->drawPath(cardPath);
  }
  p->setRenderHint(QPainter::Antialiasing, false);

  const QRect iconRect(cardRect.left() + 10,
                       cardRect.top() + (cardRect.height() - 24) / 2, 24, 24);
  drawTypeIcon(p, iconRect, mType);

  const QString posStr =
      isPlaying ? msec2string(positionMs) : QStringLiteral("00:00:000");
  const QString durStr = msec2string(durationMs);

  const int timeColW = qMax(f.mediaPos.fm.horizontalAdvance(posStr),
                            f.mediaDur.fm.horizontalAdvance(durStr));
  const int timeRightX = cardRect.right() - 12;

  const QRect posRect(timeRightX - timeColW, cardRect.top() + 5, timeColW, 18);
  const QRect durRect(timeRightX - timeColW, cardRect.top() + 24, timeColW, 16);

  p->setFont(f.mediaPos.font);
  p->setPen(kColWhite);
  p->drawText(posRect, Qt::AlignVCenter | Qt::AlignRight, posStr);

  p->setFont(f.mediaDur.font);
  p->setPen(kColWhiteDim);
  p->drawText(durRect, Qt::AlignVCenter | Qt::AlignRight, durStr);

  int rightBoundary = timeRightX - timeColW - 12;

  if (isPlaying || isNext) {
    const QString sbText =
        isPlaying ? QStringLiteral("NO AR") : QStringLiteral("PRÓXIMO");

    const int sbW = f.badge.fm.horizontalAdvance(sbText) + 12;
    const int sbH = 16;
    const int sbX = (timeRightX - timeColW) - 10 - sbW;
    const int sbY = posRect.top() + (posRect.height() - sbH) / 2;
    const QRect sbRect(sbX, sbY, sbW, sbH);

    p->setRenderHint(QPainter::Antialiasing, true);
    QPainterPath sbPath;
    sbPath.addRoundedRect(QRectF(sbRect), 3.5, 3.5);
    p->fillPath(sbPath, isPlaying ? kColBadgePlayingBg : kColBadgeNextBg);
    p->setRenderHint(QPainter::Antialiasing, false);

    p->setFont(f.badge.font);
    p->setPen(isPlaying ? kColWhite : kColBlack);
    p->drawText(sbRect, Qt::AlignCenter, sbText);

    rightBoundary = sbX - 10;
  }

  int currentX = iconRect.right() + 10;
  const int maxCenterW = rightBoundary - currentX;

  if (maxCenterW > 30) {
    p->setFont(f.mediaTitle.font);
    p->setPen(kColWhite);
    p->drawText(QRect(currentX, cardRect.top() + 5, maxCenterW, 18),
                Qt::AlignVCenter | Qt::AlignLeft,
                f.mediaTitle.fm.elidedText(title, Qt::ElideRight, maxCenterW));

    const int bottomY = cardRect.top() + 24;

    const int clockSize = 11;
    const int clockY = bottomY + (16 - clockSize) / 2;

    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(kPenClock);
    p->setBrush(Qt::NoBrush);
    p->drawEllipse(currentX, clockY, clockSize, clockSize);

    const QPointF center(currentX + clockSize / 2.0, clockY + clockSize / 2.0);
    p->drawLine(center, center + QPointF(0, -3.0));
    p->drawLine(center, center + QPointF(2.2, 0));
    p->setRenderHint(QPainter::Antialiasing, false);

    currentX += clockSize + 5;

    if (!estTime.isEmpty()) {
      p->setFont(f.est.font);
      p->setPen(kColWhite);
      const int estW = f.est.fm.horizontalAdvance(estTime);
      p->drawText(QRect(currentX, bottomY, estW, 16),
                  Qt::AlignVCenter | Qt::AlignLeft, estTime);
    }
  }
}
