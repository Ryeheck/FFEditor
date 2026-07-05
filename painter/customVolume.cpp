#include "customVolume.h"

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>

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

    // Outline
    QPen pen;
    pen.setColor(QColor(255, 255, 255));
    pen.setWidth(2);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);

    QPainterPath triangle;
    triangle.moveTo(0, height());       // Bottom left corner
    triangle.lineTo(width(), height()); // Bottom right corner
    triangle.lineTo(width(), 0);        // Top left corner
    triangle.closeSubpath();

    painter.fillPath(triangle, Qt::transparent);
    painter.drawPath(triangle);
    // Progress bar
    float barWidth = width() * _value;
    
    QPainterPath trianglePGBar;
    trianglePGBar.moveTo(0, height());
    trianglePGBar.lineTo(barWidth, height());
    trianglePGBar.lineTo(barWidth, height() * (1.0 - _value));
    trianglePGBar.closeSubpath();

    painter.fillPath(trianglePGBar, Qt::blue);

}

void CustomVolume::mouseMoveEvent(QMouseEvent *event)
{
    float newValue = event->pos().x() / (float)width();
    _value = qBound(0.0, newValue, 1.0);

    update();
    emit valueChanged(_value);
}

void CustomVolume::setValue(int newValue)
{
    if(newValue > -1 && newValue < 101) {
        _value = newValue / 100.0;
        update();
    }   
}