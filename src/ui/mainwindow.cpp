#include "mainwindow.h"
#include "MediaLoader.h"
#include "../painter/CustomVolume.h"
#include "../painter/CustomPlayhead.hpp"
#include "../player/MediaPlayer.h"
#include "../player/VideoWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QDropEvent>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QUrl>
#include <QToolButton>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    centralWidget = new QWidget(this);
    MainLayout = new QVBoxLayout(centralWidget);
    setCentralWidget(centralWidget);

    player = new MediaPlayer(this);
    
    videoWidget = new VideoWidget(this);
    connect(player, &MediaPlayer::frameChanged, videoWidget, &VideoWidget::setFrame);
    MainLayout->addWidget(videoWidget);

    loader = new MediaLoader(this);
    connect(loader, &MediaLoader::startPlayRequested, this, [this] (const QString &path) {
        if (!player->loadVideo(path)) {
            qDebug() << "mainwn: Couldn't load video";
            return;
        }
        player->play();
        loader->hide();
    });
    
    QToolButton *startBtn = new QToolButton(this);
    connect(startBtn, &QToolButton::clicked, this, [this] () {
        loader->show();
    });

    startBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    startBtn->setIcon(QIcon(":/icons/iconStart2.png"));
    startBtn->setIconSize(QSize(32, 32));
    startBtn->show();
    
    QToolButton *stopBtn = new QToolButton(this);
    connect(stopBtn, &QToolButton::clicked, player, &MediaPlayer::stop);
    
    stopBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    stopBtn->setIcon(QIcon(/* Icon */));
    stopBtn->setIconSize(QSize(32, 32));
    stopBtn->show();
    
    QToolButton *pauseBtn = new QToolButton(this);
    connect(pauseBtn, &QToolButton::clicked, player, &MediaPlayer::pause);

    pauseBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    pauseBtn->setIcon(QIcon(/* Icon */));
    pauseBtn->setIconSize(QSize(32, 32));
    pauseBtn->show();
    
    // Panel Tools (start, stop, ...) buttons
    QHBoxLayout *pnToolslLayout = new QHBoxLayout();
    pnToolslLayout->addWidget(startBtn);
    pnToolslLayout->addWidget(pauseBtn);
    pnToolslLayout->addWidget(stopBtn);
    pnToolslLayout->addStretch();
    
    // Volume songs
    volumeSlider = new CustomVolume(this);
    connect(volumeSlider, &CustomVolume::valueChanged, player, &MediaPlayer::setVolume);
    volumeSlider->setValue(10);
    pnToolslLayout->addWidget(volumeSlider);
    
    // Playhead
    playhead = new CustomPlayhead(this);
    connect(player, &MediaPlayer::positionChanged, playhead, &CustomPlayhead::onPositionChanged);
    connect(playhead, &CustomPlayhead::positionChanged, player, &MediaPlayer::onPositionChanged);
    connect(player, &MediaPlayer::durationChanged, playhead, &CustomPlayhead::setDuration);
    MainLayout->addWidget(playhead);

    MainLayout->addLayout(pnToolslLayout);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == videoWidget) {
        // If drag file
        if(event->type() == QEvent::DragEnter) { 
            qDebug() << "DragEnterEvent";

            static_cast<QDragEnterEvent *>(event)->acceptProposedAction();
            return true;
        } 
        // If drop file
        if(event->type() == QEvent::Drop) {
            qDebug() << "DropEvent";

            QDropEvent *dropEvent = static_cast<QDropEvent *>(event);
            const QList<QUrl> urls = dropEvent->mimeData()->urls();
            // player->loadVideo(urls.first());                                                 !!!!!!!!!!!!!!!!!!!!!!!
            return true;
        }
    }

    // Else skip 
    return QMainWindow::eventFilter(obj, event); 
}

MainWindow::~MainWindow()
{
    qDebug() << "MainWindow: ok";
}
