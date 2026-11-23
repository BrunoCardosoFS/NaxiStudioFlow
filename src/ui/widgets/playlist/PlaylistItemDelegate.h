#ifndef PLAYLISTITEMDELEGATE_H
#define PLAYLISTITEMDELEGATE_H

#pragma once

#include <QBrush>
#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QPainter>
#include <QStyledItemDelegate>

#include <memory>

class PlaylistListModel;
enum class CommandType : qint8;
enum class MediaType : qint8;

class PlaylistItemDelegate : public QStyledItemDelegate {
  Q_OBJECT
public:
  explicit PlaylistItemDelegate(QObject *parent = nullptr);
  ~PlaylistItemDelegate() override;

  QSize sizeHint(const QStyleOptionViewItem &option,
                 const QModelIndex &index) const override;
  void paint(QPainter *p, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;

private:
  struct FontSet {
    QFont font;
    QFontMetrics fm;
    FontSet(const QFont &base, int pointSize, bool bold);
  };
  struct Fonts {
    explicit Fonts(const QFont &base);
    FontSet blockTime, blockName, marker, mediaFinished, mediaTitle, mediaPos,
        mediaDur, badge, est;
    QString badgePlayingText;
    QString badgeMixingText;
    QString badgeNextText;
    int timeColWidth = 0;
    int badgePlayingWidth = 0;
    int badgeMixingWidth = 0;
    int badgeNextWidth = 0;
    int estTimeWidth = 0;
  };
  const Fonts &fonts(const QFont &base) const;

  bool isFirstInBlock(const QModelIndex &index, bool isBlockMarker) const;
  bool isLastInBlock(const QModelIndex &index,
                     const PlaylistListModel *plistModel) const;
  void drawBlockBackground(QPainter *p, const QRect &blockRect, bool isFirst,
                           bool isLast) const;

  void paintBlockMarker(QPainter *p, const QStyleOptionViewItem &opt,
                        const QModelIndex &index, const Fonts &f) const;
  void paintStartMarker(QPainter *p, const QStyleOptionViewItem &opt,
                        const QModelIndex &index, const Fonts &f) const;
  void paintEndMarker(QPainter *p, const QStyleOptionViewItem &opt,
                      const QModelIndex &index, const Fonts &f) const;
  void paintMediaItem(QPainter *p, const QStyleOptionViewItem &opt,
                      const QModelIndex &index, const Fonts &f,
                      const PlaylistListModel *plistModel) const;
  void paintCommandItem(QPainter *p, const QStyleOptionViewItem &opt,
                        const QModelIndex &index, const Fonts &f,
                        const PlaylistListModel *plistModel) const;

  const QIcon &iconForType(MediaType type) const;
  const QIcon &iconForCommand(CommandType commandType) const;
  void drawTypeIcon(QPainter *p, const QRect &rect, MediaType type) const;
  const QBrush &brushForType(MediaType type) const;

  QIcon m_iconMusic;
  QIcon m_iconJingle;
  QIcon m_iconCommercial;
  QIcon m_iconOther;
  QIcon m_iconCommand;
  QIcon m_iconCommandPause;

  // ObjectBoundingMode gradients: independent of the item rect.
  QBrush m_brushMusic;
  QBrush m_brushJingle;
  QBrush m_brushCommercial;
  QBrush m_brushOther;
  QBrush m_brushCommand;
  QBrush m_brushProgress;

  mutable QFont m_cachedBaseFont;
  mutable std::unique_ptr<Fonts> m_fonts;
};

#endif // PLAYLISTITEMDELEGATE_H
