#ifndef CUSTOMPLAYHEAD_H
#define CUSTOMPLAYHEAD_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>
#include <qtypes.h>

#include "../EventBus.h"

class CustomPlayhead : public QWidget
{
    Q_OBJECT
    qreal m_widthRect  = 0;
    qreal m_durationMs = 0;
    qint64 m_posMs     = 0;

signals:
    void positionChanged(qint64 posMs);

public:
    CustomPlayhead(QWidget *parent = nullptr) 
    {
        setFixedHeight(10);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        QPalette pal = palette();
        pal.setColor(QPalette::Window, QColor(50, 50, 50));
        setAutoFillBackground(true);
        setPalette(pal);

        connect(&EventBus::instance(), &EventBus::durationChanged, this, &CustomPlayhead::setDuration);
        connect(&EventBus::instance(), &EventBus::positionChanged, this, &CustomPlayhead::onPositionChanged);
    };

    void setDuration(qint64 durationMs) 
    {  
        if (durationMs <= 0) return;

        m_durationMs = durationMs;
        update();  
    }

    void onPositionChanged(qint64 posMs)
    {
        if (m_durationMs <= 0) return;
        if (posMs == m_posMs)  return;

        m_widthRect = ((qreal)posMs * width()) / m_durationMs;
        update();
    };

    QString formatTime(qint64 ms)
    {
        int seconds = ms / 1000;
        int hours = seconds / 3600;
        seconds %= 3600;
        int minutes = seconds / 60;
        seconds %= 60;

        if (hours > 0) {
            return QString("%1:%2:%3")
                    .arg(hours, 2, 10, QChar('0'))
                    .arg(minutes, 2, 10, QChar('0'))
                    .arg(seconds, 2, 10, QChar('0'));
        }
        return QString("%1:%2")
                    .arg(minutes, 2, 10, QChar('0'))
                    .arg(seconds, 2, 10, QChar('0'));
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.fillRect(0, 0, m_widthRect, height(), Qt::blue); // Rectangle playhead

        painter.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap)); // Outline
        painter.drawRect(rect());

    };

    void mouseMoveEvent(QMouseEvent *event) override
    {
        qreal posInWidget = (qreal)event->pos().x() / width();
        // posInWidget = qBound(0.0, posInWidget, 1.0);
        m_posMs = (qint64)(posInWidget * m_durationMs);
        
        onPositionChanged(m_posMs);

        emit EventBus::instance().seekRequested(m_posMs);
    };
};
#endif // CUSTOMPLAYHEAD_H