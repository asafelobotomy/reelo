cmake_minimum_required(VERSION 3.21)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" REELO_VERSION LIMIT_COUNT 1)
project(reelo VERSION ${REELO_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(REELO_TESTS "Build the core tests" ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick QuickControls2 DBus OpenGL)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv>=2.0)

qt_standard_project_setup(REQUIRES 6.5)

# Everything that needs no window or libmpv: settings, playlist, library, theme tokens, default
# app. The CLI subcommands and the tests use only this.
add_library(reelo_core STATIC
    src/config.cpp src/config.h
    src/media.cpp src/media.h
    src/playlist.cpp src/playlist.h
    src/library.cpp src/library.h
    src/theme.h src/uqtheme.cpp src/uqtheme.h
    src/defaultapp.cpp src/defaultapp.h
    src/cli.cpp src/cli.h
)
target_include_directories(reelo_core PUBLIC src)
target_compile_definitions(reelo_core PUBLIC REELO_VERSION="${PROJECT_VERSION}")
target_link_libraries(reelo_core PUBLIC Qt6::Core)

qt_add_executable(reelo
    src/main.cpp
    src/mpvobject.cpp src/mpvobject.h
    src/mpris.cpp src/mpris.h
    src/inhibit.cpp src/inhibit.h
    src/app.cpp src/app.h
)

# Must be set before the module is defined, or qmldir lists Theme as a plain type.
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(reelo
    URI Reelo
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/Home.qml
        qml/Controls.qml
        qml/SeekBar.qml
        qml/IconButton.qml
        qml/PlaylistPanel.qml
        qml/TrackMenu.qml
        qml/AudioView.qml
        qml/Osd.qml
        qml/UrlDialog.qml
        qml/PillButton.qml
        qml/logic.js
    RESOURCES
        icons/play.svg icons/pause.svg icons/next.svg icons/previous.svg
        icons/volume.svg icons/volume-low.svg icons/muted.svg
        icons/fullscreen.svg icons/fullscreen-exit.svg icons/playlist.svg
        icons/subtitles.svg icons/audio.svg icons/open.svg icons/link.svg
        icons/close.svg icons/music.svg icons/video.svg icons/reelo.svg
)

target_link_libraries(reelo PRIVATE reelo_core Qt6::Gui Qt6::Quick Qt6::QuickControls2
    Qt6::DBus Qt6::OpenGL PkgConfig::MPV)

include(GNUInstallDirs)
install(TARGETS reelo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/desktop/io.github.asafelobotomy.reelo.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/desktop/io.github.asafelobotomy.reelo.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES icons/reelo.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    RENAME io.github.asafelobotomy.reelo.svg)
install(DIRECTORY uq/pages/reelo DESTINATION ${CMAKE_INSTALL_DATADIR}/uq/settings/pages)

if(REELO_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(tst_core tests/core/tst_core.cpp)
    target_link_libraries(tst_core PRIVATE reelo_core Qt6::Test)
    add_test(NAME core COMMAND tst_core)
endif()
