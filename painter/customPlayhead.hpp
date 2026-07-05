#ifndef CUSTOMTIMELINE_H
#define CUSTOMTIMELINE_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>

class CustomPlayhead : public QWidget
{
    Q_OBJECT
    qreal widthRect = 0;
    qreal duration = 0;

signals:
    void positionChanged(qint64 pos);

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
        if (duration <= 0) return;
        this->duration = duration;

        this->widthRect = ((qreal)pos * width()) / this->duration;
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
        playhead.addRect(0, 0, widthRect, height()); // Rectangle playhead

        painter.fillPath(playhead, Qt::blue);
    };

    void mouseMoveEvent(QMouseEvent *event) override
    {
        qreal posInWidget = (qreal)event->pos().x() / width();
        qint64 pos = (qint64)(posInWidget * duration);
        
        onPositionChanged(pos, duration);
        emit positionChanged(pos);
    };
    
};
#endif // CUSTOMTIMELINE_H