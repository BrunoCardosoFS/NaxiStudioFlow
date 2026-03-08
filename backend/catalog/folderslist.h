#ifndef FOLDERSLIST_H
#define FOLDERSLIST_H

#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

QJsonArray getFolders(QString pathDB){
    QFile catalog(pathDB + "/catalog.json");
    const bool isOpen = catalog.open(QFile::ReadOnly | QFile::Text);

    QJsonDocument jsonDocument;
    QString jsonString;

    if(isOpen){
        QTextStream dataCatalog(&catalog);
        jsonString = dataCatalog.readAll();
    }else{
        jsonString = "[{'title': '', 'path': '', 'type': 0, 'id': ''}]";
    }

    jsonDocument = QJsonDocument::fromJson(jsonString.toUtf8());
    catalog.close();

    return jsonDocument.array();
}

#endif // FOLDERSLIST_H
