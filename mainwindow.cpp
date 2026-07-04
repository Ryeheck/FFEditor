#include "mainwindow.h"
#include "MediaPlayer.h"
#include "MediaLoader.h"

#include <QVBoxLayout>
#include <QDropEvent>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QUrl>
#include <QToolButton>
#include <QDialog>
#include <QDir>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    centralWidget = new QWidget(this);
    VLayout = new QVBoxLayout(centralWidget);
    setCentralWidget(centralWidget);

    player = new MediaPlayer();
    player->getVideoWidget()->installEventFilter(this);
    player->getVideoWidget()->setAcceptDrops(true);

    VLayout->addWidget(player->getVideoWidget());

    loader = new MediaLoader(this);
    connect(loader, &MediaLoader::startPlayRequested, player, &MediaPlayer::loadVideo);

    QToolButton *startBtn = new QToolButton(this);
    startBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    startBtn->setIcon(QIcon(":/icons/iconStart2.png"));
    startBtn->setIconSize(QSize(32, 32));
    startBtn->show();
    VLayout->addWidget(startBtn);
    
    connect(startBtn, &QToolButton::clicked, loader, &MediaLoader::show);

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
