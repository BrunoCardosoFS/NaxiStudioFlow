#include "./PlayerManager.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float HALF_PI = 1.5707963267948966f; // Pi / 2

inline float equalPowerFactor(float t) { return std::sin(t * HALF_PI); }
} // namespace

PlayerManager::PlayerManager(QObject *parent) : QObject{parent} {
  this->mediaPlayer->setAudioOutput(this->audioOutput);
  this->applyVolume(0.0f);

  connect(this->mediaPlayer, &QMediaPlayer::positionChanged, this,
          &PlayerManager::volumeControl);

  connect(this->mediaPlayer, &QMediaPlayer::mediaStatusChanged, this,
          &PlayerManager::mediaStatusChanged);

  connect(this->mediaPlayer, &QMediaPlayer::errorOccurred, this,
          [this](QMediaPlayer::Error error, const QString &errorString) {
            Q_UNUSED(error);
            Q_UNUSED(errorString);
            this->setStatus(PlayerStatus::Error);
          });
}

void PlayerManager::play() {
  this->volumeControl(this->mediaPlayer->position());
  this->mediaPlayer->setActiveAudioTrack(-1);
  this->mediaPlayer->play();
  this->mediaPlayer->setActiveAudioTrack(0);
  this->setStatus(PlayerStatus::Playing);
}

void PlayerManager::pause() { this->mediaPlayer->pause(); }

void PlayerManager::stop() {
  this->mediaPlayer->pause();
  this->mediaPlayer->setPosition(this->metadata.endPoint);
  this->applyVolume(0.0f);
  this->setStatus(PlayerStatus::End);
}

void PlayerManager::setMetadata(const PlayerMetadata &newMetadata) {
  this->metadata = newMetadata;
  this->updateFadeParameters();
  if (this->mediaPlayer->mediaStatus() == QMediaPlayer::LoadedMedia) {
    this->mediaPlayer->setPosition(this->metadata.startPoint);
  }
  this->volumeControl(this->mediaPlayer->position());
}

void PlayerManager::updateFadeParameters() {
  const qint64 fadeInDuration =
      this->metadata.fadeInEndPoint - this->metadata.startPoint;
  this->m_invFadeInDuration =
      (fadeInDuration > 0) ? (1.0f / static_cast<float>(fadeInDuration)) : 0.0f;

  const qint64 fadeOutDuration =
      this->metadata.endPoint - this->metadata.fadeOutStartPoint;
  this->m_invFadeOutDuration =
      (fadeOutDuration > 0) ? (1.0f / static_cast<float>(fadeOutDuration))
                            : 0.0f;
}

void PlayerManager::setTargetVolume(float volume) {
  const float clamped = std::clamp(volume, 0.0f, 1.0f);
  if (this->m_targetVolume == clamped) {
    return;
  }
  this->m_targetVolume = clamped;
  this->volumeControl(this->mediaPlayer->position());
}

float PlayerManager::targetVolume() const { return this->m_targetVolume; }

float PlayerManager::currentVolume() const {
  return this->m_currentVolume >= 0.0f ? this->m_currentVolume : 0.0f;
}

PlayerStatus PlayerManager::status() const { return this->m_status; }

void PlayerManager::setStatus(PlayerStatus status) {
  if (this->m_status == status) {
    return;
  }
  this->m_status = status;
  emit this->statusChanged(status);
}

void PlayerManager::applyVolume(float volume) {
  const float clamped = std::clamp(volume, 0.0f, 1.0f);

  if (clamped == this->m_currentVolume) {
    return;
  }

  const bool isEndpoint = (clamped == 0.0f || clamped == this->m_targetVolume);
  if (!isEndpoint && std::abs(clamped - this->m_currentVolume) < 0.002f) {
    return;
  }

  this->m_currentVolume = clamped;
  this->audioOutput->setVolume(clamped);
  emit this->volumeChanged(clamped);
}

