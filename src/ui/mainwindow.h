#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "MediaLoader.h"
#include "animations/anim.hpp"
#include "painter/CustomVolume.h"
#include "painter/CustomPlayhead.hpp"
#include "../player/MediaPlayer.h"
#include "../player/VideoWidget.h"

#include <QMainWindow>
#include <QVBoxLayout>
#include <atomic>
#include <qboxlayout.h>
#include <qtimer.h>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    
    ~MainWindow() override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    
private:
    void hidePanel();
    void showPanel();

    QWidget        *centralWidget;
    VideoWidget    *videoWidget;    
    MediaPlayer    *player;
    MediaLoader    *loader;
    QVBoxLayout    *MainLayout;
    CustomVolume   *volumeSlider;
    CustomPlayhead *playhead;
    Animation      *anim;

    QVBoxLayout    *bottomPanel;
    QTimer         *panelTimer;
    std::atomic<bool> isPanelVisible {true};

};

#endif // MAINWINDOW_H