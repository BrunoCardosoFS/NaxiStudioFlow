#include "ui/widgets/editor/MixEditor.h"
#include "ui_MixEditor.h"

#include <QAbstractSpinBox>
#include <QApplication>
#include <QFileInfo>
#include <QIcon>
#include <QLineEdit>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QUrl>

#include <algorithm>
#include <cmath>

#include "core/playlist/PlaylistManager.h"
#include "core/utils/TimeUtils.h"
#include "ui/widgets/editor/WaveformWidget.h"

namespace {
enum TimeFieldId {
  FieldStartPoint = 0,
  FieldMixStart,
  FieldMixEnd,
  FieldEndPoint,
  FieldFadeIn,
  FieldVolUpStart,
  FieldVolUpEnd,
  FieldVolDownStart,
  FieldVolDownEnd,
  FieldFadeOut
};

constexpr int kPlayheadTimerMs = 30;
constexpr qint64 kDefaultEnvelopeRampMs = 2000;

QTime msToQTime(qint64 ms) {
  const qint64 clamped = std::clamp<qint64>(ms, 0, 86399999);
  return QTime(0, 0).addMSecs(static_cast<int>(clamped));
}

qint64 qTimeToMs(const QTime &t) { return QTime(0, 0).msecsTo(t); }
} // namespace

MixEditor::MixEditor(QWidget *parent) : QWidget(parent), ui(new Ui::MixEditor) {
  ui->setupUi(this);
  setWindowFlag(Qt::Window, true);

  ui->btnPlay->setIcon(QIcon(QStringLiteral(":/images/icons/play.svg")));
  ui->btnPause->setIcon(QIcon(QStringLiteral(":/images/icons/pause.svg")));
  ui->btnStop->setIcon(QIcon(QStringLiteral(":/images/icons/Stop.svg")));
  ui->btnSave->setIcon(QIcon(QStringLiteral(":/images/icons/save.svg")));

  // TODO(M11): Implement global sidecar (.json) save and Catalog opening mode
  // once M11 is implemented.
  ui->btnSaveGlobal->setEnabled(false);

  m_waveformWidget = new WaveformWidget(ui->waveformContainer);
  ui->waveformContainerLayout->addWidget(m_waveformWidget);

  m_waveformExtractor = new WaveformExtractor(this);
  m_previewPlayer = new PlayerManager(this);
  // TODO(M13): bind preview player audioOutput to configured CUE device
  m_playheadTimer = new QTimer(this);
  m_playheadTimer->setInterval(kPlayheadTimerMs);

  initConnections();
  updateTransportButtons();
  updatePositionLabel();
}

MixEditor::~MixEditor() {
  if (m_playheadTimer) {
    m_playheadTimer->stop();
  }
  if (m_waveformExtractor) {
    m_waveformExtractor->cancel();
  }
  if (m_previewPlayer) {
    m_previewPlayer->stop();
    m_previewPlayer->unload();
  }
  delete ui;
}

