#include "core/audio/WaveformExtractor.h"

#include <QAudioBuffer>
#include <QAudioFormat>
#include <QFileInfo>
#include <QUrl>

#include <algorithm>
#include <cmath>

namespace {
constexpr qint64 kProgressThrottleMs = 80;
constexpr float kInvInt16 = 1.0f / 32768.0f;
constexpr double kInvInt32 = 1.0 / 2147483648.0;
constexpr float kInvUInt8 = 1.0f / 128.0f;

template <typename SampleT, typename ToFloatFn>
void accumulateFrames(const SampleT *data, qsizetype frameCount, int channels,
                      ToFloatFn toFloat, qint64 &framesPerBucket,
                      qint64 &currentBucketFrames, float &currentBucketPeak,
                      double &currentBucketSqSum, qint64 &totalFrames,
                      QVector<WaveformSample> &outSamples) {
  if (channels == 1) {
    for (qsizetype f = 0; f < frameCount; ++f) {
      const float mono = toFloat(data[f]);
      const float absVal = std::abs(mono);
      if (absVal > currentBucketPeak) {
        currentBucketPeak = absVal;
      }
      currentBucketSqSum +=
          static_cast<double>(mono) * static_cast<double>(mono);
      ++currentBucketFrames;
      ++totalFrames;

      if (currentBucketFrames >= framesPerBucket) {
        const float rms = static_cast<float>(std::sqrt(
            currentBucketSqSum / static_cast<double>(currentBucketFrames)));
        outSamples.append({std::clamp(currentBucketPeak, 0.0f, 1.0f),
                           std::clamp(rms, 0.0f, 1.0f)});
        currentBucketFrames = 0;
        currentBucketPeak = 0.0f;
        currentBucketSqSum = 0.0;
      }
    }
    return;
  }

  const float invChannels = 1.0f / static_cast<float>(channels);
  for (qsizetype f = 0; f < frameCount; ++f) {
    float monoSum = 0.0f;
    const qsizetype base = f * channels;
    for (int ch = 0; ch < channels; ++ch) {
      monoSum += toFloat(data[base + ch]);
    }
    const float mono = monoSum * invChannels;
    const float absVal = std::abs(mono);
    if (absVal > currentBucketPeak) {
      currentBucketPeak = absVal;
    }
    currentBucketSqSum += static_cast<double>(mono) * static_cast<double>(mono);
    ++currentBucketFrames;
    ++totalFrames;

    if (currentBucketFrames >= framesPerBucket) {
      const float rms = static_cast<float>(std::sqrt(
          currentBucketSqSum / static_cast<double>(currentBucketFrames)));
      outSamples.append({std::clamp(currentBucketPeak, 0.0f, 1.0f),
                         std::clamp(rms, 0.0f, 1.0f)});
      currentBucketFrames = 0;
      currentBucketPeak = 0.0f;
      currentBucketSqSum = 0.0;
    }
  }
}
} // namespace

WaveformExtractor::WaveformExtractor(QObject *parent) : QObject(parent) {
  qRegisterMetaType<WaveformSample>("WaveformSample");
  qRegisterMetaType<QVector<WaveformSample>>("QVector<WaveformSample>");
  recreateDecoder(true);
}

WaveformExtractor::~WaveformExtractor() { cancel(); }

void WaveformExtractor::recreateDecoder(bool forceCompactFormat) {
  if (m_decoder) {
    m_decoder->disconnect(this);
    m_decoder->stop();
    m_decoder->setSource(QUrl());
    m_decoder->deleteLater();
    m_decoder = nullptr;
  }

  m_decoder = new QAudioDecoder(this);
  m_usedForcedFormat = forceCompactFormat;

  if (forceCompactFormat) {
    QAudioFormat desiredFormat;
    desiredFormat.setSampleRate(8000);
    desiredFormat.setChannelCount(1);
    desiredFormat.setSampleFormat(QAudioFormat::Float);
    m_decoder->setAudioFormat(desiredFormat);
  }

  connect(m_decoder, &QAudioDecoder::bufferReady, this,
          &WaveformExtractor::onBufferReady);
  connect(m_decoder, &QAudioDecoder::finished, this,
          &WaveformExtractor::onFinished);
  connect(m_decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error),
          this, &WaveformExtractor::onError);
  connect(m_decoder, &QAudioDecoder::durationChanged, this,
          [this](qint64 duration) {
            if (m_isDecoding && duration > 0) {
              m_knownDurationMs = duration;
              const qsizetype expected =
                  static_cast<qsizetype>(duration / kDefaultBucketDurationMs) +
                  32;
              if (m_samples.capacity() < expected) {
                m_samples.reserve(expected);
              }
            }
          });
}

