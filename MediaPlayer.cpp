#include "MediaPlayer.h"

#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QWidget>
#include <QDebug>
#include <QDropEvent>
#include <QMimeData>

MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent)
{
    player = new QMediaPlayer(this);
    videoWidget = new QVideoWidget();
    audioOutput = new QAudioOutput(this);

    videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    videoWidget->setMinimumSize(400, 300);

    player->setVideoOutput(videoWidget);
    player->setAudioOutput(audioOutput);

    connect(player, &QMediaPlayer::mediaStatusChanged, this, &MediaPlayer::HandleMediaStatus);
}

void MediaPlayer::HandleMediaStatus(QMediaPlayer::MediaStatus status)
{
    if(QMediaPlayer::LoadedMedia == status)
        player->play();

}

void MediaPlayer::loadVideo(const QString &path) 
{  
    QUrl url = QUrl::fromLocalFile(path);

    if(!url.isEmpty())  player->setSource(url);  

}

MediaPlayer::~MediaPlayer()
{
    qDebug() << "ok";
}