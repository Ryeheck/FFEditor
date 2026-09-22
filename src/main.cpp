#include "ui/mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow windowMain;
    windowMain.resize(1280, 900);
    
    qApp->setStyleSheet(
    "QWidget { background-color: black; color: gray; }"      // All windows
    "QPushButton {"
    "   background-color: gray; "
    "   color: white; "
    "   border: none; "
    "   min-width: 100px; "
    "   max-width: 200px; "
    "   min-height: 25px; "
    "   padding: 5px; "
    "}"                                                     // All buttons
    "QPushButton:hover { background-color: darkgray; }"    // При наведении на buttonssss
    "QToolButton {"
    "   background-color: transparent; "
    "   border: 1px solid #555; "
    "}"
    "QSlider {"
    "   background-color: gray; "
    "   border: 1px solid #555; "
    "}"
    "QToolButton:hover { background-color: gray; }"
    "QProgressBar {"                                       
    "   border: 1px solid #0e0e0e; " 
    "   border-radius: 4px; "         
    "   background-color: #000000; "  
    "   text-align: center; "         
    "   color: white; "               
    "}"
    "QProgressBar::chunk {"
    "   background-color: #0e0e0e; "  
    "   border-radius: 3px; "         
    "}"
    );
    
    windowMain.show();

    return a.exec();
}