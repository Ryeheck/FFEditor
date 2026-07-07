extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
}

#include "MediaPlayer.h"

#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QWidget>
#include <QDebug>
#include <QDropEvent>
#include <QMimeData>
#include <QSlider>

MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent)
{
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);

    m_videoWidget = new QVideoWidget();
    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_videoWidget->setMinimumSize(400, 300);

    m_player->setVideoOutput(m_videoWidget);
    m_player->setAudioOutput(m_audioOutput);

    int w = 1920;
    int h = 1080;
    bool n = loadFrame("/home/ryabi/Видео/output.mp4", w, h);

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

    bool foundFrame = false;
    while (av_read_frame(formatContext, packet) >= 0)
    {
        if (packet->stream_index == videoStreamIndex) {
            avcodec_send_packet(codecContext, packet);
            
            if (avcodec_receive_frame(codecContext, frame) == 0) {
                // FRAME LOAD
                
                width = frame->width;
                height = frame->height;

            }
        }
    }

    // Free and close video file
    avformat_close_input(&formatContext);
    avcodec_free_context(&codecContext);
    av_frame_free(&frame);
    av_packet_free(&packet);

    return true;
}

MediaPlayer::~MediaPlayer()
{
    qDebug() << "ok";
}