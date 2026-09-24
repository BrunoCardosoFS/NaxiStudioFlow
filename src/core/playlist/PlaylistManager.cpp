#include "./PlaylistManager.h"

PlaylistManager::PlaylistManager(QObject *parent) : QObject{parent} {
  connect(this->timer, &QTimer::timeout, this, &PlaylistManager::timeout);
  timer->start(100);
}

void PlaylistManager::timeout() {}