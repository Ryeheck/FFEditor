#include <QVideoWidget>
#include <QAudioOutput>
#include <QDebug>
#include <QImage>
#include <QVideoSink>
#include <QVideoFrameFormat>
#include <QVideoFrame>
#include <QMetaObject>

#include "MediaPlayer.h"
#include "decoder.h"

MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent)
{
    m_audioOutput = new QAudioOutput(this);
    m_videoWidget = new QVideoWidget();

    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_videoWidget->setMinimumSize(400, 300);
    m_sink = m_videoWidget->videoSink();
    
    m_decoder = new decoder();
    m_decoder->moveToThread(&m_decoderThread);

    connect(m_decoder, &decoder::frameDecoded, this, &MediaPlayer::sentToSink);

    connect(m_decoder, &decoder::finished, &m_decoderThread, &QThread::quit);
    connect(m_decoder, &decoder::finished, m_decoder, &QObject::deleteLater);
    connect(&m_decoderThread, &QThread::started, m_decoder, &decoder::processVideo);
}

void MediaPlayer::loadVideo(const QString &path) 
{  
    if (!m_decoderThread.isRunning()) {
        if (m_decoder->loadSource(path))
            m_decoderThread.start();
    }
}

void MediaPlayer::play()
{
    if (!m_decoderThread.isRunning())
        m_decoderThread.start();
}

void MediaPlayer::stop()
{
    if (m_decoderThread.isRunning())
        // QMetaObject::invokeMethod(m_decoder, &decoder::stop, Qt::QueuedConnection);
}

void MediaPlayer::onPositionChanged(qint64 pos)
{
    // m_player->setPosition(pos);
}

void MediaPlayer::pause() 
{  

}

void MediaPlayer::sentToSink(const QImage &image)
{
    QVideoFrame frame(image);

    if (!m_sink) {
        qDebug() << "Sink isnt valid";
        return;
    }
    if (!frame.isValid()) {
        qDebug() << "Frame isnt valid";
        return;   
    } 

    m_sink->setVideoFrame(frame);
}


MediaPlayer::~MediaPlayer()
{

    qDebug() << "MediaPlayer: ok";
}