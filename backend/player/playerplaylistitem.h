#ifndef PLAYERPLAYLISTITEM_H
#define PLAYERPLAYLISTITEM_H

#include <QObject>

#include <QMediaPlayer>
#include <QAudioOutput>

class PlayerPlaylistItem : public QObject{
    Q_OBJECT

public:
    explicit PlayerPlaylistItem(QObject *parent = nullptr);

    void setSourceFromPath(QString path);
    void setFade(qint64 fadeIn, qint64 fadeOut);

    QMediaPlayer *MediaPlayer = new QMediaPlayer(this);

    qint64 fadeIn = 500;
    qint64 fadeOut = 500;
    qint64 duration;

private:
    QAudioOutput *AudioOutput = new QAudioOutput(this->MediaPlayer);

    void volumeControl(qint64 position);

signals:


};

#endif // PLAYERPLAYLISTITEM_H
