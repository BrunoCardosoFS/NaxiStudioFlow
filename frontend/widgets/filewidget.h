#ifndef FILEWIDGET_H
#define FILEWIDGET_H

#include <QWidget>
#include <QLabel>

class FileWidget : public QWidget {
    Q_OBJECT
public:
    explicit FileWidget(QWidget *parent = nullptr);

    QString path;

    void setInfo(QString title, QString path);
    void setDuration(QString duration);

private:
    QWidget *parent;

    QLabel *DurationLabel = new QLabel(this);
    QLabel *TitleLabel = new QLabel(this);
signals:
    void clicked(QString path);
protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
};

#endif // FILEWIDGET_H
