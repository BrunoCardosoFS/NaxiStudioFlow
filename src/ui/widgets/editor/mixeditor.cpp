#include "mixeditor.h"
#include "ui_mixeditor.h"

MixEditor::MixEditor(QWidget *parent):QWidget(parent), ui(new Ui::MixEditor){
    ui->setupUi(this);
}

MixEditor::~MixEditor(){
    delete ui;
}
