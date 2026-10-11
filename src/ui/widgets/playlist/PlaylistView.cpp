#include "PlaylistView.h"
#include "core/playlist/PlaylistListModel.h"

#include <QPainter>

PlaylistView::PlaylistView(QWidget *parent) : QListView(parent) {
  setSelectionMode(QAbstractItemView::SingleSelection);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(false);
  setDragDropMode(QAbstractItemView::InternalMove);
  setDefaultDropAction(Qt::MoveAction);
  setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  setAutoScroll(true);
  setAutoScrollMargin(30);
  setUniformItemSizes(false);
  setLayoutMode(QListView::SinglePass);
}

PlaylistView::~PlaylistView() = default;

int PlaylistView::calculateDropRow(const QPoint &pos) const {
  QModelIndex idx = indexAt(pos);
  if (!idx.isValid()) {
    return model() ? model()->rowCount() : 0;
  }

  QRect rect = visualRect(idx);
  if (pos.y() > rect.center().y()) {
    return idx.row() + 1;
  }
  return idx.row();
}

void PlaylistView::startDrag(Qt::DropActions supportedActions) {
  QModelIndex index = currentIndex();
  if (!index.isValid())
    return;

  PlaylistListModel *plistModel = qobject_cast<PlaylistListModel *>(model());
  if (!plistModel) {
    QListView::startDrag(supportedActions);
    return;
  }

  const PlaylistItem *item = plistModel->itemAt(index.row());
  if (!item || item->type != ItemType::Media) {
    return;
  }

  int playingRow = plistModel->currentPlayingRow();
  if (playingRow >= 0 && index.row() <= playingRow) {
    return;
  }

  QListView::startDrag(supportedActions);
}

void PlaylistView::dragEnterEvent(QDragEnterEvent *event) {
  if (event->source() == this) {
    event->setDropAction(Qt::MoveAction);
    event->accept();
  } else {
    event->ignore();
  }
}

void PlaylistView::setDropIndicatorRow(int row) {
  if (m_dropIndicatorRow == row)
    return;
  m_dropIndicatorRow = row;
  viewport()->update();
}

void PlaylistView::dragMoveEvent(QDragMoveEvent *event) {
  if (event->source() == this) {
    int targetRow = calculateDropRow(event->position().toPoint());

    PlaylistListModel *plistModel = qobject_cast<PlaylistListModel *>(model());
    if (plistModel) {
      int playingRow = plistModel->currentPlayingRow();
      if (playingRow >= 0 && targetRow <= playingRow) {
        setDropIndicatorRow(-1);
        event->ignore();
        return;
      }

      if (!plistModel->isDropRowBetweenMarkers(targetRow)) {
        setDropIndicatorRow(-1);
        event->ignore();
        return;
      }
    }

    setDropIndicatorRow(targetRow);
    event->setDropAction(Qt::MoveAction);
    event->accept();
    return;
  }

  setDropIndicatorRow(-1);
  QListView::dragMoveEvent(event);
}

void PlaylistView::dragLeaveEvent(QDragLeaveEvent *event) {
  m_dropIndicatorRow = -1;
  viewport()->update();
  QListView::dragLeaveEvent(event);
}

void PlaylistView::dropEvent(QDropEvent *event) {
  int targetRow = calculateDropRow(event->position().toPoint());
  m_dropIndicatorRow = -1;
  viewport()->update();

  if (event->source() == this) {
    QModelIndexList selected = selectedIndexes();
    if (selected.isEmpty()) {
      event->ignore();
      return;
    }

    int sourceRow = selected.first().row();
    PlaylistListModel *plistModel = qobject_cast<PlaylistListModel *>(model());
    if (plistModel) {
      int playingRow = plistModel->currentPlayingRow();
      if (playingRow >= 0 &&
          (sourceRow <= playingRow || targetRow <= playingRow)) {
        event->ignore();
        return;
      }

      if (!plistModel->isDropRowBetweenMarkers(targetRow)) {
        event->ignore();
        return;
      }

      int finalDest = targetRow;
      if (targetRow > sourceRow) {
        finalDest = targetRow - 1;
      }

      if (finalDest != sourceRow &&
          plistModel->moveItem(sourceRow, finalDest)) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
        setCurrentIndex(plistModel->index(finalDest, 0));
        return;
      }
    }
    event->ignore();
    return;
  }

  QListView::dropEvent(event);
}

void PlaylistView::paintEvent(QPaintEvent *event) {
  QListView::paintEvent(event);

  if (m_dropIndicatorRow >= 0 && model()) {
    int row = m_dropIndicatorRow;
    int y = 0;
    if (row < model()->rowCount()) {
      QRect rect = visualRect(model()->index(row, 0));
      y = rect.top();
    } else if (model()->rowCount() > 0) {
      QRect rect = visualRect(model()->index(model()->rowCount() - 1, 0));
      y = rect.bottom() + 1;
    }

    QPainter p(viewport());
    p.setRenderHint(QPainter::Antialiasing, true);

    QColor indicatorColor("#eab308");
    p.setPen(QPen(indicatorColor, 2));
    p.drawLine(12, y, viewport()->width() - 12, y);

    p.setBrush(indicatorColor);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPoint(12, y), 3, 3);
    p.drawEllipse(QPoint(viewport()->width() - 12, y), 3, 3);
  }
}
