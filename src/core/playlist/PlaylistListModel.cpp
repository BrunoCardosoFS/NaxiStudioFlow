#include "./PlaylistListModel.h"

PlaylistListModel::PlaylistListModel(QObject *parent)
    : QAbstractListModel(parent) {
  loadMockData();
}

int PlaylistListModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return m_items.size();
}

const PlaylistItem *PlaylistListModel::itemAt(int row) const {
  if (row >= 0 && row < m_items.size()) {
    return &m_items.at(row);
  }
  return nullptr;
}

QVariant PlaylistListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
    return QVariant();
  }

  const PlaylistItem &item = m_items.at(index.row());

  switch (role) {
  case Qt::DisplayRole:
  case Qt::ToolTipRole:
  case TitleRole:
    return item.title;
  case BlockTitleRole:
    return item.blockTitle;
  case UuidRole:
    return item.uuid;
  case StatusRole:
    return static_cast<int>(item.status);
  case PathRole:
    return item.path;
  case DurationRole:
    return item.durationMs;
  case PositionRole:
    return item.positionMs;
  case MixStartRole:
    return item.mixStart;
  case MixEndRole:
    return item.mixEnd;
  case BlockTypeRole:
    return static_cast<int>(item.blockType);
  case TypeRole:
    return static_cast<int>(item.type);
  case MediaTypeRole:
    return static_cast<int>(item.mediaType);
  case EstimatedTimeRole:
    return item.estimatedTime;
  case TotalDurationRole:
    return item.totalDurationMs;
  case ItemsCountRole:
    return item.itemsCount;
  case MarkerTextRole:
    return item.markerText;
  default:
    return QVariant();
  }
}

Qt::ItemFlags PlaylistListModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) {
    return Qt::ItemIsDropEnabled;
  }

  if (index.row() < 0 || index.row() >= m_items.size()) {
    return Qt::NoItemFlags;
  }

  const PlaylistItem &item = m_items.at(index.row());

  if (item.type != ItemType::Media) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }

  // Items at or above the playing row are locked.
  int playingRow = currentPlayingRow();
  if (playingRow >= 0 && index.row() <= playingRow) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

Qt::DropActions PlaylistListModel::supportedDropActions() const {
  return Qt::MoveAction;
}

Qt::DropActions PlaylistListModel::supportedDragActions() const {
  return Qt::MoveAction;
}

int PlaylistListModel::currentPlayingRow() const {
  // Cached; refreshed by recalculateBlockMetadata().
  return m_playingRow;
}

bool PlaylistListModel::isDropRowBetweenMarkers(int row) const {
  if (row <= 0 || row >= m_items.size()) {
    return false;
  }

  const PlaylistItem &prev = m_items.at(row - 1);
  const PlaylistItem &next = m_items.at(row);

  bool prevValid =
      (prev.type == ItemType::StartMarker || prev.type == ItemType::Media);
  bool nextValid =
      (next.type == ItemType::EndMarker || next.type == ItemType::Media);

  return prevValid && nextValid;
}

void PlaylistListModel::addItem(const PlaylistItem &item) {
  beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
  m_items.append(item);
  endInsertRows();

  rebuildUuidMap();
  recalculateBlockMetadata();
}

void PlaylistListModel::insertItem(int row, const PlaylistItem &item) {
  int playingRow = currentPlayingRow();
  if (playingRow >= 0 && row <= playingRow) {
    return;
  }

  int targetRow = qBound(0, row, m_items.size());
  beginInsertRows(QModelIndex(), targetRow, targetRow);
  m_items.insert(targetRow, item);
  endInsertRows();

  rebuildUuidMap();
  recalculateBlockMetadata();
}

void PlaylistListModel::removeItem(int row) {
  if (row < 0 || row >= m_items.size())
    return;

  if (m_items.at(row).type != ItemType::Media)
    return;

  int playingRow = currentPlayingRow();
  if (playingRow >= 0 && row <= playingRow)
    return;

  beginRemoveRows(QModelIndex(), row, row);
  m_items.removeAt(row);
  endRemoveRows();

  rebuildUuidMap();
  recalculateBlockMetadata();
}

void PlaylistListModel::clear() {
  beginResetModel();
  m_items.clear();
  m_uuidRowMap.clear();
  m_playingRow = -1;
  endResetModel();
}

bool PlaylistListModel::moveItem(int fromRow, int toRow) {
  if (fromRow < 0 || fromRow >= m_items.size() || toRow < 0 ||
      toRow >= m_items.size()) {
    return false;
  }
  if (fromRow == toRow)
    return false;

  if (m_items.at(fromRow).type != ItemType::Media) {
    return false;
  }

  int playingRow = currentPlayingRow();
  if (playingRow >= 0 && (fromRow <= playingRow || toRow <= playingRow)) {
    return false;
  }

  int destination = toRow > fromRow ? toRow + 1 : toRow;
  beginMoveRows(QModelIndex(), fromRow, fromRow, QModelIndex(), destination);
  m_items.move(fromRow, toRow);
  endMoveRows();

  rebuildUuidMap();
  recalculateBlockMetadata();
  return true;
}

