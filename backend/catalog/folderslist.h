#ifndef FOLDERSLIST_H
#define FOLDERSLIST_H

#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

QJsonArray getFolders(QString pathDB){
    QFile catalog(pathDB + "/catalog.json");
    catalog.open(QFile::ReadOnly | QFile::Text);
    QTextStream dataCatalog(&catalog);
    QString jsonString = dataCatalog.readAll();
    catalog.close();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonString.toUtf8());

    return jsonDocument.array();
}

#endif // FOLDERSLIST_H
