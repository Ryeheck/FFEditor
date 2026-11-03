#ifndef CUSTOMVOlUME_H
#define CUSTOMVOlUME_H

#include <QWidget>
#include <qobject.h>

class CustomVolume : public QWidget
{
    Q_OBJECT
    float m_volume = 0.0f;

public:
    CustomVolume(QWidget *parent = nullptr);
    ~CustomVolume() override;
    
    void setVolume(int value);
    int volume() const {  return m_volume;  };
    QString formatVolume(float vlm);
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    
};


#endif // CUSTOMVOlUME_H