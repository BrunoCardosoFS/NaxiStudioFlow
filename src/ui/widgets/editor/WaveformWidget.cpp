#include "ui/widgets/editor/WaveformWidget.h"

#include <QLine>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygon>
#include <QToolTip>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

#include "core/utils/TimeUtils.h"

namespace {
constexpr int kRulerHeight = 24;
constexpr int kEnvelopeMarginTop = 10;
constexpr int kEnvelopeMarginBottom = 10;
constexpr int kWaveMarginTop = 18;
constexpr int kWaveMarginBottom = 18;
constexpr int kHitTolerancePx = 7;
constexpr int kHandleRadiusPx = 5;
constexpr int kHandleHitRadiusPx = 10;

const QColor kColRulerBg = QColor::fromRgb(0x191D24);
const QColor kColCanvasBg = QColor::fromRgb(0x1F232C);
const QColor kColBorder = QColor::fromRgb(0x2D3340);
const QColor kColTextMuted = QColor::fromRgb(0x8E9EB5);
const QColor kColWhite = QColor::fromRgb(0xFFFFFF);
const QColor kColWavePeak = QColor(0x41, 0x8D, 0xB4, 115);
const QColor kColWaveRms = QColor(0x41, 0x8D, 0xB4, 230);
const QColor kColTrimOverlay = QColor(0x0D, 0x10, 0x15, 165);
const QColor kColEnvelope = QColor::fromRgb(0xFFA60A);
const QColor kColEnvelopeFill = QColor(0xFF, 0xA6, 0x0A, 28);
const QColor kColPlayhead = QColor::fromRgb(0xFFFFFF);
const QColor kColPlayheadHead = QColor::fromRgb(0xE50914);

const QColor kColStartPoint = QColor::fromRgb(0x22C55E);
const QColor kColMixStart = QColor::fromRgb(0x06B6D4);
const QColor kColFadeIn = QColor::fromRgb(0x38BDF8);
const QColor kColFadeOut = QColor::fromRgb(0xA855F7);
const QColor kColMixEnd = QColor::fromRgb(0xF97316);
const QColor kColEndPoint = QColor::fromRgb(0xEF4444);

QString formatRulerTime(qint64 ms, qint64 stepMs) {
  if (ms < 0) {
    ms = 0;
  }
  const qint64 totalSec = ms / 1000;
  const qint64 hours = totalSec / 3600;
  const qint64 minutes = (totalSec / 60) % 60;
  const qint64 seconds = totalSec % 60;
  const qint64 tenths = (ms % 1000) / 100;

  if (hours > 0) {
    return QStringLiteral("%1:%2:%3")
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
  }
  if (stepMs < 1000) {
    return QStringLiteral("%1:%2.%3")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'))
        .arg(tenths);
  }
  return QStringLiteral("%1:%2")
      .arg(minutes, 2, 10, QLatin1Char('0'))
      .arg(seconds, 2, 10, QLatin1Char('0'));
}
} // namespace

WaveformWidget::WaveformWidget(QWidget *parent)
    : QWidget(parent), m_rulerFm(font()), m_pillFm(font()),
      m_envelopePen(kColEnvelope, 2.0), m_playheadPen(kColPlayhead, 1.5) {
  setMinimumHeight(210);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setMouseTracking(true);
  setFocusPolicy(Qt::ClickFocus);

  m_rulerFont = font();
  m_rulerFont.setPointSize(8);
  m_rulerFont.setFeature("tnum", 1);
  m_rulerFm = QFontMetrics(m_rulerFont);

  m_pillFont = font();
  m_pillFont.setPointSize(7);
  m_pillFont.setBold(true);
  m_pillFm = QFontMetrics(m_pillFont);

  m_statusFont = font();
  m_statusFont.setPointSize(10);
}

void WaveformWidget::setWaveformData(const QVector<WaveformSample> &samples,
                                     qint64 durationMs,
                                     qint64 bucketDurationMs) {
  m_samples = samples;
  m_bucketDurationMs = qMax<qint64>(1, bucketDurationMs);
  if (durationMs > 0) {
    m_durationMs = durationMs;
  } else if (m_durationMs <= 0 && !m_samples.isEmpty()) {
    m_durationMs = static_cast<qint64>(m_samples.size()) * m_bucketDurationMs;
  }
  m_statusMessage.clear();
  m_staticCacheDirty = true;
  m_envelopeDirty = true;
  updateZoomAndOffset(m_zoomFactor, m_scrollOffsetMs);
  update();
}

void WaveformWidget::appendWaveformSamples(const QVector<WaveformSample> &delta,
                                           qint64 durationMs,
                                           qint64 bucketDurationMs) {
  m_bucketDurationMs = qMax<qint64>(1, bucketDurationMs);
  if (!delta.isEmpty()) {
    m_samples.append(delta);
  }
  const qint64 prevDur = m_durationMs;
  if (durationMs > 0) {
    m_durationMs = durationMs;
  } else if (m_durationMs <= 0 && !m_samples.isEmpty()) {
    m_durationMs = static_cast<qint64>(m_samples.size()) * m_bucketDurationMs;
  }
  m_statusMessage.clear();
  m_staticCacheDirty = true;
  if (m_durationMs != prevDur) {
    m_envelopeDirty = true;
    updateZoomAndOffset(m_zoomFactor, m_scrollOffsetMs);
  } else {
    update();
  }
}

void WaveformWidget::clearWaveform(const QString &statusMessage) {
  m_samples.clear();
  m_statusMessage = statusMessage;
  m_staticCacheDirty = true;
  update();
}

