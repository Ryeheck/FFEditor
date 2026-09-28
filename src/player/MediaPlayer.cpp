#define MINIAUDIO_IMPLEMENTATION

#include "MediaPlayer.h"
#include "Decoder.h"
#include "miniaudio.h"

#include <qobject.h>
#include <QAudioOutput>
#include <QDebug>
#include <QImage>
#include <QTimer>
#include <algorithm>
#include <cstddef>
#include <utility>

MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent)
{
    m_renderTimer = new QTimer(this);
    connect(m_renderTimer, &QTimer::timeout, this, &MediaPlayer::processNextVFrame);

    
}

bool MediaPlayer::initAudio()
{
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32; // Works with float32 ([-1 : 1])
    config.playback.channels = 2;             // Stereo
    config.sampleRate        = 48000;         // Ghz
    config.dataCallback      = audioCallback; // Function
    config.pUserData         = this;          // Ptr on this player

    if (ma_device_init(NULL, &config, &m_audioDevice) != MA_SUCCESS)
        return false;
    
    m_audioInit = true;
    return true;
}

void MediaPlayer::audioCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    MediaPlayer *player = static_cast<MediaPlayer *>(pDevice->pUserData);

    if (player)
        player->processNextAFrame(static_cast<float *>(pOutput), frameCount);
}

void MediaPlayer::processNextAFrame(float *pOutput, ma_uint32 frameCount)
{
    // STEREO
    size_t samplesNeeded = frameCount * 2;
    size_t samplesFilled = 0;

    while (samplesFilled < samplesNeeded) 
    {
        if (m_aFrameOffset >= m_aFrame.samples.size()) {
            audioFrame nextFrame;

            if (m_decoder && m_decoder->getNextAFrame(nextFrame)) {
                m_aFrame = std::move(nextFrame);
                m_aFrameOffset = 0;
            } else {
                std::fill_n(pOutput + samplesFilled, samplesNeeded - samplesFilled, 0.0f);
                break;
            }
        }
        size_t samplesAvailable = m_aFrame.samples.size() - m_aFrameOffset;
        size_t samplesToCopy = std::min(samplesNeeded -  samplesFilled, samplesAvailable);
        std::copy_n(m_aFrame.samples.data() + m_aFrameOffset,
                    samplesToCopy,
                    pOutput + samplesFilled);
        
        m_aFrameOffset += samplesToCopy;
        samplesFilled += samplesToCopy;

    }

}

void MediaPlayer::processNextVFrame()
{
    videoFrame vFrame;

    if (m_decoder->getNextVFrame(vFrame)) {
        if (!vFrame.frame)  return;

        emit frameChanged(vFrame.frame);

        positionChanged(vFrame.posMs);
    }
}

void MediaPlayer::cleanupDecoder()
{
    if (m_decoder) {
        m_decoder->stop();

        if (m_demuxThread.joinable())       m_demuxThread.join();
        if (m_videoDecodeThread.joinable()) m_videoDecodeThread.join();
        if (m_audioDecodeThread.joinable()) m_audioDecodeThread.join();

        m_decoder->deleteLater();
        m_decoder = nullptr;
    }     
    if (m_renderTimer)  
        m_renderTimer->stop();
    
    m_state = playbackState::Stopped;
}

bool MediaPlayer::initDecoder()
{
    m_decoder = new Decoder();
    
    connect(m_decoder, &Decoder::durationChanged, this, &MediaPlayer::durationChanged);
    connect(m_decoder, &Decoder::finished, this, &MediaPlayer::cleanupDecoder);

    return true;
}

bool MediaPlayer::loadVideo(const QString &path) 
{  
    cleanupDecoder();
    initDecoder();
    initAudio();

    if (!m_decoder->loadSource(path)) {
        qDebug() << "Couldn't open video file: " << path;
        cleanupDecoder();
        return false;
    }
    
    m_demuxThread       = std::thread(&Decoder::demuxLoop, m_decoder);
    m_videoDecodeThread = std::thread(&Decoder::decodeVideoLoop, m_decoder);
    m_audioDecodeThread = std::thread(&Decoder::decodeAudioLoop, m_decoder);

    
    return true;
}

void MediaPlayer::play()
{
    if (m_state == playbackState::Playing) return;
    m_state = playbackState::Playing;

    if (ma_device_start(&m_audioDevice) != MA_SUCCESS) {
        ma_device_uninit(&m_audioDevice);
        return;
    }

    m_renderTimer->start(16);
        
}

void MediaPlayer::stop()
{
    if (m_state == playbackState::Stopped) return;
    m_state = playbackState::Stopped;

    if (m_renderTimer)  
        m_renderTimer->stop();

    cleanupDecoder();
}

void MediaPlayer::onPositionChanged(qint64 pos)
{
    if (m_decoder)
        m_decoder->seek(pos);
}

void MediaPlayer::pause() 
{  
    if (m_state == playbackState::Paused) return;
    m_state = playbackState::Paused;

}


MediaPlayer::~MediaPlayer()
{
    cleanupDecoder();

    if (m_renderTimer)
        m_renderTimer->deleteLater();

    qDebug() << "MediaPlayer: ok";
}