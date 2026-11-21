#include "./PlayerManager.h"

#include <QAudioDevice>
#include <QFileInfo>

#include <algorithm>
#include <cmath>

namespace {
constexpr float HALF_PI = 1.5707963267948966f; // Pi / 2

inline float equalPowerFactor(float t) { return std::sin(t * HALF_PI); }
} // namespace

PlayerManager::PlayerManager(QObject *parent)
    : PlayerManager(true, parent) {}

PlayerManager::~PlayerManager() {
  if (this->mediaPlayer) {
    this->mediaPlayer->disconnect(this);
    this->mediaPlayer->stop();
    this->mediaPlayer->setSource(QUrl());
    this->mediaPlayer->setAudioOutput(nullptr);
  }
}

PlayerManager::PlayerManager(bool initializeMediaBackend, QObject *parent)
    : QObject{parent} {
  if (!initializeMediaBackend) {
    return;
  }

  this->mediaPlayer = new QMediaPlayer(this);
  this->audioOutput = new QAudioOutput(this);
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
            this->m_pendingPlay = false;
            this->applyVolume(0.0f);
            this->setStatus(PlayerStatus::Error);
          });
}

void PlayerManager::load(const QUrl &source, const PlayerMetadata &newMetadata,
                         bool autoPlay) {
  if (this->m_source == source && this->m_status == PlayerStatus::Loaded &&
      !source.isEmpty()) {
    this->setMetadata(newMetadata);
    if (autoPlay) {
      this->play();
    }
    return;
  }

  // Always transition through Loading so consecutive errors still emit Error.
  this->setStatus(PlayerStatus::Loading);

  const bool invalidLocalFile =
      source.isLocalFile() && !QFileInfo::exists(source.toLocalFile());
  if (source.isEmpty() || invalidLocalFile) {
    this->m_pendingPlay = false;
    this->m_source.clear();
    this->metadata = PlayerMetadata{};
    this->updateFadeParameters();
    this->applyVolume(0.0f);
    if (this->mediaPlayer) {
      this->mediaPlayer->stop();
      this->mediaPlayer->setSource(QUrl());
    }
    this->setStatus(PlayerStatus::Error);
    return;
  }

  if (this->mediaPlayer) {
    this->mediaPlayer->stop();
  }

  this->m_source = source;
  this->metadata = newMetadata;
  this->m_pendingPlay = autoPlay;
  this->m_pendingSeekPos = -1;
  this->normalizeMetadata(this->metadata.duration);
  this->applyVolume(0.0f);

  if (this->mediaPlayer) {
    this->mediaPlayer->setSource(source);
  }
}

void PlayerManager::unload() {
  this->m_pendingPlay = false;
  this->m_pendingSeekPos = -1;
  this->m_source.clear();
  this->metadata = PlayerMetadata{};
  this->updateFadeParameters();
  this->applyVolume(0.0f);
  this->setStatus(PlayerStatus::Idle);
  if (this->mediaPlayer) {
    this->mediaPlayer->stop();
    this->mediaPlayer->setSource(QUrl());
  }
}

void PlayerManager::play() {
  if (this->m_status == PlayerStatus::Loading) {
    this->m_pendingPlay = true;
    return;
  }

  if (this->m_status == PlayerStatus::Playing ||
      this->m_status == PlayerStatus::Error ||
      this->m_status == PlayerStatus::Idle) {
    return;
  }

  const bool startingFromCue = (this->m_status == PlayerStatus::Loaded ||
                                this->m_status == PlayerStatus::End);
  qint64 startPos = this->position();
  if (startingFromCue) {
    startPos = this->metadata.startPoint;
    if (this->mediaPlayer) {
      this->mediaPlayer->setPosition(startPos);
    }
  }

  if (this->metadata.endPoint > 0 && startPos >= this->metadata.endPoint) {
    this->stop();
    return;
  }

  this->applyVolumeForPosition(startPos);
  if (this->mediaPlayer) {
    this->mediaPlayer->setActiveAudioTrack(-1);
    this->mediaPlayer->play();
    this->mediaPlayer->setActiveAudioTrack(0);
  }
  this->setStatus(PlayerStatus::Playing);
}

