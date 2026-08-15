#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QVideoWidget>
#include <QAudioOutput>
#include <QImage>
#include <QVideoSink>
#include <QThread>

#include "decoder.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

class MediaPlayer : public QObject
{
    Q_OBJECT

signals:
    void positionChanged(qint64 pos);
    void durationChanged(qint64 duration);

public:
    explicit MediaPlayer(QObject *parent = nullptr);
    
    QVideoWidget *getVideoWidget() {  return m_videoWidget;  };
    void setVolume(float value) {  m_audioOutput->setVolume(value);  };

    void onPositionChanged(qint64 pos);
    void loadVideo(const QString &path);
    
    void pause();
    void play();
    void stop();

    ~MediaPlayer() override;

public slots:
    void sentToSink(const QImage &frame);

private slots:


private:
    decoder *m_decoder = nullptr;
    QThread m_decoderThread;

    QVideoWidget *m_videoWidget = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QVideoSink   *m_sink = nullptr;
};

#endif // MEDIAPLAYER_H