#include "CatalogWidget.h"
#include "ui_CatalogWidget.h"

#include <QAction>
#include <QMenu>
#include <QMouseEvent>

CatalogWidget::CatalogWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::CatalogWidget) {
  ui->setupUi(this);

  m_scanner = new CatalogScanner(this);
  m_catalogModel = new CatalogListModel(this);
  m_catalogDelegate = new CatalogItemDelegate(this);
  m_folderModel = new FolderListModel(this);
  m_folderDelegate = new FolderItemDelegate(this);

  initViews();

  connect(ui->SearchLocal, &QPushButton::clicked, this,
          &CatalogWidget::onSearchLocalClicked);
  connect(ui->SearchGlobal, &QPushButton::clicked, this,
          &CatalogWidget::onSearchGlobalClicked);
  connect(ui->SearchClean, &QPushButton::clicked, this,
          &CatalogWidget::onSearchCleanClicked);
  connect(ui->SearchLine, &QLineEdit::returnPressed, this,
          &CatalogWidget::onSearchReturnPressed);

  connect(m_scanner, &CatalogScanner::finish, this,
          &CatalogWidget::onFilesLoaded);

  connect(ui->FilesListView, &QListView::doubleClicked, this,
          [this](const QModelIndex &index) {
            if (index.isValid()) {
              const CatalogItem *item = m_catalogModel->itemAt(index.row());
              if (item && !item->isHeader() && !item->isLoading() &&
                  !item->isCommand()) {
                emit fileDoubleClicked(*item);
              }
            }
          });
}

CatalogWidget::~CatalogWidget() { delete ui; }

void CatalogWidget::initViews() {
  ui->FilesListView->setModel(m_catalogModel);
  ui->FilesListView->setItemDelegate(m_catalogDelegate);
  // Otimização: uniformItemSizes ativo para listas locais onde todos os itens têm 42px
  ui->FilesListView->setUniformItemSizes(true);

  ui->FoldersListView->setModel(m_folderModel);
  ui->FoldersListView->setItemDelegate(m_folderDelegate);

  connect(ui->FoldersListView, &QListView::clicked, this,
          &CatalogWidget::onFolderActivated);
  connect(ui->FoldersListView->selectionModel(),
          &QItemSelectionModel::currentChanged, this,
          [this](const QModelIndex &current, const QModelIndex &previous) {
            if (current.isValid() && current != previous) {
              onFolderActivated(current);
            }
          });

  ui->FoldersListView->setMouseTracking(true);
  ui->FoldersListView->viewport()->setMouseTracking(true);
  ui->FoldersListView->viewport()->installEventFilter(this);

  ui->FilesListView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui->FilesListView, &QListView::customContextMenuRequested, this,
          &CatalogWidget::onFilesContextMenuRequested);
}

QMenu *CatalogWidget::createFileContextMenu(const QModelIndex &index,
                                            QWidget *parent) {
  if (!index.isValid()) {
    return nullptr;
  }

  const CatalogItem *item = m_catalogModel->itemAt(index.row());
  if (!item || item->isHeader() || item->isLoading() || item->isCommand()) {
    return nullptr;
  }

  auto *menu = new QMenu(parent ? parent : this);
  menu->setStyleSheet(QStringLiteral("QMenu {"
                                     "  background-color: #1F232C;"
                                     "  color: #FFFFFF;"
                                     "  border: 1px solid #2D3340;"
                                     "  border-radius: 6px;"
                                     "  padding: 4px;"
                                     "}"
                                     "QMenu::item {"
                                     "  padding: 6px 14px;"
                                     "  border-radius: 4px;"
                                     "}"
                                     "QMenu::item:selected {"
                                     "  background-color: #3E4A5E;"
                                     "}"
                                     "QMenu::item:disabled {"
                                     "  color: #8E9EB5;"
                                     "}"));

  // TODO(M11): Enable opening MixEditor from Catalog once global sidecar (.json) saving is implemented.
  QAction *mixAction =
      menu->addAction(tr("Abrir no Editor de Mixagem (to do (M11))"));
  mixAction->setEnabled(false);

  return menu;
}

void CatalogWidget::onFilesContextMenuRequested(const QPoint &pos) {
  const QModelIndex index = ui->FilesListView->indexAt(pos);
  QMenu *menu = createFileContextMenu(index, this);
  if (!menu) {
    return;
  }

  ui->FilesListView->setCurrentIndex(index);
  menu->exec(ui->FilesListView->viewport()->mapToGlobal(pos));
  menu->deleteLater();
}

void CatalogWidget::setDatabasePath(const QString &dbPath) {
  m_dbPath = dbPath;
  refreshFolders();
}

void CatalogWidget::refreshFolders() {
  if (m_dbPath.isEmpty()) {
    return;
  }
  m_openFolderRow = -1;
  m_openFolder.clear();
  m_folderModel->loadFromDatabase(m_dbPath);
  emit foldersRefreshed();

  QModelIndex firstIdx = m_folderModel->firstFolderIndex();
  if (firstIdx.isValid()) {
    ui->FoldersListView->setCurrentIndex(firstIdx);
    onFolderActivated(firstIdx);
  } else {
    m_catalogModel->clear();
  }
}

