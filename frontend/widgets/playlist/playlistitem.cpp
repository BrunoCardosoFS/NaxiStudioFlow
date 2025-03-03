#include "playlistitem.h"
#include "ui_playlistitem.h"

#include <QPainter>

PlaylistItem::PlaylistItem(QWidget *parent):QWidget(parent), ui(new Ui::PlaylistItem) {
    ui->setupUi(this);
    this->setAttribute(Qt::WA_StyledBackground, true);

    this->progress = 0.2;
    this->opening = 0.3;
}

PlaylistItem::~PlaylistItem()
{
    delete ui;
}

void PlaylistItem::setInfo(QString title, QString path){
    this->title = title;
    this->path = path;

    this->ui->title->setText(title);
    this->setToolTip(title);
}




void PlaylistItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    QLinearGradient gradientOpening(0, 0, 0, height());
    gradientOpening.setColorAt(0.0, QColor("#6CB03D"));
    gradientOpening.setColorAt(1.0, QColor("#3A8606"));

    QRectF openingRect(0, 0, width() * opening, height());
    painter.setBrush(gradientOpening);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(openingRect, 10, 10);



    QLinearGradient gradientProgress(0, 0, 0, height());
    gradientProgress.setColorAt(0.0, QColor("#CF3B6B"));
    gradientProgress.setColorAt(1.0, QColor("#AC073C"));

    QRectF progressRect(0, 0, width() * progress, height());
    painter.setBrush(gradientProgress);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(progressRect, 10, 10);
}
