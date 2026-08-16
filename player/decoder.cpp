#include <QImage>
#include <QString>
#include <QDebug>
#include <QThread>

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
    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();

    while (av_read_frame(m_formatContext, packet) >= 0)
    {
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
            QImage renderedFrame = renderFrame(frame);

            if (!renderedFrame.isNull())
                emit frameDecoded(renderedFrame);

            av_frame_unref(frame);
        }
        av_packet_unref(packet);
    }

    // Free
    av_frame_free(&frame);
    av_packet_free(&packet);
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