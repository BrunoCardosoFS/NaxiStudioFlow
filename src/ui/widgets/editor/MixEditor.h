#ifndef MIXEDITOR_H
#define MIXEDITOR_H

#include <QAudioDevice>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QPointer>
#include <QTimer>
#include <QUuid>
#include <QWidget>

#include <functional>

#include "core/audio/WaveformExtractor.h"
#include "core/playlist/PlaylistListModel.h"
#include "core/playlist/player/PlayerManager.h"

namespace Ui {
class MixEditor;
}

class WaveformWidget;

class MixEditor : public QWidget {
  Q_OBJECT

public:
  using MediaPathResolverFn = std::function<QString(const QString &path)>;

  explicit MixEditor(QWidget *parent = nullptr);
  ~MixEditor() override;

  void setMediaPathResolver(MediaPathResolverFn resolver);
  bool loadPlaylistItem(PlaylistListModel *model, const QUuid &itemUuid);
  const PlayerMetadata &currentMetadata() const { return m_metadata; }
  QUuid currentItemUuid() const { return m_itemUuid; }
  WaveformWidget *waveformWidget() const { return m_waveformWidget; }

  bool saveToPlaylist();
  void setAudioOutputDevice(const QAudioDevice &device);

signals:
  void savedLocally(const QUuid &itemUuid, const PlayerMetadata &metadata);

protected:
  void closeEvent(QCloseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

private slots:
  void onPlayClicked();
  void onPauseClicked();
  void onStopClicked();
  void onSaveClicked();
  void onResetClicked();

  void onWaveformMetadataChanged(const PlayerMetadata &metadata);
  void onWaveformZoomChanged(double factor, qint64 scrollOffsetMs,
                             qint64 visibleDurationMs, qint64 totalDurationMs);
  void onScrollBarValueChanged(int value);

  void onPreviewStatusChanged(PlayerStatus status);
  void onPreviewPositionChanged(qint64 positionMs);
  void onPreviewDurationLoaded(qint64 durationMs);
  void onPlayheadTimerTimeout();

  void onWaveformChunk(const QVector<WaveformSample> &delta, qint64 durationMs);
  void onWaveformFinished(const QVector<WaveformSample> &samples,
                          qint64 durationMs);
  void onWaveformError(const QString &errorMessage);

private:
  void initConnections();
  void updateTimeEditRanges();
  void syncControlsFromMetadata(bool updateWaveform = true);
  void applyDurationUpdate(qint64 durationMs);
  void normalizeMetadataInPlace();
  void updatePositionLabel();
  void updateTransportButtons();

  bool hasIntroEnvelope() const;
  bool hasOutroEnvelope() const;
  void onTimeFieldEdited();
  void onStartVolumeSliderChanged(int value);
  void onEndVolumeSliderChanged(int value);
  void markFieldAtPlayhead(int fieldId);

  Ui::MixEditor *ui;
  WaveformWidget *m_waveformWidget = nullptr;
  WaveformExtractor *m_waveformExtractor = nullptr;
  PlayerManager *m_previewPlayer = nullptr;
  QTimer *m_playheadTimer = nullptr;

  QPointer<PlaylistListModel> m_playlistModel;
  QUuid m_itemUuid;
  QString m_filePath;
  MediaPathResolverFn m_mediaPathResolver;
  PlayerMetadata m_metadata;
  qint64 m_playheadMs = 0;

  bool m_fileExists = false;
  bool m_seekedWhileStopped = false;
  bool m_syncingControls = false;
  qint64 m_lastDurationForRanges = -1;
};

#endif // MIXEDITOR_H
