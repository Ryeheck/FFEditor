cmake_minimum_required(VERSION 3.19)
project(FFEditor LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS 
    Core 
    Widgets
    MultimediaWidgets
    Multimedia
)

qt_standard_project_setup()

qt_add_executable(FFEditor
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.h
    mainwindow.cpp
    MediaPlayer.h
    MediaPlayer.cpp
    MediaLoader.h
    MediaLoader.cpp
)

target_link_libraries(FFEditor
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Multimedia
        Qt::MultimediaWidgets
)

include(GNUInstallDirs)

install(TARGETS FFEditor
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET FFEditor
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})