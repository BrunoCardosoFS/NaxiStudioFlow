#include "playlistitem.h"
#include "ui_playlistitem.h"

#include "../../../backend/functions/timeFunctions.h"

#include <QPainter>

#include <QDebug>

PlaylistItem::PlaylistItem(QWidget *parent):QWidget(parent), ui(new Ui::PlaylistItem) {
    ui->setupUi(this);
    this->setAttribute(Qt::WA_StyledBackground, true);

    this->AudioOutput->setVolume(100);
    this->Player->setAudioOutput(this->AudioOutput);

    connect(this->Player, &QMediaPlayer::durationChanged, this, &PlaylistItem::updateDuration);
    connect(this->Player, &QMediaPlayer::positionChanged, this, &PlaylistItem::updateProgress);
    connect(this->Player, &QMediaPlayer::playingChanged, this, &PlaylistItem::updatePlaying);
    connect(this->Player, &QMediaPlayer::mediaStatusChanged, this, &PlaylistItem::handleMediaStatusChanged);

    this->isPlaying = false;
    this->wasPlayed = false;
    this->progressPorcent = 0.0;
    this->openingPorcent = 0.0;
}

PlaylistItem::~PlaylistItem()
{
    delete ui;
}

void PlaylistItem::setInfo(QString title, QString path){
    this->title = title;
    this->path = path;

    this->Player->setSource(QUrl::fromLocalFile(path));

    this->ui->title->setText(title);
    this->setToolTip(title);
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

    QLinearGradient gradientOpening(0, 0, 0, height());
    gradientOpening.setColorAt(0.0, QColor("#6CB03D"));
    gradientOpening.setColorAt(1.0, QColor("#3A8606"));

    QRectF openingRect(0, 0, width() * this->openingPorcent, height());
    painter.setBrush(gradientOpening);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(openingRect, 10, 10);


    QLinearGradient gradientProgress(0, 0, 0, height());
    gradientProgress.setColorAt(0.0, QColor("#CF3B6B"));
    gradientProgress.setColorAt(1.0, QColor("#AC073C"));

    QRectF progressRect(0, 0, width() * this->progressPorcent, height());
    painter.setBrush(gradientProgress);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(progressRect, 10, 10);
}
