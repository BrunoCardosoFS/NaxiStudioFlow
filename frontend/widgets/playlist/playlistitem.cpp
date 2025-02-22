#include "playlistitem.h"
#include "ui_playlistitem.h"

PlaylistItem::PlaylistItem(QWidget *parent):QWidget(parent), ui(new Ui::PlaylistItem) {
    ui->setupUi(this);
    this->setAttribute(Qt::WA_StyledBackground, true);
}

PlaylistItem::~PlaylistItem()
{
    delete ui;
}
