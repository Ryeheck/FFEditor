#ifndef CUSTOMSLIDER_H
#define CUSTOMSLIDER_H

#include <QWidget>

class CustomSlider : public QWidget
{
    Q_OBJECT
    float _value = 0.0f;

signals:
    void valueChanged(float newValue);

public:
    CustomSlider(QWidget *parent = nullptr);
    void setValue(int value);
    int value() const {  return _value;  };
protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *mouse) override;
    
};


#endif // CUSTOMSLIDER_H