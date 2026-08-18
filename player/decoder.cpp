#include <QImage>
#include <QString>
#include <QDebug>
#include <QThread>
#include <atomic>

#include "decoder.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

decoder::decoder(QObject *parent) : QObject(parent)
{

}

bool decoder::loadSource(const QString &filename)
{
    // Close old video file
    if (m_codecContext)  avcodec_free_context(&m_codecContext);
    if (m_formatContext) avformat_close_input(&m_formatContext);
    
    // Initialization codecs
    AVCodecParameters *codecParams = nullptr;
    const AVCodec *codec = nullptr;

    // Open new video file  
    if (avformat_open_input(&m_formatContext, filename.toUtf8().constData(), NULL, NULL) != 0) {
        qDebug() << "Couldn't open video file: " << filename;
        goto cleanup;
    }

    // Current video stream
    m_videoStreamIndex = -1;
    for (int i = 0; i < m_formatContext->nb_streams; ++i)
    {
        if(m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIndex = i;
            break;
        }
    }

    // Not video stream
    if (m_videoStreamIndex == -1) {
        qDebug() << "Couldn't open stream";
        goto cleanup;
    }

    // find stream info
    if (avformat_find_stream_info(m_formatContext, NULL) != 0) {
        qDebug() << "Couldn't find stream info";
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

    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext) {
        qDebug() << "Couldn't allocate codec context";
        goto cleanup;
    }
    if (avcodec_parameters_to_context(m_codecContext, codecParams) != 0) {
        qDebug() << "Couldn't copy codec params to codec context";
        goto cleanup;
    }
    if (avcodec_open2(m_codecContext, codec, NULL) != 0) {
        qDebug() << "Couldn't open codec";
        goto cleanup;
    }

    return true;

cleanup:
    if (m_codecContext)  
        avcodec_free_context(&m_codecContext);

    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
        avformat_free_context(m_formatContext);
    }
    return false;
}

void decoder::processVideo()
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
        if (avcodec_send_packet(m_codecContext, packet) != 0) {
            av_packet_unref(packet);
            continue;
        }
                
        while (avcodec_receive_frame(m_codecContext, frame) == 0) // FRAME LOAD
        {  
            qint64 posMs = getFramePosMs(frame);
            if (m_isSeeking) {
                if (posMs < m_seekTargetMs) {
                    av_frame_unref(frame);
                    continue;
                } else {
                    m_isSeeking.store(false);
                }
            }
            
            QImage renderedFrame = renderFrame(frame);
            
            emit positionChanged(posMs);
            emit frameDecoded(renderedFrame);

            av_frame_unref(frame);
        }
        av_packet_unref(packet);
        
    }

    // Free
    av_frame_free(&frame);
    av_packet_free(&packet);

    emit finished();
}

QImage decoder::renderFrame(AVFrame *frame)
{
    int align = 32;
    int requestedSize = av_image_get_buffer_size(AV_PIX_FMT_RGB32, frame->width, frame->height, align);

    if (!m_buffer || requestedSize > m_bufferLinesize) {
        if (m_buffer)  av_free(m_buffer);
        
        m_bufferLinesize = requestedSize;
        m_buffer = (uint8_t *)av_malloc(m_bufferLinesize);
    }
    
    m_swsCtx = sws_getCachedContext(
        m_swsCtx, 
        frame->width, frame->height, (AVPixelFormat)frame->format,
        frame->width, frame->height, AV_PIX_FMT_RGB32,
        SWS_BICUBIC, NULL, NULL, NULL
    );

    if (!m_swsCtx) {
        qDebug() << "Failed to create sws context";
        return QImage();
    }

    uint8_t *dest[4]    = {m_buffer, nullptr, nullptr, nullptr};
    int linesize        = FFALIGN(frame->width * 4, align);
    int destLinesize[4] = {linesize, 0, 0, 0}; // 4 bytes + align for pixel

    if (sws_scale(m_swsCtx, frame->data, frame->linesize, 0, frame->height, dest, destLinesize) != frame->height) {
        qDebug() << "Error changing frame color range";
        return QImage();
    }

    return QImage(m_buffer, frame->width, frame->height, linesize, QImage::Format_RGB32).copy();
}

void decoder::seek(double posMs)
{
    m_seekReq.store(true);
    m_seekTargetMs.store(posMs);
}

void decoder::seekTo(double posMs)
{
    if (!m_formatContext || !m_codecContext || m_videoStreamIndex < 0) {
        qDebug() << "seekTo: Not a video";
        return;
    }

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
    while (avcodec_receive_frame(m_codecContext, frame) == 0)
    {
        av_frame_unref(frame);
    }
    av_frame_free(&frame); 

    avcodec_flush_buffers(m_codecContext);

    // if (audio)          avcodec_flush_buffers(audio);

    // Flags
    m_isSeeking.store(true);

}

int64_t decoder::getFramePosMs(const AVFrame *frame) const
{
    if (!frame || !m_formatContext || !m_codecContext) {
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

decoder::~decoder()
{
    if (m_buffer) {
        av_free(m_buffer);
        m_buffer = nullptr;
    }
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
        avformat_free_context(m_formatContext);
        m_formatContext = nullptr;
    }
    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
    }

    qDebug() << "Decoder: ok";
}