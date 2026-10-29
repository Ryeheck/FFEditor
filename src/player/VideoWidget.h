#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include "Decoder.h"

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <libavutil/frame.h>
#include <qimage.h>
#include <qopenglfunctions.h>
#include <qopengltexture.h>
#include <qopenglvertexarrayobject.h>
#include <qopenglwidget.h>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

extern "C" {
#include <libswscale/swscale.h>
}

class VideoWidget : public QOpenGLWidget, protected QOpenGLFunctions
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
    
    SwsContext *m_swsCtx = nullptr;
    uint8_t *m_buffer    = nullptr;

    QImage                   m_currentFrame;
    QOpenGLBuffer            m_VertexBuf;
    QOpenGLVertexArrayObject m_vao;
    
    QOpenGLShaderProgram *m_program;
    QOpenGLTexture       *m_texture;
    
    int m_bufferLinesize = 0;
};


#endif // VIDEOWIDGET_H