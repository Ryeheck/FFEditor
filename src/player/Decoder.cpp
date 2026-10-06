#include "Decoder.h"
#include "FrameQueue.hpp"
#include "../EventBus.h"

#include <QString>
#include <QDebug>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <qdebug.h>
#include <qtypes.h>
#include <thread>

extern "C" {
#include <libswresample/swresample.h>
#include <libavcodec/packet.h>
#include <libavutil/error.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

Decoder::Decoder(QObject *parent) : QObject(parent),
                                    m_pktVQueue(300),
                                    m_pktAQueue(300),
                                    m_frameVQueue(15),
                                    m_frameAQueue(300)
{
    connect(&EventBus::instance(), &EventBus::seekRequested, this, &Decoder::seek);
}

bool Decoder::loadSource(const QString &filename)
{
    // Close old video file
    clear();

    // Open new video file  
    if (avformat_open_input(&m_formatContext, filename.toUtf8().constData(), NULL, NULL) != 0) {
        qDebug() << "Couldn't open video file: " << filename;
        clear();
        return false;
    }

    // find stream info
    if (avformat_find_stream_info(m_formatContext, NULL) != 0) {
        qDebug() << "Couldn't find stream info";
        clear();
        return false;
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
        clear();
        return false;
    }

    // Send duration in UI
    qint64 durationMs = getDuration();
    emit EventBus::instance().durationChanged(durationMs);

    // Open codecs
    AVCodecParameters *codecParams = m_formatContext->streams[m_videoStreamIndex]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecParams->codec_id);

    // Video codec
    m_codecContextVideo = avcodec_alloc_context3(codec);
    if (!m_codecContextVideo) {
        qDebug() << "Couldn't allocate codec context";
        clear();
        return false;
    }
    if (avcodec_parameters_to_context(m_codecContextVideo, codecParams) != 0) {
        qDebug() << "Couldn't copy codec params to codec context";
        clear();
        return false;
    }
    if (avcodec_open2(m_codecContextVideo, codec, NULL) != 0) {
        qDebug() << "Couldn't open codec";
        clear();
        return false;
    }

    // Open codecs
    codecParams = m_formatContext->streams[m_audioStreamIndex]->codecpar;
    codec = avcodec_find_decoder(codecParams->codec_id);

    // Audio codec
    m_codecContextAudio = avcodec_alloc_context3(codec);
    if (!m_codecContextAudio) {
        qDebug() << "Couldn't allocate codec context";
        clear();
        return false;
    }
    if (avcodec_parameters_to_context(m_codecContextAudio, codecParams) != 0) {
        qDebug() << "Couldn't copy codec params to codec context";
        clear();
        return false;
    }
    if (avcodec_open2(m_codecContextAudio, codec, NULL) != 0) {
        qDebug() << "Couldn't open codec";
        clear();
        return false;
    }

    // Init audio
    if (!initAudio())
        return false;


    return true;
}

bool Decoder::initAudio()
{
    if (!m_codecContextAudio) {
        qDebug() << "contextAudio not init";
        return false;
    }
    if (m_swrContext)
        swr_free(&m_swrContext);

    AVChannelLayout chLayout = AV_CHANNEL_LAYOUT_STEREO;
    int ret = swr_alloc_set_opts2(&m_swrContext, 
                                  &chLayout, 
                                  AV_SAMPLE_FMT_FLT, 
                                  48000, 
                                  &m_codecContextAudio->ch_layout,
                                  m_codecContextAudio->sample_fmt,
                                  m_codecContextAudio->sample_rate, 
                                  0, nullptr);
    
    if (ret < 0 || !m_swrContext) {
        qDebug() << "Failed to allocate swrContext: " << ret;
        return false;
    }
    if (swr_init(m_swrContext) < 0) {
        qDebug() << "Couldn't init swrContext";
        return false;
    }
    return true;
}

