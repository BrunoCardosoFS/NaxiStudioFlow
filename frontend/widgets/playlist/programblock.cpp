#include "programblock.h"
#include "ui_programblock.h"

#include "playlistitem.h"


ProgramBlock::ProgramBlock(QWidget *parent):QWidget(parent), ui(new Ui::ProgramBlock) {
    ui->setupUi(this);

    this->setAttribute(Qt::WA_StyledBackground, true);

    this->ui->Duration->setText("00:39:35");

    PlaylistItem *itemTeste = new PlaylistItem(this);
    this->ui->BlockContent->layout()->addWidget(itemTeste);
    PlaylistItem *itemTeste1 = new PlaylistItem(this);
    this->ui->BlockContent->layout()->addWidget(itemTeste1);
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
