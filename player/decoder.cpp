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

void decoder::processVideo(const QString &filename, int width, int height)
{
    m_running = true;

    // Open video file
    AVFormatContext *formatContext = avformat_alloc_context();
    if (avformat_open_input(&formatContext, filename.toUtf8().constData(), NULL, NULL) != 0) {
        qDebug() << "Couldn't open video file";
        return;
    }

    // Current video stream
    int videoStreamIndex = -1;
    for (int i = 0; i < formatContext->nb_streams; ++i)
    {
        if(formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    // Not video stream
    if (videoStreamIndex == -1) {
        qDebug() << "Couldn't open stream";
        return;
    }

    // Packet and frame loading
    AVCodecParameters *codecParams = formatContext->streams[videoStreamIndex]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecParams->codec_id);
    AVCodecContext *codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecContext, codecParams);
    avcodec_open2(codecContext, codec, NULL);

    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();

    while (av_read_frame(formatContext, packet) >= 0 && m_running)
    {
        if (packet->stream_index != videoStreamIndex) {
            av_packet_unref(packet);
            continue;
        }
        if (avcodec_send_packet(codecContext, packet) != 0) {
            av_packet_unref(packet);
            continue;
        }
             
        while (avcodec_receive_frame(codecContext, frame) == 0 && m_running) // FRAME LOAD
        {  
            QImage renderedFrame = renderFrame(frame);

            if (!renderedFrame.isNull())
                emit frameDecoded(renderedFrame);

            // QThread::msleep(33); // FPS on active video

            av_frame_unref(frame);
        }
        av_packet_unref(packet);
    }

    // Free and close video file
    avformat_close_input(&formatContext);
    avformat_free_context(formatContext);
    avcodec_free_context(&codecContext);
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
    
    swsCtx = sws_getCachedContext(
        swsCtx, 
        frame->width, frame->height, (AVPixelFormat)frame->format,
        frame->width, frame->height, AV_PIX_FMT_RGB32,
        SWS_BICUBIC, NULL, NULL, NULL
    );

    if (!swsCtx) {
        qDebug() << "Failed to create sws context";
        return QImage();
    }

    uint8_t *dest[4]    = {m_buffer, nullptr, nullptr, nullptr};
    int linesize        = FFALIGN(frame->width * 4, align);
    int destLinesize[4] = {linesize, 0, 0, 0}; // 4 bytes + align for pixel

    if (sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, dest, destLinesize) != frame->height) {
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
    if (swsCtx) {
        sws_freeContext(swsCtx);
        swsCtx = nullptr;
    }

    qDebug() << "ok";
}