void PlayerManager::pause() {
  if (this->m_status == PlayerStatus::Loading) {
    this->m_pendingPlay = false;
    return;
  }
  if (this->m_status != PlayerStatus::Playing) {
    return;
  }
  if (this->mediaPlayer) {
    this->mediaPlayer->pause();
  }
  this->setStatus(PlayerStatus::Paused);
}

void PlayerManager::stop() {
  this->m_pendingPlay = false;
  this->m_pendingSeekPos = -1;
  this->applyVolume(0.0f);
  // Transition to End before pausing/seeking to prevent re-entrant stop().
  this->setStatus(PlayerStatus::End);
  if (this->mediaPlayer) {
    this->mediaPlayer->pause();
    const QMediaPlayer::MediaStatus ms = this->mediaPlayer->mediaStatus();
    if (ms == QMediaPlayer::LoadedMedia ||
        ms == QMediaPlayer::BufferedMedia ||
        ms == QMediaPlayer::BufferingMedia ||
        ms == QMediaPlayer::EndOfMedia) {
      this->mediaPlayer->setPosition(this->metadata.endPoint);
    }
  }
}

void PlayerManager::seek(qint64 positionMs) {
  const qint64 dur = this->duration();
  qint64 clamped = qMax<qint64>(0, positionMs);
  if (dur > 0) {
    clamped = qBound<qint64>(0, clamped, dur);
  }

  if (this->m_status == PlayerStatus::Loading) {
    this->m_pendingSeekPos = clamped;
    return;
  }

  if (this->m_status == PlayerStatus::Idle ||
      this->m_status == PlayerStatus::Error) {
    return;
  }

  if (this->mediaPlayer) {
    this->mediaPlayer->setPosition(clamped);
  }
  this->applyVolumeForPosition(clamped);
  emit this->positionChanged(clamped);
}

void PlayerManager::playFrom(qint64 positionMs) {
  qint64 target = qMax<qint64>(this->metadata.startPoint, positionMs);
  if (this->metadata.endPoint > 0 && target >= this->metadata.endPoint) {
    target = this->metadata.startPoint;
  }

  if (this->m_status == PlayerStatus::Loading) {
    this->m_pendingSeekPos = target;
    this->m_pendingPlay = true;
    return;
  }

  if (this->m_status == PlayerStatus::Idle ||
      this->m_status == PlayerStatus::Error) {
    return;
  }

  if (this->metadata.endPoint > 0 && target >= this->metadata.endPoint) {
    this->stop();
    return;
  }

  if (this->mediaPlayer) {
    this->mediaPlayer->setPosition(target);
  }
  this->applyVolumeForPosition(target);

  if (this->m_status != PlayerStatus::Playing) {
    if (this->mediaPlayer) {
      this->mediaPlayer->setActiveAudioTrack(-1);
      this->mediaPlayer->play();
      this->mediaPlayer->setActiveAudioTrack(0);
    }
    this->setStatus(PlayerStatus::Playing);
  }
  emit this->positionChanged(target);
}

