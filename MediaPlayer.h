#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QMediaPlayer>
#include <QWidget>
#include <QVideoWidget>

class MediaPlayer : public QObject
{
    Q_OBJECT

public:
    explicit MediaPlayer(QObject *parent = nullptr);
    
    QVideoWidget *getVideoWidget() {  return videoWidget;  };
    
    void loadVideo(const QString &path);
    
    ~MediaPlayer() override;

private slots:
    void HandleMediaStatus(QMediaPlayer::MediaStatus status);

private:
    QMediaPlayer *player;
    QVideoWidget *videoWidget;
    QAudioOutput *audioOutput;

};

#endif // MEDIAPLAYER_H