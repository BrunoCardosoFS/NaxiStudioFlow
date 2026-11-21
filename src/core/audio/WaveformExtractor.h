#ifndef WAVEFORMEXTRACTOR_H
#define WAVEFORMEXTRACTOR_H

#include <QAudioDecoder>
#include <QElapsedTimer>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QVector>

struct WaveformSample {
  float peak = 0.0f;
  float rms = 0.0f;
};
Q_DECLARE_METATYPE(WaveformSample)

class WaveformExtractor : public QObject {
  Q_OBJECT
public:
  static constexpr qint64 kDefaultBucketDurationMs = 10;

  explicit WaveformExtractor(QObject *parent = nullptr);
  ~WaveformExtractor() override;

  void extract(const QString &filePath);
  void cancel();

  bool isDecoding() const { return m_isDecoding; }
  qint64 bucketDurationMs() const { return kDefaultBucketDurationMs; }

signals:
  void progressUpdated(const QVector<WaveformSample> &samples,
                       qint64 durationMs);
  void finished(const QVector<WaveformSample> &samples, qint64 durationMs);
  void errorOccurred(const QString &errorMessage);

private slots:
  void onBufferReady();
  void onFinished();
  void onError(QAudioDecoder::Error error);

private:
  void resetAccumulator();
  void consumePendingBuffers();
  void flushPartialBucket();
  qint64 resolvedDurationMs() const;

  QAudioDecoder *m_decoder = nullptr;
  quint64 m_requestId = 0;
  bool m_isDecoding = false;

  QVector<WaveformSample> m_samples;
  qint64 m_knownDurationMs = 0;
  qint64 m_totalFrames = 0;
  int m_sampleRate = 0;

  qint64 m_framesPerBucket = 80;
  qint64 m_currentBucketFrames = 0;
  float m_currentBucketPeak = 0.0f;
  double m_currentBucketSqSum = 0.0;

  QElapsedTimer m_progressTimer;
};

#endif // WAVEFORMEXTRACTOR_H
