#ifndef PLAYERMANAGER_H
#define PLAYERMANAGER_H

#include <QAudioDevice>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>
#include <QUrl>

enum class PlayerStatus { Idle, Loading, Loaded, Playing, Paused, End, Error };

struct PlayerMetadata {
  qint64 mixStart = 0;
  qint64 mixEnd = 0;

  qint64 startPoint = 0;
  qint64 endPoint = 0;

  qint64 fadeInEndPoint = 0;
  qint64 fadeOutStartPoint = 0;

  float startVolume = 1.0f;
  float endVolume = 1.0f;
  qint64 volumeUpStartPoint = 0;
  qint64 volumeUpEndPoint = 0;
  qint64 volumeDownStartPoint = 0;
  qint64 volumeDownEndPoint = 0;

  qint64 duration = 0;
};

class PlayerManager : public QObject {
  Q_OBJECT
public:
  explicit PlayerManager(QObject *parent = nullptr);
  ~PlayerManager() override;

  virtual void load(const QUrl &source, const PlayerMetadata &metadata,
                    bool autoPlay = false);
  virtual void unload();

  virtual void play();
  virtual void pause();
  virtual void stop();
  virtual void seek(qint64 positionMs);
  virtual void playFrom(qint64 positionMs);
  virtual void triggerFadeOut(qint64 fadeDurationMs);

  virtual void setMetadata(const PlayerMetadata &metadata);
  void setAudioOutputDevice(const QAudioDevice &device);

  static float computeVolumeFactor(const PlayerMetadata &meta, qint64 position);

  virtual qint64 position() const;
  virtual qint64 duration() const;
  QUrl source() const { return m_source; }

  void setTargetVolume(float volume);
  float targetVolume() const;
  float currentVolume() const;

  QMediaPlayer *mediaPlayer = nullptr;
  PlayerMetadata metadata;
  const PlayerMetadata &currentMetadata() const { return metadata; }
  PlayerStatus status() const;

signals:
  void volumeChanged(float volume);
  void statusChanged(PlayerStatus status);
  void positionChanged(qint64 position);
  void durationLoaded(qint64 durationMs);

protected:
  explicit PlayerManager(bool initializeMediaBackend,
                         QObject *parent = nullptr);

  void applyVolume(float volume);
  void applyVolumeForPosition(qint64 position);
  void setStatus(PlayerStatus status);
  void normalizeMetadata(qint64 realDuration);
  void updateFadeParameters();
  void volumeControl(qint64 position);

  QUrl m_source;
  bool m_pendingPlay = false;
  qint64 m_pendingSeekPos = -1;

private:
  QAudioOutput *audioOutput = nullptr;

  PlayerStatus m_status = PlayerStatus::Idle;
  float m_targetVolume = 1.0f;
  float m_currentVolume = -1.0f;

  float m_invFadeInDuration = 0.0f;
  float m_invFadeOutDuration = 0.0f;
  float m_invVolUpDuration = 0.0f;
  float m_invVolDownDuration = 0.0f;
  bool m_hasIntroEnvelope = false;
  bool m_hasOutroEnvelope = false;
  qint64 m_plateauStart = 0;
  qint64 m_plateauEnd = 0;

private slots:
  void mediaStatusChanged(QMediaPlayer::MediaStatus status);
};

#endif // PLAYERMANAGER_H