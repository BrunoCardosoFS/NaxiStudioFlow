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
    qint8 mediaType = 0;
    bool isPlaying = false;
    bool wasPlayed = false;

    qint64 mixStart = 0;
    qint64 mixEnd;

    void setInfo(QString title, QString path, qint8 mediaType);
    void play();
    void pause();
    void stop();

private:
    Ui::PlaylistItem *ui;

    qint64 duration, progress;
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
    void updateProgress(qint64 progress);
    void updatePlaying(qint64 playing);

    void handleMediaStatusChanged(QMediaPlayer::MediaStatus status);

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
            this->deleteLater();
            qInfo() << "Deletado";
            break;
        case Qt::RightButton:
            this->stop();

            qInfo() << "Reiniciado";
            break;
        default:
            break;
        }
    }
};

#endif // PLAYLISTITEM_H
