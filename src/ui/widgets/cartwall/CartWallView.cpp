#include "CartWallView.h"
#include "core/cartwall/CartItem.h"
#include "core/cartwall/CartWallTableModel.h"

#include <QApplication>
#include <QDataStream>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QHeaderView>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>

namespace {
const char *kCatalogMime = "application/x-catalog-item";
}

CartWallView::CartWallView(QWidget *parent) : QTableView(parent) {
  horizontalHeader()->hide();
  verticalHeader()->hide();
  horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  horizontalHeader()->setMinimumSectionSize(1);
  verticalHeader()->setMinimumSectionSize(1);

  setShowGrid(false);
  setFrameShape(QFrame::NoFrame);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setSelectionBehavior(QAbstractItemView::SelectItems);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  setMouseTracking(true);
  setFocusPolicy(Qt::NoFocus);

  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(false);
  setDragDropMode(QAbstractItemView::DragDrop);
  setDefaultDropAction(Qt::MoveAction);
  setAutoScroll(true);
  setAutoScrollMargin(30);
}

CartWallView::~CartWallView() = default;

CartWallTableModel *CartWallView::cartModel() const {
  return qobject_cast<CartWallTableModel *>(model());
}

void CartWallView::setModel(QAbstractItemModel *model) {
  if (this->model()) {
    disconnect(this->model(), nullptr, this, nullptr);
  }
  QTableView::setModel(model);
  if (model) {
    connect(model, &QAbstractItemModel::modelReset, this,
            &CartWallView::updateCellSizes);
    connect(model, &QAbstractItemModel::rowsInserted, this,
            &CartWallView::updateCellSizes);
  }
  updateCellSizes();
}

int CartWallView::columnCountSetting() const {
  return model() ? model()->columnCount() : 0;
}

void CartWallView::setCellHeight(int height) {
  if (height <= 0 || height == m_rowHeight)
    return;
  m_rowHeight = height;
  updateCellSizes();
}

void CartWallView::setColumnCount(int columns) {
  // A running drag must not outlive a layout change.
  m_dropIndicatorSlot = -1;
  m_pressedIndex = QModelIndex();
  if (CartWallTableModel *m = cartModel()) {
    m->setColumnCount(columns);
  }
  updateCellSizes();
}

void CartWallView::resizeEvent(QResizeEvent *event) {
  QTableView::resizeEvent(event);
  updateCellSizes();
}

void CartWallView::updateCellSizes() {
  if (!model())
    return;
  const int cols = model()->columnCount();
  if (cols <= 0)
    return;

  // Reserve room for the vertical scrollbar if it's visible, so the columns
  // never overflow horizontally.
  const int available = viewport()->width();
  const int cellW = qMax(1, available / cols);
  const int cellH = m_rowHeight;

  for (int c = 0; c < cols; ++c)
    horizontalHeader()->resizeSection(c, cellW);
  verticalHeader()->setDefaultSectionSize(cellH);
  for (int r = 0; r < model()->rowCount(); ++r)
    verticalHeader()->resizeSection(r, cellH);
}

void CartWallView::mousePressEvent(QMouseEvent *event) {
  m_pressedIndex = (event->button() == Qt::LeftButton)
                       ? indexAt(event->position().toPoint())
                       : QModelIndex();
  m_pressPos = event->position().toPoint();
  QTableView::mousePressEvent(event);
}

