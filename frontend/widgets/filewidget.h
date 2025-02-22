#ifndef FILEWIDGET_H
#define FILEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>

class FileWidget : public QWidget {
    Q_OBJECT
public:
    explicit FileWidget(QWidget *parent = nullptr);

    QString title, path, duration;

    void setInfo(QString title, QString path);
    void setDuration(QString duration);

private:
    QWidget *parent;

    QLabel *DurationLabel = new QLabel(this);
    QLabel *TitleLabel = new QLabel(this);
protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            QDrag *drag = new QDrag(this);
            QMimeData *mimeData = new QMimeData();

            QByteArray data;
            QDataStream stream(&data, QIODevice::WriteOnly);
            stream << title << path;
            mimeData->setData("application/x-catalog-item", data);

            drag->setMimeData(mimeData);
            drag->exec(Qt::MoveAction);
        }
    }
};

#endif // FILEWIDGET_H
