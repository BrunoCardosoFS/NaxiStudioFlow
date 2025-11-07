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

    qint64 startPoint = 0;
    qint64 endPoint = -1;

    qint64 fadeInEndPoint = 0;
    qint64 rampUpStartPoint = 0;
    qint64 rampUpEndPoint = 500;

    qint64 rampDownStartPoint = -1;
    qint64 rampDownEndPoint = -1;
    qint64 fadeOutStartPoint = -1;

    qreal fadeInCoefficientA = 0;

    qreal rampUpCoefficientA = 0;
    qreal rampUpCoefficientB = 0;

    qreal rampDownCoefficientA = 0;
    qreal rampDownCoefficientB = 0;

    qreal fadeOutCoefficientA = 0;
    qreal fadeOutCoefficientB = 0;

private:
    QAudioOutput *AudioOutput = new QAudioOutput(this->MediaPlayer);

    void volumeControl(qint64 position);
signals:
    void isFinished();

};

#endif // PLAYERPLAYLISTITEM_H
