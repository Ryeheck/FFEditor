#ifndef CUSTOMTIMELINE_H
#define CUSTOMTIMELINE_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>

class CustomPlayhead : public QWidget
{
    Q_OBJECT
    float x = 0;

signals:
    

public:
    CustomPlayhead(QWidget *parent = nullptr) 
    {
        setFixedHeight(10);
        setFixedWidth(800);

        QPalette pal = palette();
        pal.setColor(QPalette::Window, QColor(50, 50, 50));
        setAutoFillBackground(true);
        setPalette(pal);
    };

    void onPositionChanged(qint64 pos, qint64 duration)
    {
        if (duration <= 0)  return;

        this->x = ((float)pos * width()) / duration;
        update();
    };

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Outline
        QPen pen;
        pen.setColor(QColor(255, 255, 255));
        pen.setWidth(2);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);

        painter.drawRect(rect());

        QPainterPath playhead;
        playhead.addRect(0, 0, x, height()); // Rectangle playhead

        painter.fillPath(playhead, Qt::blue);
    };

    void mouseMoveEvent(QMouseEvent *mouse) override
    {

    };
    
};
#endif // CUSTOMTIMELINE_H