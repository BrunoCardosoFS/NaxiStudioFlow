#ifndef FILESLIST_H
#define FILESLIST_H

#include <QObject>
#include <QThread>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

class FilesList : public QThread{
    Q_OBJECT
public:
    explicit FilesList(QObject *parent = nullptr);

    void init(QString path, QString search);

private:
    void run();
    QString path;
    QString search;

signals:
    void finish(const QJsonArray list, const QString pathFolder);
};

#endif // FILESLIST_H
