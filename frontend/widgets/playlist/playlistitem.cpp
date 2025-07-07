#include "playlistitem.h"
#include "ui_playlistitem.h"

#include "../../../backend/functions/timeFunctions.h"

#include <QPainter>
#include <QPainterPath>
#include <QDebug>


PlaylistItem::PlaylistItem(QWidget *parent):QWidget(parent), ui(new Ui::PlaylistItem) {
    ui->setupUi(this);
    this->setAttribute(Qt::WA_StyledBackground, true);

    connect(this->Player->MediaPlayer, &QMediaPlayer::durationChanged, this, &PlaylistItem::updateDuration);
    connect(this->Player->MediaPlayer, &QMediaPlayer::positionChanged, this, &PlaylistItem::updateProgress);
    connect(this->Player->MediaPlayer, &QMediaPlayer::playingChanged, this, &PlaylistItem::updatePlaying);
    connect(this->Player->MediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &PlaylistItem::handleMediaStatusChanged);

    connect(this->Player->MediaPlayer, &QMediaPlayer::errorOccurred, this, [](QMediaPlayer::Error error) {
        qDebug() << "Error:" << error;
    });

    // this->Player->setFade(0.5, 0.3, 2000, 4000, 6000, 6000, 4000, 2000); // Test Fade
}

PlaylistItem::~PlaylistItem(){
    delete ui;
}

void PlaylistItem::setInfo(QString title, QString path, qint8 mediaType){
    this->title = title;
    this->path = path;
    this->mediaType = mediaType;

    this->Player->setSourceFromPath(path);

    this->ui->title->setText(title);
    this->setToolTip(title);
}

void PlaylistItem::play(){
    this->Player->MediaPlayer->setActiveAudioTrack(-1);
    this->Player->MediaPlayer->play();
    this->Player->MediaPlayer->setActiveAudioTrack(0);
}

void PlaylistItem::pause(){
    this->Player->MediaPlayer->pause();
}

void PlaylistItem::stop(){
    this->Player->MediaPlayer->pause();
    this->Player->MediaPlayer->setPosition(0);
}

void PlaylistItem::updateDuration(qint64 duration){
    this->duration = duration;
    this->ui->duration->setText(msec2string(duration));
}

void PlaylistItem::updateProgress(qint64 progress){
    this->progress = progress;
    this->ui->progress->setText(msec2string(progress));

    this->progressPorcent = static_cast<qreal>(progress)/this->duration;

    this->update();
}

void PlaylistItem::updatePlaying(qint64 playing){
    this->isPlaying = playing;
}

void PlaylistItem::handleMediaStatusChanged(QMediaPlayer::MediaStatus status){
    if (status == QMediaPlayer::EndOfMedia) {
        this->wasPlayed = true;
    }
}




void PlaylistItem::paintEvent(QPaintEvent *event){
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setPen(Qt::NoPen);

    QPainterPath clipPath;
    clipPath.addRoundedRect(this->rect(), 10, 10);
    painter.setClipPath(clipPath);

    QLinearGradient gradientBackground(0, 0, 0, height());
    gradientBackground.setColorAt(0.0, this->bgColor1);
    gradientBackground.setColorAt(1.0, this->bgColor2);

    QRectF backgroundRect(0, 0, width(), height());
    painter.setBrush(gradientBackground);
    painter.drawRect(backgroundRect);

    QLinearGradient gradientOpening(0, 0, 0, height());
    gradientOpening.setColorAt(0.0, this->opColor1);
    gradientOpening.setColorAt(1.0, this->opColor2);

    QRectF openingRect(0, 0, width() * this->openingPorcent, height());
    painter.setBrush(gradientOpening);
    painter.drawRect(openingRect);


    QLinearGradient gradientProgress(0, 0, 0, height());
    gradientProgress.setColorAt(0.0, this->pgColor1);
    gradientProgress.setColorAt(1.0, this->pgColor2);

    QRectF progressRect(0, 0, width() * this->progressPorcent, height());
    painter.setBrush(gradientProgress);
    painter.drawRect(progressRect);
}