void PlaylistListModel::recalculateBlockMetadata() {
  if (m_items.isEmpty()) {
    m_playingRow = -1;
    return;
  }

  // Snapshot derived fields so dataChanged is emitted only for changed rows.
  struct Derived {
    QString estimatedTime;
    QString markerText;
    qint64 totalDurationMs;
    int itemsCount;
    PlaylistItemStatus status;
    BlockType blockType;
  };
  QVector<Derived> before;
  before.reserve(m_items.size());
  for (const PlaylistItem &it : std::as_const(m_items)) {
    before.append({it.estimatedTime, it.markerText, it.totalDurationMs,
                   it.itemsCount, it.status, it.blockType});
  }

  m_playingRow = -1;
  for (int i = 0; i < m_items.size(); ++i) {
    const PlaylistItem &it = m_items.at(i);
    if (it.type == ItemType::Media && it.status == PlaylistItemStatus::Playing) {
      m_playingRow = i;
      break;
    }
  }

  QTime accumulatedTime(0, 0, 0);
  BlockType activeBlockType = BlockType::Commercial;

  int commercialCount = 0;
  qint64 commercialDurationMs = 0;

  int musicCount = 0;
  qint64 musicDurationMs = 0;

  int currentBlockMarkerIdx = -1;
  qint64 currentBlockDurationMs = 0;

  for (int i = 0; i < m_items.size(); ++i) {
    PlaylistItem &item = m_items[i];

    if (item.type == ItemType::BlockMarker) {
      if (currentBlockMarkerIdx >= 0) {
        m_items[currentBlockMarkerIdx].totalDurationMs = currentBlockDurationMs;
      }
      currentBlockMarkerIdx = i;
      currentBlockDurationMs = 0;

      QTime parsed = QTime::fromString(item.title.trimmed(), "hh:mm");
      if (parsed.isValid()) {
        accumulatedTime = parsed;
      }
      commercialCount = 0;
      commercialDurationMs = 0;
      musicCount = 0;
      musicDurationMs = 0;
    } else if (item.type == ItemType::StartMarker) {
      activeBlockType = item.blockType;
      if (item.blockType == BlockType::Commercial) {
        item.markerText = QStringLiteral("BLOCO COMERCIAL");
        commercialCount = 0;
        commercialDurationMs = 0;
      } else {
        item.markerText.clear();
        musicCount = 0;
        musicDurationMs = 0;
      }
    } else if (item.type == ItemType::Media) {
      item.blockType = activeBlockType;
      item.estimatedTime = accumulatedTime.toString("hh:mm:ss");

      qint64 effectiveDuration = item.durationMs;
      if (item.mixEnd > 0 && item.durationMs > item.mixEnd) {
        effectiveDuration = item.durationMs - item.mixEnd;
      }
      if (effectiveDuration < 0)
        effectiveDuration = item.durationMs;

      accumulatedTime =
          accumulatedTime.addMSecs(static_cast<int>(effectiveDuration));

      currentBlockDurationMs += item.durationMs;

      if (activeBlockType == BlockType::Commercial) {
        commercialCount++;
        commercialDurationMs += item.durationMs;
      } else {
        musicCount++;
        musicDurationMs += item.durationMs;
      }
    } else if (item.type == ItemType::EndMarker) {
      if (item.blockType == BlockType::Commercial) {
        item.markerText = QStringLiteral("BLOCO COMERCIAL");
        item.itemsCount = commercialCount;
        item.totalDurationMs = commercialDurationMs;
      } else {
        item.markerText.clear();
        item.itemsCount = musicCount;
        item.totalDurationMs = musicDurationMs;
      }
    }
  }

  if (currentBlockMarkerIdx >= 0) {
    m_items[currentBlockMarkerIdx].totalDurationMs = currentBlockDurationMs;
  }

  const int playingRow = m_playingRow;
  bool foundNext = false;

  for (int i = 0; i < m_items.size(); ++i) {
    PlaylistItem &item = m_items[i];
    if (item.type != ItemType::Media)
      continue;

    if (playingRow >= 0) {
      if (i < playingRow) {
        item.status = PlaylistItemStatus::Finished;
      } else if (i == playingRow) {
        item.status = PlaylistItemStatus::Playing;
      } else if (!foundNext) {
        item.status = PlaylistItemStatus::Next;
        foundNext = true;
      } else {
        item.status = PlaylistItemStatus::Default;
      }
    } else {
      if (!foundNext) {
        item.status = PlaylistItemStatus::Next;
        foundNext = true;
      } else {
        item.status = PlaylistItemStatus::Default;
      }
    }
  }

  // Coalesce changed rows into contiguous dataChanged ranges.
  int runStart = -1;
  for (int i = 0; i <= m_items.size(); ++i) {
    bool changed = false;
    if (i < m_items.size()) {
      const PlaylistItem &it = m_items.at(i);
      const Derived &b = before.at(i);
      changed = it.status != b.status || it.blockType != b.blockType ||
                it.totalDurationMs != b.totalDurationMs ||
                it.itemsCount != b.itemsCount ||
                it.estimatedTime != b.estimatedTime ||
                it.markerText != b.markerText;
    }
    if (changed) {
      if (runStart < 0)
        runStart = i;
    } else if (runStart >= 0) {
      emit dataChanged(index(runStart, 0), index(i - 1, 0));
      runStart = -1;
    }
  }
}

