#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "MediaPlayer.h"
#include "MediaLoader.h"
#include "painter/customVolume.h"
#include "painter/customPlayhead.hpp"

#include <QMainWindow>
#include <QVBoxLayout>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    
    ~MainWindow() override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    
private:
    QWidget *centralWidget;
    MediaPlayer *player;
    MediaLoader *loader;
    QVBoxLayout *MainLayout;
    CustomVolume *volumeSlider;
    CustomPlayhead *playhead;
};

#endif // MAINWINDOW_H