void PlayerManager::triggerFadeOut(qint64 fadeDurationMs) {
  if (this->m_status != PlayerStatus::Playing) {
    return;
  }
  if (fadeDurationMs <= 0) {
    this->stop();
    return;
  }

  const qint64 pos = this->position();
  qint64 audioEnd = this->metadata.endPoint;
  const qint64 totalDur = this->duration();
  if (audioEnd <= 0 || (totalDur > 0 && audioEnd > totalDur)) {
    audioEnd = totalDur;
  }
  const qint64 currentEnd =
      (audioEnd > 0) ? audioEnd : (pos + fadeDurationMs);
  const qint64 newEnd = qMin(currentEnd, pos + fadeDurationMs);
  if (newEnd <= pos) {
    this->stop();
    return;
  }

  if (this->metadata.volumeDownStartPoint <= 0 ||
      this->metadata.volumeDownStartPoint >= pos) {
    // Haven't entered outro ducking yet: fade out cleanly from plateau level.
    this->metadata.volumeDownStartPoint = 0;
    this->metadata.volumeDownEndPoint = 0;
    this->metadata.endVolume = 1.0f;
  } else {
    // Already inside outro dip: preserve the active outro volume level at pos.
    if (this->metadata.volumeDownEndPoint > pos) {
      const qint64 rampDur = this->metadata.volumeDownEndPoint -
                             this->metadata.volumeDownStartPoint;
      if (rampDur > 0) {
        const float t = std::clamp(
            static_cast<float>(pos - this->metadata.volumeDownStartPoint) /
                static_cast<float>(rampDur),
            0.0f, 1.0f);
        this->metadata.endVolume =
            1.0f - t * (1.0f - std::clamp(this->metadata.endVolume, 0.0f, 1.0f));
      }
    }
    this->metadata.volumeDownEndPoint =
        qMin(this->metadata.volumeDownEndPoint, pos);
  }

  this->metadata.fadeOutStartPoint = pos;
  this->metadata.endPoint = newEnd;
  this->updateFadeParameters();
  this->volumeControl(pos);
}

void PlayerManager::setAudioOutputDevice(const QAudioDevice &device) {
  // TODO(M13): wire configured CUE/main/cartwall QAudioDevice from settings
  if (this->audioOutput && !device.isNull()) {
    this->audioOutput->setDevice(device);
  }
}

void PlayerManager::setMetadata(const PlayerMetadata &newMetadata) {
  this->metadata = newMetadata;
  const qint64 knownDur =
      (this->metadata.duration > 0)
          ? this->metadata.duration
          : (this->mediaPlayer ? this->mediaPlayer->duration() : 0);
  this->normalizeMetadata(knownDur);
  if (this->mediaPlayer && this->m_status == PlayerStatus::Loaded) {
    this->mediaPlayer->setPosition(this->metadata.startPoint);
  }
  this->applyVolumeForPosition(this->position());
}

qint64 PlayerManager::position() const {
  return this->mediaPlayer ? this->mediaPlayer->position() : 0;
}

qint64 PlayerManager::duration() const {
  if (this->metadata.duration > 0) {
    return this->metadata.duration;
  }
  return this->mediaPlayer ? this->mediaPlayer->duration() : 0;
}

