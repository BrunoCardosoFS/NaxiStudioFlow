#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <QWidget>

#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>

#include <QDebug>

#include "../../../backend/player/playerplaylistitem.h"

namespace Ui {
class PlaylistItem;
}

class PlaylistItem : public QWidget
{
    Q_OBJECT

public:
    explicit PlaylistItem(QWidget *parent = nullptr);
    ~PlaylistItem();

    PlayerPlaylistItem *Player = new PlayerPlaylistItem(this);

    QString title, path;
    qint16 index;
    qint8 mediaType = 0;
    bool isPlaying = false;
    bool wasPlayed = false;

    qint64 mixStart = 0;
    qint64 mixEnd = 0;

    void setInfo(QString title, QString path, qint8 mediaType);
    void setFade(float startPoint, float endPoint, float preRampUpLevel, float postRampDownLevel, qint64 fadeInEndPoint, qint64 rampUpStartPoint, qint64 rampUpEndPoint, qint64 rampDownStartPoint, qint64 rampDownEndPoint, qint64 fadeOutStartPoint);
    void play();
    void pause();
    void stop();

signals:
    void beingDeleted(PlaylistItem *me);

private:
    Ui::PlaylistItem *ui;

    qint64 startPoint = -1;
    qint64 endPoint, duration, position;
    qreal progressPorcent = 0.0;
    qreal openingPorcent = 0.0;

    QColor bgColor1 = QColor(QStringLiteral("#418DB4"));
    QColor bgColor2 = QColor(QStringLiteral("#0066A5"));

    QColor opColor1 = QColor(QStringLiteral("#6CB03D"));
    QColor opColor2 = QColor(QStringLiteral("#3A8606"));

    QColor pgColor1 = QColor(QStringLiteral("#CF3B6B"));
    QColor pgColor2 = QColor(QStringLiteral("#AC073C"));

private slots:
    void updateDuration(qint64 duration);
    void updatePosition(qint64 position);
    void updatePlaying(qint64 playing);

protected:
    void paintEvent(QPaintEvent *event) override;

    // void mousePressEvent(QMouseEvent *event) override {
    //     if (event->button() == Qt::LeftButton) {
    //         QDrag *drag = new QDrag(this);
    //         QMimeData *mimeData = new QMimeData();

    //         QByteArray data;
    //         QDataStream stream(&data, QIODevice::WriteOnly);
    //         stream << title << path;
    //         mimeData->setData("application/x-catalog-item", data);

    //         drag->setMimeData(mimeData);
    //         drag->exec(Qt::MoveAction);
    //     }
    // }

    void mouseReleaseEvent(QMouseEvent *event) override{
        switch (event->button()) {
        case Qt::LeftButton:
            if(this->isPlaying){
                this->pause();
            }else{
                this->play();
            }
            break;
        case Qt::MiddleButton:
            if(!this->isPlaying){
                emit this->beingDeleted(this);
            }
            break;
        case Qt::RightButton:
            // this->stop();
            this->Player->MediaPlayer->setPosition(this->Player->MediaPlayer->position() + 5000);
            break;
        default:
            break;
        }
    }
};

#endif // PLAYLISTITEM_H
