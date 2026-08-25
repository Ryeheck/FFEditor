#include "videoWidget.h"
#include "MediaPlayer.h"
#include "decoder.h"

#include <QAudioOutput>
#include <QDebug>
#include <QImage>
#include <QTimer>
#include <iostream>

MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent)
{
    m_audioOutput = new QAudioOutput(this); 
    
    m_videoWidget = new videoWidget();
    
    m_renderTimer = new QTimer(this);
    connect(m_renderTimer, &QTimer::timeout, this, &MediaPlayer::processNextFrame);
}

void MediaPlayer::processNextFrame()
{
    videoFrame vFrame;

    if (m_decoder->getNextFrame(vFrame)) {
        
        m_videoWidget->setFrame(vFrame.frame);

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

bool MediaPlayer::loadVideo(const QString &path) 
{  
    cleanupDecoder();

    auto newDecoder = std::make_unique<decoder>();
    connect(newDecoder.get(), &decoder::durationChanged, this, &MediaPlayer::durationChanged);

    if (!newDecoder->loadSource(path)) {
        qDebug() << "Couldn't open video file: " << path;
        return false;
    }

    m_decoder = newDecoder.release();
    m_decoder->moveToThread(&m_decoderThread);
    
    connect(&m_decoderThread, &QThread::started, this,  [this] () {
        // Realise FPS on video, coming soon...
        m_renderTimer->start(16);
        
    });
    
    connect(m_decoder, &decoder::finished, &m_decoderThread, &QThread::quit);
    connect(m_decoder, &decoder::finished, m_decoder, &QObject::deleteLater);
    connect(&m_decoderThread, &QThread::started, m_decoder, &decoder::processVideo);
            
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

    m_decoderThread.wait(10000);
}


MediaPlayer::~MediaPlayer()
{

    qDebug() << "MediaPlayer: ok";
}