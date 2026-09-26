#include "CatalogWidget.h"
#include "ui_CatalogWidget.h"

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
              if (item && !item->isHeader()) {
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
}

void CatalogWidget::setDatabasePath(const QString &dbPath) {
  m_dbPath = dbPath;
  refreshFolders();
}

void CatalogWidget::refreshFolders() {
  if (m_dbPath.isEmpty()) {
    return;
  }
  m_folderModel->loadFromDatabase(m_dbPath);

  QModelIndex firstIdx = m_folderModel->firstFolderIndex();
  if (firstIdx.isValid() && m_openFolder.isEmpty()) {
    ui->FoldersListView->setCurrentIndex(firstIdx);
    onFolderActivated(firstIdx);
  }
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
  if (m_catalogModel->isLoading() && item->path == m_openFolder) {
    return;
  }

  // Se já estiver aberta com os mesmos critérios (busca vazia) e não for visão global, não precisa recarregar
  if (!m_isGlobalView && item->path == m_openFolder &&
      ui->SearchLine->text().trimmed().isEmpty() &&
      !m_catalogModel->isLoading()) {
    return;
  }

  m_isGlobalView = false;
  m_openFolder = item->path;
  m_openFolderType = item->type;
  emit folderSelected(m_openFolder, m_openFolderType);

  // Exibe o item personalizado de Carregando imediatamente
  m_catalogModel->showLoading("Carregando...");

  m_scanner->scanLocal(m_openFolder, ui->SearchLine->text(), m_openFolderType);
}

void CatalogWidget::onSearchLocalClicked() {
  if (!m_openFolder.isEmpty()) {
    m_isGlobalView = false;
    m_catalogModel->showLoading("Carregando...");
    m_scanner->scanLocal(m_openFolder, ui->SearchLine->text(),
                         m_openFolderType);
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
  if (!m_openFolder.isEmpty()) {
    m_isGlobalView = false;
    m_catalogModel->showLoading("Carregando...");
    m_scanner->scanLocal(m_openFolder, QString(), m_openFolderType);
  } else {
    m_catalogModel->clear();
  }
}

void CatalogWidget::onSearchReturnPressed() {
  if (!m_openFolder.isEmpty()) {
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
    if (!item.isHeader() && !item.path.isEmpty()) {
      CatalogFolderTarget target;
      target.title = item.title;
      target.path = item.path;
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
