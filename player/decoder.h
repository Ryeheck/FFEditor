#ifndef DECODER_H
#define DECODER_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

#include <QObject>
#include <QImage>

class decoder : public QObject
{
    Q_OBJECT

signals:
    void frameDecoded(const QImage &frame);

public:
    void processVideo(const QString &filename, int width, int height);

    ~decoder() override;
private:
    QImage renderFrame(AVFrame *frame);
    void prepareBuffer(int &width, int &height);

    SwsContext *swsCtx = nullptr;
    uint8_t *m_buffer = nullptr;
    int m_bufferLinesize = 0;
};

#endif // DECODER_H