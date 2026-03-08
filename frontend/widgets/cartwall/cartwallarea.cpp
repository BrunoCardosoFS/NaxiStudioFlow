#include "cartwallarea.h"
#include "ui_cartwallarea.h"

#include <QPushButton>
#include "./cartwallitem.h"

CartWallArea::CartWallArea(QWidget *parent):QWidget(parent), ui(new Ui::CartWallArea){
    ui->setupUi(this);

    CartWallItem *cartItem1 = new CartWallItem(this);
    CartWallItem *cartItem2 = new CartWallItem(this);
    CartWallItem *cartItem3 = new CartWallItem(this);
    QSpacerItem *spacerCart = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

    this->ui->gridLayout->addWidget(cartItem1, 0, 0);
    this->ui->gridLayout->addWidget(cartItem2, 0, 1);
    this->ui->gridLayout->addWidget(cartItem3, 1, 0);
    this->ui->gridLayout->addItem(spacerCart, 2, 0, 2, 2);
}

CartWallArea::~CartWallArea(){
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
