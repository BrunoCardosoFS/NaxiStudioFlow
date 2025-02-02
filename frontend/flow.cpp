#include "flow.h"
#include "./ui_flow.h"

#include <QSettings>

Flow::Flow(QWidget *parent):QMainWindow(parent), ui(new Ui::Flow){
    ui->setupUi(this);

    restoreLayout();

}

Flow::~Flow(){
    saveLayout();
    delete ui;
}

void Flow::saveLayout(){
    QSettings settings("NaxiStudio", "PlayerFlow");
    settings.setValue("layout", saveState());
}

void Flow::restoreLayout(){
    QSettings settings("NaxiStudio", "PlayerFlow");

    QByteArray layoutData = settings.value("layout").toByteArray();
    if (!layoutData.isEmpty()) {
        restoreState(layoutData);  // Restaura estado salvo
    }
}