void MixEditor::initConnections() {
  connect(ui->btnPlay, &QPushButton::clicked, this, &MixEditor::onPlayClicked);
  connect(ui->btnPause, &QPushButton::clicked, this,
          &MixEditor::onPauseClicked);
  connect(ui->btnStop, &QPushButton::clicked, this, &MixEditor::onStopClicked);
  connect(ui->btnSave, &QPushButton::clicked, this, &MixEditor::onSaveClicked);
  connect(ui->btnCancel, &QPushButton::clicked, this, &MixEditor::close);
  connect(ui->btnReset, &QPushButton::clicked, this,
          &MixEditor::onResetClicked);

  connect(ui->btnZoomIn, &QPushButton::clicked, m_waveformWidget,
          &WaveformWidget::zoomIn);
  connect(ui->btnZoomOut, &QPushButton::clicked, m_waveformWidget,
          &WaveformWidget::zoomOut);
  connect(ui->btnZoomReset, &QPushButton::clicked, m_waveformWidget,
          &WaveformWidget::resetZoom);

  connect(m_waveformWidget, &WaveformWidget::metadataChanged, this,
          &MixEditor::onWaveformMetadataChanged);
  connect(m_waveformWidget, &WaveformWidget::seekRequested, this,
          [this](qint64 posMs) {
            m_playheadMs = posMs;
            m_seekedWhileStopped =
                (m_previewPlayer->status() != PlayerStatus::Playing);
            m_previewPlayer->seek(posMs);
            m_waveformWidget->setPlayheadMs(posMs);
            updatePositionLabel();
          });
  connect(m_waveformWidget, &WaveformWidget::playFromRequested, this,
          [this](qint64 posMs) {
            m_playheadMs = posMs;
            m_seekedWhileStopped = false;
            m_previewPlayer->playFrom(posMs);
            m_waveformWidget->setPlayheadMs(posMs);
            updatePositionLabel();
          });
  connect(m_waveformWidget, &WaveformWidget::zoomChanged, this,
          &MixEditor::onWaveformZoomChanged);
  connect(ui->waveformScrollBar, &QScrollBar::valueChanged, this,
          &MixEditor::onScrollBarValueChanged);

  connect(m_previewPlayer, &PlayerManager::statusChanged, this,
          &MixEditor::onPreviewStatusChanged);
  connect(m_previewPlayer, &PlayerManager::positionChanged, this,
          &MixEditor::onPreviewPositionChanged);
  connect(m_previewPlayer, &PlayerManager::durationLoaded, this,
          &MixEditor::onPreviewDurationLoaded);
  connect(m_playheadTimer, &QTimer::timeout, this,
          &MixEditor::onPlayheadTimerTimeout);

  connect(m_waveformExtractor, &WaveformExtractor::progressChunk, this,
          &MixEditor::onWaveformChunk);
  connect(m_waveformExtractor, &WaveformExtractor::finished, this,
          &MixEditor::onWaveformFinished);
  connect(m_waveformExtractor, &WaveformExtractor::errorOccurred, this,
          &MixEditor::onWaveformError);

  const QVector<QTimeEdit *> timeEdits = {
      ui->timeStartPoint, ui->timeMixStart,     ui->timeMixEnd,
      ui->timeEndPoint,   ui->timeFadeIn,       ui->timeVolUpStart,
      ui->timeVolUpEnd,   ui->timeVolDownStart, ui->timeVolDownEnd,
      ui->timeFadeOut};
  for (QTimeEdit *te : timeEdits) {
    connect(te, &QTimeEdit::timeChanged, this, &MixEditor::onTimeFieldEdited);
  }

  connect(ui->sliderVolStart, &QSlider::valueChanged, this,
          &MixEditor::onStartVolumeSliderChanged);
  connect(ui->sliderVolEnd, &QSlider::valueChanged, this,
          &MixEditor::onEndVolumeSliderChanged);

  connect(ui->btnMarkStartPoint, &QPushButton::clicked, this,
          [this]() { markFieldAtPlayhead(FieldStartPoint); });
  connect(ui->btnMarkMixStart, &QPushButton::clicked, this,
          [this]() { markFieldAtPlayhead(FieldMixStart); });
  connect(ui->btnMarkMixEnd, &QPushButton::clicked, this,
          [this]() { markFieldAtPlayhead(FieldMixEnd); });
  connect(ui->btnMarkEndPoint, &QPushButton::clicked, this,
          [this]() { markFieldAtPlayhead(FieldEndPoint); });
  connect(ui->btnMarkFadeIn, &QPushButton::clicked, this,
          [this]() { markFieldAtPlayhead(FieldFadeIn); });
  connect(ui->btnMarkVolUpStart, &QPushButton::clicked, this,
          [this]() { markFieldAtPlayhead(FieldVolUpStart); });
  connect(ui->btnMarkVolUpEnd, &QPushButton::clicked, this,
          [this]() { markFieldAtPlayhead(FieldVolUpEnd); });
  connect(ui->btnMarkVolDownStart, &QPushButton::clicked, this,
          [this]() { markFieldAtPlayhead(FieldVolDownStart); });
  connect(ui->btnMarkVolDownEnd, &QPushButton::clicked, this,
          [this]() { markFieldAtPlayhead(FieldVolDownEnd); });
  connect(ui->btnMarkFadeOut, &QPushButton::clicked, this,
          [this]() { markFieldAtPlayhead(FieldFadeOut); });
}

bool MixEditor::loadPlaylistItem(PlaylistListModel *model,
                                 const QUuid &itemUuid) {
  if (!model || itemUuid.isNull()) {
    return false;
  }

  const int row = model->rowForUuid(itemUuid);
  const PlaylistItem *item = model->itemAt(row);
  if (!item || item->type != ItemType::Media) {
    return false;
  }

  m_playheadTimer->stop();
  m_waveformExtractor->cancel();
  m_previewPlayer->stop();
  m_previewPlayer->unload();

  m_playlistModel = model;
  m_itemUuid = itemUuid;
  m_filePath = item->path;
  m_metadata = PlaylistManager::metadataFromItem(*item);
  normalizeMetadataInPlace();

  m_playheadMs = m_metadata.startPoint;
  m_seekedWhileStopped = false;
  m_lastDurationForRanges = -1;

  ui->labelFileName->setText(item->title);
  ui->labelFileName->setToolTip(item->path);

  m_waveformWidget->resetZoom();
  m_waveformWidget->setDurationMs(m_metadata.duration);
  m_waveformWidget->setMetadata(m_metadata);
  m_waveformWidget->setPlayheadMs(m_playheadMs);

  m_fileExists = !m_filePath.isEmpty() && QFileInfo::exists(m_filePath);
  if (m_fileExists) {
    m_waveformWidget->clearWaveform(tr("Carregando forma de onda..."));
    m_waveformExtractor->extract(m_filePath);
    m_previewPlayer->load(QUrl::fromLocalFile(m_filePath), m_metadata, false);
  } else {
    m_waveformWidget->clearWaveform(
        tr("Arquivo de áudio não encontrado no disco — editando apenas "
           "marcadores de tempo"));
  }

  syncControlsFromMetadata(false);
  updateTransportButtons();
  updatePositionLabel();
  return true;
}

