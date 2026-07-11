#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

extern "C" {
#include "libavcodec/avcodec.h"
}

#include <QMediaPlayer>
#include <QWidget>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QImage>
#include <QVideoSink>

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
    
    void sentToSink(QImage &image, QVideoSink *sink);
    bool loadFrame(const char *filename, int &width, int &height);
    QImage renderFrame(AVFrame *frame, AVCodecContext *codecContext);

    ~MediaPlayer() override;

private slots:
    void HandleMediaStatus(QMediaPlayer::MediaStatus status);

private:
    QMediaPlayer *m_player;
    QVideoWidget *m_videoWidget;
    QAudioOutput *m_audioOutput;
    QVideoSink   *sink;
};

#endif // MEDIAPLAYER_H