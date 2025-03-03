#include "programblock.h"
#include "ui_programblock.h"

#include "playlistitem.h"

#include <QDebug>


ProgramBlock::ProgramBlock(QWidget *parent):QWidget(parent), ui(new Ui::ProgramBlock) {
    ui->setupUi(this);

    this->setAcceptDrops(true);
    this->setAttribute(Qt::WA_StyledBackground, true);

    this->blockContentLayout = qobject_cast<QVBoxLayout*>(this->ui->BlockContent->layout());

    this->ui->Duration->setText("00:39:35");
}

ProgramBlock::~ProgramBlock()
{
    delete ui;
}

void ProgramBlock::setHour(QString hour){
    this->ui->Hour->setText(hour);
}

void ProgramBlock::setTitle(QString title){
    this->ui->Title->setText(title);
}


void ProgramBlock::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat("application/x-catalog-item")) {
        event->acceptProposedAction();
    }
}

void ProgramBlock::dragMoveEvent(QDragMoveEvent *event) {
    event->acceptProposedAction();
}

void ProgramBlock::dropEvent(QDropEvent *event) {
    QByteArray data = event->mimeData()->data("application/x-catalog-item");
    QDataStream stream(&data, QIODevice::ReadOnly);
    QString title, path;
    stream >> title >> path;

    QPoint dropPos = event->position().toPoint();
    int dropY = dropPos.y() - this->ui->BlockContent->geometry().top();

    int insertIndex = ui->BlockContent->layout()->count();

    for (int i = 0; i < ui->BlockContent->layout()->count(); ++i) {
        QWidget *child = ui->BlockContent->layout()->itemAt(i)->widget();
        if (child) {
            if (dropY < child->geometry().center().y()) {
                insertIndex = i;
                break;
            }
        }
    }

    PlaylistItem *item = new PlaylistItem(this->ui->BlockContent);
    item->setInfo(title, path);

    this->blockContentLayout->insertWidget(insertIndex, item);

    event->acceptProposedAction();
}
