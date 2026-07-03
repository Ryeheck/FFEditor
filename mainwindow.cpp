#include "mainwindow.h"
#include "MediaPlayer.h"
#include "MediaLoader.h"

#include <QVBoxLayout>
#include <QDropEvent>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    player = new MediaPlayer();
    player->getVideoWidget()->installEventFilter(this);
    player->getVideoWidget()->setAcceptDrops(true);
    
    loader = new MediaLoader();

    VLayout = new QVBoxLayout(centralWidget);
    VLayout->addWidget(player->getVideoWidget());
    VLayout->addWidget(loader);

    connect(loader, &MediaLoader::startPlayRequested, player, &MediaPlayer::loadVideo);
    // player->loadVideo(QUrl::fromLocalFile("/home/ryabi/Видео/output.mp4"));
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == player->getVideoWidget()) {
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
            player->loadVideo(urls.first().toLocalFile());
            return true;
        }
    }

    // Else skip 
    return QMainWindow::eventFilter(obj, event); 
}

MainWindow::~MainWindow()
{
    qDebug() << "ok";
}
