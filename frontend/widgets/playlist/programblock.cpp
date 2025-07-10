#include "programblock.h"
#include "ui_programblock.h"

#include "playlistitem.h"

#include <QDebug>


ProgramBlock::ProgramBlock(QWidget *parent):QWidget(parent), ui(new Ui::ProgramBlock) {
    ui->setupUi(this);

    this->setAcceptDrops(true);
    this->setAttribute(Qt::WA_StyledBackground, true);

    this->blockContentLayout = qobject_cast<QVBoxLayout*>(this->ui->BlockContent->layout());

    this->ui->Duration->setText("00:00:00");




    /////// Test
    PlaylistItem *item1 = new PlaylistItem(this->ui->BlockContent);
    item1->setInfo("Vht - Entrada de Bloco  Comercial - Jacobina Fm - 2.mp3", "D:/MEDIA/PLASTICA 30 ANOS ANIVERSARIO/Plastica Jacobina Parte 001/Verso¦âes Principais - Jacobina Fm/Vht - Entrada de Bloco  Comercial - Jacobina Fm - 2.mp3", 0);
    item1->mixStart = 300;
    item1->mixEnd = 14900;
    this->blockContentLayout->addWidget(item1);
    this->controller->playlist.append(item1);

    PlaylistItem *item2 = new PlaylistItem(this->ui->BlockContent);
    item2->setInfo("O sucesso na dose certa 1.mp3", "D:/MEDIA/VINHETAS/Jacobina FM/Passagem/O sucesso na dose certa 1.mp3", 0);
    item2->mixStart = 300;
    item2->mixEnd = 9500;
    this->blockContentLayout->addWidget(item2);
    this->controller->playlist.append(item2);

    PlaylistItem *item3 = new PlaylistItem(this->ui->BlockContent);
    item3->setInfo("Lady Gaga - 911.mp3", "D:/MEDIA/MÚSICAS/Internacionais/Lady Gaga - 911.mp3", 0);
    item3->mixStart = 8000;
    item3->mixEnd = 166500;
    item3->Player->setFade(0.1, 0.0, 1000, 7300, 8000, 2, 1, 1);
    this->blockContentLayout->addWidget(item3);
    this->controller->playlist.append(item3);

    PlaylistItem *item4 = new PlaylistItem(this->ui->BlockContent);
    item4->setInfo("A RADIO QUE VOCE AJUDOU A CONSTRUIR.mp3", "D:/MEDIA/VINHETAS/Jacobina FM/Passagem/A RADIO QUE VOCE AJUDOU A CONSTRUIR.mp3", 0);
    item4->mixStart = 850;
    item4->mixEnd = 11800;
    item4->Player->setFade(0.0, 0.0, 0, 0, 850, 1808, 1, 1);
    this->blockContentLayout->addWidget(item4);
    this->controller->playlist.append(item4);

    PlaylistItem *item5 = new PlaylistItem(this->ui->BlockContent);
    item5->setInfo("The Weeknd - Blinding Lights.mp3", "D:/MEDIA/MÚSICAS/Dance/The Weeknd - Blinding Lights.mp3", 0);
    item5->mixStart = 11300;
    item5->mixEnd = 194800;
    item5->Player->setFade(0.1, 0.3, 7000, 10000, 11300, 3000, 2000, 1000);
    this->blockContentLayout->addWidget(item5);
    this->controller->playlist.append(item5);
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
    qint8 mediaType;
    stream >> title >> path >> mediaType;

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
    item->setInfo(title, path, mediaType);

    this->blockContentLayout->insertWidget(insertIndex, item);
    this->controller->playlist.insert(insertIndex, item);

    event->acceptProposedAction();
}