bool MixEditor::saveToPlaylist() {
  if (!m_playlistModel || m_itemUuid.isNull()) {
    return false;
  }

  const int row = m_playlistModel->rowForUuid(m_itemUuid);
  const PlaylistItem *item = m_playlistModel->itemAt(row);
  if (!item || item->type != ItemType::Media ||
      item->status == PlaylistItemStatus::Finished) {
    return false;
  }

  normalizeMetadataInPlace();

  m_playlistModel->updateItemMixMetadata(
      row, m_metadata.duration, m_metadata.startPoint, m_metadata.endPoint,
      m_metadata.mixStart, m_metadata.mixEnd, m_metadata.fadeInEndPoint,
      m_metadata.fadeOutStartPoint, m_metadata.startVolume,
      m_metadata.endVolume, m_metadata.volumeUpStartPoint,
      m_metadata.volumeUpEndPoint, m_metadata.volumeDownStartPoint,
      m_metadata.volumeDownEndPoint);

  emit savedLocally(m_itemUuid, m_metadata);
  return true;
}

void MixEditor::setAudioOutputDevice(const QAudioDevice &device) {
  if (m_previewPlayer) {
    m_previewPlayer->setAudioOutputDevice(device);
  }
}

void MixEditor::closeEvent(QCloseEvent *event) {
  if (m_playheadTimer) {
    m_playheadTimer->stop();
  }
  if (m_waveformExtractor) {
    m_waveformExtractor->cancel();
  }
  if (m_previewPlayer) {
    m_previewPlayer->stop();
    m_previewPlayer->unload();
  }
  QWidget::closeEvent(event);
}

void MixEditor::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    close();
    event->accept();
    return;
  }

  if (event->key() == Qt::Key_Space) {
    QWidget *fw = QApplication::focusWidget();
    if (!qobject_cast<QAbstractSpinBox *>(fw) &&
        !qobject_cast<QLineEdit *>(fw)) {
      if (m_previewPlayer &&
          m_previewPlayer->status() == PlayerStatus::Playing) {
        onPauseClicked();
      } else {
        onPlayClicked();
      }
      event->accept();
      return;
    }
  }

  QWidget::keyPressEvent(event);
}

void MixEditor::onPlayClicked() {
  if (!m_fileExists || !m_previewPlayer) {
    return;
  }
  const PlayerStatus st = m_previewPlayer->status();
  if (st == PlayerStatus::Playing) {
    return;
  }
  if (st == PlayerStatus::Paused && !m_seekedWhileStopped) {
    m_previewPlayer->play();
  } else {
    m_previewPlayer->playFrom(m_playheadMs);
  }
  m_seekedWhileStopped = false;
}

void MixEditor::onPauseClicked() {
  if (m_previewPlayer) {
    m_previewPlayer->pause();
  }
}

void MixEditor::onStopClicked() {
  if (m_previewPlayer) {
    m_previewPlayer->stop();
  }
  m_playheadMs = m_metadata.startPoint;
  m_seekedWhileStopped = false;
  m_lastDurationForRanges = -1;
  m_waveformWidget->setPlayheadMs(m_playheadMs);
  updatePositionLabel();
}

void MixEditor::onSaveClicked() {
  if (saveToPlaylist()) {
    if (m_previewPlayer) {
      m_previewPlayer->stop();
    }
    close();
    return;
  }

  auto *box = new QMessageBox(
      QMessageBox::Warning, tr("Item indisponível"),
      tr("Não foi possível salvar: o item foi removido da playlist ou já foi "
         "finalizado."),
      QMessageBox::Ok, this);
  box->setAttribute(Qt::WA_DeleteOnClose);
  box->setWindowModality(Qt::WindowModal);
  box->open();
}

void MixEditor::onResetClicked() {
  const qint64 dur =
      (m_metadata.duration > 0) ? m_metadata.duration : m_metadata.endPoint;
  m_metadata = PlayerMetadata{};
  m_metadata.duration = dur;
  m_metadata.startPoint = 0;
  m_metadata.endPoint = dur;
  m_metadata.mixStart = 0;
  m_metadata.mixEnd = dur;
  m_metadata.fadeInEndPoint = 0;
  m_metadata.fadeOutStartPoint = dur;
  m_metadata.startVolume = 1.0f;
  m_metadata.endVolume = 1.0f;

  normalizeMetadataInPlace();
  syncControlsFromMetadata(true);
  if (m_previewPlayer) {
    m_previewPlayer->setMetadata(m_metadata);
  }
}

void MixEditor::onWaveformMetadataChanged(const PlayerMetadata &metadata) {
  if (m_syncingControls) {
    return;
  }
  m_metadata = metadata;
  normalizeMetadataInPlace();
  syncControlsFromMetadata(false);
  if (m_previewPlayer) {
    m_previewPlayer->setMetadata(m_metadata);
  }
}

