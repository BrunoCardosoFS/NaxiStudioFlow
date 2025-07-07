#include "playerplaylistitem.h"

#include <QDebug>

PlayerPlaylistItem::PlayerPlaylistItem(QObject *parent):QObject{parent}{
    this->MediaPlayer->setAudioOutput(this->AudioOutput);
    this->AudioOutput->setVolume(0.0);

    connect(this->MediaPlayer, &QMediaPlayer::positionChanged, this, &PlayerPlaylistItem::volumeControl);
    connect(this->MediaPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 durationChanged){
        this->duration = durationChanged;
        this->updateCoefficients();
    });
}

void PlayerPlaylistItem::setSourceFromPath(QString path){
    this->MediaPlayer->setSource(QUrl::fromLocalFile(path));
}

void PlayerPlaylistItem::updateCoefficients(){
    this->duckingStartPosition = this->duration - this->duckingStartPoint;
    this->duckingEndPosition = this->duration - this->duckingEndPoint;
    this->fadeOutStartPosition = this->duration - this->fadeOutStartPoint;

    this->fadeInSlope = (this->introLevel/this->fadeInEndPoint);

    this->mainRampUpSlope = (this->introLevel-1)/(this->mainRampUpStartPoint-this->peakStartPoin);
    this->mainRampUpIntercept = (this->mainRampUpStartPoint-(this->peakStartPoin*this->introLevel))
                                /(this->mainRampUpStartPoint-this->peakStartPoin);

    this->duckingRampDownSlope = (1-this->duckingLevel)/(this->duckingStartPosition-this->duckingEndPosition);
    this->duckingRampDownIntercept = ((this->duckingStartPosition*this->duckingLevel)-this->duckingEndPosition)
                                     /(this->duckingStartPosition-this->duckingEndPosition);

    this->fadeOutSlope = (-this->duckingLevel/this->fadeOutStartPoint);
    this->fadeOutIntercept = ((this->duration*this->duckingLevel)/this->fadeOutStartPoint);
}

void PlayerPlaylistItem::setFade(float introLevel, float duckingLevel, qint64 fadeInEndPoint, qint64 mainRampUpStartPoint, qint64 peakStartPoin, qint64 duckingStartPoint, qint64 duckingEndPoint, qint64 fadeOutStartPoint){
    this->introLevel = introLevel;
    this->duckingLevel = duckingLevel;
    this->fadeInEndPoint = fadeInEndPoint;
    this->mainRampUpStartPoint = mainRampUpStartPoint;
    this->peakStartPoin = peakStartPoin;
    this->duckingStartPoint = duckingStartPoint;
    this->duckingEndPoint = duckingEndPoint;
    this->fadeOutStartPoint = fadeOutStartPoint;

    this->updateCoefficients();
}

void PlayerPlaylistItem::volumeControl(qint64 position){
    float vol = 1.0;

    if(position < this->fadeInEndPoint){
        vol = this->fadeInSlope*position;
    }else if(position < this->mainRampUpStartPoint){
        vol = this->introLevel;
    }else if(position < this->peakStartPoin){
        vol = (position*this->mainRampUpSlope) + this->mainRampUpIntercept;
    }else if(position < this->duckingStartPosition){
        vol = 1;
    }else if(position < this->duckingEndPosition){
        vol = (position*this->duckingRampDownSlope) + this->duckingRampDownIntercept;
    }else if(position < this->fadeOutStartPosition){
        vol = this->duckingLevel;
    }else{
        vol = (position*this->fadeOutSlope) + this->fadeOutIntercept;
    }

    this->AudioOutput->setVolume(qBound(0.0, vol, 1.0));
}
