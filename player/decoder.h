#ifndef DECODER_H
#define DECODER_H

#include <QObject>
#include <QImage>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

class decoder : public QObject
{
    Q_OBJECT

signals:
    void frameDecoded(const QImage &frame);
    void finished();

public:
    explicit decoder(QObject *parent = nullptr);

    
    void stop() {  m_running = false;  };
    void play() {  m_running = true;   };

    ~decoder() override;

public slots:
    void processVideo();
    bool loadSource(const QString &filename);

private:
    QImage renderFrame(AVFrame *frame);
    

    AVCodecContext *m_codecContext   = nullptr;
    AVFormatContext *m_formatContext = nullptr;
    SwsContext *m_swsCtx             = nullptr;
    
    uint8_t *m_buffer = nullptr;

    int m_videoStreamIndex = -1;
    int m_bufferLinesize   = 0;
    
    bool m_running = false;

    
};

#endif // DECODER_H