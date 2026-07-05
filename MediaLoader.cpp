#include "MediaLoader.h"

#include <QDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QTabWidget>
#include <QLabel>
#include <QWidget>

MediaLoader::MediaLoader(QWidget *parent)
    : QDialog(parent)
{
    m_loaderLayout = new QVBoxLayout(this);
    m_tabWidget = new QTabWidget(this);

    m_fileTab = new QWidget();
    m_networkTab = new QWidget();

    m_tabWidget->addTab(m_fileTab, "File");
    m_tabWidget->addTab(m_networkTab, "Network");

    m_addButton = new QPushButton("Add file", this);
    m_removeButton = new QPushButton("Remove file", this);
    m_playButton = new QPushButton("Play", this);
    m_closeButton = new QPushButton("Close", this);

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
    connect(m_playButton, &QPushButton::clicked, this, [this] () {  
        QListWidgetItem *item = m_listWidget->item(0);

        if(item)  emit startPlayRequested(item->text());  
    });

    resize(560, 400);
}

void MediaLoader::onAddButtonClicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "File",
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
        "Video Files (*.mp4 *.avi *.mkv);;All Files (*)"
    );

    if(!filePath.isEmpty())
        m_listWidget->addItem(filePath);
}