void WaveformWidget::setStatusMessage(const QString &message) {
  m_statusMessage = message;
  m_staticCacheDirty = true;
  update();
}

void WaveformWidget::setDurationMs(qint64 durationMs) {
  if (durationMs < 0) {
    durationMs = 0;
  }
  if (m_durationMs == durationMs) {
    return;
  }
  m_durationMs = durationMs;
  m_staticCacheDirty = true;
  m_envelopeDirty = true;
  updateZoomAndOffset(m_zoomFactor, m_scrollOffsetMs);
  update();
}

void WaveformWidget::setMetadata(const PlayerMetadata &metadata) {
  m_metadata = metadata;
  if (m_metadata.duration > 0 && m_durationMs <= 0) {
    m_durationMs = m_metadata.duration;
    m_staticCacheDirty = true;
  }
  m_envelopeDirty = true;
  update();
}

void WaveformWidget::setPlayheadMs(qint64 positionMs) {
  const qint64 dur = totalDurationMs();
  const qint64 clamped = (dur > 0) ? qBound<qint64>(0, positionMs, dur)
                                   : qMax<qint64>(0, positionMs);
  if (m_playheadMs == clamped) {
    return;
  }
  const int oldX = xForTime(m_playheadMs);
  m_playheadMs = clamped;
  const int newX = xForTime(m_playheadMs);
  if (oldX != newX) {
    const QRect oldRect(oldX - 8, 0, 17, height());
    const QRect newRect(newX - 8, 0, 17, height());
    update(oldRect.united(newRect));
  }
}

qint64 WaveformWidget::totalDurationMs() const {
  if (m_durationMs > 0) {
    return m_durationMs;
  }
  if (m_metadata.duration > 0) {
    return m_metadata.duration;
  }
  if (m_metadata.endPoint > 0) {
    return m_metadata.endPoint;
  }
  return static_cast<qint64>(m_samples.size()) * m_bucketDurationMs;
}

qint64 WaveformWidget::visibleDurationMs() const {
  const qint64 total = totalDurationMs();
  if (total <= 0) {
    return 0;
  }
  return qMax<qint64>(100, static_cast<qint64>(std::llround(
                               static_cast<double>(total) / m_zoomFactor)));
}

void WaveformWidget::setZoomFactor(double factor) {
  const double clamped = std::clamp(factor, kMinZoom, kMaxZoom);
  const qint64 total = totalDurationMs();
  if (total <= 0) {
    m_zoomFactor = clamped;
    m_scrollOffsetMs = 0;
    emit zoomChanged(m_zoomFactor, 0, 0, 0);
    update();
    return;
  }

  const qint64 oldVis = visibleDurationMs();
  const qint64 centerMs = m_scrollOffsetMs + oldVis / 2;
  const qint64 newVis = qMax<qint64>(
      100,
      static_cast<qint64>(std::llround(static_cast<double>(total) / clamped)));
  const qint64 newOffset = centerMs - newVis / 2;
  updateZoomAndOffset(clamped, newOffset);
}

void WaveformWidget::zoomIn() { setZoomFactor(m_zoomFactor * 1.5); }

void WaveformWidget::zoomOut() { setZoomFactor(m_zoomFactor / 1.5); }

void WaveformWidget::resetZoom() { updateZoomAndOffset(1.0, 0); }

void WaveformWidget::setScrollOffsetMs(qint64 offsetMs) {
  updateZoomAndOffset(m_zoomFactor, offsetMs);
}

void WaveformWidget::updateZoomAndOffset(double newFactor, qint64 newOffsetMs) {
  m_zoomFactor = std::clamp(newFactor, kMinZoom, kMaxZoom);
  const qint64 total = totalDurationMs();
  const qint64 vis = visibleDurationMs();
  const qint64 maxOff = qMax<qint64>(0, total - vis);
  m_scrollOffsetMs = (total > 0) ? qBound<qint64>(0, newOffsetMs, maxOff) : 0;

  m_staticCacheDirty = true;
  m_envelopeDirty = true;
  emit zoomChanged(m_zoomFactor, m_scrollOffsetMs, vis, total);
  update();
}

QRect WaveformWidget::rulerRect() const {
  return QRect(0, 0, width(), kRulerHeight);
}

QRect WaveformWidget::waveRect() const {
  return QRect(0, kRulerHeight, width(), qMax(1, height() - kRulerHeight));
}

QRect WaveformWidget::envelopeRect() const {
  const QRect wr = waveRect();
  return wr.adjusted(0, kEnvelopeMarginTop, 0, -kEnvelopeMarginBottom);
}

int WaveformWidget::xForTime(qint64 ms) const {
  const qint64 vis = visibleDurationMs();
  if (vis <= 0 || width() <= 1) {
    return 0;
  }
  const double ratio =
      static_cast<double>(ms - m_scrollOffsetMs) / static_cast<double>(vis);
  return static_cast<int>(
      std::llround(ratio * static_cast<double>(width() - 1)));
}

qint64 WaveformWidget::timeForX(int x) const {
  const qint64 total = totalDurationMs();
  const qint64 vis = visibleDurationMs();
  if (total <= 0 || vis <= 0 || width() <= 1) {
    return 0;
  }
  const double ratio =
      static_cast<double>(x) / static_cast<double>(width() - 1);
  const qint64 raw =
      m_scrollOffsetMs +
      static_cast<qint64>(std::llround(ratio * static_cast<double>(vis)));
  return qBound<qint64>(0, raw, total);
}

