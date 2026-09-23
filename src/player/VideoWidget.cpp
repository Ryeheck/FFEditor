#include "VideoWidget.h"

#include <QImage>
#include <QPainter>
#include <QWidget>

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
}

VideoWidget::VideoWidget(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void VideoWidget::setFrame(const AVFramePtr &vFrame)
{
    if (!vFrame)  return;

    m_currentFrame = renderFrame(vFrame.get());

    update();
}

QImage VideoWidget::renderFrame(AVFrame *frame)
{
    int align = 32;
    int requestedSize = av_image_get_buffer_size(AV_PIX_FMT_RGB32, 
                                                 frame->width, frame->height, 
                                                 align);

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

    if (sws_scale(m_swsCtx, frame->data, frame->linesize, 
                  0, frame->height, dest, destLinesize) 
                  != frame->height) {
        qDebug() << "Error changing frame color range";
        return QImage();
    }

    return QImage(m_buffer, frame->width, frame->height, 
                  linesize, QImage::Format_RGB32).copy();
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    painter.fillRect(rect(), Qt::black);

    if (!m_currentFrame.isNull()) {
        QImage scaledImg = m_currentFrame.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        int x = (width() - scaledImg.width()) / 2;
        int y = (height() - scaledImg.height()) / 2;

        painter.drawImage(x, y, scaledImg);
    }
}

VideoWidget::~VideoWidget()
{
    if (m_buffer) {
        av_free(m_buffer);
        m_buffer = nullptr;
    }
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
}