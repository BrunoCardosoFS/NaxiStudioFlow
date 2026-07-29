#ifndef MIXEDITOR_H
#define MIXEDITOR_H

#include <QWidget>

namespace Ui {
    class MixEditor;
}

class MixEditor : public QWidget{
    Q_OBJECT

public:
    explicit MixEditor(QWidget *parent = nullptr);
    ~MixEditor();

private:
    Ui::MixEditor *ui;
};

#endif // MIXEDITOR_H