int WaveformWidget::yForVolume(float vol) const {
  const QRect er = envelopeRect();
  const float clamped = std::clamp(vol, 0.0f, 1.0f);
  return er.bottom() - static_cast<int>(std::llround(
                           clamped * static_cast<float>(er.height())));
}

float WaveformWidget::volumeForY(int y) const {
  const QRect er = envelopeRect();
  if (er.height() <= 0) {
    return 1.0f;
  }
  const float norm =
      static_cast<float>(er.bottom() - y) / static_cast<float>(er.height());
  return std::clamp(norm, 0.0f, 1.0f);
}

bool WaveformWidget::hasIntroEnvelope() const {
  return (m_metadata.startVolume < 0.999f) ||
         (m_metadata.volumeUpStartPoint > m_metadata.fadeInEndPoint) ||
         (m_metadata.volumeUpEndPoint > m_metadata.fadeInEndPoint);
}

bool WaveformWidget::hasOutroEnvelope() const {
  return (m_metadata.endVolume < 0.999f) ||
         (m_metadata.volumeDownStartPoint > m_metadata.startPoint &&
          m_metadata.volumeDownStartPoint < m_metadata.fadeOutStartPoint) ||
         (m_metadata.volumeDownEndPoint > m_metadata.startPoint &&
          m_metadata.volumeDownEndPoint < m_metadata.fadeOutStartPoint);
}

void WaveformWidget::resizeEvent(QResizeEvent *event) {
  m_staticCacheDirty = true;
  m_envelopeDirty = true;
  QWidget::resizeEvent(event);
}

void WaveformWidget::rebuildStaticCache() const {
  const qreal dpr = devicePixelRatioF();
  const int pw = qMax(1, static_cast<int>(std::ceil(width() * dpr)));
  const int ph = qMax(1, static_cast<int>(std::ceil(height() * dpr)));

  if (m_staticCache.size() != QSize(pw, ph) ||
      !qFuzzyCompare(m_staticCache.devicePixelRatio(), dpr)) {
    m_staticCache = QPixmap(pw, ph);
    m_staticCache.setDevicePixelRatio(dpr);
  }

  QPainter p(&m_staticCache);
  const QRect rRect = rulerRect();
  const QRect wRect = waveRect();

  p.fillRect(rRect, kColRulerBg);
  p.fillRect(wRect, kColCanvasBg);

  p.setPen(kColBorder);
  p.drawRect(rect().adjusted(0, 0, -1, -1));

  drawRuler(&p, rRect);
  drawWaveformBars(&p, wRect);

  m_staticCacheDirty = false;
}

void WaveformWidget::rebuildEnvelopeCache() const {
  m_envelopeCurvePath = QPainterPath();
  m_envelopeFillPath = QPainterPath();
  m_envelopeDirty = false;

  const qint64 total = totalDurationMs();
  if (total <= 0 || width() <= 1) {
    return;
  }

  const qint64 effectiveEnd =
      (m_metadata.endPoint > 0) ? m_metadata.endPoint : total;
  const int startPx = qBound(0, xForTime(m_metadata.startPoint), width() - 1);
  const int endPx = qBound(startPx, xForTime(effectiveEnd), width() - 1);
  if (endPx <= startPx) {
    return;
  }

  PlayerMetadata evalMeta = m_metadata;
  if (evalMeta.endPoint <= 0) {
    evalMeta.endPoint = effectiveEnd;
  }

  constexpr int step = 2;
  bool started = false;
  for (int px = startPx; px <= endPx; px += step) {
    const qint64 t =
        (px == endPx) ? qMax<qint64>(0, effectiveEnd - 1) : timeForX(px);
    const float factor = PlayerManager::computeVolumeFactor(
        evalMeta, qBound(evalMeta.startPoint, t, evalMeta.endPoint - 1));
    const int y = yForVolume(factor);
    if (!started) {
      m_envelopeCurvePath.moveTo(px, y);
      started = true;
    } else {
      m_envelopeCurvePath.lineTo(px, y);
    }
  }
  if ((endPx - startPx) % step != 0) {
    const float factor = PlayerManager::computeVolumeFactor(
        evalMeta, qMax(evalMeta.startPoint, evalMeta.endPoint - 1));
    m_envelopeCurvePath.lineTo(endPx, yForVolume(factor));
  }

  m_envelopeFillPath = m_envelopeCurvePath;
  const int baselineY = envelopeRect().bottom();
  m_envelopeFillPath.lineTo(endPx, baselineY);
  m_envelopeFillPath.lineTo(startPx, baselineY);
  m_envelopeFillPath.closeSubpath();
}

void WaveformWidget::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  const qreal dpr = devicePixelRatioF();
  const int expectedW = qMax(1, static_cast<int>(std::ceil(width() * dpr)));
  const int expectedH = qMax(1, static_cast<int>(std::ceil(height() * dpr)));
  if (m_staticCacheDirty ||
      m_staticCache.size() != QSize(expectedW, expectedH) ||
      !qFuzzyCompare(m_staticCache.devicePixelRatio(), dpr)) {
    rebuildStaticCache();
  }

  QPainter p(this);
  p.drawPixmap(0, 0, m_staticCache);

  const QRect rRect = rulerRect();
  const QRect wRect = waveRect();

  drawTrimShading(&p, wRect);
  drawVolumeEnvelope(&p, wRect);
  drawMarkers(&p, wRect);
  drawPlayhead(&p, rRect, wRect);
}

