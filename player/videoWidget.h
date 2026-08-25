#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include "frameQueue.hpp"

#include <QWidget>
#include <QImage>
#include <QPainter>

extern "C" {
#include <libswscale/swscale.h>
}

class videoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit videoWidget(QWidget *parent = nullptr);
    ~videoWidget() override;

    void setFrame(const AVFramePtr &vFrame);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage renderFrame(AVFrame *frame);

    SwsContext *m_swsCtx = nullptr;
    uint8_t *m_buffer    = nullptr;

    int m_bufferLinesize = 0;
    QImage m_currentFrame;
};


#endif // VIDEOWIDGET_H