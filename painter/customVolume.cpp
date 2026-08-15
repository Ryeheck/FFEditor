#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>

#include "customVolume.h"

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
    float barWidth = width() * m_value;
    
    QPainterPath ProgressBar;
    ProgressBar.moveTo(0, height());
    ProgressBar.lineTo(barWidth, height());
    ProgressBar.lineTo(barWidth, height() * (1.0 - m_value));
    ProgressBar.closeSubpath();

    painter.fillPath(ProgressBar, Qt::blue);
    painter.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap)); // Outline
}

void CustomVolume::mouseMoveEvent(QMouseEvent *event)
{
    float newValue = event->pos().x() / (float)width();
    m_value = qBound(0.0, newValue, 1.0);

    update();
    emit valueChanged(m_value);
}

void CustomVolume::setValue(int newValue)
{
    if(newValue > -1 && newValue < 101) {
        m_value = newValue / 100.0;
        update();
    }   
}