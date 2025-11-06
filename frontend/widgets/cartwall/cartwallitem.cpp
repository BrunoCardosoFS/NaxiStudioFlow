#include "cartwallitem.h"
#include "ui_cartwallitem.h"

#include <QPainter>
#include <QPainterPath>

CartWallItem::CartWallItem(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CartWallItem)
{
    ui->setupUi(this);
}

CartWallItem::~CartWallItem()
{
    delete ui;
}


void CartWallItem::paintEvent(QPaintEvent *event){
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setPen(Qt::NoPen);

    QPainterPath clipPath;
    clipPath.addRoundedRect(this->rect(), 10, 10);
    painter.setClipPath(clipPath);

    QLinearGradient gradientBackground(0, 0, 0, height());
    gradientBackground.setColorAt(0.0, "#ffffff");
    gradientBackground.setColorAt(1.0, "#b3b3b3");

    QRectF backgroundRect(0, 0, width(), height());
    painter.setBrush(gradientBackground);
    painter.drawRect(backgroundRect);
}
