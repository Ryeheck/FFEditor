#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include "Decoder.h"
#include "FrameQueue.hpp"

#include <QAudioOutput>
#include <QImage>
#include <QThread>
#include <QTimer>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

enum class playbackState {
    Stopped,
    Playing,
    Paused
};

class MediaPlayer : public QObject
{
    Q_OBJECT

signals:
    void positionChanged(qint64 pos);
    void durationChanged(qint64 duration);
    void frameChanged(const AVFramePtr &vFrame);

public:
    explicit MediaPlayer(QObject *parent = nullptr);
    ~MediaPlayer() override;

    void setVolume(float value) {  m_audioOutput->setVolume(value);  };

    bool loadVideo(const QString &path);
    void pause();
    void play();
    void stop();

public slots:
    void onPositionChanged(qint64 pos);

private slots:


private:
    void cleanupDecoder();
    void processNextFrame();

    Decoder *m_decoder = nullptr;
    QThread m_decoderThread;

    QTimer *m_renderTimer = nullptr;
    playbackState m_state = playbackState::Stopped;
    QAudioOutput *m_audioOutput = nullptr;
};

#endif // MEDIAPLAYER_H