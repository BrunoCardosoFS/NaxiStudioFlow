#ifndef CARTWALLVIEW_H
#define CARTWALLVIEW_H

#include <QModelIndex>
#include <QPoint>
#include <QTableView>

class CartWallTableModel;

class CartWallView : public QTableView {
  Q_OBJECT
public:
  explicit CartWallView(QWidget *parent = nullptr);
  ~CartWallView() override;

  void setModel(QAbstractItemModel *model) override;

  int columnCountSetting() const;
  // Fixed height (px) of every row. Width follows the column count.
  void setCellHeight(int height);
  int cellHeight() const { return m_rowHeight; }

public slots:
  void setColumnCount(int columns);

signals:
  // Linear index (row * columns + col) of the clicked slot.
  void slotTriggered(int linearIndex);

protected:
  void resizeEvent(QResizeEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void startDrag(Qt::DropActions supportedActions) override;
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dragLeaveEvent(QDragLeaveEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

private:
  CartWallTableModel *cartModel() const;
  void updateCellSizes();
  // Linear insertion slot (0..itemCount) for a viewport position.
  int calculateDropSlot(const QPoint &pos) const;
  void setDropIndicatorSlot(int slot);

  int m_rowHeight = 90;
  int m_dropIndicatorSlot = -1;
  QModelIndex m_pressedIndex;
  QPoint m_pressPos;
};

#endif // CARTWALLVIEW_H
