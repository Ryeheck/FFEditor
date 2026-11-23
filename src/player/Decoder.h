#ifndef DECODER_H
#define DECODER_H

#include "FrameQueue.hpp"

#include <QObject>
#include <QImage>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <qhashfunctions.h>
#include <qtypes.h>
#include <vector>

extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/packet.h>
#include "libswresample/swresample.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

struct AVFrameDeleter {
    void operator()(AVFrame *f) const {  av_frame_free(&f);  }
};

using AVFramePtr = std::shared_ptr<AVFrame>;

struct videoFrame {
    AVFramePtr frame = nullptr;
    qint64 posMs = 0;
    bool preview = false;
};

struct audioFrame {
    std::vector<float> samples{0};
    qint64 posMs = 0;
};

class Decoder : public QObject
{
    Q_OBJECT

public:
    explicit Decoder(QObject *parent = nullptr);
    ~Decoder() override;

    bool getNextVFrame(videoFrame &vFrame);
    bool getNextAFrame(audioFrame &aFrame);
    bool peekNextVFrame(videoFrame &vFrame);

public slots:
    void stop();
    void pause();
    void resume();
    void reset();
    void clear();
    void notify();
    void seek(qint64 posMs); 
    void decodeVideoLoop();
    void demuxLoop();
    void decodeAudioLoop();
    bool loadSource(const QString &filename);
    bool initAudio(int channels = 2);
    qint64 getDuration();
    
private:
    void waitIfPause();
    void seekTo(int64_t posMs);
    bool isSeeking() const;
    bool seekInBuffer(int64_t posMs);
    bool isPaused();
    bool demuxShouldWait();
    bool drainVideoFrame(AVFrame *frame);
    bool drainAudioFrame(AVFrame *frame);

    QString errorString(int ret);
    AVFramePtr cloneToSharedPtr(AVFrame *frame);
    int64_t getFramePosMs(const AVFrame *frame, int streamIndex) const;

    AVCodecContext *m_codecContextAudio = nullptr;
    AVCodecContext *m_codecContextVideo = nullptr;
    AVFormatContext *m_formatContext    = nullptr;
    SwrContext *m_swrContext            = nullptr;
    
    FrameQueue<AVPacket *> m_pktVQueue;
    FrameQueue<AVPacket *> m_pktAQueue;
    
    FrameQueue<videoFrame > m_frameVQueue;
    FrameQueue<audioFrame > m_frameAQueue;

    std::mutex m_videoMtx;
    std::mutex m_audioMtx;
    std::mutex m_pauseMtx;

    std::condition_variable m_pauseCv;

    std::atomic<bool> m_abort   {false};
    std::atomic<bool> m_seekReq {false};
    std::atomic<bool> m_isSeekV {false};
    std::atomic<bool> m_isSeekA {false};
    std::atomic<bool> m_Iframe  {false};
    std::atomic<bool> m_preview {false};
    std::atomic<bool> m_paused  {false};

    std::atomic<int64_t> m_seekTargetMs{0};

    int m_sampleRate       = -1;
    int m_channels         = -1;
    int m_videoStreamIndex = -1;
    int m_audioStreamIndex = -1;
    
    int64_t m_duration     = -1;
};

#endif // DECODER_H