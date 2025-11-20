#include "cartwallarea.h"
#include "ui_cartwallarea.h"

#include "cartwallitem.h"

CartWallArea::CartWallArea(QWidget *parent):QWidget(parent), ui(new Ui::CartWallArea){
    ui->setupUi(this);

    CartWallItem *cartItem1 = new CartWallItem(this);
    CartWallItem *cartItem2 = new CartWallItem(this);
    QSpacerItem *spacerCart = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

    QGridLayout *gridLayout = qobject_cast<QGridLayout*>(this->layout());

    gridLayout->addWidget(cartItem1, 0, 0);
    gridLayout->addWidget(cartItem2, 0, 1);
    gridLayout->addItem(spacerCart, 1, 0, 1, 2);
}

CartWallArea::~CartWallArea()
{
    delete ui;
}

void CartWallArea::dragEnterEvent(QDragEnterEvent *event){
    if (event->mimeData()->hasFormat("application/x-catalog-item")) {
        event->acceptProposedAction();
    }
}

void CartWallArea::dragMoveEvent(QDragMoveEvent *event){
    event->acceptProposedAction();
}

void CartWallArea::dropEvent(QDropEvent *event){
    QByteArray data = event->mimeData()->data("application/x-catalog-item");
    QDataStream stream(&data, QIODevice::ReadOnly);
    QString title, path;
    qint8 mediaType;
    stream >> title >> path >> mediaType;

    qInfo() << title << path << mediaType;

    event->acceptProposedAction();
}
