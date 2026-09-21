#include "FoldersList.h"

#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>

QJsonArray getFolders(const QString &pathDB) {
    QFile catalog(pathDB + "/catalog.json");
    const bool isOpen = catalog.open(QFile::ReadOnly | QFile::Text);

    QJsonDocument jsonDocument;
    QString jsonString;

    if (isOpen) {
        QTextStream dataCatalog(&catalog);
        jsonString = dataCatalog.readAll();
    } else {
        jsonString = "[{'title': '', 'path': '', 'type': 0, 'id': ''}]";
    }

    jsonDocument = QJsonDocument::fromJson(jsonString.toUtf8());
    catalog.close();

    return jsonDocument.array();
}
