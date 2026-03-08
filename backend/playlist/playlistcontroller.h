#ifndef PLAYLISTCONTROLLER_H
#define PLAYLISTCONTROLLER_H

#include <QObject>
#include <QList>
#include "../../frontend/widgets/playlist/playlistitem.h"

class PlaylistController : public QObject{
    Q_OBJECT
public:
    static PlaylistController* getInstance(QObject *parent = nullptr);

    explicit PlaylistController(QObject *parent = nullptr);

    void play();
    void pause();
    void stop();
    void toNext();

    QList<PlaylistItem*> playlist;

    int currentIndex = -1;

private:

public slots:
    void onPositionChanged(qint64 position);

signals:
};

#endif // PLAYLISTCONTROLLER_H
