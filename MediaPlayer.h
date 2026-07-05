#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QMediaPlayer>
#include <QWidget>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QSlider>

class MediaPlayer : public QObject
{
    Q_OBJECT

signals:
    void positionChanged(qint64 pos, qint64 totalMS);

public:
    explicit MediaPlayer(QObject *parent = nullptr);
    
    QVideoWidget *getVideoWidget() {  return videoWidget;  };
    
    void loadVideo(const QString &path);
    void loadVideo(const QUrl &url);
    void setVolume(float value) {  audioOutput->setVolume(value);  };
    void stopVideo()  {  player->stop();  };
    void playVideo()  {  player->play();  };
    void pauseVideo();
    
    ~MediaPlayer() override;

private slots:
    void HandleMediaStatus(QMediaPlayer::MediaStatus status);

private:
    void onInternalPositionChanged(qint64 pos);
    QMediaPlayer *player;
    QVideoWidget *videoWidget;
    QAudioOutput *audioOutput;
    QSlider *volumeSlider;
};

#endif // MEDIAPLAYER_H