#ifndef FLOW_H
#define FLOW_H

#include <QMainWindow>
#include <QSettings>
#include <QCloseEvent>
#include <QLocale>

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

private:
    Ui::Flow *ui;

    QSettings *settings = new QSettings("NaxiStudio", "NaxiStudio Flow");

    void loadFolders();

    void saveLayout();
    void restoreLayout();
    void updateClock();

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

signals:
    void loadFiles(QString folder, QString search);
};

#endif // FLOW_H
