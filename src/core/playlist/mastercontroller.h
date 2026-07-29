#ifndef MASTERCONTROLLER_H
#define MASTERCONTROLLER_H

#include <QObject>
#include <QTimer>

#include "../player/playerplaylistitem.h"

class MasterController : public QObject
{
    Q_OBJECT
public:
    explicit MasterController(QObject *parent = nullptr);

private:
    QTimer *m_timer = new QTimer(this);

    PlayerPlaylistItem *m_deck1 = new PlayerPlaylistItem(this);
    PlayerPlaylistItem *m_deck2 = new PlayerPlaylistItem(this);
    PlayerPlaylistItem *m_deck3 = new PlayerPlaylistItem(this);

private slots:
    void onTick();

signals:
};

#endif // MASTERCONTROLLER_H
