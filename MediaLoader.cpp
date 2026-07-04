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
    loaderLayout = new QVBoxLayout(this);
    tabWidget = new QTabWidget(this);

    fileTab = new QWidget();
    networkTab = new QWidget();

    tabWidget->addTab(fileTab, "File");
    tabWidget->addTab(networkTab, "Network");

    addButton = new QPushButton("Add file", this);
    removeButton = new QPushButton("Remove file", this);
    playButton = new QPushButton("Play", this);
    closeButton = new QPushButton("Close", this);

    // Main vertical layout to tab 1 ("File")
    QVBoxLayout *mainFileLayout = new QVBoxLayout(fileTab);

    // Top horizontal layout to tab 1
    QHBoxLayout *top1Layout = new QHBoxLayout();
    
    listWidget = new QListWidget(this);
    top1Layout->addWidget(listWidget);

    // Top buttons right
    QVBoxLayout *top1BtnsLayout = new QVBoxLayout();
    
    top1BtnsLayout->addWidget(addButton);
    top1BtnsLayout->addWidget(removeButton);
    top1BtnsLayout->addStretch();

    top1Layout->addLayout(top1BtnsLayout);

    // Bottom buttons right
    QHBoxLayout *Bttm1BtnsLayout = new QHBoxLayout();
    Bttm1BtnsLayout->addStretch();
    Bttm1BtnsLayout->addWidget(playButton);
    Bttm1BtnsLayout->addWidget(closeButton);

    // Main layout
    mainFileLayout->addLayout(top1Layout);
    mainFileLayout->addLayout(Bttm1BtnsLayout);
    loaderLayout->addWidget(tabWidget);

    connect(closeButton, &QPushButton::clicked, this, &MediaLoader::hide);
    connect(addButton, &QPushButton::clicked, this, &MediaLoader::onAddButtonClicked);
    connect(playButton, &QPushButton::clicked, this, [this] () {  
        QListWidgetItem *item = listWidget->item(0);

        if(item)  emit startPlayRequested(item->text());  
    });

    resize(400, 300);
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
        listWidget->addItem(filePath);
}