void WaveformWidget::drawRuler(QPainter *p, const QRect &rect) const {
  p->save();
  p->setPen(kColBorder);
  p->drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());

  const qint64 vis = visibleDurationMs();
  if (vis <= 0 || width() <= 0) {
    p->restore();
    return;
  }

  static constexpr qint64 kSteps[] = {100,    250,    500,    1000,   2000,
                                      5000,   10000,  15000,  30000,  60000,
                                      120000, 300000, 600000, 1200000};
  qint64 stepMs = 600000;
  const double pxPerMs =
      static_cast<double>(width()) / static_cast<double>(vis);
  for (qint64 candidate : kSteps) {
    if (static_cast<double>(candidate) * pxPerMs >= 75.0) {
      stepMs = candidate;
      break;
    }
  }

  const qint64 startTick = (m_scrollOffsetMs / stepMs) * stepMs;
  const qint64 endTick = m_scrollOffsetMs + vis;
  const qint64 total = totalDurationMs();

  p->setFont(m_rulerFont);

  for (qint64 t = startTick; t <= endTick && t <= total; t += stepMs) {
    const int x = xForTime(t);
    if (x < 0 || x >= width()) {
      continue;
    }
    p->setPen(kColBorder);
    p->drawLine(x, rect.bottom() - 6, x, rect.bottom());

    const QString label = formatRulerTime(t, stepMs);
    const int textW = m_rulerFm.horizontalAdvance(label);
    const int labelX = qBound(4, x + 4, qMax(4, width() - textW - 4));
    p->setPen(kColTextMuted);
    p->drawText(QRect(labelX, rect.top(), textW + 4, rect.height() - 4),
                Qt::AlignLeft | Qt::AlignVCenter, label);
  }
  p->restore();
}

void WaveformWidget::drawWaveformBars(QPainter *p, const QRect &rect) const {
  p->save();

  const QRect inner = rect.adjusted(0, kWaveMarginTop, 0, -kWaveMarginBottom);
  const int midY = inner.center().y();
  const int halfH = qMax(1, inner.height() / 2);

  p->setPen(kColBorder);
  p->drawLine(inner.left(), midY, inner.right(), midY);

  const qint64 vis = visibleDurationMs();
  const qint64 total = totalDurationMs();

  if (!m_samples.isEmpty() && vis > 0 && total > 0 && inner.width() > 0) {
    const int w = inner.width();
    const qint64 bucketMs = m_bucketDurationMs;
    const int numSamples = m_samples.size();
    const double msPerPx = static_cast<double>(vis) / static_cast<double>(w);

    QVector<QLine> peakLines;
    QVector<QLine> rmsLines;
    peakLines.reserve(w);
    rmsLines.reserve(w);

    for (int px = 0; px < w; ++px) {
      const qint64 t0 = m_scrollOffsetMs +
                        static_cast<qint64>(static_cast<double>(px) * msPerPx);
      const qint64 t1 =
          m_scrollOffsetMs +
          static_cast<qint64>(static_cast<double>(px + 1) * msPerPx);

      const int i0 = qBound(0, static_cast<int>(t0 / bucketMs), numSamples - 1);
      const int i1 =
          qBound(i0, static_cast<int>(t1 / bucketMs), numSamples - 1);

      float maxPeak = 0.0f;
      float maxRms = 0.0f;
      for (int i = i0; i <= i1; ++i) {
        const WaveformSample &s = m_samples.at(i);
        if (s.peak > maxPeak) {
          maxPeak = s.peak;
        }
        if (s.rms > maxRms) {
          maxRms = s.rms;
        }
      }

      const int x = inner.left() + px;
      const int peakH =
          qMax(1, static_cast<int>(std::llround(maxPeak * halfH)));
      peakLines.append(QLine(x, midY - peakH, x, midY + peakH));

      const int rmsH = static_cast<int>(std::llround(maxRms * halfH));
      if (rmsH > 0) {
        rmsLines.append(QLine(x, midY - rmsH, x, midY + rmsH));
      }
    }

    p->setPen(kColWavePeak);
    p->drawLines(peakLines);
    if (!rmsLines.isEmpty()) {
      p->setPen(kColWaveRms);
      p->drawLines(rmsLines);
    }
  }

  if (!m_statusMessage.isEmpty() && m_samples.isEmpty()) {
    p->setFont(m_statusFont);
    p->setPen(kColTextMuted);
    p->drawText(rect.adjusted(16, 8, -16, -8),
                Qt::AlignCenter | Qt::TextWordWrap, m_statusMessage);
  }

  p->restore();
}

void WaveformWidget::drawTrimShading(QPainter *p, const QRect &rect) const {
  const qint64 total = totalDurationMs();
  if (total <= 0) {
    return;
  }

  p->save();
  const int startX = xForTime(m_metadata.startPoint);
  const qint64 effectiveEnd =
      (m_metadata.endPoint > 0) ? m_metadata.endPoint : total;
  const int endX = xForTime(effectiveEnd);

  if (startX > rect.left()) {
    p->fillRect(QRect(rect.left(), rect.top(),
                      qMin(startX, rect.right() + 1) - rect.left(),
                      rect.height()),
                kColTrimOverlay);
  }
  if (endX < rect.right()) {
    const int left = qMax(rect.left(), endX);
    p->fillRect(QRect(left, rect.top(), rect.right() - left + 1, rect.height()),
                kColTrimOverlay);
  }
  p->restore();
}

void WaveformWidget::drawVolumeEnvelope(QPainter *p, const QRect &rect) const {
  Q_UNUSED(rect);
  if (m_envelopeDirty) {
    rebuildEnvelopeCache();
  }
  if (m_envelopeCurvePath.isEmpty()) {
    return;
  }

  p->save();
  p->setRenderHint(QPainter::Antialiasing, true);
  p->fillPath(m_envelopeFillPath, kColEnvelopeFill);
  p->setPen(m_envelopePen);
  p->drawPath(m_envelopeCurvePath);
  p->restore();
}

