#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#pragma once

#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QListView>
#include <QUuid>

class QMenu;

class PlaylistView : public QListView {
  Q_OBJECT
public:
  explicit PlaylistView(QWidget *parent = nullptr);
  ~PlaylistView() override;

  bool removeSelectedItem();
  QMenu *createContextMenu(const QModelIndex &index, QWidget *parent = nullptr);

signals:
  void mixEditorRequested(const QUuid &itemUuid);

protected slots:
  void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                   const QList<int> &roles = QList<int>()) override;

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dragLeaveEvent(QDragLeaveEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  void startDrag(Qt::DropActions supportedActions) override;
  void keyPressEvent(QKeyEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

private:
  int calculateDropRow(const QPoint &pos) const;
  void setDropIndicatorRow(int row);
  int m_dropIndicatorRow = -1;
};

#endif // PLAYLISTVIEW_H
