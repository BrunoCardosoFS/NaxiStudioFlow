#include "playlistitem.h"
#include "ui_playlistitem.h"

#include "../../../backend/functions/timeFunctions.h"

#include <QPainter>

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

    this->isPlaying = false;
    this->wasPlayed = false;
    this->progressPorcent = 0.0;
    this->openingPorcent = 0.0;

    this->Player->setFade(5000, 3000); // Test Fade
}

PlaylistItem::~PlaylistItem()
{
    delete ui;
}

void PlaylistItem::setInfo(QString title, QString path){
    this->title = title;
    this->path = path;

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

void PlaylistItem::handleMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        this->wasPlayed = true;
    }
}




void PlaylistItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    QPainterPath clipPath;
    clipPath.addRoundedRect(this->rect(), 10, 10);
    painter.setClipPath(clipPath);

    QLinearGradient gradientOpening(0, 0, 0, height());
    gradientOpening.setColorAt(0.0, QColor("#6CB03D"));
    gradientOpening.setColorAt(1.0, QColor("#3A8606"));

    QRectF openingRect(0, 0, width() * this->openingPorcent, height());
    painter.setBrush(gradientOpening);
    painter.setPen(Qt::NoPen);
    painter.drawRect(openingRect);


    QLinearGradient gradientProgress(0, 0, 0, height());
    gradientProgress.setColorAt(0.0, QColor("#CF3B6B"));
    gradientProgress.setColorAt(1.0, QColor("#AC073C"));

    QRectF progressRect(0, 0, width() * this->progressPorcent, height());
    painter.setBrush(gradientProgress);
    painter.setPen(Qt::NoPen);
    painter.drawRect(progressRect);
}
