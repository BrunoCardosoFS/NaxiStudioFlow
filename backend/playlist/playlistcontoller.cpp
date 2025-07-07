#include "playlistcontoller.h"
#include <QMediaPlayer>

PlaylistContoller::PlaylistContoller(QObject *parent):QObject{parent}{

}

void PlaylistContoller::onPositionChanged(qint64 position){

    if(this->playlist.count() > 1){
        if((position >= (this->playlist[0]->mixEnd - this->playlist[1]->mixStart))){
            if(!this->playlist[1]->isPlaying){
                this->playlist[1]->play();
            }
        }
    }

    if(this->playlist.count() > 2){
        if(this->playlist[2]->mixStart > this->playlist[1]->mixEnd){
            qint64 mixFic = this->playlist[2]->mixStart - this->playlist[1]->mixEnd + this->playlist[1]->mixStart;

            if(position >= (this->playlist[0]->mixEnd - mixFic)){
                if(!this->playlist[2]->isPlaying){
                    this->playlist[2]->play();
                }
            }
        }
    }

    if(position >= this->playlist[0]->mixEnd){
        this->toNext();
        return;
    }
}

void PlaylistContoller::play(int index){
    this->playlist[index]->play();
}

void PlaylistContoller::toNext(){
    if(this->currentIndex == 0){
        disconnect(this->playlist[0]->Player->MediaPlayer, &QMediaPlayer::positionChanged, this, &PlaylistContoller::onPositionChanged);
        this->playlist.removeFirst();
    }

    qInfo() << "Current: " << this->playlist[0]->title;
    this->currentIndex = 0;
    connect(this->playlist[0]->Player->MediaPlayer, &QMediaPlayer::positionChanged, this, &PlaylistContoller::onPositionChanged);
}
