#include "CustomVolume.h"
#include "../../EventBus.h"

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>
#include <qobject.h>

CustomVolume::CustomVolume(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(30);
    setFixedWidth(80);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(50, 50, 50));
    setAutoFillBackground(true);
    setPalette(pal);

}

CustomVolume::~CustomVolume()
{
    qDebug() << "CustomVolume destroy";
}

void CustomVolume::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QPainterPath triangle;
    triangle.moveTo(0, height());       // Bottom left corner
    triangle.lineTo(width(), height()); // Bottom right corner
    triangle.lineTo(width(), 0);        // Top left corner
    triangle.closeSubpath();

    painter.fillPath(triangle, Qt::transparent);
    painter.drawPath(triangle);

    // Progress bar
    float barWidth = width() * m_volume;
    
    QPainterPath ProgressBar;
    ProgressBar.moveTo(0, height());
    ProgressBar.lineTo(barWidth, height());
    ProgressBar.lineTo(barWidth, height() * (1.0 - m_volume));
    ProgressBar.closeSubpath();

    painter.fillPath(ProgressBar, Qt::blue);
    painter.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap)); // Outline
}

void CustomVolume::mouseMoveEvent(QMouseEvent *event)
{
    float newValue = event->pos().x() / (float)width();
    m_volume = qBound(0.0, newValue, 1.0);

    emit EventBus::instance().setVolume(m_volume);
    update();
}

void CustomVolume::setVolume(int value)
{
    if(value > -1 && value < 101) {
        m_volume = value / 100.0;

        emit EventBus::instance().setVolume(m_volume);
        update();
    }   
}

QString CustomVolume::formatVolume(const float vlm)
{
    qint32 percent = (vlm / 1.0f) * 100;

    // Idk how this works
    return QString("%1%").arg(percent);
}