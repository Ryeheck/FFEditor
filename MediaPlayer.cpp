extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
// #include "libavutil/imgutils.h"
}

#include "MediaPlayer.h"

#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QWidget>
#include <QDebug>
#include <QDropEvent>
#include <QMimeData>
#include <QImage>
#include <QVideoSink>
#include <QVideoFrameFormat>
#include <QVideoFrame>

MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent)
{
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
   
    m_videoWidget = new QVideoWidget();
    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_videoWidget->setMinimumSize(400, 300);
    sink = m_videoWidget->videoSink();

    m_player->setVideoOutput(m_videoWidget);
    m_player->setAudioOutput(m_audioOutput);

    connect(m_player, &QMediaPlayer::positionChanged, this, &MediaPlayer::positionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &MediaPlayer::durationChanged);

    // connect(player, &QMediaPlayer::mediaStatusChanged, this, &MediaPlayer::HandleMediaStatus);
}

void MediaPlayer::HandleMediaStatus(QMediaPlayer::MediaStatus status)
{
    if(QMediaPlayer::LoadedMedia == status)
        m_player->play();
    
}

void MediaPlayer::loadVideo(const QUrl &url) 
{  
    if(!url.isEmpty()) {  
        m_player->setSource(url);
        m_player->play();
    }  
}

void MediaPlayer::loadVideo(const QString &path) 
{  
    QUrl url = QUrl::fromLocalFile(path);

    if(!url.isEmpty()) {  
        m_player->setSource(url);  
        m_player->play();
    }  
}

void MediaPlayer::onPositionChanged(qint64 pos)
{
    m_player->setPosition(pos);
}

void MediaPlayer::pauseVideo() 
{  
    if(m_player->playbackState() == QMediaPlayer::PlayingState)
        m_player->pause();  
    else
        m_player->play();

};

bool MediaPlayer::loadFrame(const char *filename, int &width, int &height)
{
    // Open video file
    AVFormatContext *formatContext = avformat_alloc_context();
    if (avformat_open_input(&formatContext, filename, NULL, NULL) != 0) {
        qDebug() << "Couldn't open video file";
        return false;
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
        return false;
    }

    // Packet and frame loading
    AVCodecParameters *codecParams = formatContext->streams[videoStreamIndex]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecParams->codec_id);
    AVCodecContext *codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecContext, codecParams);
    avcodec_open2(codecContext, codec, NULL);

    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();

    int i = 0;
    while (av_read_frame(formatContext, packet) >= 0)
    {
        if (packet->stream_index == videoStreamIndex) {
            if (avcodec_send_packet(codecContext, packet) == 0) {
                while (avcodec_receive_frame(codecContext, frame) == 0) {
                    // FRAME LOAD

                    QImage image = renderFrame(frame, codecContext);

                    sentToSink(image, sink);

                    av_frame_unref(frame);
                }
            } else
                qDebug() << "Couldn't send packet";
        }
        av_packet_unref(packet);
    }

    // Free and close video file
    avformat_close_input(&formatContext);
    avformat_free_context(formatContext);
    avcodec_free_context(&codecContext);
    av_frame_free(&frame);
    av_packet_free(&packet);

    return true;
}

QImage MediaPlayer::renderFrame(AVFrame *frame, AVCodecContext *codecCntx)
{
    SwsContext *swsCtx = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format,
                                        frame->width, frame->height, AV_PIX_FMT_RGBA,
                                        SWS_BILINEAR, NULL, NULL, NULL);
    if (swsCtx)  return QImage();

    // Memory allocation different from ffmpeg
    // frame->height + 1
    QImage image(frame->width, frame->height + 1, QImage::Format_RGBA8888);

    uint8_t *dest[4] {image.bits(), nullptr, nullptr, nullptr};
    int destLinesize[4] {static_cast<int>(image.bytesPerLine()), 0, 0, 0};

    sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, dest, destLinesize);

    sws_freeContext(swsCtx);

    return image;
}

void MediaPlayer::sentToSink(QImage &image, QVideoSink *sink)
{
    QVideoFrameFormat format(image.size(), QVideoFrameFormat::Format_RGBA8888);

    QVideoFrame frame(format);

    if (frame.map(QVideoFrame::WriteOnly)) {
        memcpy(frame.bits(0), image.constBits(), image.sizeInBytes());

        frame.unmap();
        sink->setVideoFrame(frame);
    }
}

MediaPlayer::~MediaPlayer()
{
    qDebug() << "ok";
}