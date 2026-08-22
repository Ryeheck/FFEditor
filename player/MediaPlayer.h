#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QAudioOutput>
#include <QImage>
#include <QThread>
#include <QTimer>

#include "decoder.h"
#include "videoWidget.h"

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
    
    videoWidget *getVideoWidget() {  return m_videoWidget;  };
    void setVolume(float value) {  m_audioOutput->setVolume(value);  };

    void loadVideo(const QString &path);
    
    void pause();
    void play();
    void stop();

    ~MediaPlayer() override;

public slots:
    void onPositionChanged(qint64 pos);

private slots:


private:
    void processNextFrame();
    bool initDecoder();

    decoder *m_decoder = nullptr;
    QThread m_decoderThread;

    videoWidget *m_videoWidget  = nullptr;
    QTimer *m_renderTimer = nullptr;

    QAudioOutput *m_audioOutput = nullptr;
};

#endif // MEDIAPLAYER_H