#ifndef FLOW_H
#define FLOW_H

#include <QMainWindow>
#include <QSettings>
#include <QCloseEvent>
#include <QLocale>
#include <QList>

#include "../backend/catalog/fileslist.h"
#include "./widgets/playlist/programblock.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Flow;
}
QT_END_NAMESPACE

class Flow : public QMainWindow
{
    Q_OBJECT

public:
    Flow(QWidget *parent = nullptr);
    ~Flow();

private slots:
    void on_btnFull_clicked();
    void on_SearchLocal_clicked();
    void on_SearchClean_clicked();
    void on_SearchLine_returnPressed();

    void on_btnPlay_clicked();
    void on_btnPause_clicked();

    void on_btnStop_clicked();

private:
    Ui::Flow *ui;

    QSettings *settings = new QSettings("NaxiStudio", "NaxiStudio Flow");

    FilesList *filesList = new FilesList(this);

    void loadFolders();
    void loadFiles(QJsonArray list, QString pathFolder);
    QString openFolder = "";

    void saveLayout();
    void restoreLayout();
    void updateClock();

    QList<ProgramBlock*> blocks;

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

signals:
    void getFiles(QString folder, QString search);
};

#endif // FLOW_H
