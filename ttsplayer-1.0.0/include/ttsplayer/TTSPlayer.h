#ifndef TTSPLAYER_H
#define TTSPLAYER_H

#include "ttsplayer_global.h"
#include <QObject>
#include <memory>

class TTSPlayerImpl;

class TTSPLAYER_EXPORT TTSPlayer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)

public:
    explicit TTSPlayer(QObject *parent = nullptr);
    ~TTSPlayer();

    bool initialize(const QString &modelPath);
    bool initializeWithResources(const QString &modelPath, void *resourceManager);
    bool isReady() const;

    Q_INVOKABLE bool isPlaying() const;
    Q_INVOKABLE qreal volume() const;
    Q_INVOKABLE bool isMuted() const;

public slots:
    void play(QString text);
    void stop();
    void setVolume(qreal volume);
    void setMuted(bool muted);

signals:
    void readyChanged(bool ready);
    void playingChanged(bool playing);
    void volumeChanged(qreal volume);
    void mutedChanged(bool muted);
    void playbackStarted();
    void finished();
    void stopped();
    void errorOccurred(const QString &errorMsg);

private:
    friend class TTSPlayerImpl;

    Q_SIGNAL void requestInitialize(const QString &modelPath);
    Q_SIGNAL void requestInitializeWithResources(const QString &modelPath, void *resourceManager);
    Q_SIGNAL void requestPlay(const QString &text);
    Q_SIGNAL void requestStop();
    Q_SIGNAL void requestSetVolume(qreal volume);
    Q_SIGNAL void requestSetMuted(bool muted);

    void setReady(bool ready);
    void setPlaying(bool playing);
    void onVolumeChanged(qreal volume);
    void onMutedChanged(bool muted);

    std::unique_ptr<TTSPlayerImpl> d;
};

#endif
