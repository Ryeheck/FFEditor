#include "MediaPlayer.h"
#include "AudioOutput.h"
#include "Decoder.h"
#include "../EventBus.h"

#include <qdebug.h>
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

    qint64 aPosMs = m_audio ? m_audio->getAudioClockMs() : vFrame.posMs;
    int diffMs = vFrame.posMs - aPosMs;
    
    /* Range [-100 ; 20] ms */
    // Videoframe is 100 ms behind
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
    
    return true;
}

bool MediaPlayer::initAudio()
{
    m_audio = new AudioOutput();

    if (!m_decoder) {
        qDebug() << "Core: Decoder not init";
        return false;
    }
    if (!m_audio->init(m_decoder, 48000)) {
        qDebug() << "Core: Coulnd't init audio";
        return false;
    }

    return true;
}

bool MediaPlayer::loadVideo(const QString &path) 
{  
    cleanupDecoder();
    if (!initDecoder())  return false;
    if (!initAudio())    return false;

    if (!m_decoder->loadSource(path)) {
        qDebug() << "Core: Couldn't open video file: " << path;
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

    if (m_renderTimer)  
        m_renderTimer->stop();
    if (m_audio)
        m_audio->stop();
    
    cleanupDecoder();
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

    qDebug() << "MediaPlayer destroy";
}