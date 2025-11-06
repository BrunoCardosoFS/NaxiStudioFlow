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
    void setFade(float startPoint, float endPoint, float preRampUpLevel, float postRampDownLevel, qint64 fadeInEndPoint, qint64 rampUpStartPoint, qint64 rampUpEndPoint, qint64 rampDownStartPoint, qint64 rampDownEndPoint, qint64 fadeOutStartPoint);

    QMediaPlayer *MediaPlayer = new QMediaPlayer(this);

    bool isLoaded = false;

    float preRampUpLevel = 0.0;
    float postRampDownLevel = 0.0;

    void updateCoefficients();

    qint64 startPoint;
    qint64 endPoint;

    qint64 fadeInEndPoint = 0;
    qint64 rampUpStartPoint = 0;
    qint64 rampUpEndPoint = 500;
    qint64 rampDownStartPoint;
    qint64 rampDownEndPoint;
    qint64 fadeOutStartPoint;

    qreal fadeInCoefficientA;

    qreal rampUpCoefficientA;
    qreal rampUpCoefficientB;

    qreal rampDownCoefficientA;
    qreal rampDownCoefficientB;

    qreal fadeOutCoefficientA;
    qreal fadeOutCoefficientB;

private:
    QAudioOutput *AudioOutput = new QAudioOutput(this->MediaPlayer);

    void volumeControl(qint64 position);
signals:
    void isFinished();

};

#endif // PLAYERPLAYLISTITEM_H
