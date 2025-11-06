#include "playlistcontoller.h"
#include <QMediaPlayer>

PlaylistContoller::PlaylistContoller(QObject *parent):QObject{parent}{

}

void PlaylistContoller::onPositionChanged(qint64 position){
    int countItems = this->playlist.count() - this->currentIndex;

    if(countItems > 1){
        if((position >= (this->playlist[this->currentIndex]->mixEnd - this->playlist[this->currentIndex+1]->mixStart))){
            if(!this->playlist[this->currentIndex+1]->isPlaying){
                this->playlist[this->currentIndex+1]->play();
            }
        }
    }

    if(countItems > 2){
        if(this->playlist[this->currentIndex+2]->mixStart > this->playlist[this->currentIndex+1]->mixEnd){
            qint64 mixFic = this->playlist[this->currentIndex+2]->mixStart - this->playlist[this->currentIndex+1]->mixEnd + this->playlist[this->currentIndex+1]->mixStart;

            if(position >= (this->playlist[this->currentIndex]->mixEnd - mixFic)){
                if(!this->playlist[this->currentIndex+2]->isPlaying){
                    this->playlist[this->currentIndex+2]->play();
                }
            }
        }
    }

    if(position >= this->playlist[this->currentIndex]->mixEnd){
        this->toNext();
        return;
    }
}

void PlaylistContoller::play(){
    if(this->currentIndex < 0){
        this->toNext();
    }

    if(this->currentIndex >= 0){
        this->playlist[this->currentIndex]->play();
    }
}

void PlaylistContoller::pause(){
    if(this->currentIndex == -1){
        return;
    }

    this->playlist[this->currentIndex]->pause();
    if(this->playlist.count() > this->currentIndex+1){
        this->playlist[this->currentIndex + 1]->pause();
    }
    if(this->playlist.count() > this->currentIndex+2){
        this->playlist[this->currentIndex + 2]->pause();
    }
}

void PlaylistContoller::stop(){
    this->pause();

    if(this->currentIndex > -1){
        int current = this->currentIndex;
        this->toNext();
        this->playlist[current]->stop();
    }
}

void PlaylistContoller::toNext(){
    if(this->currentIndex > -1){
        disconnect(this->playlist[this->currentIndex]->Player->MediaPlayer, &QMediaPlayer::positionChanged, this, &PlaylistContoller::onPositionChanged);
    }

    if(this->playlist.count() > this->currentIndex+1){
        this->currentIndex++;

        qInfo() << "Current: " << this->playlist[this->currentIndex]->title;
        connect(this->playlist[this->currentIndex]->Player->MediaPlayer, &QMediaPlayer::positionChanged, this, &PlaylistContoller::onPositionChanged);
    }
}