void CartWallView::mouseMoveEvent(QMouseEvent *event) {
  QTableView::mouseMoveEvent(event);

  // Pointer cursor only while hovering an actual item (not empty cells).
  const QModelIndex idx = indexAt(event->position().toPoint());
  const bool overItem = idx.isValid() && (idx.flags() & Qt::ItemIsEnabled);
  viewport()->setCursor(overItem ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void CartWallView::leaveEvent(QEvent *event) {
  viewport()->unsetCursor();
  QTableView::leaveEvent(event);
}

void CartWallView::mouseReleaseEvent(QMouseEvent *event) {
  QTableView::mouseReleaseEvent(event);

  if (event->button() != Qt::LeftButton || !m_pressedIndex.isValid())
    return;

  const QPoint pos = event->position().toPoint();
  const bool moved =
      (pos - m_pressPos).manhattanLength() >= QApplication::startDragDistance();
  const QModelIndex released = indexAt(pos);
  if (!moved && released == m_pressedIndex) {
    if (CartWallTableModel *m = cartModel()) {
      const int linear = m->linearIndex(released);
      if (linear >= 0)
        emit slotTriggered(linear);
    }
  }
  m_pressedIndex = QModelIndex();
}

int CartWallView::calculateDropSlot(const QPoint &pos) const {
  CartWallTableModel *m = cartModel();
  if (!m)
    return 0;

  const QModelIndex idx = indexAt(pos);
  const int linear = m->linearIndex(idx);
  if (linear < 0)
    return m->itemCount(); // empty area: append

  const QRect rect = visualRect(idx);
  return (pos.x() > rect.center().x()) ? linear + 1 : linear;
}

void CartWallView::setDropIndicatorSlot(int slot) {
  if (m_dropIndicatorSlot == slot)
    return;
  m_dropIndicatorSlot = slot;
  viewport()->update();
}

void CartWallView::startDrag(Qt::DropActions supportedActions) {
  if (!currentIndex().isValid())
    return;
  CartWallTableModel *m = cartModel();
  if (!m || m->linearIndex(currentIndex()) < 0)
    return;
  QTableView::startDrag(supportedActions);
}

void CartWallView::dragEnterEvent(QDragEnterEvent *event) {
  if (event->source() == this) {
    event->setDropAction(Qt::MoveAction);
    event->accept();
  } else if (event->mimeData()->hasFormat(kCatalogMime)) {
    event->setDropAction(Qt::CopyAction);
    event->accept();
  } else {
    event->ignore();
  }
}

void CartWallView::dragMoveEvent(QDragMoveEvent *event) {
  const bool internal = event->source() == this;
  if (!internal && !event->mimeData()->hasFormat(kCatalogMime)) {
    setDropIndicatorSlot(-1);
    event->ignore();
    return;
  }

  setDropIndicatorSlot(calculateDropSlot(event->position().toPoint()));
  event->setDropAction(internal ? Qt::MoveAction : Qt::CopyAction);
  event->accept();
}

void CartWallView::dragLeaveEvent(QDragLeaveEvent *event) {
  setDropIndicatorSlot(-1);
  QTableView::dragLeaveEvent(event);
}

void CartWallView::dropEvent(QDropEvent *event) {
  CartWallTableModel *m = cartModel();
  const int slot = calculateDropSlot(event->position().toPoint());
  setDropIndicatorSlot(-1);
  if (!m) {
    event->ignore();
    return;
  }

  if (event->source() == this) {
    const int source = m->linearIndex(currentIndex());
    if (source < 0) {
      event->ignore();
      return;
    }
    int dest = (slot > source) ? slot - 1 : slot;
    dest = qBound(0, dest, m->itemCount() - 1);
    if (dest != source && m->moveItem(source, dest)) {
      setCurrentIndex(m->indexFromLinear(dest));
      event->setDropAction(Qt::MoveAction);
      event->accept();
      return;
    }
    event->ignore();
    return;
  }

  if (event->mimeData()->hasFormat(kCatalogMime)) {
    QByteArray data = event->mimeData()->data(kCatalogMime);
    QDataStream stream(&data, QIODevice::ReadOnly);
    QString title, path;
    qint8 mediaType = 0;
    stream >> title >> path >> mediaType;

    CartItem item;
    item.title = title;
    item.path = path;
    item.mediaType = static_cast<MediaType>(mediaType);
    m->insertItem(slot, item);
    setCurrentIndex(m->indexFromLinear(qMin(slot, m->itemCount() - 1)));
    event->setDropAction(Qt::CopyAction);
    event->accept();
    return;
  }

  event->ignore();
}

void CartWallView::paintEvent(QPaintEvent *event) {
  QTableView::paintEvent(event);

  CartWallTableModel *m = cartModel();
  if (m_dropIndicatorSlot < 0 || !m || m->itemCount() == 0)
    return;

  int x = 0;
  QRect cell;
  if (m_dropIndicatorSlot < m->itemCount()) {
    cell = visualRect(m->indexFromLinear(m_dropIndicatorSlot));
    x = cell.left();
  } else {
    cell = visualRect(m->indexFromLinear(m->itemCount() - 1));
    x = cell.right() + 1;
  }
  if (cell.isNull())
    return;

  QPainter p(viewport());
  p.setRenderHint(QPainter::Antialiasing, true);
  const QColor indicatorColor("#eab308");
  const int top = cell.top() + 6;
  const int bottom = cell.bottom() - 6;
  p.setPen(QPen(indicatorColor, 3, Qt::SolidLine, Qt::RoundCap));
  p.drawLine(x, top, x, bottom);
  p.setBrush(indicatorColor);
  p.setPen(Qt::NoPen);
  p.drawEllipse(QPoint(x, top), 3, 3);
  p.drawEllipse(QPoint(x, bottom), 3, 3);
}