void CatalogWidget::loadOpenFolder(const QString &search) {
  const FolderItem *folder = m_folderModel->itemAt(m_openFolderRow);
  if (!folder || folder->isHeader()) {
    m_catalogModel->clear();
    return;
  }

  if (folder->isCommandFolder()) {
    m_scanner->cancelCurrentScan();
    const QString trimmedSearch = search.trimmed();
    QVector<CatalogItem> items;
    items.reserve(folder->items.size());
    for (const CatalogItem &cmd : folder->items) {
      if (trimmedSearch.isEmpty() ||
          cmd.title.contains(trimmedSearch, Qt::CaseInsensitive) ||
          cmd.commandId.contains(trimmedSearch, Qt::CaseInsensitive)) {
        items.append(cmd);
      }
    }
    onFilesLoaded(items, folder->path, false);
    return;
  }

  if (!folder->hasAnyPath()) {
    m_scanner->cancelCurrentScan();
    onFilesLoaded({}, QString(), false);
    return;
  }

  // Exibe o item personalizado de Carregando imediatamente
  m_catalogModel->showLoading("Carregando...");

  m_scanner->scanLocal(folder->path, folder->mirrorPaths, search, folder->type);
}

void CatalogWidget::onFolderActivated(const QModelIndex &index) {
  if (!index.isValid()) {
    return;
  }
  const FolderItem *item = m_folderModel->itemAt(index.row());
  if (!item || item->isHeader()) {
    return;
  }

  // Se já está carregando esta exata pasta, não reinicie nem cancele o scan em andamento
  if (m_catalogModel->isLoading() && index.row() == m_openFolderRow) {
    return;
  }

  // Se já estiver aberta com os mesmos critérios (busca vazia) e não for visão global, não precisa recarregar
  if (!m_isGlobalView && index.row() == m_openFolderRow &&
      ui->SearchLine->text().trimmed().isEmpty() &&
      !m_catalogModel->isLoading()) {
    return;
  }

  m_isGlobalView = false;
  m_openFolderRow = index.row();
  m_openFolder = item->activePath.isEmpty() ? item->path : item->activePath;
  m_openFolderType = item->type;
  emit folderSelected(m_openFolder, m_openFolderType);

  loadOpenFolder(ui->SearchLine->text());
}

void CatalogWidget::onSearchLocalClicked() {
  if (m_openFolderRow >= 0) {
    m_isGlobalView = false;
    loadOpenFolder(ui->SearchLine->text());
  }
}

void CatalogWidget::onSearchGlobalClicked() {
  m_isGlobalView = true;
  const QList<CatalogFolderTarget> targets = getFolderTargets();
  m_catalogModel->showLoading("Carregando...");
  m_scanner->scanGlobal(targets, ui->SearchLine->text());
}

void CatalogWidget::onSearchCleanClicked() {
  ui->SearchLine->clear();
  if (m_openFolderRow >= 0) {
    m_isGlobalView = false;
    loadOpenFolder(QString());
  } else {
    m_catalogModel->clear();
  }
}

void CatalogWidget::onSearchReturnPressed() {
  if (m_openFolderRow >= 0) {
    onSearchLocalClicked();
  } else {
    onSearchGlobalClicked();
  }
}

void CatalogWidget::onFilesLoaded(const QVector<CatalogItem> &list,
                                  const QString &pathFolder, bool isGlobal) {
  m_isGlobalView = isGlobal;
  if (!isGlobal) {
    m_openFolder = pathFolder;
  }
  // Ativa uniformItemSizes para visualização local (todos 42px), desativa para busca global (headers 34px e itens 42px)
  ui->FilesListView->setUniformItemSizes(!isGlobal);
  m_catalogModel->setItems(list);
}

QList<CatalogFolderTarget> CatalogWidget::getFolderTargets() const {
  QList<CatalogFolderTarget> targets;
  for (const FolderItem &item : m_folderModel->items()) {
    if (item.isHeader()) {
      continue;
    }
    if (item.isCommandFolder()) {
      CatalogFolderTarget target;
      target.title = item.title;
      target.path = item.path;
      target.type = item.type;
      target.isCommandFolder = true;
      target.inlineItems = item.items;
      targets.append(std::move(target));
    } else if (item.hasAnyPath()) {
      CatalogFolderTarget target;
      target.title = item.title;
      target.path = item.path;
      target.mirrorPaths = item.mirrorPaths;
      target.type = item.type;
      targets.append(std::move(target));
    }
  }
  return targets;
}

bool CatalogWidget::eventFilter(QObject *watched, QEvent *event) {
  if (ui && ui->FoldersListView && watched == ui->FoldersListView->viewport()) {
    if (event->type() == QEvent::MouseMove) {
      auto *mouseEvent = static_cast<QMouseEvent *>(event);
      QModelIndex index = ui->FoldersListView->indexAt(mouseEvent->pos());
      if (index.isValid()) {
        int itemType = index.data(FolderItemTypeRole).toInt();
        if (itemType == static_cast<int>(FolderItemType::Folder)) {
          ui->FoldersListView->viewport()->setCursor(Qt::PointingHandCursor);
          return false;
        }
      }
      ui->FoldersListView->viewport()->setCursor(Qt::ArrowCursor);
    } else if (event->type() == QEvent::Leave) {
      ui->FoldersListView->viewport()->setCursor(Qt::ArrowCursor);
    }
  }
  return QWidget::eventFilter(watched, event);
}
