#ifndef CARTWALLITEM_H
#define CARTWALLITEM_H

#include <QWidget>

namespace Ui {
class CartWallItem;
}

class CartWallItem : public QWidget
{
    Q_OBJECT

public:
    explicit CartWallItem(QWidget *parent = nullptr);
    ~CartWallItem();

private:
    Ui::CartWallItem *ui;

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // CARTWALLITEM_H
