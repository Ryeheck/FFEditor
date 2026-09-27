#ifndef DECODER_H
#define DECODER_H

#include "FrameQueue.hpp"

#include <QObject>
#include <QImage>
#include <atomic>
#include <memory>
#include <vector>

extern "C" {
#include <libavcodec/packet.h>
#include "libswresample/swresample.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

using AVFramePtr = std::shared_ptr<AVFrame>;

struct videoFrame {
    AVFramePtr frame;
    qint64 posMs;
};

struct audioFrame {
    std::vector<float> samples;
    qint64 posMs;
};

class Decoder : public QObject
{
    Q_OBJECT

signals:
    void finished();
    void durationChanged(qint64 durationMs);
    void positionChanged(qint64 posMs);
    
public:
    explicit Decoder(QObject *parent = nullptr);
    ~Decoder() override;

    bool getNextVFrame(videoFrame &vFrame);
    bool getNextAFrame(audioFrame &aFrame);
public slots:
    void processVideo();
    void stop();
    void clear();
    bool loadSource(const QString &filename);
    void seek(double posMs); 
    void decodeVideoLoop();
    void demuxLoop();
    void decodeAudioLoop();

private:
    bool initAudio();
    void flushBuffers();
    void seekTo(double posMs);
    AVFramePtr cloneToSharedPtr(AVFrame *frame);
    int64_t getFramePosMs(const AVFrame *frame) const;

    AVCodecContext *m_codecContextAudio = nullptr;
    AVCodecContext *m_codecContextVideo = nullptr;
    AVFormatContext *m_formatContext    = nullptr;
    SwrContext *m_swrContext            = nullptr;

    FrameQueue<AVPacket *> m_pktVQueue;
    FrameQueue<AVPacket *> m_pktAQueue;
    
    FrameQueue<videoFrame> m_frameVQueue;
    FrameQueue<audioFrame> m_frameAQueue;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_seekReq{false};
    std::atomic<double> m_seekTargetMs{0};
    std::atomic<bool> m_isSeeking{false};
    std::atomic<bool> m_flushbuffers{false};

    int m_videoStreamIndex = -1;
    int m_audioStreamIndex = -1;
};

#endif // DECODER_H