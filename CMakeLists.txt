cmake_minimum_required(VERSION 3.19)
project(FFEditor LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS 
    Core 
    Widgets
    MultimediaWidgets
    Multimedia
)

qt_standard_project_setup()

qt_add_resources(RESOURCES "resources.qrc")


find_package(PkgConfig REQUIRED)

pkg_check_modules(FFMPEG REQUIRED 
    libavcodec
    libavformat
    libavutil
    libswscale
)

qt_add_executable(FFEditor
    WIN32 MACOSX_BUNDLE
    ${RESOURCES}
    main.cpp
    mainwindow.h
    mainwindow.cpp
    MediaLoader.h
    MediaLoader.cpp
    painter/CustomVolume.h
    painter/CustomVolume.cpp
    painter/CustomPlayhead.hpp
    player/MediaPlayer.h
    player/MediaPlayer.cpp
    player/Decoder.h
    player/Decoder.cpp
    player/FrameQueue.hpp
    player/VideoWidget.h
    player/VideoWidget.cpp
)

include(GNUInstallDirs)

install(TARGETS FFEditor
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

target_include_directories(FFEditor PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(FFEditor
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
        Qt6::MultimediaWidgets
        ${FFMPEG_LIBRARIES}
)

qt_generate_deploy_app_script(
    TARGET FFEditor
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})