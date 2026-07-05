#include "MediaPlayer.h"

#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QWidget>
#include <QDebug>
#include <QDropEvent>
#include <QMimeData>
#include <QSlider>

MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent)
{
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);

    m_videoWidget = new QVideoWidget();
    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_videoWidget->setMinimumSize(400, 300);

    m_player->setVideoOutput(m_videoWidget);
    m_player->setAudioOutput(m_audioOutput);

    connect(m_player, &QMediaPlayer::positionChanged, this, &MediaPlayer::onInternalPositionChanged);
    // connect(player, &QMediaPlayer::mediaStatusChanged, this, &MediaPlayer::HandleMediaStatus);
}

void MediaPlayer::HandleMediaStatus(QMediaPlayer::MediaStatus status)
{
    if(QMediaPlayer::LoadedMedia == status)
        m_player->play();
    
}

void MediaPlayer::loadVideo(const QUrl &url) 
{  
    if(!url.isEmpty()) {  
        m_player->setSource(url);
        m_player->play();
    }  
}

void MediaPlayer::loadVideo(const QString &path) 
{  
    QUrl url = QUrl::fromLocalFile(path);

    if(!url.isEmpty()) {  
        m_player->setSource(url);  
        m_player->play();
    }  
}

void MediaPlayer::onInternalPositionChanged(qint64 pos)
{
    emit positionChanged(pos, m_player->duration());
}

void MediaPlayer::onPositionChanged(qint64 pos)
{
    m_player->setPosition(pos);
}

void MediaPlayer::pauseVideo() 
{  
    if(m_player->playbackState() == QMediaPlayer::PlayingState)
        m_player->pause();  
    else
        m_player->play();

};

MediaPlayer::~MediaPlayer()
{
    qDebug() << "ok";
}