void PlayerManager::normalizeMetadata(qint64 realDuration) {
  if (realDuration > 0) {
    this->metadata.duration = realDuration;
    if (this->metadata.endPoint <= 0 ||
        this->metadata.endPoint > realDuration) {
      this->metadata.endPoint = realDuration;
    }
  } else if (this->metadata.endPoint <= 0 && this->metadata.duration > 0) {
    this->metadata.endPoint = this->metadata.duration;
  }

  this->metadata.startVolume = std::clamp(this->metadata.startVolume, 0.0f, 1.0f);
  this->metadata.endVolume = std::clamp(this->metadata.endVolume, 0.0f, 1.0f);

  if (this->metadata.endPoint > 0) {
    this->metadata.startPoint =
        qBound<qint64>(0, this->metadata.startPoint, this->metadata.endPoint);
    this->metadata.mixStart =
        qBound<qint64>(this->metadata.startPoint, this->metadata.mixStart,
                       this->metadata.endPoint);
    if (this->metadata.mixEnd <= this->metadata.startPoint ||
        this->metadata.mixEnd > this->metadata.endPoint) {
      this->metadata.mixEnd = this->metadata.endPoint;
    }
    this->metadata.fadeInEndPoint =
        qBound<qint64>(this->metadata.startPoint,
                       this->metadata.fadeInEndPoint, this->metadata.endPoint);
    if (this->metadata.fadeOutStartPoint <= 0) {
      this->metadata.fadeOutStartPoint = this->metadata.endPoint;
    } else {
      this->metadata.fadeOutStartPoint = qBound<qint64>(
          this->metadata.startPoint, this->metadata.fadeOutStartPoint,
          this->metadata.endPoint);
    }

    if (this->metadata.volumeUpStartPoint > 0 ||
        this->metadata.volumeUpEndPoint > 0) {
      this->metadata.volumeUpStartPoint = qBound<qint64>(
          this->metadata.fadeInEndPoint, this->metadata.volumeUpStartPoint,
          this->metadata.endPoint);
      this->metadata.volumeUpEndPoint = qBound<qint64>(
          this->metadata.volumeUpStartPoint, this->metadata.volumeUpEndPoint,
          this->metadata.endPoint);
    }

    if (this->metadata.volumeDownStartPoint > 0 ||
        this->metadata.volumeDownEndPoint > 0) {
      const qint64 minDown = (this->metadata.volumeUpEndPoint > 0)
                                 ? this->metadata.volumeUpEndPoint
                                 : this->metadata.fadeInEndPoint;
      this->metadata.volumeDownStartPoint =
          qBound<qint64>(qMin(minDown, this->metadata.fadeOutStartPoint),
                         this->metadata.volumeDownStartPoint,
                         this->metadata.fadeOutStartPoint);
      this->metadata.volumeDownEndPoint =
          qBound<qint64>(this->metadata.volumeDownStartPoint,
                         this->metadata.volumeDownEndPoint,
                         this->metadata.fadeOutStartPoint);
    }
  } else {
    this->metadata.startPoint = qMax<qint64>(0, this->metadata.startPoint);
    this->metadata.mixStart =
        qMax<qint64>(this->metadata.startPoint, this->metadata.mixStart);
    this->metadata.fadeInEndPoint =
        qMax<qint64>(this->metadata.startPoint, this->metadata.fadeInEndPoint);
    if (this->metadata.volumeUpStartPoint > 0 ||
        this->metadata.volumeUpEndPoint > 0) {
      this->metadata.volumeUpStartPoint = qMax<qint64>(
          this->metadata.fadeInEndPoint, this->metadata.volumeUpStartPoint);
      this->metadata.volumeUpEndPoint = qMax<qint64>(
          this->metadata.volumeUpStartPoint, this->metadata.volumeUpEndPoint);
    }
    if (this->metadata.volumeDownStartPoint > 0 ||
        this->metadata.volumeDownEndPoint > 0) {
      const qint64 minDown = (this->metadata.volumeUpEndPoint > 0)
                                 ? this->metadata.volumeUpEndPoint
                                 : this->metadata.fadeInEndPoint;
      this->metadata.volumeDownStartPoint =
          qMax<qint64>(minDown, this->metadata.volumeDownStartPoint);
      this->metadata.volumeDownEndPoint = qMax<qint64>(
          this->metadata.volumeDownStartPoint, this->metadata.volumeDownEndPoint);
    }
  }

  this->updateFadeParameters();
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

  this->m_hasIntroEnvelope =
      (this->metadata.startVolume < 1.0f) &&
      (this->metadata.volumeUpEndPoint > this->metadata.fadeInEndPoint ||
       this->metadata.volumeUpStartPoint > this->metadata.fadeInEndPoint);

  this->m_hasOutroEnvelope =
      (this->metadata.endVolume < 1.0f) &&
      (this->metadata.volumeDownStartPoint > this->metadata.startPoint &&
       this->metadata.volumeDownStartPoint < this->metadata.fadeOutStartPoint);

  const qint64 volUpDuration =
      this->metadata.volumeUpEndPoint - this->metadata.volumeUpStartPoint;
  this->m_invVolUpDuration =
      (volUpDuration > 0) ? (1.0f / static_cast<float>(volUpDuration)) : 0.0f;

  const qint64 volDownDuration =
      this->metadata.volumeDownEndPoint - this->metadata.volumeDownStartPoint;
  this->m_invVolDownDuration =
      (volDownDuration > 0) ? (1.0f / static_cast<float>(volDownDuration)) : 0.0f;

  this->m_plateauStart = this->m_hasIntroEnvelope
                             ? this->metadata.volumeUpEndPoint
                             : this->metadata.fadeInEndPoint;
  this->m_plateauEnd = this->m_hasOutroEnvelope
                           ? this->metadata.volumeDownStartPoint
                           : this->metadata.fadeOutStartPoint;
}

