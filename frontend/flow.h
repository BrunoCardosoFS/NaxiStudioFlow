#ifndef FLOW_H
#define FLOW_H

#include <QMainWindow>

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
    void on_dockWidget_4_topLevelChanged(bool topLevel);
    void on_dockWidget_3_topLevelChanged(bool topLevel);

private:
    Ui::Flow *ui;
};
#endif // FLOW_H
