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

private:
    Ui::Flow *ui;

    void saveLayout();
    void restoreLayout();
};
#endif // FLOW_H
