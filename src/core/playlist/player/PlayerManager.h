#ifndef PLAYERMANAGER_H
#define PLAYERMANAGER_H

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>

enum class PlayerStatus { Loaded, Playing, End, Error };

struct PlayerMetadata {
  qint64 mixStart = 0;
  qint64 mixEnd = 0;

  qint64 startPoint = 0;
  qint64 endPoint = 0;

  qint64 fadeInEndPoint = 0;
  qint64 fadeOutStartPoint = 0;

  qint64 duration = 0;
};

class PlayerManager : public QObject {
  Q_OBJECT
public:
  explicit PlayerManager(QObject *parent = nullptr);

  void play();
  void pause();
  void stop();

  void setMetadata(const PlayerMetadata &metadata);

  QMediaPlayer *mediaPlayer = new QMediaPlayer(this);
  PlayerMetadata metadata;
  PlayerStatus status() const;

signals:
  void volumeChanged(float volume);
  void statusChanged(PlayerStatus status);

private:
  QAudioOutput *audioOutput = new QAudioOutput(this);

  PlayerStatus m_status = PlayerStatus::End;
  float m_targetVolume = 1.0f;
  float m_currentVolume = -1.0f;

  float m_invFadeInDuration = 0.0f;
  float m_invFadeOutDuration = 0.0f;

  void applyVolume(float volume);
  void setStatus(PlayerStatus status);

  void updateFadeParameters();

  void setTargetVolume(float volume);
  float targetVolume() const;
  float currentVolume() const;

private slots:
  void volumeControl(qint64 position);
  void mediaStatusChanged(QMediaPlayer::MediaStatus status);
};

#endif // PLAYERMANAGER_H