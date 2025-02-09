#ifndef FLOW_H
#define FLOW_H

#include <QMainWindow>
#include <QCloseEvent>

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

private:
    Ui::Flow *ui;

    void saveLayout();
    void restoreLayout();
    void updateClock();

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
};
#endif // FLOW_H
