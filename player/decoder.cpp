#include <QImage>
#include <QString>
#include <QDebug>

#include "decoder.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}


void decoder::processVideo(const QString &filename, int width, int height)
{
    // Open video file
    AVFormatContext *formatContext = avformat_alloc_context();
    if (avformat_open_input(&formatContext, filename.toUtf8().constData(), NULL, NULL) != 0) {
        qDebug() << "Couldn't open video file";
        return;
    }

    // Current video stream
    int videoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, -1);

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

    prepareBuffer(width, height);

    while (av_read_frame(formatContext, packet) >= 0)
    {
        if (packet->stream_index != videoStreamIndex) {
            av_packet_unref(packet);
            qDebug() << "Not correct video stream";
            continue;
        }
        if (avcodec_send_packet(codecContext, packet) != 0) {
            av_packet_unref(packet);
            qDebug() << "Couldn't send packet";
            continue;
        }
             
        while (avcodec_receive_frame(codecContext, frame) == 0) // FRAME LOAD
        {  
            QImage image = renderFrame(frame);

            emit frameDecoded(image);

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
    if (!swsCtx) {
        qDebug() << "Failed to create sws context";
        return QImage();
    }

    uint8_t *dest[4]    = {m_buffer, nullptr, nullptr, nullptr};
    int destLinesize[4] = {frame->width * 4, 0, 0, 0}; // 4 bytes for pixel

    if (sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, dest, destLinesize) != frame->height) {
        qDebug() << "Error changing frame color range";
        return QImage();
    }

    return QImage(m_buffer, frame->width, frame->height, destLinesize[0], QImage::Format_RGB32);
}


void decoder::prepareBuffer(int &width, int &height)
{
    m_bufferLinesize = av_image_get_buffer_size(AV_PIX_FMT_RGB32, width, height, 32);
    m_buffer = (uint8_t *)av_malloc(m_bufferLinesize);

    swsCtx = sws_getContext(width, height, AV_PIX_FMT_YUV420P,
                            width, height, AV_PIX_FMT_RGB32,
                            SWS_BICUBIC, NULL, NULL, NULL);
}

decoder::~decoder()
{
    if (m_buffer)  av_free(m_buffer);
    if (swsCtx)    sws_freeContext(swsCtx);

    qDebug() << "ok";
}