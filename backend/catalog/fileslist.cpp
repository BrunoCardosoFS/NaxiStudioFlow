#include "fileslist.h"

#include <QDir>

FilesList::FilesList(QObject *parent):QThread{parent}{}

void FilesList::init(QString path, QString search){
    if(this->isRunning()){
        this->terminate();
    }

    this->path = path;
    this->search = search;

    this->start();
}

void FilesList::run(){
    QJsonArray jsonArray = {};
    QDir files(this->path);

    QString suffix;
    QString fileName;
    QJsonArray item;

    foreach (QFileInfo qfi, files.entryInfoList()) {
        suffix = qfi.completeSuffix().toLower();
        fileName = qfi.fileName();

        if(qfi.isFile() && (fileName.contains(search, Qt::CaseInsensitive)) && (suffix == "mp3" || suffix == "wav" || suffix == "opus" || suffix == "aac" || suffix == "flac" || suffix == "webm" || suffix == "m4a")){
            item = {qfi.fileName(), qfi.absoluteFilePath()};
            jsonArray.append(item);
        }
    }

    emit finish(jsonArray, this->path);
}
