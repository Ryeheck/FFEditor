#ifndef DECODER_H
#define DECODER_H

#include <QObject>
#include <QImage>
#include <atomic>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

#include "frameQueue.hpp"

class decoder : public QObject
{
    Q_OBJECT

signals:
    void finished();
    void durationChanged(qint64 durationMs);
    void positionChanged(qint64 posMs);
    
public:
    explicit decoder(QObject *parent = nullptr);
    ~decoder() override;

    bool getNextFrame(videoFrame &vFrame);

public slots:
    void processVideo();
    bool loadSource(const QString &filename);
    void seek(double posMs);

private:
    AVFramePtr cloneToSharedPtr(AVFrame *frame);
    int64_t getFramePosMs(const AVFrame *frame) const;
    void seekTo(double posMs);

    AVCodecContext *m_codecContext   = nullptr;
    AVFormatContext *m_formatContext = nullptr;

    frameQueue m_frameQueue;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_seekReq{false};
    std::atomic<double> m_seekTargetMs{0};
    std::atomic<bool> m_isSeeking{false};

    int m_videoStreamIndex = -1;
};

#endif // DECODER_H