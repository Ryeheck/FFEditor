#include "Decoder.h"
#include "FrameQueue.hpp"

#include <QImage>
#include <QString>
#include <QDebug>
#include <QThread>
#include <atomic>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

Decoder::Decoder(QObject *parent) : QObject(parent),
                                    m_pktVQueue(300),
                                    m_pktAQueue(100),
                                    m_frameVQueue(15),
                                    m_frameAQueue(100)
{
    
}

bool Decoder::loadSource(const QString &filename)
{
    // Close old video file
    if (m_codecContextVideo)  avcodec_free_context(&m_codecContextVideo);
    if (m_codecContextAudio)  avcodec_free_context(&m_codecContextAudio);
    if (m_formatContext)      avformat_close_input(&m_formatContext);
    
    // Initialization codecs
    AVCodecParameters *codecParams = nullptr;
    const AVCodec *codec = nullptr;

    // Open new video file  
    if (avformat_open_input(&m_formatContext, filename.toUtf8().constData(), NULL, NULL) != 0) {
        qDebug() << "Couldn't open video file: " << filename;
        goto cleanup;
    }

    // find stream info
    if (avformat_find_stream_info(m_formatContext, NULL) != 0) {
        qDebug() << "Couldn't find stream info";
        goto cleanup;
    }

    // Current streams
    m_videoStreamIndex = -1;
    m_audioStreamIndex = -1;
    for (int i = 0; i < m_formatContext->nb_streams; ++i)
    {
        if(m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) 
            m_videoStreamIndex = i;
        if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) 
            m_audioStreamIndex = i;
            
        if (m_videoStreamIndex != -1 && m_audioStreamIndex != -1)
            break;
    }
    
    // Not any streams
    if (m_videoStreamIndex == -1 || m_audioStreamIndex == -1) {
        qDebug() << "Couldn't open stream";
        goto cleanup;
    }

    // Send duration in UI
    if (m_formatContext->duration != AV_NOPTS_VALUE) {
        qint64 durationMs = (m_formatContext->duration * 1000) / AV_TIME_BASE;

        emit durationChanged(durationMs);
    }

    // Open codecs
    codecParams = m_formatContext->streams[m_videoStreamIndex]->codecpar;
    codec = avcodec_find_decoder(codecParams->codec_id);

    // Video codec
    m_codecContextVideo = avcodec_alloc_context3(codec);
    if (!m_codecContextVideo) {
        qDebug() << "Couldn't allocate codec context";
        goto cleanup;
    }
    if (avcodec_parameters_to_context(m_codecContextVideo, codecParams) != 0) {
        qDebug() << "Couldn't copy codec params to codec context";
        goto cleanup;
    }
    if (avcodec_open2(m_codecContextVideo, codec, NULL) != 0) {
        qDebug() << "Couldn't open codec";
        goto cleanup;
    }

    // Open codecs
    codecParams = m_formatContext->streams[m_audioStreamIndex]->codecpar;
    codec = avcodec_find_decoder(codecParams->codec_id);

    // Audio codec
    m_codecContextAudio = avcodec_alloc_context3(codec);
    if (!m_codecContextAudio) {
        qDebug() << "Couldn't allocate codec context";
        goto cleanup;
    }
    if (avcodec_parameters_to_context(m_codecContextAudio, codecParams) != 0) {
        qDebug() << "Couldn't copy codec params to codec context";
        goto cleanup;
    }
    if (avcodec_open2(m_codecContextAudio, codec, NULL) != 0) {
        qDebug() << "Couldn't open codec";
        goto cleanup;
    }

    return true;

cleanup:
    if (m_codecContextVideo)  
        avcodec_free_context(&m_codecContextVideo);
    if (m_codecContextAudio)
        avcodec_free_context(&m_codecContextAudio);

    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
        avformat_free_context(m_formatContext);
    }
    return false;
}

