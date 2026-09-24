#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QHash>
#include <QTimer>
#include <QVector>

#include "core/models.h"
#include "core/playlist/player/PlayerManager.h"

class PlaylistManager : public QObject {
  Q_OBJECT
public:
  explicit PlaylistManager(QObject *parent = nullptr);

  void play();
  void pause();
  void next();
  void stop();

  QVector<PlaylistItem> items;
  QHash<QUuid, int> uuidRowMap;

signals:
  void removeFirstItem();

private:
  PlaylistStatus status = PlaylistStatus::Stopped;
  int activePlayer = -1;

  QList<QAudioOutput *> outputs = {
      new QAudioOutput(this), new QAudioOutput(this), new QAudioOutput(this)};
  QList<PlayerManager *> players = {new PlayerManager(this),
                                    new PlayerManager(this),
                                    new PlayerManager(this)};

  QTimer *timer = new QTimer(this);

private slots:
  void timeout();
};

#endif // PLAYLISTMANAGER_H