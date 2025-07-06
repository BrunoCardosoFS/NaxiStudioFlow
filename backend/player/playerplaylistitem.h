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
    // void setFade(qint64 fadeIn, qint64 fadeOut);
    void setFade(float introLevel, float duckingLevel, qint64 fadeInEndPoint, qint64 mainRampUpStartPoint, qint64 peakStartPoin, qint64 duckingStartPoint, qint64 duckingEndPoint, qint64 fadeOutStartPoint);

    QMediaPlayer *MediaPlayer = new QMediaPlayer(this);

    float introLevel = 0.0;
    float duckingLevel = 0.0;

    void updateCoefficients();

    qint64 fadeInEndPoint = 0;
    qint64 mainRampUpStartPoint = 0;
    qint64 peakStartPoin = 500;
    qint64 duckingStartPoint = 500;
    qint64 duckingEndPoint = 0;
    qint64 fadeOutStartPoint = 1;

    qint64 duckingStartPosition;
    qint64 duckingEndPosition;
    qint64 fadeOutStartPosition;

    qreal fadeInSlope;

    qreal mainRampUpSlope;
    qreal mainRampUpIntercept;

    qreal duckingRampDownSlope;
    qreal duckingRampDownIntercept;

    qreal fadeOutSlope;
    qreal fadeOutIntercept;


    qint64 fadeIn = 500;
    qint64 fadeOut = 500;
    qint64 duration;

private:
    QAudioOutput *AudioOutput = new QAudioOutput(this->MediaPlayer);

    void volumeControl(qint64 position);
signals:


};

#endif // PLAYERPLAYLISTITEM_H
