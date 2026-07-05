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

        painter.fillRect(0, 0, widthRect, height(), Qt::blue); // Rectangle playhead

        painter.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap)); // Outline
        painter.drawRect(rect());
    };

    void mouseMoveEvent(QMouseEvent *event) override
    {
        qreal posInWidget = (qreal)event->pos().x() / width();
        // posInWidget = qBound(0.0, posInWidget, 1.0);
        qint64 pos = (qint64)(posInWidget * duration);
        
        onPositionChanged(pos, duration);
        emit positionChanged(pos);
    };
    
};
#endif // CUSTOMTIMELINE_H