void PlayerManager::setTargetVolume(float volume) {
  const float clamped = std::clamp(volume, 0.0f, 1.0f);
  if (this->m_targetVolume == clamped) {
    return;
  }
  this->m_targetVolume = clamped;
  this->applyVolumeForPosition(this->position());
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
  if (this->audioOutput) {
    this->audioOutput->setVolume(clamped);
  }
  emit this->volumeChanged(clamped);
}

void PlayerManager::mediaStatusChanged(QMediaPlayer::MediaStatus status) {
  if (status == QMediaPlayer::InvalidMedia) {
    this->m_pendingPlay = false;
    this->applyVolume(0.0f);
    this->setStatus(PlayerStatus::Error);
    return;
  }

  if (status == QMediaPlayer::EndOfMedia) {
    if (this->m_status == PlayerStatus::Playing ||
        this->m_status == PlayerStatus::Paused) {
      this->m_pendingPlay = false;
      this->applyVolume(0.0f);
      this->setStatus(PlayerStatus::End);
    }
    return;
  }

  if (status != QMediaPlayer::LoadedMedia) {
    return;
  }

  // FFmpeg backend can re-emit LoadedMedia during seek/pause/playback.
  if (this->m_status != PlayerStatus::Loading &&
      this->m_status != PlayerStatus::Idle) {
    return;
  }

  const qint64 dur = this->mediaPlayer ? this->mediaPlayer->duration() : 0;
  this->normalizeMetadata(dur);

  qint64 initialPos = (this->m_pendingSeekPos >= 0) ? this->m_pendingSeekPos
                                                    : this->metadata.startPoint;
  if (this->metadata.endPoint > 0) {
    initialPos = qBound<qint64>(this->metadata.startPoint, initialPos,
                                this->metadata.endPoint);
    if (initialPos >= this->metadata.endPoint) {
      initialPos = this->metadata.startPoint;
    }
  } else {
    initialPos = qMax<qint64>(this->metadata.startPoint, initialPos);
  }

  if (this->mediaPlayer) {
    this->mediaPlayer->setPosition(initialPos);
  }
  this->applyVolumeForPosition(initialPos);
  this->setStatus(PlayerStatus::Loaded);

  if (dur > 0) {
    emit this->durationLoaded(dur);
  }

  if (this->m_pendingPlay) {
    this->m_pendingPlay = false;
    const qint64 resumePos = this->m_pendingSeekPos;
    this->m_pendingSeekPos = -1;
    if (resumePos >= 0) {
      this->playFrom(resumePos);
    } else {
      this->play();
    }
  } else {
    this->m_pendingSeekPos = -1;
  }
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

  this->applyVolumeForPosition(position);

  if (this->m_status == PlayerStatus::Playing) {
    emit this->positionChanged(position);
  }
}