void PlayerManager::mediaStatusChanged(QMediaPlayer::MediaStatus status) {
  if (status == QMediaPlayer::InvalidMedia) {
    this->setStatus(PlayerStatus::Error);
    return;
  }

  if (status == QMediaPlayer::EndOfMedia) {
    this->setStatus(PlayerStatus::End);
    return;
  }

  if (status != QMediaPlayer::LoadedMedia) {
    return;
  }

  const qint64 dur = this->mediaPlayer->duration();
  this->metadata.duration = dur;
  // Por enquanto está fixo para testes
  this->metadata.startPoint = 0;
  this->metadata.endPoint = dur;

  this->metadata.mixStart = qMin<qint64>(1000, dur);
  this->metadata.mixEnd = qMax<qint64>(0, dur - 1000);

  this->metadata.fadeInEndPoint = qMin<qint64>(1500, dur);
  this->metadata.fadeOutStartPoint = qMax<qint64>(0, dur - 1500);

  this->updateFadeParameters();

  this->mediaPlayer->setPosition(this->metadata.startPoint);
  this->volumeControl(this->metadata.startPoint);
  this->setStatus(PlayerStatus::Loaded);
}

void PlayerManager::volumeControl(qint64 position) {
  // Finalize execution when reaching or passing endPoint
  if (this->metadata.endPoint > 0 && position >= this->metadata.endPoint) {
    this->applyVolume(0.0f);
    if (this->m_status == PlayerStatus::Playing) {
      this->stop();
    }
    return;
  }

  // Outside playable boundaries before startPoint (silence)
  if (position < this->metadata.startPoint) {
    this->applyVolume(0.0f);
    return;
  }

  // High-performance fast path: Plateau region (full target volume)
  // Statistically >95% of calls will hit this branch.
  if (Q_LIKELY(position >= this->metadata.fadeInEndPoint &&
               position < this->metadata.fadeOutStartPoint)) {
    this->applyVolume(this->m_targetVolume);
    return;
  }

  // Fade In region
  if (position < this->metadata.fadeInEndPoint) {
    const qint64 fadeInDuration =
        this->metadata.fadeInEndPoint - this->metadata.startPoint;
    if (Q_UNLIKELY(fadeInDuration <= 0)) {
      this->applyVolume(this->m_targetVolume);
      return;
    }

    if (Q_UNLIKELY(this->m_invFadeInDuration <= 0.0f)) {
      this->m_invFadeInDuration = 1.0f / static_cast<float>(fadeInDuration);
    }

    const float progress =
        std::clamp(static_cast<float>(position - this->metadata.startPoint) *
                       this->m_invFadeInDuration,
                   0.0f, 1.0f);
    float factor = equalPowerFactor(progress);

    // Guard against overlapping fade-in and fade-out (e.g. very short audio
    // files)
    if (Q_UNLIKELY(position >= this->metadata.fadeOutStartPoint)) {
      const qint64 fadeOutDuration =
          this->metadata.endPoint - this->metadata.fadeOutStartPoint;
      if (fadeOutDuration > 0) {
        if (Q_UNLIKELY(this->m_invFadeOutDuration <= 0.0f)) {
          this->m_invFadeOutDuration =
              1.0f / static_cast<float>(fadeOutDuration);
        }
        const float outProgress =
            std::clamp(static_cast<float>(this->metadata.endPoint - position) *
                           this->m_invFadeOutDuration,
                       0.0f, 1.0f);
        const float outFactor = equalPowerFactor(outProgress);
        factor = std::min(factor, outFactor);
      }
    }

    this->applyVolume(factor * this->m_targetVolume);
    return;
  }

  // Fade Out region
  if (position >= this->metadata.fadeOutStartPoint) {
    const qint64 fadeOutDuration =
        this->metadata.endPoint - this->metadata.fadeOutStartPoint;
    if (Q_UNLIKELY(fadeOutDuration <= 0)) {
      this->applyVolume(0.0f);
      return;
    }

    if (Q_UNLIKELY(this->m_invFadeOutDuration <= 0.0f)) {
      this->m_invFadeOutDuration = 1.0f / static_cast<float>(fadeOutDuration);
    }

    const float progress =
        std::clamp(static_cast<float>(this->metadata.endPoint - position) *
                       this->m_invFadeOutDuration,
                   0.0f, 1.0f);
    const float factor = equalPowerFactor(progress);
    this->applyVolume(factor * this->m_targetVolume);
    return;
  }

  // Fallback plateau
  this->applyVolume(this->m_targetVolume);
}