void WaveformExtractor::startDecode(bool forceCompactFormat) {
  ++m_requestId;
  resetAccumulator();
  if (!m_decoder || m_usedForcedFormat != forceCompactFormat) {
    recreateDecoder(forceCompactFormat);
  } else {
    m_decoder->stop();
    m_decoder->setSource(QUrl());
  }

  m_isDecoding = true;
  m_progressTimer.start();

  m_decoder->setSource(QUrl::fromLocalFile(m_currentFilePath));
  m_decoder->start();
}

void WaveformExtractor::extract(const QString &filePath) {
  cancel();

  if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
    emit errorOccurred(tr("Arquivo de áudio não encontrado no disco."));
    return;
  }

  m_currentFilePath = filePath;
  startDecode(true);
}

void WaveformExtractor::cancel() {
  ++m_requestId;
  m_isDecoding = false;
  m_currentFilePath.clear();
  if (m_decoder) {
    m_decoder->stop();
    m_decoder->setSource(QUrl());
  }
  resetAccumulator();
}

void WaveformExtractor::resetAccumulator() {
  m_samples.clear();
  m_lastEmittedCount = 0;
  m_knownDurationMs = 0;
  m_totalFrames = 0;
  m_sampleRate = 0;
  m_framesPerBucket = 80;
  m_currentBucketFrames = 0;
  m_currentBucketPeak = 0.0f;
  m_currentBucketSqSum = 0.0;
}

void WaveformExtractor::consumePendingBuffers() {
  const quint64 activeRequest = m_requestId;
  while (m_isDecoding && m_requestId == activeRequest &&
         m_decoder->bufferAvailable()) {
    const QAudioBuffer buffer = m_decoder->read();
    if (!buffer.isValid()) {
      continue;
    }

    const QAudioFormat fmt = buffer.format();
    const int channels = fmt.channelCount();
    const int sampleRate = fmt.sampleRate();
    const qsizetype frameCount = buffer.frameCount();
    if (channels <= 0 || sampleRate <= 0 || frameCount <= 0) {
      continue;
    }

    if (m_sampleRate != sampleRate) {
      m_sampleRate = sampleRate;
      m_framesPerBucket = qMax<qint64>(
          1,
          (static_cast<qint64>(sampleRate) * kDefaultBucketDurationMs) / 1000);
    }

    switch (fmt.sampleFormat()) {
    case QAudioFormat::Float:
      accumulateFrames(
          buffer.constData<float>(), frameCount, channels,
          [](float s) { return s; }, m_framesPerBucket, m_currentBucketFrames,
          m_currentBucketPeak, m_currentBucketSqSum, m_totalFrames, m_samples);
      break;
    case QAudioFormat::Int16:
      accumulateFrames(
          buffer.constData<qint16>(), frameCount, channels,
          [](qint16 s) { return static_cast<float>(s) * kInvInt16; },
          m_framesPerBucket, m_currentBucketFrames, m_currentBucketPeak,
          m_currentBucketSqSum, m_totalFrames, m_samples);
      break;
    case QAudioFormat::Int32:
      accumulateFrames(
          buffer.constData<qint32>(), frameCount, channels,
          [](qint32 s) {
            return static_cast<float>(static_cast<double>(s) * kInvInt32);
          },
          m_framesPerBucket, m_currentBucketFrames, m_currentBucketPeak,
          m_currentBucketSqSum, m_totalFrames, m_samples);
      break;
    case QAudioFormat::UInt8:
      accumulateFrames(
          buffer.constData<quint8>(), frameCount, channels,
          [](quint8 s) { return (static_cast<float>(s) - 128.0f) * kInvUInt8; },
          m_framesPerBucket, m_currentBucketFrames, m_currentBucketPeak,
          m_currentBucketSqSum, m_totalFrames, m_samples);
      break;
    default: {
      const int bytesPerSample = fmt.bytesPerSample();
      const auto *raw = buffer.constData<char>();
      if (bytesPerSample > 0 && raw) {
        const float invChannels = 1.0f / static_cast<float>(channels);
        for (qsizetype f = 0; f < frameCount; ++f) {
          float monoSum = 0.0f;
          for (int ch = 0; ch < channels; ++ch) {
            const qsizetype offset = (f * channels + ch) * bytesPerSample;
            monoSum += fmt.normalizedSampleValue(raw + offset);
          }
          const float mono = monoSum * invChannels;
          m_currentBucketPeak = std::max(m_currentBucketPeak, std::abs(mono));
          m_currentBucketSqSum +=
              static_cast<double>(mono) * static_cast<double>(mono);
          ++m_currentBucketFrames;
          ++m_totalFrames;
          if (m_currentBucketFrames >= m_framesPerBucket) {
            const float rms = static_cast<float>(
                std::sqrt(m_currentBucketSqSum /
                          static_cast<double>(m_currentBucketFrames)));
            m_samples.append({std::clamp(m_currentBucketPeak, 0.0f, 1.0f),
                              std::clamp(rms, 0.0f, 1.0f)});
            m_currentBucketFrames = 0;
            m_currentBucketPeak = 0.0f;
            m_currentBucketSqSum = 0.0;
          }
        }
      }
      break;
    }
    }
  }
}

