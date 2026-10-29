#include "VideoWidget.h"
#include "../EventBus.h"

#include <QImage>
#include <QPainter>
#include <QWidget>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <libavutil/pixfmt.h>
#include <qopengltexture.h>

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
}

VideoWidget::VideoWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    connect(&EventBus::instance(), &EventBus::frameChanged, this, &VideoWidget::setFrame);
}
void VideoWidget::setFrame(const AVFramePtr &vFrame)
{
    if (!vFrame)  return;
    
    m_currentFrame = renderFrame(vFrame.get());

    if (!m_texture) {
        m_texture = m_texture = setTexture(vFrame->width, vFrame->height);
    }
    if (m_texture->width()  != vFrame->width || 
        m_texture->height() != vFrame->height) {
        delete m_texture;

        m_texture = setTexture(vFrame->width, vFrame->height);
    }

    update();
}

QOpenGLTexture *VideoWidget::setTexture(int width, int height)
{
    QOpenGLTexture *texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    texture->setSize(width, height);
    texture->setFormat(QOpenGLTexture::RGBFormat);
    texture->setMinificationFilter(QOpenGLTexture::Linear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    texture->setWrapMode(QOpenGLTexture::ClampToEdge);

    texture->allocateStorage();
    return texture;
}

QImage VideoWidget::renderFrame(AVFrame *frame)
{
    int align = 32;
    int requestedSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, 
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
        frame->width, frame->height, AV_PIX_FMT_RGB24,
        SWS_BICUBIC, NULL, NULL, NULL
    );

    if (!m_swsCtx) {
        qDebug() << "Failed to create sws context";
        return QImage();
    }

    uint8_t *dest[4]    = {m_buffer, nullptr, nullptr, nullptr};
    int linesize        = FFALIGN(frame->width * 3, align);
    int destLinesize[4] = {linesize, 0, 0, 0}; // 3 bytes + align for pixel

    if (sws_scale(m_swsCtx, frame->data, frame->linesize, 
                  0, frame->height, dest, destLinesize) 
                  != frame->height) {
        qDebug() << "Error changing frame color range";
        return QImage();
    }

    return QImage(m_buffer, frame->width, frame->height, 
                  linesize, QImage::Format_RGB888).copy();
}


void VideoWidget::initializeGL()
{
    initializeOpenGLFunctions();

    m_program = new QOpenGLShaderProgram(this);

    if (!m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vertex.vert")) {
        qDebug() << "Couldn't add the vertex shader:" << m_program->log();
        return;
    }
    if (!m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fragment.frag")) {
        qDebug() << "Couldn't add the fragment shader:" << m_program->log();
        return;
    }

    if (!m_program->link()) {
        qDebug() << "Error link:" << m_program->log();
        return;
    } else {
        qDebug() << "Shaders success";
    }

    float vertices[] = {
        -1.0f,  1.0f,      0.0f, 0.0f,
        -1.0f, -1.0f,      0.0f, 1.0f,
         1.0f, -1.0f,      1.0f, 1.0f,

        -1.0f,  1.0f,      0.0f, 0.0f,
         1.0f, -1.0f,      1.0f, 1.0f,
         1.0f,  1.0f,      1.0f, 0.0f
    };

    m_vao.create();
    m_vao.bind();

    m_VertexBuf.create();
    m_VertexBuf.bind();
    m_VertexBuf.allocate(vertices, sizeof(vertices));
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 
                          GL_FLOAT, GL_FALSE, 
                          4 * sizeof(float), (void *)0);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, 
                          GL_FLOAT, GL_FALSE, 
                          4 * sizeof(float), (void *) (2 * sizeof(float)));

    m_vao.release();
    m_VertexBuf.release();
}

void VideoWidget::resizeGL(int w, int h) 
{
    glViewport(0, 0, w, h);
}    

void VideoWidget::paintGL() 
{
    glClearColor(0, 1, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!m_texture) {
        qDebug() << "Texture is NULL";
        return;
    }
    if (m_currentFrame.isNull()) {
        qDebug() << "QImage is NULL";
        return;
    }

    m_texture->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt8, m_currentFrame.bits());

    m_program->bind();
    m_texture->bind(0);
    m_program->setUniformValue("vTex", 0);

    m_vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    m_vao.release();
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
    qDebug() << "VideoWidget destroy";
}