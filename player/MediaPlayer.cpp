#include "MediaPlayer.h"
#include "Decoder.h"
#include "miniaudio.h"

#include <QAudioOutput>
#include <QDebug>
#include <QImage>
#include <QTimer>

MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent)
{
    m_renderTimer = new QTimer(this);
    connect(m_renderTimer, &QTimer::timeout, this, &MediaPlayer::processNextFrame);

    
}

bool MediaPlayer::initAudio()
{
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32; // Works with float32 (-1 - 1)
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
    /* Comming soon... */
}

void MediaPlayer::processNextFrame()
{
    videoFrame vFrame;

    if (m_decoder->getNextFrame(vFrame)) {
        
        emit frameChanged(vFrame.frame);

        positionChanged(vFrame.posMs);
    }
}

void MediaPlayer::cleanupDecoder()
{
    if (m_decoderThread.isRunning()) {
        m_decoderThread.quit();
        m_decoderThread.deleteLater();
    }

    m_renderTimer->stop();

    m_decoder = nullptr;
}

bool MediaPlayer::initDecoder()
{
    m_decoder = new Decoder();
    m_decoder->moveToThread(&m_decoderThread);
    
    connect(&m_decoderThread, &QThread::started, this,  [this] () {
        // Release FPS on video, coming soon...
        m_renderTimer->start(16);
        
    });
    connect(m_decoder, &Decoder::durationChanged, this, &MediaPlayer::durationChanged);
    connect(m_decoder, &Decoder::finished, &m_decoderThread, &QThread::quit);
    connect(m_decoder, &Decoder::finished, m_decoder, &QObject::deleteLater);
    connect(&m_decoderThread, &QThread::started, m_decoder, &Decoder::processVideo);
            
    return true;
}

bool MediaPlayer::loadVideo(const QString &path) 
{  
    cleanupDecoder();
    initAudio();
    initDecoder();

    if (!m_decoder->loadSource(path)) {
        qDebug() << "Couldn't open video file: " << path;
        return false;
    }

    return true;
}

void MediaPlayer::play()
{
    if (m_state == playbackState::Playing) return;
    m_state = playbackState::Playing;

    if (!m_decoderThread.isRunning()) {
        m_decoderThread.start();
    }
        
}

void MediaPlayer::stop()
{
    if (m_state == playbackState::Stopped) return;
    m_state = playbackState::Stopped;

    cleanupDecoder();
}

void MediaPlayer::onPositionChanged(qint64 pos)
{
    m_decoder->seek(pos);
}

void MediaPlayer::pause() 
{  
    if (m_state == playbackState::Paused) return;
    m_state = playbackState::Paused;

    if (m_decoderThread.isRunning())
        m_decoderThread.wait();
}


MediaPlayer::~MediaPlayer()
{
    cleanupDecoder();

    qDebug() << "MediaPlayer: ok";
}