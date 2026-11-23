#include "PlaylistView.h"
#include "core/catalog/CatalogItem.h"
#include "core/models.h"
#include "core/playlist/PlaylistListModel.h"

#include <QAction>
#include <QDataStream>
#include <QIODevice>
#include <QKeySequence>
#include <QMenu>
#include <QMimeData>
#include <QPainter>

namespace {
const QString &catalogMime() {
  static const QString kMime = QStringLiteral("application/x-catalog-item");
  return kMime;
}
} // namespace

PlaylistView::PlaylistView(QWidget *parent) : QListView(parent) {
  setSelectionMode(QAbstractItemView::SingleSelection);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(false);
  setDragDropMode(QAbstractItemView::DragDrop);
  setDefaultDropAction(Qt::MoveAction);
  setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  setAutoScroll(true);
  setAutoScrollMargin(30);
  setUniformItemSizes(false);
  setLayoutMode(QListView::SinglePass);
}

PlaylistView::~PlaylistView() = default;

void PlaylistView::dataChanged(const QModelIndex &topLeft,
                               const QModelIndex &bottomRight,
                               const QList<int> &roles) {
  QListView::dataChanged(topLeft, bottomRight, roles);

  // Fast path: position updates during playback do not change item heights.
  if (roles.size() == 1 && roles.constFirst() == PositionRole) {
    return;
  }

  if (roles.isEmpty() || roles.contains(StatusRole) ||
      roles.contains(Qt::SizeHintRole)) {
    scheduleDelayedItemsLayout();
    viewport()->update();
  }
}

bool PlaylistView::removeSelectedItem() {
  PlaylistListModel *plistModel = qobject_cast<PlaylistListModel *>(model());
  const QModelIndex idx = currentIndex();
  if (!plistModel || !idx.isValid()) {
    return false;
  }

  const int row = idx.row();
  if (!plistModel->removeItem(row)) {
    return false;
  }

  if (plistModel->rowCount() > 0) {
    setCurrentIndex(
        plistModel->index(qMin(row, plistModel->rowCount() - 1), 0));
  }
  return true;
}

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
  if (!item || !item->isEntry()) {
    return;
  }

  const int lockedRow = plistModel->lastLockedRow();
  if (lockedRow >= 0 && index.row() <= lockedRow) {
    return;
  }

  QListView::startDrag(supportedActions);
}