void WaveformWidget::drawMarkers(QPainter *p, const QRect &rect) {
  m_markerPillRects.clear();
  m_envelopeHandles.clear();

  const qint64 total = totalDurationMs();
  if (total <= 0) {
    return;
  }

  const qint64 effectiveEnd =
      (m_metadata.endPoint > 0) ? m_metadata.endPoint : total;
  const qint64 effectiveMixEnd =
      (m_metadata.mixEnd > 0) ? m_metadata.mixEnd : effectiveEnd;
  const qint64 effectiveFadeOut = (m_metadata.fadeOutStartPoint > 0)
                                      ? m_metadata.fadeOutStartPoint
                                      : effectiveEnd;

  struct MarkerSpec {
    MarkerId id;
    qint64 timeMs;
    QColor color;
    QString label;
    bool topPill;
  };

  const QVector<MarkerSpec> lines = {
      {MarkerId::StartPoint, m_metadata.startPoint, kColStartPoint,
       QStringLiteral("IN"), true},
      {MarkerId::MixStart, m_metadata.mixStart, kColMixStart,
       QStringLiteral("MIX IN"), false},
      {MarkerId::FadeInEnd, m_metadata.fadeInEndPoint, kColFadeIn,
       QStringLiteral("FI"), true},
      {MarkerId::FadeOutStart, effectiveFadeOut, kColFadeOut,
       QStringLiteral("FO"), true},
      {MarkerId::MixEnd, effectiveMixEnd, kColMixEnd, QStringLiteral("MIX OUT"),
       false},
      {MarkerId::EndPoint, effectiveEnd, kColEndPoint, QStringLiteral("OUT"),
       true},
  };

  p->save();
  p->setFont(m_pillFont);

  for (const auto &m : lines) {
    const int x = xForTime(m.timeMs);
    if (x < -30 || x > width() + 30) {
      continue;
    }

    const bool active = (m_activeDragMarker == m.id || m_hoveredMarker == m.id);
    QPen linePen(m.color, active ? 2.0 : 1.5);
    if (m.id == MarkerId::MixStart || m.id == MarkerId::MixEnd) {
      linePen.setStyle(Qt::DashLine);
    }
    p->setRenderHint(QPainter::Antialiasing, false);
    p->setPen(linePen);
    p->drawLine(x, rect.top(), x, rect.bottom());

    const int textW = m_pillFm.horizontalAdvance(m.label);
    const int pillW = textW + 10;
    const int pillH = 15;
    int pillX = x - pillW / 2;
    pillX = qBound(2, pillX, qMax(2, width() - pillW - 2));
    const int pillY =
        m.topPill ? (rect.top() + 3) : (rect.bottom() - pillH - 3);
    const QRect pillRect(pillX, pillY, pillW, pillH);

    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(active ? QPen(kColWhite, 1.2) : Qt::NoPen);
    p->setBrush(m.color);
    p->drawRoundedRect(pillRect, 4.0, 4.0);

    p->setPen(kColWhite);
    p->drawText(pillRect, Qt::AlignCenter, m.label);
    m_markerPillRects.insert(m.id, pillRect);
  }

  // Draggable volume envelope handles
  const bool introEnv = hasIntroEnvelope();
  const bool outroEnv = hasOutroEnvelope();
  const float introLvl =
      introEnv ? std::clamp(m_metadata.startVolume, 0.0f, 1.0f) : 1.0f;
  const float outroLvl =
      outroEnv ? std::clamp(m_metadata.endVolume, 0.0f, 1.0f) : 1.0f;

  auto registerHandle = [&](MarkerId id, qint64 ms, float vol,
                            const QColor &fill) {
    const int x = xForTime(ms);
    if (x < -12 || x > width() + 12) {
      return;
    }
    const int y = yForVolume(vol);
    const QPoint pt(x, y);
    m_envelopeHandles.insert(id, pt);

    const bool active = (m_activeDragMarker == id || m_hoveredMarker == id);
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(QPen(kColWhite, active ? 2.0 : 1.2));
    p->setBrush(fill);
    p->drawEllipse(pt, active ? kHandleRadiusPx + 1 : kHandleRadiusPx,
                   active ? kHandleRadiusPx + 1 : kHandleRadiusPx);
  };

  registerHandle(MarkerId::FadeInEnd, m_metadata.fadeInEndPoint, introLvl,
                 kColEnvelope);
  if (introEnv) {
    registerHandle(MarkerId::VolUpStart, m_metadata.volumeUpStartPoint,
                   introLvl, kColEnvelope);
    registerHandle(MarkerId::VolUpEnd, m_metadata.volumeUpEndPoint, 1.0f,
                   kColEnvelope);
  }
  if (outroEnv) {
    registerHandle(MarkerId::VolDownStart, m_metadata.volumeDownStartPoint,
                   1.0f, kColEnvelope);
    registerHandle(MarkerId::VolDownEnd, m_metadata.volumeDownEndPoint,
                   outroLvl, kColEnvelope);
  }
  registerHandle(MarkerId::FadeOutStart, effectiveFadeOut, outroLvl,
                 kColEnvelope);

  p->restore();
}

