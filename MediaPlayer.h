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
    void positionChanged(qint64 pos);
    void durationChanged(qint64 duration);

public:
    explicit MediaPlayer(QObject *parent = nullptr);
    
    QVideoWidget *getVideoWidget() {  return m_videoWidget;  };
    
    void onPositionChanged(qint64 pos);
    void loadVideo(const QString &path);
    void loadVideo(const QUrl &url);
    void setVolume(float value) {  m_audioOutput->setVolume(value);  };
    void stopVideo()  {  m_player->stop();  };
    void playVideo()  {  m_player->play();  };
    void pauseVideo();
    
    ~MediaPlayer() override;

private slots:
    void HandleMediaStatus(QMediaPlayer::MediaStatus status);

private:
    QMediaPlayer *m_player;
    QVideoWidget *m_videoWidget;
    QAudioOutput *m_audioOutput;

};

#endif // MEDIAPLAYER_H