#ifndef PLAYLISTCONTOLLER_H
#define PLAYLISTCONTOLLER_H

#include <QObject>
#include <QList>
#include "../../frontend/widgets/playlist/playlistitem.h"

class PlaylistContoller : public QObject{
    Q_OBJECT
public:
    explicit PlaylistContoller(QObject *parent = nullptr);

    void onPositionChanged(qint64 position);
    void play();
    void pause();
    void stop();
    void toNext();

    QList<PlaylistItem*> playlist;

    int currentIndex = -1;

private:


signals:
};

#endif // PLAYLISTCONTOLLER_H
