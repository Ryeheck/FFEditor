cmake_minimum_required(VERSION 3.19)
project(FFEditor LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS 
    Core 
    Widgets
    OpenGL
    OpenGLWidgets
)

qt_standard_project_setup()

qt_add_resources(RESOURCES "assets/resources.qrc")


find_package(PkgConfig REQUIRED)

pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
    libswresample
)

include(FetchContent)

FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY "https://github.com/mackron/miniaudio.git"
    GIT_TAG        "master"
)
FetchContent_MakeAvailable(miniaudio)

qt_add_executable(FFEditor
    WIN32 MACOSX_BUNDLE
    ${RESOURCES}
    src/main.cpp
    src/EventBus.h
    src/ui/mainwindow.h
    src/ui/mainwindow.cpp
    src/ui/MediaLoader.h
    src/ui/MediaLoader.cpp
    src/painter/CustomVolume.h
    src/painter/CustomVolume.cpp
    src/painter/CustomPlayhead.hpp
    src/player/MediaPlayer.h
    src/player/MediaPlayer.cpp
    src/player/Decoder.h
    src/player/Decoder.cpp
    src/player/FrameQueue.hpp
    src/player/VideoWidget.h
    src/player/VideoWidget.cpp
    src/player/AudioOutput.cpp
    src/player/AudioOutput.h    
)

include(GNUInstallDirs)

install(TARGETS FFEditor
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

target_include_directories(FFEditor PRIVATE 
    ${miniaudio_SOURCE_DIR}
)

if(WIN32 OR APPLE)
    qt_generate_deploy_app_script(
        TARGET FFEditor
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()

target_link_libraries(FFEditor
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        PkgConfig::FFMPEG
)