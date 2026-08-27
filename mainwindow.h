#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "MediaLoader.h"
#include "painter/CustomVolume.h"
#include "painter/CustomPlayhead.hpp"
#include "player/MediaPlayer.h"
#include "player/VideoWidget.h"

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
    VideoWidget *videoWidget;
    QWidget *centralWidget;
    MediaPlayer *player;
    MediaLoader *loader;
    QVBoxLayout *MainLayout;
    CustomVolume *volumeSlider;
    CustomPlayhead *playhead;
};

#endif // MAINWINDOW_H