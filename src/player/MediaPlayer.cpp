#include "MediaPlayer.h"
#include "AudioOutput.h"
#include "Decoder.h"

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

        qint64 posRange = vFrame.posMs - aPosMs;
        if (posRange < -35 || posRange > 35) {
            m_audio->seekTo(vFrame.posMs); // Prototype
        }

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

    if (!m_decoder) {
        qDebug() << "Core: Decoder not init";
        return false;
    }
    if (!m_audio->init(m_decoder, 48000)) {
        qDebug() << "Core: Coulnd't init audio";
        return false;
    }

    connect(this, &MediaPlayer::volumeChanged, m_audio, &AudioOutput::setVolume);

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
        m_renderTimer->start(16);
    } else {
        qDebug() << "Core: Timer not init";
        // int ret = initTimer();
    }
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

    qDebug() << "Core: MediaPlayer send ok";
}