void Decoder::processVideo()
{
    m_running.store(true);

    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();

    while (m_running.load()) 
    {
        if (m_seekReq.exchange(false)) {
            seekTo(m_seekTargetMs.load());
            continue;
        }

        int ret = av_read_frame(m_formatContext, packet);
        if (ret < 0) break;

        if (packet->stream_index != m_videoStreamIndex) {
            av_packet_unref(packet);
            continue;
        }
        if (avcodec_send_packet(m_codecContextVideo, packet) != 0) {
            av_packet_unref(packet);
            continue;
        }
                
        while (avcodec_receive_frame(m_codecContextVideo, frame) == 0) // FRAME LOAD
        {  
            qint64 posMs = getFramePosMs(frame);
            if (m_isSeeking.load()) {
                if (posMs < m_seekTargetMs.load()) {
                    av_frame_unref(frame);
                    continue;
                } else {
                    m_isSeeking.store(false);
                }
            }
            videoFrame vFrame;
            vFrame.posMs = posMs;
            vFrame.frame = cloneToSharedPtr(frame);

            // Push frame to queue
            if (!m_frameVQueue.push(vFrame)) {
                // If queue is full
                av_frame_unref(frame);
                break;
            }
            av_frame_unref(frame);
        }
        av_packet_unref(packet);
        
    }

    // Free and abort queue
    av_frame_free(&frame);
    av_packet_free(&packet);
    m_frameVQueue.abort();

    emit finished();
}

void Decoder::seek(double posMs)
{
    m_seekReq.store(true);
    m_seekTargetMs.store(posMs);
}

void Decoder::seekTo(double posMs)
{
    if (!m_formatContext     || 
        !m_codecContextVideo || m_videoStreamIndex < 0 ||
        !m_codecContextAudio || m_audioStreamIndex < 0) {
        qDebug() << "seekTo: Not a video";
        return;
    }

    // clear queue
    m_frameVQueue.clear();

    // Correct stream
    AVStream *stream = m_formatContext->streams[m_videoStreamIndex];

    // From MS to TS
    int64_t targetTs = av_rescale_q(posMs, AVRational{1, 1000}, stream->time_base);

    // If there is a start time 
    if (stream->start_time != AV_NOPTS_VALUE) 
        targetTs += stream->start_time;

    // Seek
    int ret = avformat_seek_file(
        m_formatContext, m_videoStreamIndex,
        INT64_MIN, targetTs, INT64_MAX,
        AVSEEK_FLAG_BACKWARD       // 
    );
    if (ret < 0) {
        qDebug() << "Seek error: " << ret;
        return;
    }

    // Clear buffers
    AVFrame *frame = av_frame_alloc();
    while (avcodec_receive_frame(m_codecContextVideo, frame) == 0)
    {
        av_frame_unref(frame);
    }
    av_frame_free(&frame); 

    avcodec_flush_buffers(m_codecContextVideo);

    // if (audio)          avcodec_flush_buffers(audio);

    // Flags
    m_isSeeking.store(true);

}

int64_t Decoder::getFramePosMs(const AVFrame *frame) const
{
    if (!frame || !m_formatContext || !m_codecContextAudio) {
        qDebug() << "getFramePosMs: not a frame";
        return -1;
    }

    // Presentation Timestamp
    int64_t pts = frame->best_effort_timestamp;
    if (pts == AV_NOPTS_VALUE) {
        qDebug() << "getFramePosMs: not a pts";
        return -1;
    }

    AVStream *stream = m_formatContext->streams[m_videoStreamIndex];

    // If there is a start time
    if (stream->start_time == AV_NOPTS_VALUE)
        pts -= stream->start_time;
    
    // From TS to MS
    return av_rescale_q(pts, stream->time_base, AVRational{1, 1000});
}

AVFramePtr Decoder::cloneToSharedPtr(AVFrame *frame)
{
    AVFrame *dstFrame = av_frame_clone(frame);

    return AVFramePtr(dstFrame, [](AVFrame *frame) {  av_frame_free(&frame);  });
}

bool Decoder::getNextFrame(videoFrame &vFrame)
{
    return m_frameVQueue.pop(vFrame);
}

Decoder::~Decoder()
{
    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
        avformat_free_context(m_formatContext);
        m_formatContext = nullptr;
    }
    if (m_codecContextVideo) {
        avcodec_free_context(&m_codecContextVideo);
        m_codecContextVideo = nullptr;
    }
    if (m_codecContextAudio) {
        avcodec_free_context(&m_codecContextAudio);
        m_codecContextAudio = nullptr;
    }

    qDebug() << "Decoder: ok";
}