void PlaylistListModel::rebuildUuidMap() {
  m_uuidRowMap.clear();
  for (int i = 0; i < m_items.size(); ++i) {
    m_uuidRowMap.insert(m_items.at(i).uuid, i);
  }
}

void PlaylistListModel::loadMockData() {
  m_items.clear();

  auto createItem = [](ItemType type, BlockType bType, MediaType mType,
                       const QString &title, const QString &blockTitle,
                       qint64 durationMs,
                       PlaylistItemStatus status = PlaylistItemStatus::Default,
                       int playerId = -1, qint64 positionMs = 0) {
    PlaylistItem item;
    item.uuid = QUuid::createUuid();
    item.type = type;
    item.blockType = bType;
    item.mediaType = mType;
    item.title = title;
    item.blockTitle = blockTitle;
    item.durationMs = durationMs;
    item.status = status;
    item.playerId = playerId;
    item.positionMs = positionMs;
    return item;
  };

  m_items.append(createItem(ItemType::BlockMarker, BlockType::Commercial,
                            MediaType::Other, "14:00", "Manhã Show", 0));
  m_items.append(createItem(ItemType::StartMarker, BlockType::Commercial,
                            MediaType::Other, "", "", 0));
  m_items.append(createItem(ItemType::Media, BlockType::Commercial,
                            MediaType::Commercial, "Armazém Paraíba 2026.mp3",
                            "", 30000, PlaylistItemStatus::Finished));
  m_items.append(createItem(ItemType::EndMarker, BlockType::Commercial,
                            MediaType::Other, "", "", 0));

  m_items.append(createItem(ItemType::StartMarker, BlockType::Music,
                            MediaType::Other, "", "", 0));
  m_items.append(createItem(ItemType::Media, BlockType::Music, MediaType::Music,
                            "Lady Gaga - Abracadabra.mp3", "", 215000,
                            PlaylistItemStatus::Finished));
  m_items.append(createItem(ItemType::Media, BlockType::Music, MediaType::Music,
                            "Michael Jackson - Thriller.mp3", "", 224125,
                            PlaylistItemStatus::Playing, 1, 121125));
  m_items.append(createItem(ItemType::Media, BlockType::Music,
                            MediaType::Jingle, "EB 1 - Jacobina FM.mp3", "",
                            224125, PlaylistItemStatus::Next, 2));
  m_items.append(createItem(ItemType::Media, BlockType::Music, MediaType::Music,
                            "Lady Gaga - Judas.mp3", "", 224125,
                            PlaylistItemStatus::Default));
  m_items.append(createItem(ItemType::Media, BlockType::Music,
                            MediaType::Jingle, "Transição Acústica.mp3", "",
                            11000, PlaylistItemStatus::Default));
  m_items.append(createItem(ItemType::EndMarker, BlockType::Music,
                            MediaType::Other, "", "", 0));

  m_items.append(createItem(ItemType::BlockMarker, BlockType::Commercial,
                            MediaType::Other, "14:30", "Tarde Show", 0));
  m_items.append(createItem(ItemType::StartMarker, BlockType::Commercial,
                            MediaType::Other, "", "", 0));
  m_items.append(createItem(ItemType::Media, BlockType::Commercial,
                            MediaType::Jingle, "Chamada de Intervalo.mp3", "",
                            14000));
  m_items.append(createItem(ItemType::Media, BlockType::Commercial,
                            MediaType::Commercial, "AutoTop Veículos.mp3", "",
                            30000));
  m_items.append(createItem(ItemType::Media, BlockType::Commercial,
                            MediaType::Commercial, "Drogaria Vida & Saúde.mp3",
                            "", 30000));
  m_items.append(createItem(ItemType::EndMarker, BlockType::Commercial,
                            MediaType::Other, "", "", 0));

  m_items.append(createItem(ItemType::StartMarker, BlockType::Music,
                            MediaType::Other, "", "", 0));
  m_items.append(createItem(ItemType::Media, BlockType::Music,
                            MediaType::Jingle, "Vinheta Prime.mp3", "", 10000));
  m_items.append(createItem(ItemType::Media, BlockType::Music, MediaType::Music,
                            "Harry Styles - As It Was.mp3", "", 167000));
  m_items.append(createItem(ItemType::Media, BlockType::Music, MediaType::Music,
                            "The Weeknd - Save Your Tears.mp3", "", 215000));
  m_items.append(createItem(ItemType::EndMarker, BlockType::Music,
                            MediaType::Other, "", "", 0));

  rebuildUuidMap();
  recalculateBlockMetadata();
}