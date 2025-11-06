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
    connect(this->Player->MediaPlayer, &QMediaPlayer::positionChanged, this, &PlaylistItem::updatePosition);
    connect(this->Player->MediaPlayer, &QMediaPlayer::playingChanged, this, &PlaylistItem::updatePlaying);
    connect(this->Player, &PlayerPlaylistItem::isFinished, this, &PlaylistItem::stop);

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

void PlaylistItem::setFade(float startPoint, float endPoint, float preRampUpLevel, float postRampDownLevel, qint64 fadeInEndPoint, qint64 rampUpStartPoint, qint64 rampUpEndPoint, qint64 rampDownStartPoint, qint64 rampDownEndPoint, qint64 fadeOutStartPoint){
    this->startPoint = startPoint;
    this->endPoint = endPoint;
    this->duration = endPoint - startPoint;

    this->ui->duration->setText(msec2string(this->duration));

    this->Player->setFade(startPoint, endPoint, preRampUpLevel, postRampDownLevel, fadeInEndPoint, rampUpStartPoint, rampUpEndPoint, rampDownStartPoint, rampDownEndPoint, fadeOutStartPoint);
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
    this->Player->MediaPlayer->setPosition(this->duration);

    this->wasPlayed = true;
    this->setMaximumHeight(30);
    this->setMinimumHeight(30);
    this->ui->image->setFixedSize(30, 30);
    this->ui->timeArea->hide();
    this->ui->infos->hide();

    QFont font = this->ui->title->font();
    font.setItalic(true);
    font.setBold(false);
    this->ui->title->setFont(font);

    this->update();
}

void PlaylistItem::updateDuration(qint64 duration){
    if(this->startPoint == -1){
        this->startPoint = 0;
        this->endPoint = this->duration = duration;
        this->ui->duration->setText(msec2string(duration));
    }

    if(!this->mixEnd){
        this->mixEnd = duration - 800;
    }
}

void PlaylistItem::updatePosition(qint64 position){
    this->position = position - this->startPoint;

    this->ui->progress->setText(msec2string(this->position));
    this->progressPorcent = static_cast<qreal>(this->position)/this->duration;

    this->update();
}

void PlaylistItem::updatePlaying(qint64 playing){
    this->isPlaying = playing;
}


void PlaylistItem::paintEvent(QPaintEvent *event){
    Q_UNUSED(event);

    if(this->wasPlayed){
        QPainter painter(this);
        painter.setPen(Qt::NoPen);

        // QPainterPath clipPath;
        // clipPath.addRoundedRect(this->rect(), 10, 10);
        // painter.setClipPath(clipPath);

        // QLinearGradient gradientBackground(0, 0, 0, height());
        // gradientBackground.setColorAt(0.0, QColor(QStringLiteral("#6B6B6B")));
        // gradientBackground.setColorAt(1.0, QColor(QStringLiteral("#3B3B3B")));

        QRectF backgroundRect(0, 0, width(), height());
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(backgroundRect);

        return;
    }

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
