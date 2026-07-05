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
qt_add_executable(FFEditor
    WIN32 MACOSX_BUNDLE
    ${RESOURCES}
    main.cpp
    mainwindow.h
    mainwindow.cpp
    MediaPlayer.h
    MediaPlayer.cpp
    MediaLoader.h
    MediaLoader.cpp
    painter/customVolume.h
    painter/customVolume.cpp
    painter/customPlayhead.hpp

)

find_package(PkgConfig REQUIRED)

pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(SWSCALE REQUIRED libswscale)

target_link_libraries(FFEditor
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Multimedia
        Qt::MultimediaWidgets
        ${AVCODEC_LIBRARIES} 
        ${AVFORMAT_LIBRARIES} 
        ${AVUTIL_LIBRARIES}
        ${SWSCALE_LIBRARIES}
)

include(GNUInstallDirs)

install(TARGETS FFEditor
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

target_include_directories(FFEditor PRIVATE ${AVCODEC_INCLUDE_DIRS})

qt_generate_deploy_app_script(
    TARGET FFEditor
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})