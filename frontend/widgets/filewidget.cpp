#include "filewidget.h"

#include <QHBoxLayout>
#include <QMouseEvent>

FileWidget::FileWidget(QWidget *parent):QWidget{parent} {
    this->parent = parent;

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10,7,10,7);
    this->setLayout(layout);
    this->setFixedHeight(30);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    this->setCursor(Qt::PointingHandCursor);
    this->setAttribute(Qt::WA_StyledBackground, true);

    this->DurationLabel->setAlignment(Qt::AlignHCenter);
    this->DurationLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    this->DurationLabel->setFixedWidth(0);

    layout->addWidget(this->DurationLabel);
    layout->addWidget(this->TitleLabel);
}

void FileWidget::setInfo(QString title, QString path){
    this->title = title;
    this->path = path;

    this->TitleLabel->setText(title);
    this->setToolTip(title);
}

void FileWidget::setDuration(QString duration){
    this->duration = duration;
    this->DurationLabel->setText(duration);
    this->DurationLabel->setFixedWidth(60);
}

void FileWidget::mouseReleaseEvent(QMouseEvent *event){
    switch (event->button()) {
        case Qt::LeftButton:
            break;
        case Qt::MiddleButton:
            break;
        case Qt::RightButton:
            break;
        default:
            break;
    }
}
