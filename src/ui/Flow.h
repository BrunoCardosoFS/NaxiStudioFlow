#ifndef FLOW_H
#define FLOW_H

#include <QCloseEvent>
#include <QList>
#include <QLocale>
#include <QMainWindow>
#include <QSettings>


#include "core/catalog/CatalogItem.h"
#include "core/catalog/CatalogListModel.h"
#include "core/catalog/FilesList.h"
#include "ui/widgets/catalog/CatalogItemDelegate.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Flow;
}
QT_END_NAMESPACE

class Flow : public QMainWindow {
  Q_OBJECT

public:
  Flow(QWidget *parent = nullptr);
  ~Flow();

private slots:
  void on_btnFull_clicked();
  void on_SearchLocal_clicked();
  void on_SearchGlobal_clicked();
  void on_SearchClean_clicked();
  void on_SearchLine_returnPressed();

  void on_btnPlay_clicked();
  void on_btnPause_clicked();
  void on_btnStop_clicked();

private:
  Ui::Flow *ui;

  QSettings *settings = new QSettings("NaxStudio", "Flow");

  FilesList *filesList = new FilesList(this);
  CatalogListModel *catalogModel = nullptr;
  CatalogItemDelegate *catalogDelegate = nullptr;

  void loadFolders();
  void loadFiles(const QVector<CatalogItem> &list, const QString &pathFolder, bool isGlobal);
  QList<CatalogFolderTarget> getFolderTargets() const;

  QString openFolder = "";
  int openFolderType = 0;

  void saveLayout();
  void restoreLayout();
  void updateClock();

protected:
  void changeEvent(QEvent *event) override;
  void closeEvent(QCloseEvent *event) override;

signals:
  void getFiles(QString folder, QString search);
};

#endif // FLOW_H
