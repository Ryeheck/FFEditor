#ifndef DECODER_H
#define DECODER_H

#include <QObject>
#include <QImage>

extern "C" {
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

    void processVideo(const QString &filename, int width, int height);

    void stop() {  m_running = false;  };
    void play() {  m_running = true;   };

    ~decoder() override;

private:
    QImage renderFrame(AVFrame *frame);
    void prepareBuffer(int &width, int &height);

    SwsContext *swsCtx = nullptr;
    uint8_t *m_buffer = nullptr;
    int m_bufferLinesize = 0;
    bool m_running = false;
};

#endif // DECODER_H