void MixEditor::onWaveformZoomChanged(double factor, qint64 scrollOffsetMs,
                                      qint64 visibleDurationMs,
                                      qint64 totalDurationMs) {
  const int pct = static_cast<int>(std::llround(factor * 100.0));
  ui->btnZoomReset->setText(QStringLiteral("%1%").arg(pct));

  const QSignalBlocker blocker(ui->waveformScrollBar);
  const qint64 maxScroll = qMax<qint64>(0, totalDurationMs - visibleDurationMs);
  if (maxScroll <= 0 || factor <= 1.001) {
    ui->waveformScrollBar->setEnabled(false);
    ui->waveformScrollBar->setRange(0, 0);
    ui->waveformScrollBar->setValue(0);
  } else {
    ui->waveformScrollBar->setEnabled(true);
    ui->waveformScrollBar->setRange(0, static_cast<int>(maxScroll));
    ui->waveformScrollBar->setPageStep(static_cast<int>(visibleDurationMs));
    ui->waveformScrollBar->setSingleStep(
        qMax(10, static_cast<int>(visibleDurationMs / 10)));
    ui->waveformScrollBar->setValue(static_cast<int>(scrollOffsetMs));
  }
}

void MixEditor::onScrollBarValueChanged(int value) {
  m_waveformWidget->setScrollOffsetMs(static_cast<qint64>(value));
}

void MixEditor::onPreviewStatusChanged(PlayerStatus status) {
  if (status == PlayerStatus::Playing) {
    m_playheadTimer->start();
  } else {
    m_playheadTimer->stop();
  }

  if (status == PlayerStatus::End) {
    m_playheadMs = m_metadata.startPoint;
    m_seekedWhileStopped = false;
    m_waveformWidget->setPlayheadMs(m_playheadMs);
    updatePositionLabel();
  } else if (status == PlayerStatus::Error) {
    m_waveformWidget->setStatusMessage(
        tr("Não foi possível carregar o áudio para pré-escuta."));
  }

  updateTransportButtons();
}

void MixEditor::onPreviewPositionChanged(qint64 positionMs) {
  if (m_previewPlayer && m_previewPlayer->status() == PlayerStatus::Playing) {
    m_playheadMs = positionMs;
    m_waveformWidget->setPlayheadMs(positionMs);
    updatePositionLabel();
  }
}

void MixEditor::onPreviewDurationLoaded(qint64 durationMs) {
  applyDurationUpdate(durationMs);
}

void MixEditor::onPlayheadTimerTimeout() {
  if (!m_previewPlayer || m_previewPlayer->status() != PlayerStatus::Playing) {
    m_playheadTimer->stop();
    return;
  }
  const qint64 pos = m_previewPlayer->position();
  if (pos == m_playheadMs) {
    return;
  }
  m_playheadMs = pos;
  m_waveformWidget->setPlayheadMs(pos);
  updatePositionLabel();
}

void MixEditor::onWaveformChunk(const QVector<WaveformSample> &delta,
                                qint64 durationMs) {
  const qint64 effectiveDur =
      (durationMs > 0) ? durationMs : m_metadata.duration;
  m_waveformWidget->appendWaveformSamples(
      delta, effectiveDur, m_waveformExtractor->bucketDurationMs());
}

void MixEditor::onWaveformFinished(const QVector<WaveformSample> &samples,
                                   qint64 durationMs) {
  if (durationMs > 0) {
    applyDurationUpdate(durationMs);
  }
  const qint64 effectiveDur =
      (m_metadata.duration > 0) ? m_metadata.duration : durationMs;
  m_waveformWidget->setWaveformData(samples, effectiveDur,
                                    m_waveformExtractor->bucketDurationMs());
  m_waveformWidget->setMetadata(m_metadata);
}

void MixEditor::onWaveformError(const QString &errorMessage) {
  m_waveformWidget->clearWaveform(errorMessage);
}

void MixEditor::applyDurationUpdate(qint64 durationMs) {
  if (durationMs <= 0) {
    return;
  }

  const bool wasUnknown = (m_metadata.duration <= 0);
  const qint64 oldEnd = m_metadata.endPoint;
  const qint64 oldMixEnd = m_metadata.mixEnd;
  const qint64 oldFadeOut = m_metadata.fadeOutStartPoint;

  m_metadata.duration = durationMs;
  if (wasUnknown || oldEnd <= 0 || oldEnd > durationMs) {
    m_metadata.endPoint = durationMs;
  }
  if (wasUnknown || oldMixEnd <= m_metadata.startPoint || oldMixEnd == oldEnd ||
      oldMixEnd > m_metadata.endPoint) {
    if (oldMixEnd <= m_metadata.startPoint || oldMixEnd == oldEnd) {
      m_metadata.mixEnd = m_metadata.endPoint;
    } else {
      m_metadata.mixEnd = qMin(oldMixEnd, m_metadata.endPoint);
    }
  }
  if (wasUnknown || oldFadeOut <= 0 || oldFadeOut == oldEnd ||
      oldFadeOut > m_metadata.endPoint) {
    if (oldFadeOut <= 0 || oldFadeOut == oldEnd) {
      m_metadata.fadeOutStartPoint = m_metadata.endPoint;
    } else {
      m_metadata.fadeOutStartPoint = qMin(oldFadeOut, m_metadata.endPoint);
    }
  }

  normalizeMetadataInPlace();
  m_waveformWidget->setDurationMs(m_metadata.duration);
  syncControlsFromMetadata(true);
  if (m_previewPlayer) {
    m_previewPlayer->setMetadata(m_metadata);
  }
}

