#include "playerplaylistitem.h"

#include <QDebug>

PlayerPlaylistItem::PlayerPlaylistItem(QObject *parent):QObject{parent}{
    this->MediaPlayer->setAudioOutput(this->AudioOutput);
    this->AudioOutput->setVolume(0.0);

    connect(this->MediaPlayer, &QMediaPlayer::positionChanged, this, &PlayerPlaylistItem::volumeControl);
    connect(this->MediaPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 durationChanged){
        if(this->endPoint == -1){
            this->endPoint = this->rampDownEndPoint = this->fadeOutStartPoint = durationChanged;
            this->rampDownStartPoint = durationChanged - 500;

            this->updateCoefficients();
        }
    });

    connect(this->MediaPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status){
        if(status == QMediaPlayer::LoadedMedia && !this->isLoaded){
            this->isLoaded = true;
            this->MediaPlayer->setPosition(this->startPoint);
        }

        if(status == QMediaPlayer::EndOfMedia){
            emit this->isFinished();
        }
    });
}

void PlayerPlaylistItem::setSourceFromPath(QString path){
    this->MediaPlayer->setSource(QUrl::fromLocalFile(path));
}

void PlayerPlaylistItem::updateCoefficients(){
    this->fadeInCoefficientA = this->preRampUpLevel/(this->fadeInEndPoint - this->startPoint);

    this->rampUpCoefficientA = (1 - this->preRampUpLevel)/(this->rampUpEndPoint - this->rampUpStartPoint);
    this->rampUpCoefficientB = ((this->rampUpEndPoint*this->preRampUpLevel) - this->rampUpStartPoint)/(this->rampUpEndPoint - this->rampUpStartPoint);

    this->rampDownCoefficientA = (1 - this->postRampDownLevel)/(this->rampDownStartPoint - this->rampDownEndPoint);
    this->rampDownCoefficientB = ((this->postRampDownLevel*this->rampDownStartPoint) - this->rampDownEndPoint)/(this->rampDownStartPoint - this->rampDownEndPoint);

    this->fadeOutCoefficientA = this->postRampDownLevel/(this->fadeOutStartPoint - this->endPoint);
    this->fadeOutCoefficientB = (-this->postRampDownLevel*this->endPoint)/(this->fadeOutStartPoint - this->endPoint);
}

void PlayerPlaylistItem::setFade(float startPoint, float endPoint, float preRampUpLevel, float postRampDownLevel, qint64 fadeInEndPoint, qint64 rampUpStartPoint, qint64 rampUpEndPoint, qint64 rampDownStartPoint, qint64 rampDownEndPoint, qint64 fadeOutStartPoint){
    this->startPoint = startPoint;
    this->endPoint = endPoint;
    this->preRampUpLevel = preRampUpLevel;
    this->postRampDownLevel = postRampDownLevel;
    this->fadeInEndPoint = fadeInEndPoint;
    this->rampUpStartPoint = rampUpStartPoint;
    this->rampUpEndPoint = rampUpEndPoint;
    this->rampDownStartPoint = rampDownStartPoint;
    this->rampDownEndPoint = rampDownEndPoint;
    this->fadeOutStartPoint = fadeOutStartPoint;

    if(this->MediaPlayer->mediaStatus() == QMediaPlayer::LoadedMedia){
        this->MediaPlayer->setPosition(startPoint);
    }

    this->updateCoefficients();
}

void PlayerPlaylistItem::volumeControl(qint64 position){
    if(this->endPoint && (position >= this->endPoint)){
        emit this->isFinished();
        return;
    }

    float vol;

    if (position <= this->fadeInEndPoint){
        vol = position * this->fadeInCoefficientA;
    }else if(position < this->rampUpStartPoint){
        vol = this->preRampUpLevel;
    }else if(position < this->rampUpEndPoint){
        vol = (position * this->rampUpCoefficientA) + this->rampUpCoefficientB;
    }else if(position < this->rampDownStartPoint){
        vol = 1.0;
    }else if(position < this->rampDownEndPoint){
        vol = (position * this->rampDownCoefficientA) + this->rampDownCoefficientB;
    }else if(position < this->fadeOutStartPoint){
        vol = this->postRampDownLevel;
    }else{
        vol = (position * this->fadeOutCoefficientA) + this->fadeOutCoefficientB;
    }

    this->AudioOutput->setVolume(qBound(0.0, vol, 1.0));
}
