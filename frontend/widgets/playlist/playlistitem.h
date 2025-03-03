#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <QWidget>

#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>

namespace Ui {
class PlaylistItem;
}

class PlaylistItem : public QWidget
{
    Q_OBJECT

public:
    explicit PlaylistItem(QWidget *parent = nullptr);
    ~PlaylistItem();

    QString title, path;

    void setInfo(QString title, QString path);

private:
    Ui::PlaylistItem *ui;

    qreal progress;
    qreal opening;

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
        if (event->button() == Qt::RightButton){
            this->deleteLater();
        }
    }
};

#endif // PLAYLISTITEM_H