void PlaylistView::dragEnterEvent(QDragEnterEvent *event) {
  if (event->source() == this) {
    event->setDropAction(Qt::MoveAction);
    event->accept();
  } else if (event->mimeData() && event->mimeData()->hasFormat(catalogMime())) {
    event->setDropAction(Qt::CopyAction);
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
  const bool internal = (event->source() == this);
  const bool fromCatalog =
      !internal && event->mimeData() &&
      event->mimeData()->hasFormat(catalogMime());

  if (!internal && !fromCatalog) {
    setDropIndicatorRow(-1);
    event->ignore();
    return;
  }

  const int targetRow = calculateDropRow(event->position().toPoint());
  PlaylistListModel *plistModel = qobject_cast<PlaylistListModel *>(model());
  if (!plistModel) {
    setDropIndicatorRow(-1);
    event->ignore();
    return;
  }

  const int lockedRow = plistModel->lastLockedRow();
  if ((lockedRow >= 0 && targetRow <= lockedRow) ||
      !plistModel->isDropRowBetweenMarkers(targetRow)) {
    setDropIndicatorRow(-1);
    event->ignore();
    return;
  }

  setDropIndicatorRow(targetRow);
  event->setDropAction(internal ? Qt::MoveAction : Qt::CopyAction);
  event->accept();
}

void PlaylistView::dragLeaveEvent(QDragLeaveEvent *event) {
  m_dropIndicatorRow = -1;
  viewport()->update();
  QListView::dragLeaveEvent(event);
}

void PlaylistView::dropEvent(QDropEvent *event) {
  const int targetRow = calculateDropRow(event->position().toPoint());
  setDropIndicatorRow(-1);

  PlaylistListModel *plistModel = qobject_cast<PlaylistListModel *>(model());
  if (!plistModel) {
    event->ignore();
    return;
  }

  const int lockedRow = plistModel->lastLockedRow();

  if (event->source() == this) {
    QModelIndexList selected = selectedIndexes();
    if (selected.isEmpty()) {
      event->ignore();
      return;
    }

    const int sourceRow = selected.first().row();
    if ((lockedRow >= 0 &&
         (sourceRow <= lockedRow || targetRow <= lockedRow)) ||
        !plistModel->isDropRowBetweenMarkers(targetRow)) {
      event->ignore();
      return;
    }

    int finalDest = targetRow;
    if (targetRow > sourceRow) {
      finalDest = targetRow - 1;
    }

    if (finalDest != sourceRow && plistModel->moveItem(sourceRow, finalDest)) {
      event->setDropAction(Qt::MoveAction);
      event->accept();
      setCurrentIndex(plistModel->index(finalDest, 0));
      return;
    }
    event->ignore();
    return;
  }

  if (event->mimeData() && event->mimeData()->hasFormat(catalogMime())) {
    if ((lockedRow >= 0 && targetRow <= lockedRow) ||
        !plistModel->isDropRowBetweenMarkers(targetRow)) {
      event->ignore();
      return;
    }

    QByteArray data = event->mimeData()->data(catalogMime());
    QDataStream stream(&data, QIODevice::ReadOnly);
    QString title;
    QString path;
    qint8 rawMediaType = 0;
    qint64 durationMs = 0;
    qint8 rawCatalogItemType = static_cast<qint8>(CatalogItemType::File);
    qint8 rawCommandType = static_cast<qint8>(CommandType::None);
    QString commandId;

    stream >> title >> path >> rawMediaType;
    if (stream.status() == QDataStream::Ok && !stream.atEnd()) {
      stream >> durationMs;
    }
    if (stream.status() == QDataStream::Ok && !stream.atEnd()) {
      stream >> rawCatalogItemType >> rawCommandType >> commandId;
    }

    if (stream.status() != QDataStream::Ok ||
        (title.isEmpty() && path.isEmpty() && commandId.isEmpty())) {
      event->ignore();
      return;
    }

    if (rawMediaType == static_cast<qint8>(MediaType::Command) ||
        rawCatalogItemType == static_cast<qint8>(CatalogItemType::Command)) {
      CommandType cmdType = static_cast<CommandType>(rawCommandType);
      if (cmdType == CommandType::None) {
        cmdType = commandTypeFromString(commandId);
      }
      if (plistModel->insertCommandItem(targetRow, title, cmdType, commandId)) {
        setCurrentIndex(plistModel->index(targetRow, 0));
        event->setDropAction(Qt::CopyAction);
        event->accept();
        return;
      }
      event->ignore();
      return;
    }

    const MediaType mediaType =
        (rawMediaType >= static_cast<qint8>(MediaType::Jingle) &&
         rawMediaType <= static_cast<qint8>(MediaType::Other))
            ? static_cast<MediaType>(rawMediaType)
            : MediaType::Other;

    if (plistModel->insertCatalogItem(targetRow, title, path, mediaType,
                                      qMax<qint64>(0, durationMs))) {
      setCurrentIndex(plistModel->index(targetRow, 0));
      event->setDropAction(Qt::CopyAction);
      event->accept();
      return;
    }
  }

  event->ignore();
}

void PlaylistView::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
    removeSelectedItem();
    event->accept();
    return;
  }
  QListView::keyPressEvent(event);
}

QMenu *PlaylistView::createContextMenu(const QModelIndex &index,
                                       QWidget *parent) {
  PlaylistListModel *plistModel = qobject_cast<PlaylistListModel *>(model());
  if (!plistModel || !index.isValid()) {
    return nullptr;
  }

  const PlaylistItem *item = plistModel->itemAt(index.row());
  if (!item || !item->isEntry()) {
    return nullptr;
  }

  static const QString kMenuStyle = QStringLiteral(
      "QMenu {"
      "  background-color: #1F232C;"
      "  color: #FFFFFF;"
      "  border: 1px solid #2D3340;"
      "  border-radius: 6px;"
      "  padding: 4px;"
      "}"
      "QMenu::item {"
      "  padding: 6px 14px;"
      "  border-radius: 4px;"
      "}"
      "QMenu::item:selected {"
      "  background-color: #3E4A5E;"
      "}"
      "QMenu::item:disabled {"
      "  color: #8E9EB5;"
      "}");
  auto *menu = new QMenu(parent ? parent : this);
  menu->setStyleSheet(kMenuStyle);

  if (item->type == ItemType::Media) {
    QAction *mixEditorAction = menu->addAction(tr("Abrir no Editor de Mixagem"));
    mixEditorAction->setEnabled(item->status != PlaylistItemStatus::Finished &&
                                item->status != PlaylistItemStatus::Mixing);
    const QUuid itemUuid = item->uuid;
    connect(mixEditorAction, &QAction::triggered, this,
            [this, itemUuid]() { emit mixEditorRequested(itemUuid); });

    menu->addSeparator();
  }

  QAction *removeAction = menu->addAction(tr("Remover"));
  removeAction->setShortcut(QKeySequence::Delete);
  removeAction->setEnabled(plistModel->canRemoveRow(index.row()));
  connect(removeAction, &QAction::triggered, this,
          [this]() { removeSelectedItem(); });

  return menu;
}

void PlaylistView::contextMenuEvent(QContextMenuEvent *event) {
  const QModelIndex index = indexAt(event->pos());
  QMenu *menu = createContextMenu(index, this);
  if (!menu) {
    return;
  }

  setCurrentIndex(index);
  menu->exec(event->globalPos());
  menu->deleteLater();
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