bool MixEditor::hasIntroEnvelope() const {
  return (m_metadata.startVolume < 0.999f) ||
         (m_metadata.volumeUpStartPoint > m_metadata.fadeInEndPoint) ||
         (m_metadata.volumeUpEndPoint > m_metadata.fadeInEndPoint);
}

bool MixEditor::hasOutroEnvelope() const {
  return (m_metadata.endVolume < 0.999f) ||
         (m_metadata.volumeDownStartPoint > m_metadata.startPoint &&
          m_metadata.volumeDownStartPoint < m_metadata.fadeOutStartPoint) ||
         (m_metadata.volumeDownEndPoint > m_metadata.startPoint &&
          m_metadata.volumeDownEndPoint < m_metadata.fadeOutStartPoint);
}

void MixEditor::normalizeMetadataInPlace() {
  const qint64 maxDur =
      (m_metadata.duration > 0) ? m_metadata.duration : m_metadata.endPoint;

  m_metadata.startVolume = std::clamp(m_metadata.startVolume, 0.0f, 1.0f);
  m_metadata.endVolume = std::clamp(m_metadata.endVolume, 0.0f, 1.0f);

  if (maxDur > 0) {
    if (m_metadata.endPoint <= 0 || m_metadata.endPoint > maxDur) {
      m_metadata.endPoint = maxDur;
    }
    m_metadata.startPoint =
        qBound<qint64>(0, m_metadata.startPoint, m_metadata.endPoint);
    m_metadata.mixStart = qBound<qint64>(
        m_metadata.startPoint, m_metadata.mixStart, m_metadata.endPoint);
    if (m_metadata.mixEnd <= m_metadata.startPoint ||
        m_metadata.mixEnd > m_metadata.endPoint) {
      m_metadata.mixEnd = m_metadata.endPoint;
    }
    m_metadata.fadeInEndPoint = qBound<qint64>(
        m_metadata.startPoint, m_metadata.fadeInEndPoint, m_metadata.endPoint);
    if (m_metadata.fadeOutStartPoint <= 0 ||
        m_metadata.fadeOutStartPoint > m_metadata.endPoint) {
      m_metadata.fadeOutStartPoint = m_metadata.endPoint;
    } else {
      m_metadata.fadeOutStartPoint =
          qBound<qint64>(m_metadata.fadeInEndPoint,
                         m_metadata.fadeOutStartPoint, m_metadata.endPoint);
    }

    if (m_metadata.volumeUpStartPoint > 0 || m_metadata.volumeUpEndPoint > 0) {
      m_metadata.volumeUpStartPoint = qBound<qint64>(
          m_metadata.fadeInEndPoint, m_metadata.volumeUpStartPoint,
          m_metadata.fadeOutStartPoint);
      m_metadata.volumeUpEndPoint = qBound<qint64>(
          m_metadata.volumeUpStartPoint, m_metadata.volumeUpEndPoint,
          m_metadata.fadeOutStartPoint);
    }

    if (m_metadata.volumeDownStartPoint > 0 ||
        m_metadata.volumeDownEndPoint > 0) {
      const qint64 minDown = (m_metadata.volumeUpEndPoint > 0)
                                 ? m_metadata.volumeUpEndPoint
                                 : m_metadata.fadeInEndPoint;
      m_metadata.volumeDownStartPoint = qBound<qint64>(
          qMin(minDown, m_metadata.fadeOutStartPoint),
          m_metadata.volumeDownStartPoint, m_metadata.fadeOutStartPoint);
      m_metadata.volumeDownEndPoint = qBound<qint64>(
          m_metadata.volumeDownStartPoint, m_metadata.volumeDownEndPoint,
          m_metadata.fadeOutStartPoint);
    }
  } else {
    m_metadata.startPoint = qMax<qint64>(0, m_metadata.startPoint);
    m_metadata.mixStart =
        qMax<qint64>(m_metadata.startPoint, m_metadata.mixStart);
    m_metadata.fadeInEndPoint =
        qMax<qint64>(m_metadata.startPoint, m_metadata.fadeInEndPoint);
  }
}

void MixEditor::updateTimeEditRanges() {
  const qint64 dur =
      (m_metadata.duration > 0) ? m_metadata.duration : m_metadata.endPoint;
  if (dur == m_lastDurationForRanges) {
    return;
  }
  m_lastDurationForRanges = dur;

  const QString fmt = (dur >= 3600000) ? QStringLiteral("hh:mm:ss:zzz")
                                       : QStringLiteral("mm:ss:zzz");
  const QTime maxTime = (dur > 0) ? msToQTime(dur) : QTime(23, 59, 59, 999);
  const QTime minTime(0, 0, 0, 0);

  const QTimeEdit *edits[] = {ui->timeStartPoint, ui->timeMixStart,
                              ui->timeMixEnd,     ui->timeEndPoint,
                              ui->timeFadeIn,     ui->timeVolUpStart,
                              ui->timeVolUpEnd,   ui->timeVolDownStart,
                              ui->timeVolDownEnd, ui->timeFadeOut};
  for (const QTimeEdit *cTe : edits) {
    auto *te = const_cast<QTimeEdit *>(cTe);
    const QSignalBlocker blocker(te);
    if (te->displayFormat() != fmt) {
      te->setDisplayFormat(fmt);
    }
    te->setMinimumTime(minTime);
    te->setMaximumTime(maxTime);
  }

  ui->labelDuration->setText(msec2string(dur));
}

