#include "MediaPlayer.h"
#include "AudioOutput.h"
#include "Decoder.h"

#include <qobject.h>
#include <QAudioOutput>
#include <QDebug>
#include <QImage>
#include <QTimer>
#include <qtypes.h>

MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent)
{
    m_renderTimer = new QTimer(this);
    connect(m_renderTimer, &QTimer::timeout, this, &MediaPlayer::processNextVFrame);

    
}

void MediaPlayer::processNextVFrame()
{
    videoFrame vFrame;
    qint64 aPosMs = 0;

    if (m_audio)
        aPosMs = m_audio->getAudioClockMs();

    if (m_decoder->getNextVFrame(vFrame)) {
        if (!vFrame.frame)          return;
        if (vFrame.posMs > aPosMs)  return;

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

bool MediaPlayer::initAudio()
{
    m_audio = new AudioOutput();

    // connects
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

    /*if (ma_device_start(&m_audioDevice) != MA_SUCCESS) {
        ma_device_uninit(&m_audioDevice);
        return;
    }*/

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
    if (m_audio)
        m_audio->seekTo(pos);


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