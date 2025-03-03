#ifndef FLOW_H
#define FLOW_H

#include <QMainWindow>
#include <QSettings>
#include <QCloseEvent>
#include <QLocale>

#include "../backend/catalog/fileslist.h"

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

    void on_pushButton_7_clicked();

    void on_SearchLocal_clicked();

    void on_SearchClean_clicked();

    void on_SearchLine_returnPressed();

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

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

signals:
    void getFiles(QString folder, QString search);
};

#endif // FLOW_H