void WaveformWidget::drawPlayhead(QPainter *p, const QRect &rRect,
                                  const QRect &wRect) const {
  if (totalDurationMs() <= 0) {
    return;
  }
  const int x = xForTime(m_playheadMs);
  if (x < -8 || x > width() + 8) {
    return;
  }

  p->save();
  p->setRenderHint(QPainter::Antialiasing, false);
  p->setPen(m_playheadPen);
  p->drawLine(x, rRect.bottom(), x, wRect.bottom());

  p->setRenderHint(QPainter::Antialiasing, true);
  p->setPen(Qt::NoPen);
  p->setBrush(kColPlayheadHead);
  QPolygon tri;
  tri << QPoint(x - 5, rRect.bottom() - 8) << QPoint(x + 5, rRect.bottom() - 8)
      << QPoint(x, rRect.bottom());
  p->drawPolygon(tri);
  p->restore();
}

WaveformWidget::MarkerId
WaveformWidget::hitTestMarker(const QPoint &pos) const {
  if (totalDurationMs() <= 0) {
    return MarkerId::None;
  }

  for (auto it = m_envelopeHandles.constBegin();
       it != m_envelopeHandles.constEnd(); ++it) {
    const QPoint diff = pos - it.value();
    if (diff.x() * diff.x() + diff.y() * diff.y() <=
        kHandleHitRadiusPx * kHandleHitRadiusPx) {
      return it.key();
    }
  }

  for (auto it = m_markerPillRects.constBegin();
       it != m_markerPillRects.constEnd(); ++it) {
    if (it.value().adjusted(-4, -3, 4, 3).contains(pos)) {
      return it.key();
    }
  }

  if (pos.y() < kRulerHeight) {
    return MarkerId::None;
  }

  const qint64 effectiveEnd =
      (m_metadata.endPoint > 0) ? m_metadata.endPoint : totalDurationMs();
  const qint64 effectiveMixEnd =
      (m_metadata.mixEnd > 0) ? m_metadata.mixEnd : effectiveEnd;
  const qint64 effectiveFadeOut = (m_metadata.fadeOutStartPoint > 0)
                                      ? m_metadata.fadeOutStartPoint
                                      : effectiveEnd;

  const QVector<QPair<MarkerId, qint64>> candidates = {
      {MarkerId::StartPoint, m_metadata.startPoint},
      {MarkerId::MixStart, m_metadata.mixStart},
      {MarkerId::FadeInEnd, m_metadata.fadeInEndPoint},
      {MarkerId::FadeOutStart, effectiveFadeOut},
      {MarkerId::MixEnd, effectiveMixEnd},
      {MarkerId::EndPoint, effectiveEnd},
  };

  MarkerId best = MarkerId::None;
  int bestDist = kHitTolerancePx + 1;
  for (const auto &c : candidates) {
    const int dist = std::abs(pos.x() - xForTime(c.second));
    if (dist < bestDist) {
      bestDist = dist;
      best = c.first;
    }
  }
  return best;
}

void WaveformWidget::mousePressEvent(QMouseEvent *event) {
  if (totalDurationMs() <= 0) {
    QWidget::mousePressEvent(event);
    return;
  }

  m_pressPos = event->pos();
  m_dragMoved = false;

  if (event->button() == Qt::MiddleButton ||
      (event->button() == Qt::LeftButton &&
       (event->modifiers() & Qt::ShiftModifier))) {
    if (m_zoomFactor > 1.0) {
      m_isPanning = true;
      m_panLastX = event->pos().x();
      setCursor(Qt::ClosedHandCursor);
      event->accept();
      return;
    }
  }

  if (event->button() == Qt::LeftButton) {
    const MarkerId hit = hitTestMarker(event->pos());
    if (hit != MarkerId::None) {
      m_activeDragMarker = hit;
      setCursor(Qt::SizeHorCursor);
      update();
      event->accept();
      return;
    }

    m_isPotentialClickSeek = true;
    event->accept();
    return;
  }

  QWidget::mousePressEvent(event);
}

void WaveformWidget::mouseMoveEvent(QMouseEvent *event) {
  if (m_isPanning) {
    const int dx = event->pos().x() - m_panLastX;
    m_panLastX = event->pos().x();
    if (width() > 1) {
      const qint64 deltaMs = static_cast<qint64>(std::llround(
          (-static_cast<double>(dx) / static_cast<double>(width() - 1)) *
          static_cast<double>(visibleDurationMs())));
      updateZoomAndOffset(m_zoomFactor, m_scrollOffsetMs + deltaMs);
    }
    event->accept();
    return;
  }

  if (m_activeDragMarker != MarkerId::None) {
    m_dragMoved = true;
    const qint64 t = timeForX(event->pos().x());
    const float vol = volumeForY(event->pos().y());
    applyMarkerEdit(m_activeDragMarker, t, vol);

    const QString tip = QStringLiteral("%1: %2").arg(
        markerDisplayName(m_activeDragMarker),
        msec2string(markerTime(m_activeDragMarker)));
    QToolTip::showText(event->globalPosition().toPoint(), tip, this);
    event->accept();
    return;
  }

  const MarkerId hover = hitTestMarker(event->pos());
  if (hover != m_hoveredMarker) {
    m_hoveredMarker = hover;
    update();
  }

  if (hover != MarkerId::None) {
    const bool verticalCapable =
        (hover == MarkerId::VolUpStart || hover == MarkerId::VolDownEnd ||
         (hover == MarkerId::FadeInEnd && hasIntroEnvelope()) ||
         (hover == MarkerId::FadeOutStart && hasOutroEnvelope()));
    setCursor(verticalCapable ? Qt::SizeAllCursor : Qt::SizeHorCursor);
    const QString tip = QStringLiteral("%1: %2").arg(
        markerDisplayName(hover), msec2string(markerTime(hover)));
    setToolTip(tip);
  } else {
    setCursor(Qt::ArrowCursor);
    setToolTip(QString());
  }

  QWidget::mouseMoveEvent(event);
}

void WaveformWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (m_isPanning) {
    m_isPanning = false;
    setCursor(Qt::ArrowCursor);
    event->accept();
    return;
  }

  if (m_activeDragMarker != MarkerId::None) {
    m_activeDragMarker = MarkerId::None;
    setCursor(m_hoveredMarker != MarkerId::None ? Qt::SizeHorCursor
                                                : Qt::ArrowCursor);
    update();
    event->accept();
    return;
  }

  if (m_isPotentialClickSeek && event->button() == Qt::LeftButton) {
    m_isPotentialClickSeek = false;
    if ((event->pos() - m_pressPos).manhattanLength() < 6) {
      emit seekRequested(timeForX(event->pos().x()));
    }
    event->accept();
    return;
  }

  QWidget::mouseReleaseEvent(event);
}

void WaveformWidget::mouseDoubleClickEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton && totalDurationMs() > 0) {
    if (hitTestMarker(event->pos()) == MarkerId::None) {
      emit playFromRequested(timeForX(event->pos().x()));
      event->accept();
      return;
    }
  }
  QWidget::mouseDoubleClickEvent(event);
}

void WaveformWidget::wheelEvent(QWheelEvent *event) {
  const qint64 total = totalDurationMs();
  if (total <= 0) {
    QWidget::wheelEvent(event);
    return;
  }

  const int deltaY = event->angleDelta().y();
  if (deltaY == 0) {
    QWidget::wheelEvent(event);
    return;
  }

  const int mouseX = static_cast<int>(event->position().x());
  const qint64 anchorTime = timeForX(mouseX);
  const double ratioX = (width() > 1)
                            ? std::clamp(static_cast<double>(mouseX) /
                                             static_cast<double>(width() - 1),
                                         0.0, 1.0)
                            : 0.5;

  const double factor = (deltaY > 0) ? 1.25 : (1.0 / 1.25);
  const double newZoom = std::clamp(m_zoomFactor * factor, kMinZoom, kMaxZoom);
  const qint64 newVis = qMax<qint64>(
      100,
      static_cast<qint64>(std::llround(static_cast<double>(total) / newZoom)));
  const qint64 newOffset =
      anchorTime -
      static_cast<qint64>(std::llround(ratioX * static_cast<double>(newVis)));
  updateZoomAndOffset(newZoom, newOffset);
  event->accept();
}

void WaveformWidget::leaveEvent(QEvent *event) {
  if (m_hoveredMarker != MarkerId::None) {
    m_hoveredMarker = MarkerId::None;
    update();
  }
  if (m_activeDragMarker == MarkerId::None && !m_isPanning) {
    setCursor(Qt::ArrowCursor);
  }
  QWidget::leaveEvent(event);
}