void MixEditor::syncControlsFromMetadata(bool updateWaveform) {
  m_syncingControls = true;

  updateTimeEditRanges();

  const qint64 dur =
      (m_metadata.duration > 0) ? m_metadata.duration : m_metadata.endPoint;
  const qint64 maxMs = (dur > 0) ? dur : 86399999;

  auto setEdit = [maxMs](QTimeEdit *te, qint64 valueMs) {
    const QTime target = msToQTime(qBound<qint64>(0, valueMs, maxMs));
    if (te->time() != target) {
      const QSignalBlocker blocker(te);
      te->setTime(target);
    }
  };

  setEdit(ui->timeStartPoint, m_metadata.startPoint);
  setEdit(ui->timeMixStart, m_metadata.mixStart);
  setEdit(ui->timeMixEnd, m_metadata.mixEnd);
  setEdit(ui->timeEndPoint, m_metadata.endPoint);
  setEdit(ui->timeFadeIn, m_metadata.fadeInEndPoint);
  setEdit(ui->timeVolUpStart, m_metadata.volumeUpStartPoint);
  setEdit(ui->timeVolUpEnd, m_metadata.volumeUpEndPoint);
  setEdit(ui->timeVolDownStart, m_metadata.volumeDownStartPoint);
  setEdit(ui->timeVolDownEnd, m_metadata.volumeDownEndPoint);
  setEdit(ui->timeFadeOut, m_metadata.fadeOutStartPoint);

  const int startPct =
      static_cast<int>(std::llround(m_metadata.startVolume * 100.0f));
  const int endPct =
      static_cast<int>(std::llround(m_metadata.endVolume * 100.0f));
  if (ui->sliderVolStart->value() != startPct) {
    const QSignalBlocker b1(ui->sliderVolStart);
    ui->sliderVolStart->setValue(startPct);
    ui->textVolStart->setText(QStringLiteral("%1%").arg(startPct));
  }

  if (ui->sliderVolEnd->value() != endPct) {
    const QSignalBlocker b2(ui->sliderVolEnd);
    ui->sliderVolEnd->setValue(endPct);
    ui->textVolEnd->setText(QStringLiteral("%1%").arg(endPct));
  }

  updatePositionLabel();

  if (updateWaveform && m_waveformWidget) {
    m_waveformWidget->setMetadata(m_metadata);
  }

  m_syncingControls = false;
}

