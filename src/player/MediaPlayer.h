#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include "Decoder.h"

#include <QAudioOutput>
#include <QImage>
#include <QThread>
#include <QTimer>
#include <cstddef>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include "miniaudio.h"
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

    void setVolume(float value) {  /* m_audioOutput->setVolume(value); */ };

    bool loadVideo(const QString &path);
    void pause();
    void play();
    void stop();

public slots:
    void onPositionChanged(qint64 pos);

private slots:


private:
    static void audioCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount);
    bool initAudio();
    bool initDecoder();
    void cleanupDecoder();
    void processNextVFrame();
    void processNextAFrame(float *pOutput, ma_uint32 frameCount);

    std::thread m_demuxThread;
    std::thread m_videoDecodeThread;
    std::thread m_audioDecodeThread;

    ma_device m_audioDevice;
    Decoder *m_decoder      = nullptr; 
    QTimer *m_renderTimer   = nullptr;
    playbackState m_state   = playbackState::Stopped;
    bool m_audioInit        = false;
    
    audioFrame m_aFrame;
    size_t m_aFrameOffset;
    
};

#endif // MEDIAPLAYER_H