#include "mainwindow.h"
#include "MediaLoader.h"
#include "painter/CustomVolume.h"
#include "painter/CustomPlayhead.hpp"
#include "../player/MediaPlayer.h"
#include "../player/VideoWidget.h"
#include "../EventBus.h"
#include "animations/anim.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QDropEvent>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QUrl>
#include <QToolButton>
#include <qboxlayout.h>
#include <qcoreevent.h>
#include <qicon.h>
#include <qlabel.h>
#include <qobject.h>
#include <qpushbutton.h>
#include <qtimer.h>
#include <qtypes.h>
#include <qwidget.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    centralWidget = new QWidget(this);
    MainLayout    = new QVBoxLayout(centralWidget);
    setCentralWidget(centralWidget);

    MainLayout->setContentsMargins(0, 0, 0, 0);
    MainLayout->addSpacing(0);

    player      = new MediaPlayer(this);
    videoWidget = new VideoWidget(this);

    MainLayout->addWidget(videoWidget, 1);

    loader = new MediaLoader(this);
    connect(&EventBus::instance(), &EventBus::startPlayRequested, this, [this] (const QString &path) {
        QString filepath = path;

        if (!player->loadVideo(filepath)) {
            qDebug() << "mainwn: Couldn't load video";
            return;
        }
        player->play();
        loader->hide();
    });
    
    QToolButton *startBtn = new QToolButton(this);
    connect(startBtn, &QToolButton::clicked, loader, &MediaLoader::show);

    startBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    startBtn->setIcon(QIcon(":/buttons/play_24.svg"));
    startBtn->setIconSize(QSize(32, 32));
    startBtn->show();
    
    QToolButton *stopBtn = new QToolButton(this);
    connect(stopBtn, &QToolButton::clicked, player, &MediaPlayer::stop);
    
    stopBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    stopBtn->setIcon(QIcon(":/buttons/stop_24.svg"));
    stopBtn->setIconSize(QSize(24, 24));
    stopBtn->show();
    
    QToolButton *pauseBtn = new QToolButton(this);
    connect(pauseBtn, &QToolButton::clicked, player, &MediaPlayer::playPause);

    pauseBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    pauseBtn->setIcon(QIcon(":/buttons/resume_24.svg"));
    pauseBtn->setIconSize(QSize(24, 24));
    pauseBtn->show();
    
    // Panel Tools (start, stop, ...) buttons
    QHBoxLayout *pnToolslLayout = new QHBoxLayout();
    pnToolslLayout->addWidget(startBtn);
    pnToolslLayout->addWidget(pauseBtn);
    pnToolslLayout->addWidget(stopBtn);
    pnToolslLayout->addStretch();
    
    // Volume songs
    volumeSlider = new CustomVolume(this);
    volumeSlider->setVolume(50);

    // Icons (volume)
    QLabel *vlmLblIcon = new QLabel(this);
    vlmLblIcon->setPixmap(QIcon(":/volume/volumeUp.svg").pixmap(20, 20));
    connect(&EventBus::instance(), &EventBus::volumeChanged, this, [this, vlmLblIcon] (const float vlm) {
        if      (vlm == 0)  vlmLblIcon->setPixmap(QIcon(":/volume/volumeMute.svg").pixmap(20, 20));
        else if (vlm > 0.5) vlmLblIcon->setPixmap(QIcon(":/volume/volumeUp.svg")  .pixmap(20, 20));
        else if (vlm < 0.5) vlmLblIcon->setPixmap(QIcon(":/volume/volumeDown.svg").pixmap(20, 20));
    });
    
    pnToolslLayout->addWidget(vlmLblIcon);
    pnToolslLayout->addWidget(volumeSlider);
    
    // Playhead
    playhead = new CustomPlayhead(this);

    // Time (playhead)
    // Duration video
    QLabel *durLabel = new QLabel("00:00", this);
    connect(&EventBus::instance(), &EventBus::durationChanged, this, [this, durLabel] (qint64 dur) {
        durLabel->setText(QString("%1").arg(playhead->formatTime(dur)));
    });
    
    // Pos now
    QLabel *timeLabel = new QLabel("00:00", this);
    connect(&EventBus::instance(), &EventBus::positionChanged, this, [this, timeLabel] (qint64 ms) {
        timeLabel->setText(QString("%1").arg(playhead->formatTime(ms)));
    });
    
    // Hlayout (playhead)
    QHBoxLayout *hLayoutPlayhead = new QHBoxLayout();
    hLayoutPlayhead->addWidget(timeLabel);
    hLayoutPlayhead->addWidget(playhead);
    hLayoutPlayhead->addWidget(durLabel);

    // Bottom panel
    bottomPanel = new QVBoxLayout();
    bottomPanel->setContentsMargins(10, 15, 10, 15);
    bottomPanel->setSpacing(5);
    
    bottomPanel->addLayout(hLayoutPlayhead);
    bottomPanel->addLayout(pnToolslLayout);
    
    MainLayout->addLayout(bottomPanel);
    
    // Timer (bottom panel)
    panelTimer = new QTimer(this);
    panelTimer->setSingleShot(true);
    panelTimer->setInterval(500);
    connect(panelTimer, &QTimer::timeout, this, &MainWindow::hidePanel);

    // Animations
    anim = new Animation();

    centralWidget->setMouseTracking(true);
    centralWidget->installEventFilter(this);
}

void MainWindow::showPanel()
{
    if (isPanelVisible.load())  return;
    isPanelVisible.store(true);
    anim->fadeNestedLayout(bottomPanel, Fade::In, 200);
}
void MainWindow::hidePanel()
{
    if (!isPanelVisible.load())  return;
    isPanelVisible.store(false);
    anim->fadeNestedLayout(bottomPanel, Fade::Out, 200);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == centralWidget) {
        if (event->type() == QEvent::MouseMove ||
            event->type() == QEvent::Resize) {
            if (window()->isFullScreen() && isPanelVisible.load()) {  
                panelTimer->start();
                showPanel();
            } else {
                panelTimer->stop();
                showPanel();
            }
        }
    }

    // Else skip 
    return QMainWindow::eventFilter(obj, event); 
}

MainWindow::~MainWindow()
{
    if (player) {
        player->stop();
    }
    
    
    qDebug() << "MainWindow destroy";
}
