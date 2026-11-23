#ifndef CATALOGWIDGET_H
#define CATALOGWIDGET_H

#include <QModelIndex>
#include <QWidget>

#include "core/catalog/CatalogItem.h"
#include "core/catalog/CatalogListModel.h"
#include "core/catalog/CatalogScanner.h"
#include "core/catalog/FolderListModel.h"
#include "ui/widgets/catalog/CatalogItemDelegate.h"
#include "ui/widgets/catalog/FolderItemDelegate.h"

namespace Ui {
class CatalogWidget;
}

class QMenu;

class CatalogWidget : public QWidget {
  Q_OBJECT

public:
  explicit CatalogWidget(QWidget *parent = nullptr);
  ~CatalogWidget() override;

  void setDatabasePath(const QString &dbPath);
  void refreshFolders();

  CatalogListModel *catalogModel() const { return m_catalogModel; }
  FolderListModel *folderModel() const { return m_folderModel; }

  QString currentOpenFolder() const { return m_openFolder; }
  MediaType currentOpenFolderType() const { return m_openFolderType; }

  QMenu *createFileContextMenu(const QModelIndex &index,
                               QWidget *parent = nullptr);

signals:
  void fileDoubleClicked(const CatalogItem &item);
  void folderSelected(const QString &path, MediaType type);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
  void onSearchLocalClicked();
  void onSearchGlobalClicked();
  void onSearchCleanClicked();
  void onSearchReturnPressed();
  void onFolderActivated(const QModelIndex &index);
  void onFilesLoaded(const QVector<CatalogItem> &list,
                     const QString &pathFolder, bool isGlobal);
  void onFilesContextMenuRequested(const QPoint &pos);

private:
  Ui::CatalogWidget *ui;

  CatalogScanner *m_scanner = nullptr;
  CatalogListModel *m_catalogModel = nullptr;
  CatalogItemDelegate *m_catalogDelegate = nullptr;
  FolderListModel *m_folderModel = nullptr;
  FolderItemDelegate *m_folderDelegate = nullptr;

  QString m_dbPath;
  QString m_openFolder;
  int m_openFolderRow = -1;
  MediaType m_openFolderType = MediaType::Other;
  bool m_isGlobalView = false;

  QList<CatalogFolderTarget> getFolderTargets() const;
  void loadOpenFolder(const QString &search);
  void initViews();
};

#endif // CATALOGWIDGET_H