void MixEditor::onTimeFieldEdited() {
  if (m_syncingControls) {
    return;
  }

  QObject *changedObj = sender();
  const qint64 dur =
      (m_metadata.duration > 0) ? m_metadata.duration : m_metadata.endPoint;
  const bool introEnv = hasIntroEnvelope();
  const bool outroEnv = hasOutroEnvelope();

  if (changedObj == ui->timeStartPoint) {
    m_metadata.startPoint = qTimeToMs(ui->timeStartPoint->time());
    if (dur > 0) {
      m_metadata.startPoint =
          qBound<qint64>(0, m_metadata.startPoint, m_metadata.endPoint);
    }
    m_metadata.mixStart = qMax(m_metadata.mixStart, m_metadata.startPoint);
    m_metadata.fadeInEndPoint =
        qMax(m_metadata.fadeInEndPoint, m_metadata.startPoint);
  } else if (changedObj == ui->timeMixStart) {
    m_metadata.mixStart = qTimeToMs(ui->timeMixStart->time());
  } else if (changedObj == ui->timeMixEnd) {
    m_metadata.mixEnd = qTimeToMs(ui->timeMixEnd->time());
  } else if (changedObj == ui->timeEndPoint) {
    m_metadata.endPoint = qTimeToMs(ui->timeEndPoint->time());
    if (dur > 0) {
      m_metadata.endPoint =
          qBound<qint64>(m_metadata.startPoint, m_metadata.endPoint, dur);
    }
    m_metadata.mixStart = qMin(m_metadata.mixStart, m_metadata.endPoint);
    m_metadata.mixEnd = qMin(m_metadata.mixEnd, m_metadata.endPoint);
    m_metadata.fadeInEndPoint =
        qMin(m_metadata.fadeInEndPoint, m_metadata.endPoint);
    m_metadata.fadeOutStartPoint =
        qMin(m_metadata.fadeOutStartPoint, m_metadata.endPoint);
  } else if (changedObj == ui->timeFadeIn) {
    const qint64 rightWall = outroEnv ? m_metadata.volumeDownStartPoint
                                      : m_metadata.fadeOutStartPoint;
    m_metadata.fadeInEndPoint =
        qBound<qint64>(m_metadata.startPoint, qTimeToMs(ui->timeFadeIn->time()),
                       qMax(m_metadata.startPoint, rightWall));
    if (m_metadata.volumeUpStartPoint > 0 || m_metadata.volumeUpEndPoint > 0) {
      m_metadata.volumeUpStartPoint =
          qMax(m_metadata.volumeUpStartPoint, m_metadata.fadeInEndPoint);
      m_metadata.volumeUpEndPoint =
          qMax(m_metadata.volumeUpEndPoint, m_metadata.volumeUpStartPoint);
    }
  } else if (changedObj == ui->timeVolUpStart) {
    const qint64 rightWall = outroEnv ? m_metadata.volumeDownStartPoint
                                      : m_metadata.fadeOutStartPoint;
    m_metadata.volumeUpStartPoint = qBound<qint64>(
        m_metadata.fadeInEndPoint, qTimeToMs(ui->timeVolUpStart->time()),
        qMax(m_metadata.fadeInEndPoint, rightWall));
    m_metadata.volumeUpEndPoint =
        qMax(m_metadata.volumeUpEndPoint, m_metadata.volumeUpStartPoint);
  } else if (changedObj == ui->timeVolUpEnd) {
    const qint64 rightWall = outroEnv ? m_metadata.volumeDownStartPoint
                                      : m_metadata.fadeOutStartPoint;
    m_metadata.volumeUpEndPoint = qBound<qint64>(
        m_metadata.fadeInEndPoint, qTimeToMs(ui->timeVolUpEnd->time()),
        qMax(m_metadata.fadeInEndPoint, rightWall));
    m_metadata.volumeUpStartPoint =
        qBound<qint64>(m_metadata.fadeInEndPoint, m_metadata.volumeUpStartPoint,
                       m_metadata.volumeUpEndPoint);
  } else if (changedObj == ui->timeVolDownStart) {
    const qint64 leftWall =
        introEnv ? m_metadata.volumeUpEndPoint : m_metadata.fadeInEndPoint;
    m_metadata.volumeDownStartPoint =
        qBound<qint64>(leftWall, qTimeToMs(ui->timeVolDownStart->time()),
                       qMax(leftWall, m_metadata.fadeOutStartPoint));
    m_metadata.volumeDownEndPoint =
        qMax(m_metadata.volumeDownEndPoint, m_metadata.volumeDownStartPoint);
  } else if (changedObj == ui->timeVolDownEnd) {
    const qint64 leftWall =
        introEnv ? m_metadata.volumeUpEndPoint : m_metadata.fadeInEndPoint;
    m_metadata.volumeDownEndPoint =
        qBound<qint64>(leftWall, qTimeToMs(ui->timeVolDownEnd->time()),
                       qMax(leftWall, m_metadata.fadeOutStartPoint));
    m_metadata.volumeDownStartPoint =
        qBound<qint64>(leftWall, m_metadata.volumeDownStartPoint,
                       m_metadata.volumeDownEndPoint);
  } else if (changedObj == ui->timeFadeOut) {
    const qint64 leftWall =
        introEnv ? m_metadata.volumeUpEndPoint : m_metadata.fadeInEndPoint;
    m_metadata.fadeOutStartPoint =
        qBound<qint64>(leftWall, qTimeToMs(ui->timeFadeOut->time()),
                       qMax(leftWall, m_metadata.endPoint));
    if (m_metadata.volumeDownStartPoint > 0 ||
        m_metadata.volumeDownEndPoint > 0) {
      m_metadata.volumeDownEndPoint =
          qMin(m_metadata.volumeDownEndPoint, m_metadata.fadeOutStartPoint);
      m_metadata.volumeDownStartPoint =
          qMin(m_metadata.volumeDownStartPoint, m_metadata.volumeDownEndPoint);
    }
  }

  normalizeMetadataInPlace();
  syncControlsFromMetadata(true);
  if (m_previewPlayer) {
    m_previewPlayer->setMetadata(m_metadata);
  }
}

void MixEditor::onStartVolumeSliderChanged(int value) {
  if (m_syncingControls) {
    return;
  }

  m_metadata.startVolume =
      std::clamp(static_cast<float>(value) / 100.0f, 0.0f, 1.0f);

  const qint64 dur =
      (m_metadata.duration > 0) ? m_metadata.duration : m_metadata.endPoint;
  if (value < 100 && dur > 0 && m_metadata.volumeUpStartPoint == 0 &&
      m_metadata.volumeUpEndPoint == 0) {
    const qint64 base = qMax(m_metadata.fadeInEndPoint, m_metadata.startPoint);
    qint64 maxCeiling =
        (m_metadata.volumeDownStartPoint > 0 &&
         m_metadata.volumeDownStartPoint < m_metadata.fadeOutStartPoint)
            ? m_metadata.volumeDownStartPoint
            : m_metadata.fadeOutStartPoint;
    if (maxCeiling <= base) {
      maxCeiling = (m_metadata.endPoint > base) ? m_metadata.endPoint : dur;
    }
    const qint64 avail = qMax<qint64>(0, maxCeiling - base);
    if (avail > 0) {
      const qint64 hold = qMin<qint64>(kDefaultEnvelopeRampMs, avail / 3);
      const qint64 ramp =
          qMin<qint64>(kDefaultEnvelopeRampMs, qMax<qint64>(0, avail - hold));
      m_metadata.volumeUpStartPoint = base + hold;
      m_metadata.volumeUpEndPoint = m_metadata.volumeUpStartPoint + ramp;
    }
  }

  normalizeMetadataInPlace();
  syncControlsFromMetadata(true);
  if (m_previewPlayer) {
    m_previewPlayer->setMetadata(m_metadata);
  }
}

