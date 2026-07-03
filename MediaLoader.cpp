#include "MediaLoader.h"

#include <QWidget>
#include <QFileDialog>
#include <QStandardPaths>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QTabWidget>
#include <QLabel>

MediaLoader::MediaLoader(QWidget *parent)
    : QWidget(parent)
{
    loaderLayout = new QVBoxLayout(this);
    tabWidget = new QTabWidget(this);

    tab1Content = new QWidget();
    tab2Content = new QWidget();

    tabWidget->addTab(tab1Content, "File");
    tabWidget->addTab(tab2Content, "Network");

    addButton = new QPushButton("Add file", this);
    removeButton = new QPushButton("Remove file", this);
    playButton = new QPushButton("Play", this);

    // Main vertical layout to tab 1 ("File")
    QVBoxLayout *mainTab1Layout = new QVBoxLayout(tab1Content);

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

    // Main layout
    mainTab1Layout->addLayout(top1Layout);
    mainTab1Layout->addWidget(playButton);
    
    connect(addButton, &QPushButton::clicked, this, &MediaLoader::onAddButtonClicked);
    connect(playButton, &QPushButton::clicked, this, [this] () {  
        QListWidgetItem *item = listWidget->currentItem();

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
        listWidget->addItem(filePath);
}