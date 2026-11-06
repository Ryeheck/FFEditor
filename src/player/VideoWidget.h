#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include "Decoder.h"

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <qimage.h>
#include <qopenglfunctions_3_3_core.h>
#include <qopengltexture.h>
#include <qopenglvertexarrayobject.h>
#include <qopenglwidget.h>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

extern "C" {
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

class VideoWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit VideoWidget(QWidget *parent = nullptr);
    ~VideoWidget() override;
    void initializeGL() override; 
    void resizeGL(int w, int h) override;
    void paintGL() override; 

public slots:
    void setFrame(const AVFramePtr &vFrame);

protected:
    

private:
    QImage renderFrame(AVFrame *frame);
    QOpenGLTexture *setTexture(int width = 1920, int height = 1080);
    void createTextures(int width, int height);
    void uploadFrame();
    void cleanupGL();
    
    SwsContext *m_swsCtx = nullptr;
    uint8_t *m_buffer    = nullptr;

    int m_bufferLinesize = 0;
    
    QImage m_currentFrame;

    // OpenGL
    QOpenGLTexture *m_tex[3] = {nullptr, nullptr, nullptr};
    AVFramePtr m_frame;
    bool m_newFrame = false;

    QOpenGLBuffer            m_VertexBuf;
    QOpenGLVertexArrayObject m_vao;
    
    QOpenGLShaderProgram *m_program = nullptr;
    QOpenGLTexture       *m_texture = nullptr;
};


#endif // VIDEOWIDGET_H