float PlayerManager::computeVolumeFactor(const PlayerMetadata &meta,
                                         qint64 position) {
  if (position < meta.startPoint) {
    return 0.0f;
  }
  if (meta.endPoint > 0 && position >= meta.endPoint) {
    return 0.0f;
  }

  const float startVol = std::clamp(meta.startVolume, 0.0f, 1.0f);
  const float endVol = std::clamp(meta.endVolume, 0.0f, 1.0f);
  const bool hasIntroEnv =
      (startVol < 1.0f) && (meta.volumeUpEndPoint > meta.fadeInEndPoint ||
                            meta.volumeUpStartPoint > meta.fadeInEndPoint);
  const bool hasOutroEnv =
      (endVol < 1.0f) && (meta.volumeDownStartPoint > meta.startPoint &&
                          meta.volumeDownStartPoint < meta.fadeOutStartPoint);

  const qint64 plateauStart =
      hasIntroEnv ? meta.volumeUpEndPoint : meta.fadeInEndPoint;
  const qint64 plateauEnd =
      hasOutroEnv ? meta.volumeDownStartPoint : meta.fadeOutStartPoint;

  if (Q_LIKELY(position >= plateauStart &&
               (meta.endPoint <= 0 || position < plateauEnd))) {
    return 1.0f;
  }

  // Left-side envelope: Fade In -> Intro Hold -> Volume Up Ramp
  float leftFactor = 1.0f;
  const float introTarget = hasIntroEnv ? startVol : 1.0f;
  if (position < meta.fadeInEndPoint) {
    const qint64 fadeInDuration = meta.fadeInEndPoint - meta.startPoint;
    if (fadeInDuration > 0) {
      const float progress = std::clamp(
          static_cast<float>(position - meta.startPoint) /
              static_cast<float>(fadeInDuration),
          0.0f, 1.0f);
      leftFactor = equalPowerFactor(progress) * introTarget;
    } else {
      leftFactor = introTarget;
    }
  } else if (hasIntroEnv && position < meta.volumeUpEndPoint) {
    if (position <= meta.volumeUpStartPoint ||
        meta.volumeUpEndPoint <= meta.volumeUpStartPoint) {
      leftFactor = startVol;
    } else {
      const qint64 rampDuration =
          meta.volumeUpEndPoint - meta.volumeUpStartPoint;
      const float t = std::clamp(
          static_cast<float>(position - meta.volumeUpStartPoint) /
              static_cast<float>(rampDuration),
          0.0f, 1.0f);
      leftFactor = startVol + t * (1.0f - startVol);
    }
  }

  // Right-side envelope: Volume Down Ramp -> Outro Hold -> Fade Out
  float rightFactor = 1.0f;
  const float outroTarget = hasOutroEnv ? endVol : 1.0f;
  if (hasOutroEnv && position >= meta.volumeDownStartPoint &&
      position < meta.fadeOutStartPoint) {
    if (position >= meta.volumeDownEndPoint ||
        meta.volumeDownEndPoint <= meta.volumeDownStartPoint) {
      rightFactor = endVol;
    } else {
      const qint64 rampDuration =
          meta.volumeDownEndPoint - meta.volumeDownStartPoint;
      const float t = std::clamp(
          static_cast<float>(position - meta.volumeDownStartPoint) /
              static_cast<float>(rampDuration),
          0.0f, 1.0f);
      rightFactor = 1.0f - t * (1.0f - endVol);
    }
  } else if (meta.endPoint > 0 && position >= meta.fadeOutStartPoint) {
    const qint64 fadeOutDuration = meta.endPoint - meta.fadeOutStartPoint;
    if (fadeOutDuration <= 0) {
      rightFactor = 0.0f;
    } else {
      const float progress = std::clamp(
          static_cast<float>(meta.endPoint - position) /
              static_cast<float>(fadeOutDuration),
          0.0f, 1.0f);
      rightFactor = equalPowerFactor(progress) * outroTarget;
    }
  }

  return std::min(leftFactor, rightFactor);
}