void Decoder::demuxLoop()
{
    m_running.store(true);

    while (m_running.load())
    {
        if (m_seekReq.exchange(false)) {
            seekTo(m_seekTargetMs.load());
            continue;
        }

        if (m_pktVQueue.isFull() || m_pktAQueue.isFull()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        AVPacket *packet = av_packet_alloc();

        int ret = av_read_frame(m_formatContext, packet);
        if (ret < 0) { // If any error
            av_packet_free(&packet);

            // end file
            if (ret == AVERROR_EOF) {
                m_pktAQueue.push(nullptr);
                m_pktVQueue.push(nullptr);
            }
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, sizeof(errbuf));
            qDebug() << "Demux: " << errbuf;
            break;
        }

        if (packet->stream_index == m_videoStreamIndex) {         // Video packet
            if (!m_pktVQueue.push(packet)) {
                av_packet_free(&packet);
                break;
            }
        } else if (packet->stream_index == m_audioStreamIndex) {  // Audio packet
            if (!m_pktAQueue.push(packet)) {
                av_packet_free(&packet);
                break;
            }
        } else {                                                  // Another
            av_packet_free(&packet);
        }
    }
}

void Decoder::decodeVideoLoop()
{
    m_running.store(true);
    
    AVFrame *frame = av_frame_alloc();

    while (m_running.load()) 
    {
        AVPacket *packet = nullptr;
        if (!m_pktVQueue.pop(packet)) 
            break;

        // End file
        if (packet == nullptr) {
            std::lock_guard<std::mutex> lock(videoMtx);

            avcodec_send_packet(m_codecContextVideo, packet);

            // Last B-frame
            while (m_running.load() && avcodec_receive_frame(m_codecContextVideo, frame) == 0) // FRAME LOAD
            {  
                qint64 posMs = getFramePosMs(frame, m_videoStreamIndex);
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

            m_frameVQueue.push(videoFrame{});
            break;
        }

        {
            std::lock_guard<std::mutex> lock(videoMtx);

            int ret = avcodec_send_packet(m_codecContextVideo, packet);
            av_packet_free(&packet);

            if (ret < 0) continue;

            while (m_running.load() && avcodec_receive_frame(m_codecContextVideo, frame) == 0) // FRAME LOAD
            {  
                qint64 posMs = getFramePosMs(frame, m_videoStreamIndex);
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
        }
    }
    av_frame_free(&frame);

    emit EventBus::instance().decoderFinished();
}

void Decoder::decodeAudioLoop() // Prototype
{
    m_running.store(true);
    
    AVFrame *frame = av_frame_alloc();

    while (m_running.load()) 
    {
        AVPacket *packet = nullptr;
        if (!m_pktAQueue.pop(packet)) 
            break;

        {
            std::lock_guard<std::mutex> lock(audioMtx);

            // End file
            if (packet == nullptr) {
                m_frameAQueue.push(audioFrame{});
                break;
            }

            int ret = avcodec_send_packet(m_codecContextAudio, packet);
            av_packet_free(&packet);

            if (ret < 0) continue;

            while (m_running.load() && avcodec_receive_frame(m_codecContextAudio, frame) == 0) // FRAME LOAD
            {  
                qint64 posMs = getFramePosMs(frame, m_audioStreamIndex);
                if (m_Iframe.exchange(false)) 
                    emit EventBus::instance().positionAudioChanged(posMs);

                audioFrame aFrame;
                aFrame.posMs = posMs;

                //  maxOutSamples = (delay + in_rate) * 48000 / in_rate
                int maxOutSamples = av_rescale_rnd(
                                        swr_get_delay(m_swrContext, m_codecContextAudio->sample_rate) 
                                            + frame->nb_samples, 
                                        48000, m_codecContextAudio->sample_rate, AV_ROUND_UP
                );

                // Alloc for STEREO
                aFrame.samples.resize(maxOutSamples * 2);
                uint8_t *outData = reinterpret_cast<uint8_t *>(aFrame.samples.data());

                int samples = swr_convert(m_swrContext, &outData, maxOutSamples, 
                                        (const uint8_t **)frame->data, frame->nb_samples);
                    
                // Actuall samples for STEREO
                aFrame.samples.resize(samples * 2);

                // Push frame to queue
                if (!m_frameAQueue.push(aFrame)) {
                    // If queue is full
                    av_frame_unref(frame);
                    break;
                }
                av_frame_unref(frame);
            }
        }
    }
    av_frame_free(&frame);
}

void Decoder::stop()
{
    m_running.store(false);

    m_pktAQueue.abort();
    m_pktVQueue.abort();
    m_frameAQueue.abort();
    m_frameVQueue.abort();
}

void Decoder::clear()
{
    stop();
    m_pktAQueue.clear();
    m_pktVQueue.clear();
    m_frameAQueue.clear();
    m_frameVQueue.clear();

    m_seekReq.store(false);
    m_seekTargetMs.store(0);

    if (m_codecContextVideo)  avcodec_free_context(&m_codecContextVideo);
    if (m_codecContextAudio)  avcodec_free_context(&m_codecContextAudio);
    if (m_formatContext)      avformat_close_input(&m_formatContext);
    if (m_swrContext)         swr_free(&m_swrContext);
}

void Decoder::seek(qint64 posMs)
{
    m_seekReq.store(true);
    m_seekTargetMs.store(static_cast<int64_t>(posMs));
}

void Decoder::seekTo(int64_t posMs)
{
    if (!m_formatContext || m_videoStreamIndex < 0) {
        qDebug() << "seekTo: Not a video";
        return;
    }

    qint64 durationMs = getDuration();
    if (posMs > durationMs)  posMs = durationMs;
    if (posMs < 0)           posMs = 0;
    
    m_isSeeking.store(true);
    
    // Stream
    AVStream *stream = m_formatContext->streams[m_videoStreamIndex];

    // From MS to TS
    int64_t targetTs = av_rescale_q(posMs, AVRational{1, 1000}, stream->time_base);

    // If there is a start time 
    if (stream->start_time != AV_NOPTS_VALUE) 
        targetTs += stream->start_time;

    // Seek
    int ret = avformat_seek_file(
        m_formatContext, m_videoStreamIndex,
        INT64_MIN, targetTs, INT64_MAX,    // Range [INT64_MIN : targetTs : INT64_MAX]
        AVSEEK_FLAG_BACKWARD                           // first I-frame
    );
    if (ret < 0) {
        qDebug() << "Seek error: " << ret;
        return;
    }

    // Clear queue
    m_pktVQueue.clear();
    m_pktAQueue.clear();
    m_frameVQueue.clear();
    m_frameAQueue.clear();
    
    {
        std::lock_guard<std::mutex> lock(videoMtx);
        if (m_codecContextVideo)  avcodec_flush_buffers(m_codecContextVideo);
    }
    {
        std::lock_guard<std::mutex> lock(audioMtx);
        if (m_codecContextAudio) {
            avcodec_flush_buffers(m_codecContextAudio);
            
            // Rewind audio to first I-frame
            m_Iframe.store(true);
        }
    }
}

int64_t Decoder::getFramePosMs(const AVFrame *frame, int streamIndex) const
{
    if (!frame || !m_formatContext) {
        qDebug() << "getFramePosMs: not a frame";
        return -1;
    }

    // Presentation Timestamp
    int64_t pts = frame->best_effort_timestamp;
    if (pts == AV_NOPTS_VALUE) {
        qDebug() << "getFramePosMs: not a pts";
        return -1;
    }

    AVStream *stream = m_formatContext->streams[streamIndex];
    // If there is a start time
    if (stream->start_time != AV_NOPTS_VALUE)
        pts -= stream->start_time;
    
    // From TS to MS
    return av_rescale_q(pts, stream->time_base, AVRational{1, 1000});
}

AVFramePtr Decoder::cloneToSharedPtr(AVFrame *frame)
{
    AVFrame *dstFrame = av_frame_clone(frame);

    return AVFramePtr(dstFrame, [](AVFrame *frame) {  av_frame_free(&frame);  });
}

qint64 Decoder::getDuration()
{
    if (!m_formatContext) {
        qDebug() << "Decoder: can't send duration, formatContext is not init";
        return 0;
    }
    if (m_formatContext->duration == AV_NOPTS_VALUE) {
        qDebug() << "Decoder: can't send duration, formatContext is not duration";
        return 0;
    }
    return (m_formatContext->duration * 1000) / AV_TIME_BASE;
}

bool Decoder::getNextVFrame(videoFrame &vFrame)
{
    return m_frameVQueue.pop(vFrame);
}

bool Decoder::peekNextVFrame(videoFrame &vFrame)
{
    return m_frameVQueue.peek(vFrame);
}

bool Decoder::getNextAFrame(audioFrame &aFrame)
{
    return m_frameAQueue.pop(aFrame);
}

Decoder::~Decoder()
{
    clear();
    
    qDebug() << "Decoder destroy";
}