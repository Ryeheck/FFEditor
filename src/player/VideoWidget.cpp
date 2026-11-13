#include "VideoWidget.h"
#include "../EventBus.h"

#include <QWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>
#include <qopengltexture.h>
#include <QOpenGLPixelTransferOptions>

VideoWidget::VideoWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    connect(&EventBus::instance(), &EventBus::frameChanged, this, &VideoWidget::setFrame);
}
void VideoWidget::setFrame(const AVFramePtr &vFrame)
{
    if (!vFrame)  return;
    
    m_frame = vFrame;
    m_newFrame = true;

    update();
}

void VideoWidget::createTextures(int w, int h)
{
    for (int i = 0; i < 3; ++i)
    {
        delete m_tex[i];

        int pw = (i == 0) ? w : (w + 1) / 2;
        int ph = (i == 0) ? h : (h + 1) / 2;

        m_tex[i] = new QOpenGLTexture(QOpenGLTexture::Target2D);
        m_tex[i]->setSize(pw, ph);
        m_tex[i]->setFormat(QOpenGLTexture::R8_UNorm);
        m_tex[i]->setMinificationFilter(QOpenGLTexture::Linear);
        m_tex[i]->setMagnificationFilter(QOpenGLTexture::Linear);
        m_tex[i]->setWrapMode(QOpenGLTexture::ClampToEdge);

        m_tex[i]->allocateStorage();
    }
}

void VideoWidget::uploadFrame()
{
    for (int i = 0; i < 3; ++i)
    {
        QOpenGLPixelTransferOptions opt;
        opt.setAlignment(1);
        opt.setRowLength(m_frame->linesize[i]);

        m_tex[i]->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, 
                          m_frame->data[i], &opt);
    }
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

void VideoWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0, 1, 0, 0);
    
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
    glClear(GL_COLOR_BUFFER_BIT);

    if (!m_frame) {
        qDebug() << "OpenGL hasn't frame";
        return;
    }

    if (m_newFrame) {
        if (!m_tex[0] || m_tex[0]->width() != m_frame->width 
                      || m_tex[0]->height() != m_frame->height)
            createTextures(m_frame->width, m_frame->height);
        
        uploadFrame();
        m_newFrame = false;
    }

    m_program->bind();
    for (int i = 0; i < 3; ++i)
        m_tex[i]->bind(i);

    m_program->setUniformValue("texY", 0);
    m_program->setUniformValue("texU", 1);
    m_program->setUniformValue("texV", 2);

    m_vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    m_vao.release();
}

void VideoWidget::cleanupGL()
{
    makeCurrent();

    delete m_texture;  m_texture = nullptr;
    delete m_program;  m_program = nullptr;

    for (int i = 0; i < 3; ++i) 
    {
        delete m_tex[i]; m_tex[i] = nullptr;
    }

    m_vao.destroy();
    m_VertexBuf.destroy();

    doneCurrent();
}

VideoWidget::~VideoWidget()
{   
    cleanupGL();

    qDebug() << "VideoWidget destroy";
}