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
    player = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);

    videoWidget = new QVideoWidget();
    videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    videoWidget->setMinimumSize(400, 300);

    player->setVideoOutput(videoWidget);
    player->setAudioOutput(audioOutput);

    connect(player, &QMediaPlayer::positionChanged, this, &MediaPlayer::onInternalPositionChanged);
    // connect(player, &QMediaPlayer::mediaStatusChanged, this, &MediaPlayer::HandleMediaStatus);
}

void MediaPlayer::HandleMediaStatus(QMediaPlayer::MediaStatus status)
{
    if(QMediaPlayer::LoadedMedia == status)
        player->play();
    
}

void MediaPlayer::loadVideo(const QUrl &url) 
{  
    if(!url.isEmpty()) {  
        player->setSource(url);
        player->play();
    }  
}

void MediaPlayer::loadVideo(const QString &path) 
{  
    QUrl url = QUrl::fromLocalFile(path);

    if(!url.isEmpty()) {  
        player->setSource(url);  
        player->play();
    }  
}

void MediaPlayer::onInternalPositionChanged(qint64 pos)
{
    emit positionChanged(pos, player->duration());
}

void MediaPlayer::pauseVideo() 
{  
    if(player->playbackState() == QMediaPlayer::PlayingState)
        player->pause();  
    else
        player->play();

};

MediaPlayer::~MediaPlayer()
{
    qDebug() << "ok";
}