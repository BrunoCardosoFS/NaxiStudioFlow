#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <QWidget>

#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>

#include <QtMultimedia>

namespace Ui {
class PlaylistItem;
}

class PlaylistItem : public QWidget
{
    Q_OBJECT

public:
    explicit PlaylistItem(QWidget *parent = nullptr);
    ~PlaylistItem();

    QMediaPlayer *Player = new QMediaPlayer(this);
    QAudioOutput *AudioOutput = new QAudioOutput(this->Player);

    QString title, path;

    void setInfo(QString title, QString path);

private:
    Ui::PlaylistItem *ui;

    qint64 duration, progress;
    bool isPlaying, wasPlayed;

    qreal progressPorcent;
    qreal openingPorcent;

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
                this->Player->pause();
            }else{
                this->Player->play();
            }
            break;
        case Qt::MiddleButton:
            this->deleteLater();
            break;
        case Qt::RightButton:
            this->Player->pause();
            this->Player->setPosition(0);
            break;
        default:
            break;
        }
    }
};

#endif // PLAYLISTITEM_H
