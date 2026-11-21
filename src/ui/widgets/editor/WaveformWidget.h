#ifndef WAVEFORMWIDGET_H
#define WAVEFORMWIDGET_H

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QMap>
#include <QPen>
#include <QRect>
#include <QString>
#include <QVector>
#include <QWidget>

#include "core/audio/WaveformExtractor.h"
#include "core/playlist/player/PlayerManager.h"

class WaveformWidget : public QWidget {
  Q_OBJECT
public:
  static constexpr double kMinZoom = 1.0;
  static constexpr double kMaxZoom = 32.0;

  enum class MarkerId {
    None,
    StartPoint,
    MixStart,
    FadeInEnd,
    VolUpStart,
    VolUpEnd,
    VolDownStart,
    VolDownEnd,
    FadeOutStart,
    MixEnd,
    EndPoint
  };

  explicit WaveformWidget(QWidget *parent = nullptr);
  ~WaveformWidget() override = default;

  void setWaveformData(const QVector<WaveformSample> &samples,
                       qint64 durationMs,
                       qint64 bucketDurationMs = WaveformExtractor::kDefaultBucketDurationMs);
  void clearWaveform(const QString &statusMessage = QString());
  void setStatusMessage(const QString &message);
  void setDurationMs(qint64 durationMs);

  void setMetadata(const PlayerMetadata &metadata);
  const PlayerMetadata &metadata() const { return m_metadata; }

  void setPlayheadMs(qint64 positionMs);
  qint64 playheadMs() const { return m_playheadMs; }

  void setZoomFactor(double factor);
  double zoomFactor() const { return m_zoomFactor; }
  void zoomIn();
  void zoomOut();
  void resetZoom();

  void setScrollOffsetMs(qint64 offsetMs);
  qint64 scrollOffsetMs() const { return m_scrollOffsetMs; }
  qint64 visibleDurationMs() const;
  qint64 totalDurationMs() const;

  // Exposed for testing and exact coordinate mapping.
  int xForTime(qint64 ms) const;
  qint64 timeForX(int x) const;
  MarkerId hitTestMarker(const QPoint &pos) const;

signals:
  void metadataChanged(const PlayerMetadata &metadata);
  void seekRequested(qint64 positionMs);
  void playFromRequested(qint64 positionMs);
  void zoomChanged(double factor, qint64 scrollOffsetMs,
                   qint64 visibleDurationMs, qint64 totalDurationMs);

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void leaveEvent(QEvent *event) override;

private:
  QRect rulerRect() const;
  QRect waveRect() const;
  QRect envelopeRect() const;
  int yForVolume(float vol) const;
  float volumeForY(int y) const;

  bool hasIntroEnvelope() const;
  bool hasOutroEnvelope() const;

  void drawRuler(QPainter *p, const QRect &rect) const;
  void drawWaveformBars(QPainter *p, const QRect &rect) const;
  void drawTrimShading(QPainter *p, const QRect &rect) const;
  void drawVolumeEnvelope(QPainter *p, const QRect &rect) const;
  void drawMarkers(QPainter *p, const QRect &rect);
  void drawPlayhead(QPainter *p, const QRect &rRect, const QRect &wRect) const;

  void applyMarkerEdit(MarkerId marker, qint64 newTimeMs, float newVol);
  void updateZoomAndOffset(double newFactor, qint64 newOffsetMs);
  QString markerDisplayName(MarkerId marker) const;
  qint64 markerTime(MarkerId marker) const;

  QVector<WaveformSample> m_samples;
  qint64 m_bucketDurationMs = WaveformExtractor::kDefaultBucketDurationMs;
  qint64 m_durationMs = 0;
  PlayerMetadata m_metadata;
  qint64 m_playheadMs = 0;
  QString m_statusMessage;

  double m_zoomFactor = 1.0;
  qint64 m_scrollOffsetMs = 0;

  MarkerId m_activeDragMarker = MarkerId::None;
  MarkerId m_hoveredMarker = MarkerId::None;
  bool m_isPanning = false;
  bool m_isPotentialClickSeek = false;
  bool m_dragMoved = false;
  QPoint m_pressPos;
  int m_panLastX = 0;

  QMap<MarkerId, QRect> m_markerPillRects;
  QMap<MarkerId, QPoint> m_envelopeHandles;

  QFont m_rulerFont;
  QFont m_pillFont;
  QFont m_statusFont;
  QPen m_envelopePen;
  QPen m_playheadPen;
};

#endif // WAVEFORMWIDGET_H