void WaveformExtractor::flushPartialBucket() {
  if (m_currentBucketFrames > 0) {
    const float rms = static_cast<float>(std::sqrt(
        m_currentBucketSqSum / static_cast<double>(m_currentBucketFrames)));
    m_samples.append({std::clamp(m_currentBucketPeak, 0.0f, 1.0f),
                      std::clamp(rms, 0.0f, 1.0f)});
    m_currentBucketFrames = 0;
    m_currentBucketPeak = 0.0f;
    m_currentBucketSqSum = 0.0;
  }
}

qint64 WaveformExtractor::resolvedDurationMs() const {
  if (m_knownDurationMs > 0) {
    return m_knownDurationMs;
  }
  if (m_decoder && m_decoder->duration() > 0) {
    return m_decoder->duration();
  }
  if (m_sampleRate > 0 && m_totalFrames > 0) {
    return static_cast<qint64>(
        std::llround((static_cast<double>(m_totalFrames) * 1000.0) /
                     static_cast<double>(m_sampleRate)));
  }
  return static_cast<qint64>(m_samples.size()) * kDefaultBucketDurationMs;
}

void WaveformExtractor::onBufferReady() {
  if (!m_isDecoding) {
    return;
  }
  consumePendingBuffers();
  if (m_samples.size() > m_lastEmittedCount &&
      m_progressTimer.elapsed() >= kProgressThrottleMs) {
    m_progressTimer.restart();
    const qsizetype deltaCount = m_samples.size() - m_lastEmittedCount;
    QVector<WaveformSample> delta;
    delta.reserve(deltaCount);
    for (qsizetype i = m_lastEmittedCount; i < m_samples.size(); ++i) {
      delta.append(m_samples.at(i));
    }
    m_lastEmittedCount = m_samples.size();
    emit progressChunk(delta, resolvedDurationMs());
  }
}

void WaveformExtractor::onFinished() {
  if (!m_isDecoding) {
    return;
  }
  consumePendingBuffers();
  flushPartialBucket();

  if (m_samples.isEmpty()) {
    if (m_usedForcedFormat && !m_currentFilePath.isEmpty()) {
      // Fallback: retry decoding in native file format (fixes MP3s where
      // Qt6 FFmpeg QAudioDecoder fails/skips buffers under forced resampling).
      startDecode(false);
      return;
    }
    m_isDecoding = false;
    if (m_decoder) {
      m_decoder->stop();
      m_decoder->setSource(QUrl());
    }
    emit errorOccurred(
        tr("Não foi possível extrair a forma de onda deste arquivo de áudio."));
    return;
  }

  const qint64 finalDuration = resolvedDurationMs();
  m_isDecoding = false;
  if (m_decoder) {
    m_decoder->stop();
    m_decoder->setSource(QUrl());
  }
  emit finished(m_samples, finalDuration);
}

void WaveformExtractor::onError(QAudioDecoder::Error err) {
  if (!m_isDecoding) {
    return;
  }
  if (m_usedForcedFormat && !m_currentFilePath.isEmpty()) {
    startDecode(false);
    return;
  }
  m_isDecoding = false;
  QString message = m_decoder ? m_decoder->errorString() : QString();
  if (m_decoder) {
    m_decoder->stop();
    m_decoder->setSource(QUrl());
  }
  if (message.isEmpty()) {
    message = tr("Erro ao decodificar arquivo de áudio (%1).")
                  .arg(static_cast<int>(err));
  }
  emit errorOccurred(message);
}
