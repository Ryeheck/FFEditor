#include "MediaPlayer.h"
#include "AudioOutput.h"
#include "Decoder.h"
#include "../EventBus.h"

#include <qdebug.h>
#include <qobject.h>
#include <QDebug>
#include <QImage>
#include <QTimer>
#include <qtypes.h>

MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent)
{
    m_renderTimer = new QTimer(this);
    connect(m_renderTimer, &QTimer::timeout, this, &MediaPlayer::processNextFrame);

    connect(&EventBus::instance(), &EventBus::decoderFinished, this, &MediaPlayer::cleanupDecoder);
}

void MediaPlayer::processNextFrame()
{
    if (!m_decoder)  return;
    
    videoFrame vFrame;
    if (!m_decoder->peekNextVFrame(vFrame)) {
        // Queue empty, wait 5 ms 
        m_renderTimer->start(5);
        return;
    }

    // EOF
    if (!vFrame.frame) {
        stop();
        return;
    }

    // Preview
    if (vFrame.preview) {
        qDebug() << "Its preview!";
        m_decoder->getNextVFrame(vFrame);
        emit EventBus::instance().frameChanged(vFrame.frame);
        m_renderTimer->start(0);
    }

    qint64 aPosMs = m_audio ? m_audio->getAudioClockMs() : vFrame.posMs;
    int diffMs    = vFrame.posMs - aPosMs;
    
    /* Range [-50 ; 20] ms */
    // Videoframe is 50 ms behind
    if (diffMs < -50) {
        m_decoder->getNextVFrame(vFrame);
        m_renderTimer->start(0);
        return;
    }

    // Videoframe is 20 ms ahead
    if (diffMs > 20) {
        int delay = std::min(diffMs, 200);
        
        m_renderTimer->start(delay);
        return;
    }

    // Everything is ok
    m_decoder->getNextVFrame(vFrame);
    emit EventBus::instance().frameChanged(vFrame.frame);
    emit EventBus::instance().positionChanged(aPosMs);
    
    // Next frame
    int nextDelay = m_renderMs; // Is default 
    videoFrame nextVFrame;

    if (m_decoder->peekNextVFrame(nextVFrame) && nextVFrame.frame) {
        nextDelay = nextVFrame.posMs - vFrame.posMs;
    }

    int delay = std::max(1, nextDelay + diffMs);
    m_renderTimer->start(delay);
}

void MediaPlayer::cleanupDecoder()
{
    if (m_state == playbackState::Stopped) return;
    if (m_renderTimer)  m_renderTimer->stop();
    
    if (m_decoder) {
        m_decoder->clear();
        m_decoder->deleteLater();
    }
    m_decoder = nullptr;

    m_state = playbackState::Stopped;
}

bool MediaPlayer::initDecoder()
{
    m_decoder = new Decoder();
    
    return true;
}

void MediaPlayer::cleanupAudio()
{
    // m_state

    if (m_audio) {
        m_audio->uninit();
        m_audio->deleteLater();
    }
    m_audio = nullptr;

    // m_state
}   

bool MediaPlayer::initAudio(int channels)
{
    m_audio = new AudioOutput();

    if (!m_decoder) {
        qDebug() << "Core: Decoder not init";
        return false;
    }
    if (!m_audio->init(m_decoder, -1, channels)) {
        qDebug() << "Core: Coulnd't init audio";
        return false;
    }
    if (!m_decoder->initAudio(channels)) {
        qDebug() << "Core: Decoder couldn't init audio";
        return false;
    }

    return true;
}

void MediaPlayer::stopPlayback()
{
    if (m_renderTimer)  m_renderTimer->stop();
    if (m_decoder)      m_decoder->stop();

    if (m_demuxThread.joinable())       m_demuxThread.join();
    if (m_videoDecodeThread.joinable()) m_videoDecodeThread.join();
    if (m_audioDecodeThread.joinable()) m_audioDecodeThread.join();
        
    if (m_audio) {  m_audio->stop();  m_audio->uninit();  }
    if (m_decoder)  m_decoder->clear();

    m_state = playbackState::Stopped;
}

bool MediaPlayer::loadVideo(const QString &path) 
{  
    stopPlayback();

    if (!m_decoder) {
        if (!initDecoder())  return false;
    } else {
        m_decoder->reset();
    }

    if (!m_decoder->loadSource(path)) {
        qDebug() << "Core: Couldn't open video file: " << path;
        stop();
        return false;
    }

    if (!initAudio())  return false;
    
    m_decoder->notify();
    
    m_demuxThread       = std::thread(&Decoder::demuxLoop, m_decoder);
    m_videoDecodeThread = std::thread(&Decoder::decodeVideoLoop, m_decoder);
    m_audioDecodeThread = std::thread(&Decoder::decodeAudioLoop, m_decoder);

    return true;
}

void MediaPlayer::play()
{
    if (m_state == playbackState::Playing) return;
    m_state = playbackState::Playing;

    if (m_audio) {
        qDebug() << "Core: Audio start..";
        m_audio->start();        
    } else {
        qDebug() << "Core: Audio not init";
        // int ret = initAudio();
    }
    if (!m_decoder) {
        qDebug() << "Core: Decoder not init";
        // int ret = initDecoder();
        return;
    }
    if (m_renderTimer) {
        qDebug() << "Core: Timer start..";
        qDebug() << "Core: Decoder start..";
        m_renderTimer->start(m_renderMs);
    } else {
        qDebug() << "Core: Timer not init";
        // int ret = initTimer();
        return;
    }
}

void MediaPlayer::stop()
{
    if (m_state == playbackState::Stopped) return;
    m_state = playbackState::Stopped;

    if (m_renderTimer) {
        m_renderTimer->stop();
        qDebug() << "Core: Timer stop...";
    }
    if (m_audio) {
        m_audio->stop();
        qDebug() << "Core: Audio stop...";
    }
    if (m_decoder) {
        m_decoder->stop();
        qDebug() << "Core: Decoder stop...";
    }

    if (m_demuxThread.joinable())       m_demuxThread.join();
    if (m_videoDecodeThread.joinable()) m_videoDecodeThread.join();
    if (m_audioDecodeThread.joinable()) m_audioDecodeThread.join();

    cleanupDecoder();
}

void MediaPlayer::pause() 
{  
    if (m_state == playbackState::Paused) return;
    m_state = playbackState::Paused;

}


MediaPlayer::~MediaPlayer()
{
    if (m_renderTimer)  m_renderTimer->stop();
    if (m_decoder)      m_decoder->stop();
    if (m_audio)        m_audio->stop();

    if (m_demuxThread.joinable())       m_demuxThread.join();
    if (m_videoDecodeThread.joinable()) m_videoDecodeThread.join();
    if (m_audioDecodeThread.joinable()) m_audioDecodeThread.join();
    
    cleanupAudio();
    cleanupDecoder();
    
    qDebug() << "MediaPlayer destroy";
}