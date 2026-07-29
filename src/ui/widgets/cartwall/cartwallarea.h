#ifndef CARTWALLAREA_H
#define CARTWALLAREA_H

#include <QWidget>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QByteArray>
#include <QMimeData>

namespace Ui {
class CartWallArea;
}

class CartWallArea : public QWidget
{
    Q_OBJECT

public:
    explicit CartWallArea(QWidget *parent = nullptr);
    ~CartWallArea();

private:
    Ui::CartWallArea *ui;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};

#endif // CARTWALLAREA_H
