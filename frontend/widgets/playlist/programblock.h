#ifndef PROGRAMBLOCK_H
#define PROGRAMBLOCK_H

#include <QWidget>

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
};

#endif // PROGRAMBLOCK_H
