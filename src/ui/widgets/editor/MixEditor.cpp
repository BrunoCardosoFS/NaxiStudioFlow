#include "MixEditor.h"
#include "ui_MixEditor.h"

MixEditor::MixEditor(QWidget *parent):QWidget(parent), ui(new Ui::MixEditor){
    ui->setupUi(this);
}

MixEditor::~MixEditor(){
    delete ui;
}