void PlayerManager::applyVolumeForPosition(qint64 position) {
  // Outside playable boundaries before startPoint (silence)
  if (position < this->metadata.startPoint) {
    this->applyVolume(0.0f);
    return;
  }

  if (this->metadata.endPoint > 0 && position >= this->metadata.endPoint) {
    this->applyVolume(0.0f);
    return;
  }

  // High-performance fast path: Plateau region (full target volume)
  // Statistically >95% of calls will hit this branch.
  if (Q_LIKELY(position >= this->m_plateauStart &&
               (this->metadata.endPoint <= 0 || position < this->m_plateauEnd))) {
    this->applyVolume(this->m_targetVolume);
    return;
  }

  float leftFactor = 1.0f;
  const float startVol = std::clamp(this->metadata.startVolume, 0.0f, 1.0f);
  const float introTarget = this->m_hasIntroEnvelope ? startVol : 1.0f;
  if (position < this->metadata.fadeInEndPoint) {
    const qint64 fadeInDuration =
        this->metadata.fadeInEndPoint - this->metadata.startPoint;
    if (Q_UNLIKELY(fadeInDuration <= 0)) {
      leftFactor = introTarget;
    } else {
      if (Q_UNLIKELY(this->m_invFadeInDuration <= 0.0f)) {
        this->m_invFadeInDuration = 1.0f / static_cast<float>(fadeInDuration);
      }
      const float progress = std::clamp(
          static_cast<float>(position - this->metadata.startPoint) *
              this->m_invFadeInDuration,
          0.0f, 1.0f);
      leftFactor = equalPowerFactor(progress) * introTarget;
    }
  } else if (this->m_hasIntroEnvelope &&
             position < this->metadata.volumeUpEndPoint) {
    if (position <= this->metadata.volumeUpStartPoint ||
        this->metadata.volumeUpEndPoint <= this->metadata.volumeUpStartPoint) {
      leftFactor = startVol;
    } else {
      if (Q_UNLIKELY(this->m_invVolUpDuration <= 0.0f)) {
        this->m_invVolUpDuration = 1.0f / static_cast<float>(
                                              this->metadata.volumeUpEndPoint -
                                              this->metadata.volumeUpStartPoint);
      }
      const float t = std::clamp(
          static_cast<float>(position - this->metadata.volumeUpStartPoint) *
              this->m_invVolUpDuration,
          0.0f, 1.0f);
      leftFactor = startVol + t * (1.0f - startVol);
    }
  }

  float rightFactor = 1.0f;
  const float endVol = std::clamp(this->metadata.endVolume, 0.0f, 1.0f);
  const float outroTarget = this->m_hasOutroEnvelope ? endVol : 1.0f;
  if (this->m_hasOutroEnvelope &&
      position >= this->metadata.volumeDownStartPoint &&
      position < this->metadata.fadeOutStartPoint) {
    if (position >= this->metadata.volumeDownEndPoint ||
        this->metadata.volumeDownEndPoint <=
            this->metadata.volumeDownStartPoint) {
      rightFactor = endVol;
    } else {
      if (Q_UNLIKELY(this->m_invVolDownDuration <= 0.0f)) {
        this->m_invVolDownDuration =
            1.0f / static_cast<float>(this->metadata.volumeDownEndPoint -
                                      this->metadata.volumeDownStartPoint);
      }
      const float t = std::clamp(
          static_cast<float>(position - this->metadata.volumeDownStartPoint) *
              this->m_invVolDownDuration,
          0.0f, 1.0f);
      rightFactor = 1.0f - t * (1.0f - endVol);
    }
  } else if (this->metadata.endPoint > 0 &&
             position >= this->metadata.fadeOutStartPoint) {
    const qint64 fadeOutDuration =
        this->metadata.endPoint - this->metadata.fadeOutStartPoint;
    if (Q_UNLIKELY(fadeOutDuration <= 0)) {
      rightFactor = 0.0f;
    } else {
      if (Q_UNLIKELY(this->m_invFadeOutDuration <= 0.0f)) {
        this->m_invFadeOutDuration = 1.0f / static_cast<float>(fadeOutDuration);
      }
      const float progress = std::clamp(
          static_cast<float>(this->metadata.endPoint - position) *
              this->m_invFadeOutDuration,
          0.0f, 1.0f);
      rightFactor = equalPowerFactor(progress) * outroTarget;
    }
  }

  this->applyVolume(std::min(leftFactor, rightFactor) * this->m_targetVolume);
}