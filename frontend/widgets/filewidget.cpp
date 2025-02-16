#include "filewidget.h"

#include <QHBoxLayout>
#include <QMouseEvent>

FileWidget::FileWidget(QWidget *parent):QWidget{parent} {
    this->parent = parent;

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10,7,10,7);
    this->setLayout(layout);
    this->setObjectName("item_list");
    this->setFixedHeight(30);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    this->setCursor(Qt::PointingHandCursor);
    this->setAttribute(Qt::WA_StyledBackground, true);

    this->DurationLabel->setFixedWidth(60);
    this->DurationLabel->setAlignment(Qt::AlignHCenter);

    layout->addWidget(this->DurationLabel);
    layout->addWidget(this->TitleLabel);
}

void FileWidget::setInfo(QString title, QString path){
    this->TitleLabel->setText(title);
    this->setToolTip(title);
    this->path = path;
}

void FileWidget::setDuration(QString duration){
    this->DurationLabel->setText(duration);
}

void FileWidget::mouseReleaseEvent(QMouseEvent *event){
    switch (event->button()) {
        case Qt::LeftButton:
            emit clicked(this->path);
            break;
        case Qt::MiddleButton:
            break;
        case Qt::RightButton:
            break;
        default:
            break;
    }
}