void MixEditor::onEndVolumeSliderChanged(int value) {
  if (m_syncingControls) {
    return;
  }

  m_metadata.endVolume =
      std::clamp(static_cast<float>(value) / 100.0f, 0.0f, 1.0f);

  const qint64 dur =
      (m_metadata.duration > 0) ? m_metadata.duration : m_metadata.endPoint;
  if (value < 100 && dur > 0 && m_metadata.volumeDownStartPoint == 0 &&
      m_metadata.volumeDownEndPoint == 0) {
    const qint64 rightBound =
        (m_metadata.fadeOutStartPoint > m_metadata.startPoint)
            ? m_metadata.fadeOutStartPoint
            : m_metadata.endPoint;
    const qint64 minFloor =
        (m_metadata.volumeUpEndPoint > m_metadata.fadeInEndPoint)
            ? m_metadata.volumeUpEndPoint
            : m_metadata.fadeInEndPoint;
    const qint64 avail = qMax<qint64>(0, rightBound - minFloor);
    if (avail > 0) {
      const qint64 hold = qMin<qint64>(kDefaultEnvelopeRampMs, avail / 3);
      const qint64 ramp =
          qMin<qint64>(kDefaultEnvelopeRampMs, qMax<qint64>(0, avail - hold));
      m_metadata.volumeDownEndPoint = rightBound - hold;
      m_metadata.volumeDownStartPoint = m_metadata.volumeDownEndPoint - ramp;
    }
  }

  normalizeMetadataInPlace();
  syncControlsFromMetadata(true);
  if (m_previewPlayer) {
    m_previewPlayer->setMetadata(m_metadata);
  }
}

void MixEditor::markFieldAtPlayhead(int fieldId) {
  const qint64 pos = m_playheadMs;
  switch (fieldId) {
  case FieldStartPoint:
    ui->timeStartPoint->setTime(msToQTime(pos));
    break;
  case FieldMixStart:
    ui->timeMixStart->setTime(msToQTime(pos));
    break;
  case FieldMixEnd:
    ui->timeMixEnd->setTime(msToQTime(pos));
    break;
  case FieldEndPoint:
    ui->timeEndPoint->setTime(msToQTime(pos));
    break;
  case FieldFadeIn:
    ui->timeFadeIn->setTime(msToQTime(pos));
    break;
  case FieldVolUpStart:
    ui->timeVolUpStart->setTime(msToQTime(pos));
    break;
  case FieldVolUpEnd:
    ui->timeVolUpEnd->setTime(msToQTime(pos));
    break;
  case FieldVolDownStart:
    ui->timeVolDownStart->setTime(msToQTime(pos));
    break;
  case FieldVolDownEnd:
    ui->timeVolDownEnd->setTime(msToQTime(pos));
    break;
  case FieldFadeOut:
    ui->timeFadeOut->setTime(msToQTime(pos));
    break;
  default:
    break;
  }
}

void MixEditor::updatePositionLabel() {
  const qint64 dur =
      (m_metadata.duration > 0) ? m_metadata.duration : m_metadata.endPoint;
  ui->labelPosition->setText(QStringLiteral("%1 / %2").arg(
      msec2string(m_playheadMs), msec2string(dur)));
}

void MixEditor::updateTransportButtons() {
  if (!m_fileExists || !m_previewPlayer) {
    ui->btnPlay->setEnabled(false);
    ui->btnPause->setEnabled(false);
    ui->btnStop->setEnabled(false);
    return;
  }

  switch (m_previewPlayer->status()) {
  case PlayerStatus::Playing:
    ui->btnPlay->setEnabled(false);
    ui->btnPause->setEnabled(true);
    ui->btnStop->setEnabled(true);
    break;
  case PlayerStatus::Paused:
    ui->btnPlay->setEnabled(true);
    ui->btnPause->setEnabled(false);
    ui->btnStop->setEnabled(true);
    break;
  case PlayerStatus::Error:
  case PlayerStatus::Idle:
    ui->btnPlay->setEnabled(false);
    ui->btnPause->setEnabled(false);
    ui->btnStop->setEnabled(false);
    break;
  case PlayerStatus::Loading:
  case PlayerStatus::Loaded:
  case PlayerStatus::End:
    ui->btnPlay->setEnabled(true);
    ui->btnPause->setEnabled(false);
    ui->btnStop->setEnabled(false);
    break;
  }
}
