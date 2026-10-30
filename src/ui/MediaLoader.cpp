#include "MediaLoader.h"
#include "../EventBus.h"

#include <QDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QTabWidget>
#include <QWidget>
#include <qfileinfo.h>
#include <qlistwidget.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpushbutton.h>

MediaLoader::MediaLoader(QWidget *parent)
    : QDialog(parent)
{
    m_loaderLayout = new QVBoxLayout(this);
    m_tabWidget    = new QTabWidget(this);

    m_fileTab    = new QWidget();
    m_networkTab = new QWidget();

    m_tabWidget->addTab(m_fileTab, "File");
    m_tabWidget->addTab(m_networkTab, "Network");

    m_addButton    = new QPushButton("Add file", this);
    m_removeButton = new QPushButton("Remove file", this);
    m_playButton   = new QPushButton("Play", this);
    m_closeButton  = new QPushButton("Close", this);

    // Main vertical layout to tab 1 ("File")
    QVBoxLayout *mainFileLayout = new QVBoxLayout(m_fileTab);

    // Top horizontal layout to tab 1
    QHBoxLayout *top1Layout = new QHBoxLayout();
    
    m_listWidget = new QListWidget(this);
    top1Layout->addWidget(m_listWidget);

    // Top buttons right
    QVBoxLayout *top1BtnsLayout = new QVBoxLayout();
    
    top1BtnsLayout->addWidget(m_addButton);
    top1BtnsLayout->addWidget(m_removeButton);
    top1BtnsLayout->addStretch();

    top1Layout->addLayout(top1BtnsLayout);

    // Bottom buttons right
    QHBoxLayout *Bttm1BtnsLayout = new QHBoxLayout();
    Bttm1BtnsLayout->addStretch();
    Bttm1BtnsLayout->addWidget(m_playButton);
    Bttm1BtnsLayout->addWidget(m_closeButton);

    // Main layout
    mainFileLayout->addLayout(top1Layout);
    mainFileLayout->addLayout(Bttm1BtnsLayout);
    m_loaderLayout->addWidget(m_tabWidget);

    connect(m_closeButton, &QPushButton::clicked, this, &MediaLoader::hide);
    connect(m_addButton, &QPushButton::clicked, this, &MediaLoader::onAddButtonClicked);
    connect(m_removeButton, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *item = m_listWidget->currentItem();
        if (item)  delete item;
        
    });
    connect(m_playButton, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *item = m_listWidget->currentItem();
        
        playMedia(item);
    });

    resize(560, 400);
}

MediaLoader::~MediaLoader()
{
    if (m_addButton) {
        m_addButton->deleteLater();
        m_addButton = nullptr;
    }
    if (m_removeButton) {
        m_removeButton->deleteLater();
        m_removeButton = nullptr;
    }
    if (m_playButton) {
        m_playButton->deleteLater();
        m_playButton = nullptr;
    }
    if (m_closeButton) {
        m_closeButton->deleteLater();
        m_closeButton = nullptr;
    }
    if (m_loaderLayout) {
        m_loaderLayout->deleteLater();
        m_loaderLayout = nullptr;
    }
    if (m_fileTab) {
        m_fileTab->deleteLater();
        m_fileTab = nullptr;
    }
    if (m_networkTab) {
        m_networkTab->deleteLater();
        m_networkTab = nullptr;
    }
    if (m_tabWidget) {
        m_tabWidget->deleteLater();
        m_tabWidget = nullptr;
    }
    if (m_listWidget) {
        m_listWidget->deleteLater();
        m_listWidget = nullptr;
    }
    
}

void MediaLoader::onAddButtonClicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "File",
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
        "Video Files (*.mp4 *.avi *.mkv);;All Files (*)"
    );
    if (filePath.isEmpty())  return;

    QListWidgetItem *item = new QListWidgetItem(QFileInfo(filePath).fileName(), m_listWidget);
    item->setData(Qt::UserRole, filePath);

    m_listWidget->setCurrentItem(item);
}

void MediaLoader::playMedia(QListWidgetItem *item)
{
    if (!item)  return;

    QString path = item->data(Qt::UserRole).toString();
    emit EventBus::instance().startPlayRequested(path);
}