void WaveformWidget::applyMarkerEdit(MarkerId marker, qint64 newTimeMs,
                                     float newVol) {
  const qint64 total = totalDurationMs();
  if (total <= 0) {
    return;
  }

  PlayerMetadata m = m_metadata;
  if (m.endPoint <= 0 || m.endPoint > total) {
    m.endPoint = total;
  }
  if (m.mixEnd <= m.startPoint || m.mixEnd > m.endPoint) {
    m.mixEnd = m.endPoint;
  }
  if (m.fadeOutStartPoint <= 0 || m.fadeOutStartPoint > m.endPoint) {
    m.fadeOutStartPoint = m.endPoint;
  }

  const bool introEnv = hasIntroEnvelope();
  const bool outroEnv = hasOutroEnvelope();
  const qint64 t = qBound<qint64>(0, newTimeMs, total);

  switch (marker) {
  case MarkerId::StartPoint: {
    m.startPoint = qBound<qint64>(0, t, m.endPoint);
    m.mixStart = qMax(m.mixStart, m.startPoint);
    m.fadeInEndPoint = qMax(m.fadeInEndPoint, m.startPoint);
    if (introEnv) {
      m.volumeUpStartPoint = qMax(m.volumeUpStartPoint, m.fadeInEndPoint);
      m.volumeUpEndPoint = qMax(m.volumeUpEndPoint, m.volumeUpStartPoint);
    }
    const qint64 rightWall =
        outroEnv ? m.volumeDownStartPoint : m.fadeOutStartPoint;
    if (m.fadeInEndPoint > rightWall) {
      m.fadeInEndPoint = rightWall;
    }
    if (introEnv) {
      m.volumeUpEndPoint = qMin(m.volumeUpEndPoint, rightWall);
      m.volumeUpStartPoint = qMin(m.volumeUpStartPoint, m.volumeUpEndPoint);
    }
    break;
  }
  case MarkerId::MixStart:
    m.mixStart = qBound<qint64>(m.startPoint, t, m.endPoint);
    break;
  case MarkerId::FadeInEnd: {
    const qint64 maxFi =
        outroEnv ? m.volumeDownStartPoint : m.fadeOutStartPoint;
    m.fadeInEndPoint = qBound<qint64>(m.startPoint, t, qMin(maxFi, m.endPoint));
    if (introEnv) {
      m.volumeUpStartPoint = qMax(m.volumeUpStartPoint, m.fadeInEndPoint);
      m.volumeUpEndPoint =
          qBound(m.volumeUpStartPoint, m.volumeUpEndPoint, maxFi);
      m.startVolume = std::clamp(newVol, 0.0f, 1.0f);
    }
    break;
  }
  case MarkerId::VolUpStart: {
    const qint64 maxUp = (m.volumeUpEndPoint >= m.fadeInEndPoint)
                             ? m.volumeUpEndPoint
                             : m.fadeInEndPoint;
    m.volumeUpStartPoint = qBound<qint64>(m.fadeInEndPoint, t, maxUp);
    m.startVolume = std::clamp(newVol, 0.0f, 1.0f);
    break;
  }
  case MarkerId::VolUpEnd: {
    const qint64 maxUp =
        outroEnv ? m.volumeDownStartPoint : m.fadeOutStartPoint;
    m.volumeUpEndPoint =
        qBound<qint64>(m.volumeUpStartPoint, t, qMin(maxUp, m.endPoint));
    break;
  }
  case MarkerId::VolDownStart: {
    const qint64 minDown = introEnv ? m.volumeUpEndPoint : m.fadeInEndPoint;
    m.volumeDownStartPoint = qBound<qint64>(minDown, t, m.volumeDownEndPoint);
    break;
  }
  case MarkerId::VolDownEnd: {
    const qint64 minDown = introEnv ? m.volumeUpEndPoint : m.fadeInEndPoint;
    const qint64 low = qMax(minDown, m.volumeDownStartPoint);
    m.volumeDownEndPoint = qBound<qint64>(low, t, m.fadeOutStartPoint);
    m.endVolume = std::clamp(newVol, 0.0f, 1.0f);
    break;
  }
  case MarkerId::FadeOutStart: {
    const qint64 minFo = introEnv ? m.volumeUpEndPoint : m.fadeInEndPoint;
    m.fadeOutStartPoint = qBound<qint64>(minFo, t, m.endPoint);
    if (outroEnv) {
      m.volumeDownEndPoint = qMin(m.volumeDownEndPoint, m.fadeOutStartPoint);
      m.volumeDownStartPoint =
          qMin(m.volumeDownStartPoint, m.volumeDownEndPoint);
      m.endVolume = std::clamp(newVol, 0.0f, 1.0f);
    }
    break;
  }
  case MarkerId::MixEnd:
    m.mixEnd = qBound<qint64>(m.startPoint, t, m.endPoint);
    break;
  case MarkerId::EndPoint: {
    m.endPoint = qBound<qint64>(m.startPoint, t, total);
    m.mixStart = qBound<qint64>(m.startPoint, m.mixStart, m.endPoint);
    m.mixEnd = qBound<qint64>(m.startPoint, m.mixEnd, m.endPoint);
    m.fadeInEndPoint =
        qBound<qint64>(m.startPoint, m.fadeInEndPoint, m.endPoint);
    m.fadeOutStartPoint =
        qBound<qint64>(m.fadeInEndPoint, m.fadeOutStartPoint, m.endPoint);
    if (introEnv) {
      m.volumeUpEndPoint = qBound<qint64>(m.fadeInEndPoint, m.volumeUpEndPoint,
                                          m.fadeOutStartPoint);
      m.volumeUpStartPoint = qBound<qint64>(
          m.fadeInEndPoint, m.volumeUpStartPoint, m.volumeUpEndPoint);
    }
    if (outroEnv) {
      const qint64 minDown = introEnv ? m.volumeUpEndPoint : m.fadeInEndPoint;
      m.volumeDownEndPoint =
          qBound<qint64>(minDown, m.volumeDownEndPoint, m.fadeOutStartPoint);
      m.volumeDownStartPoint =
          qBound<qint64>(minDown, m.volumeDownStartPoint, m.volumeDownEndPoint);
    }
    break;
  }
  case MarkerId::None:
    return;
  }

  m_metadata = m;
  m_envelopeDirty = true;
  update();
  emit metadataChanged(m_metadata);
}

QString WaveformWidget::markerDisplayName(MarkerId marker) const {
  switch (marker) {
  case MarkerId::StartPoint:
    return tr("Ponto inicial");
  case MarkerId::MixStart:
    return tr("Mixagem inicial");
  case MarkerId::FadeInEnd:
    return tr("Fim do Fade In");
  case MarkerId::VolUpStart:
    return tr("Início do aumento de volume");
  case MarkerId::VolUpEnd:
    return tr("Fim do aumento de volume");
  case MarkerId::VolDownStart:
    return tr("Início da diminuição de volume");
  case MarkerId::VolDownEnd:
    return tr("Fim da diminuição de volume");
  case MarkerId::FadeOutStart:
    return tr("Início do Fade Out");
  case MarkerId::MixEnd:
    return tr("Mixagem final");
  case MarkerId::EndPoint:
    return tr("Ponto final");
  case MarkerId::None:
    return QString();
  }
  return QString();
}

qint64 WaveformWidget::markerTime(MarkerId marker) const {
  switch (marker) {
  case MarkerId::StartPoint:
    return m_metadata.startPoint;
  case MarkerId::MixStart:
    return m_metadata.mixStart;
  case MarkerId::FadeInEnd:
    return m_metadata.fadeInEndPoint;
  case MarkerId::VolUpStart:
    return m_metadata.volumeUpStartPoint;
  case MarkerId::VolUpEnd:
    return m_metadata.volumeUpEndPoint;
  case MarkerId::VolDownStart:
    return m_metadata.volumeDownStartPoint;
  case MarkerId::VolDownEnd:
    return m_metadata.volumeDownEndPoint;
  case MarkerId::FadeOutStart:
    return m_metadata.fadeOutStartPoint;
  case MarkerId::MixEnd:
    return m_metadata.mixEnd;
  case MarkerId::EndPoint:
    return m_metadata.endPoint;
  case MarkerId::None:
    return 0;
  }
  return 0;
}
