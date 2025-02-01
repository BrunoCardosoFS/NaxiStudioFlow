#include "flow.h"
#include "./ui_flow.h"

Flow::Flow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Flow)
{
    ui->setupUi(this);

    setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);
}

Flow::~Flow()
{
    delete ui;
}

void Flow::on_dockWidget_4_topLevelChanged(bool topLevel)
{
    if(topLevel){
        this->ui->dockWidget_4->setWindowFlags(Qt::CustomizeWindowHint |
                                               Qt::Window | Qt::WindowMinimizeButtonHint |
                                               Qt::WindowMaximizeButtonHint |
                                               Qt::WindowCloseButtonHint);
        this->ui->dockWidget_4->show();
    }
}

void Flow::on_dockWidget_3_topLevelChanged(bool topLevel)
{
    if(topLevel){
        this->ui->dockWidget_3->setWindowFlags(Qt::CustomizeWindowHint |
                                               Qt::Window | Qt::WindowMinimizeButtonHint |
                                               Qt::WindowMaximizeButtonHint |
                                               Qt::WindowCloseButtonHint);
        this->ui->dockWidget_3->show();
    }
}


