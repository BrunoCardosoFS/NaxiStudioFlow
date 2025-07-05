#include "playerplaylistitem.h"

#include <QDebug>

PlayerPlaylistItem::PlayerPlaylistItem(QObject *parent):QObject{parent}{
    this->MediaPlayer->setAudioOutput(this->AudioOutput);
    this->AudioOutput->setVolume(0.0);

    connect(this->MediaPlayer, &QMediaPlayer::positionChanged, this, &PlayerPlaylistItem::volumeControl);
    connect(this->MediaPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 durationChanged){
        this->duration = durationChanged;
    });
}

void PlayerPlaylistItem::setSourceFromPath(QString path){
    this->MediaPlayer->setSource(QUrl::fromLocalFile(path));
}

void PlayerPlaylistItem::setFade(qint64 fadeIn, qint64 fadeOut){
    this->fadeIn = fadeIn;
    this->fadeOut = fadeOut;
}

void PlayerPlaylistItem::volumeControl(qint64 position){
    float vol = 1.0;

    if(position <= this->fadeIn){
        vol = (1.0*position)/this->fadeIn;
        this->AudioOutput->setVolume(vol);
        return;
    }else if(position >= (this->duration - this->fadeOut)){
        vol = ((-1.0/this->fadeOut)*position) + ((1.0*this->duration)/this->fadeOut);
        this->AudioOutput->setVolume(vol);
        return;
    }

    this->AudioOutput->setVolume(vol);
}
