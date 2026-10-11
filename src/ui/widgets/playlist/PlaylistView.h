#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#pragma once

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QListView>

class PlaylistView : public QListView {
  Q_OBJECT
public:
  explicit PlaylistView(QWidget *parent = nullptr);
  ~PlaylistView() override;

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dragLeaveEvent(QDragLeaveEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  void startDrag(Qt::DropActions supportedActions) override;
  void paintEvent(QPaintEvent *event) override;

private:
  int calculateDropRow(const QPoint &pos) const;
  void setDropIndicatorRow(int row);
  int m_dropIndicatorRow = -1;
};

#endif // PLAYLISTVIEW_H
