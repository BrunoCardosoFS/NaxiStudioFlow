#ifndef PROGRAMBLOCK_H
#define PROGRAMBLOCK_H

#include <QWidget>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QByteArray>
#include <QMimeData>
#include <QLabel>
#include <QVBoxLayout>

namespace Ui {
class ProgramBlock;
}

class ProgramBlock : public QWidget
{
    Q_OBJECT

public:
    explicit ProgramBlock(QWidget *parent = nullptr);
    ~ProgramBlock();

    void setHour(QString hour);
    void setTitle(QString title);

private:
    Ui::ProgramBlock *ui;

    QVBoxLayout *blockContentLayout;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};

#endif // PROGRAMBLOCK_H
