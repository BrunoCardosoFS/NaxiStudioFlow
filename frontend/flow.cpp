#include "flow.h"
#include "./ui_flow.h"

Flow::Flow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Flow)
{
    ui->setupUi(this);
}

Flow::~Flow()
{
    delete ui;
}
