#ifndef CUSTOMVOlUME_H
#define CUSTOMVOlUME_H

#include <QWidget>

class CustomVolume : public QWidget
{
    Q_OBJECT
    float m_value = 0.0f;

signals:
    void valueChanged(float newValue);

public:
    CustomVolume(QWidget *parent = nullptr);
    void setValue(int value);
    int value() const {  return m_value;  };
protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *mouse) override;
    
};


#endif // CUSTOMVOlUME_H