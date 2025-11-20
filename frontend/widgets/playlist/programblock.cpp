#include "programblock.h"
#include "ui_programblock.h"

#include <QDebug>


ProgramBlock::ProgramBlock(QWidget *parent):QWidget(parent), ui(new Ui::ProgramBlock) {
    ui->setupUi(this);

    this->setAcceptDrops(true);
    this->setAttribute(Qt::WA_StyledBackground, true);

    this->blockContentLayout = qobject_cast<QVBoxLayout*>(this->ui->BlockContent->layout());

    this->ui->Duration->setText("00:00:00");




    /////// Test
    PlaylistItem *item1 = new PlaylistItem(this->ui->BlockContent);
    item1->setInfo("Prefixo 1.mp3", "D:/MEDIA/VINHETAS/Jacobina FM/Entrada de Hora/Prefixo 1.mp3", 0);
    item1->mixStart = 500;
    item1->mixEnd = 27100;
    this->blockContentLayout->addWidget(item1);
    this->controller->playlist.append(item1);
    connect(item1, &PlaylistItem::beingDeleted, this, &ProgramBlock::removePlaylistItem);

    PlaylistItem *item2 = new PlaylistItem(this->ui->BlockContent);
    item2->setInfo("Vht - Entrada de Bloco  Comercial - Jacobina Fm - 2.mp3", "D:/MEDIA/PLASTICA 30 ANOS ANIVERSARIO/Plastica Jacobina Parte 001/Verso¦âes Principais - Jacobina Fm/Vht - Entrada de Bloco  Comercial - Jacobina Fm - 2.mp3", 0);
    // item2->setInfo("Vht - Entrada de Bloco  Comercial - Jacobina Fm - 2.mp3", "http://localhost:8000/cdn/principais/Vht%20-%20Entrada%20de%20Bloco%20%20Comercial%20-%20Jacobina%20Fm%20-%202.mp3", 0);
    //http://localhost:8000/cdn/principais/Vht%20-%20Entrada%20de%20Bloco%20%20Comercial%20-%20Jacobina%20Fm%20-%202.mp3
    item2->mixStart = 300;
    item2->mixEnd = 14800;
    this->blockContentLayout->addWidget(item2);
    this->controller->playlist.append(item2);
    connect(item2, &PlaylistItem::beingDeleted, this, &ProgramBlock::removePlaylistItem);

    PlaylistItem *item3 = new PlaylistItem(this->ui->BlockContent);
    item3->setInfo("The Weeknd - Blinding Lights.mp3", "D:/MEDIA/MÚSICAS/Dance/The Weeknd - Blinding Lights.mp3", 0);
    item3->mixStart = 6300;
    item3->mixEnd = 194800;
    item3->setFade(5000, 201600, 0.01, 0.3, 8000, 10500, 11300, 194800, 201600, 201600);
    this->blockContentLayout->addWidget(item3);
    this->controller->playlist.append(item3);
    connect(item3, &PlaylistItem::beingDeleted, this, &ProgramBlock::removePlaylistItem);

    PlaylistItem *item4 = new PlaylistItem(this->ui->BlockContent);
    item4->setInfo("O sucesso na dose certa 1.mp3", "D:/MEDIA/VINHETAS/Jacobina FM/Passagem/O sucesso na dose certa 1.mp3", 0);
    item4->mixStart = 300;
    item4->mixEnd = 9500;
    this->blockContentLayout->addWidget(item4);
    this->controller->playlist.append(item4);
    connect(item4, &PlaylistItem::beingDeleted, this, &ProgramBlock::removePlaylistItem);

    PlaylistItem *item5 = new PlaylistItem(this->ui->BlockContent);
    item5->setInfo("Lady Gaga - 911.mp3", "D:/MEDIA/MÚSICAS/Internacionais/Lady Gaga - 911.mp3", 0);
    item5->mixStart = 8000;
    item5->mixEnd = 166500;
    item5->setFade(0, 170000, 0.05, 0.0, 5000, 7300, 8000, 166000, 170000, 170000);
    this->blockContentLayout->addWidget(item5);
    this->controller->playlist.append(item5);
    connect(item5, &PlaylistItem::beingDeleted, this, &ProgramBlock::removePlaylistItem);

    PlaylistItem *item6 = new PlaylistItem(this->ui->BlockContent);
    item6->setInfo("A RADIO QUE VOCE AJUDOU A CONSTRUIR.mp3", "D:/MEDIA/VINHETAS/Jacobina FM/Passagem/A RADIO QUE VOCE AJUDOU A CONSTRUIR.mp3", 0);
    item6->mixStart = 850;
    item6->mixEnd = 11800;
    item6->setFade(0, 13718, 0.0, 0.0, 0, 0, 850, 11800, 13718, 13718);
    this->blockContentLayout->addWidget(item6);
    this->controller->playlist.append(item6);
    connect(item6, &PlaylistItem::beingDeleted, this, &ProgramBlock::removePlaylistItem);

    PlaylistItem *item7 = new PlaylistItem(this->ui->BlockContent);
    item7->setInfo("Foxy Shazam - Oh Lord.mp3", "D:/MEDIA/MÚSICAS/Rock/Foxy Shazam - Oh Lord.mp3", 0);
    item7->mixStart = 15500;
    item7->mixEnd = 226000;
    item7->setFade(10000, 230000, 0.01, 0.0, 15000, 24000, 25500, 225000, 230000, 230000);
    this->blockContentLayout->addWidget(item7);
    this->controller->playlist.append(item7);
    connect(item7, &PlaylistItem::beingDeleted, this, &ProgramBlock::removePlaylistItem);

    PlaylistItem *item8 = new PlaylistItem(this->ui->BlockContent);
    item8->setInfo("Vht - Jacobina Fm - Passagem - 2.mp3", "D:/MEDIA/PLASTICA 30 ANOS ANIVERSARIO/Plastica Jacobina Parte 001/Verso¦âes Principais - Jacobina Fm/Vht - Jacobina Fm - Passagem - 2.mp3", 0);
    item8->mixStart = 600;
    item8->mixEnd = 15000;
    this->blockContentLayout->addWidget(item8);
    this->controller->playlist.append(item8);
    connect(item8, &PlaylistItem::beingDeleted, this, &ProgramBlock::removePlaylistItem);

    PlaylistItem *item9 = new PlaylistItem(this->ui->BlockContent);
    item9->setInfo("Ed Sheeran - Photograph.mp3", "D:/MEDIA/MÚSICAS/Internacionais/Ed Sheeran - Photograph.mp3", 0);
    item9->mixStart = 100;
    item9->mixEnd = 254000;
    item9->setFade(0, 256000, 0.01, 0.0, 0, 0, 600, 254000, 256000, 256000);
    this->blockContentLayout->addWidget(item9);
    this->controller->playlist.append(item9);
    connect(item9, &PlaylistItem::beingDeleted, this, &ProgramBlock::removePlaylistItem);

    // PlaylistItem *item10 = new PlaylistItem(this->ui->BlockContent);
    // item10->setInfo("CANTADA - FAST 2 - 03.mp3", "D:/MEDIA/OUTROS/CANTADA - FAST 2 - 03.mp3", 0);
    // item10->mixStart = 800;
    // item10->mixEnd = 11200;
    // this->blockContentLayout->addWidget(item10);
    // this->controller->playlist.append(item10);
    // connect(item10, &PlaylistItem::beingDeleted, this, &ProgramBlock::removePlaylistItem);

    PlaylistItem *item10 = new PlaylistItem(this->ui->BlockContent);
    item10->setInfo("VHT ID 6.mp3", "D:/MEDIA/VINHETAS/MIX/VHT ID 6.mp3", 0);
    item10->mixStart = 100;
    item10->mixEnd = 9500;
    this->blockContentLayout->addWidget(item10);
    this->controller->playlist.append(item10);
    connect(item10, &PlaylistItem::beingDeleted, this, &ProgramBlock::removePlaylistItem);

    PlaylistItem *item11 = new PlaylistItem(this->ui->BlockContent);
    item11->setInfo("Dua Lipa - Levitating.mp3", "D:/MEDIA/MÚSICAS/Dance/Dua Lipa - Levitating.mp3", 0);
    item11->mixStart = 4200;
    item11->mixEnd = 201200;
    item11->setFade(0, 202300, 0.05, 0.0, 1000, 3200, 4200, 201800, 202200, 202300);
    this->blockContentLayout->addWidget(item11);
    this->controller->playlist.append(item11);
    connect(item11, &PlaylistItem::beingDeleted, this, &ProgramBlock::removePlaylistItem);
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

void ProgramBlock::removePlaylistItem(PlaylistItem *item){
    int index = this->blockContentLayout->indexOf(item);

    if(index != -1){
        this->controller->playlist.removeAt(index);
        this->blockContentLayout->itemAt(index)->widget()->deleteLater();
    }
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

    connect(item, &PlaylistItem::beingDeleted, this, &ProgramBlock::removePlaylistItem);

    this->blockContentLayout->insertWidget(insertIndex, item);
    this->controller->playlist.insert(insertIndex, item);

    event->acceptProposedAction();
}
