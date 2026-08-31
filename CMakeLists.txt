cmake_minimum_required(VERSION 3.22)

project(madqt VERSION 0.1.0 LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "madqt is Linux-only.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Corrosion QUIET)
if(NOT Corrosion_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        Corrosion
        GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
        GIT_TAG 1499b14e4906a2890f5cee1547c8848db261753d
    )
    FetchContent_MakeAvailable(Corrosion)
endif()

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets WebEngineWidgets)

if(QT_VERSION_MAJOR EQUAL 6 AND COMMAND qt_standard_project_setup)
    qt_standard_project_setup()
endif()

corrosion_import_crate(
    MANIFEST_PATH rust/Cargo.toml
    PROFILE release
    LOCKED
)

corrosion_add_cxxbridge(madqt_rs
        CRATE madqt_markdown_rs
        REGEN_TARGET madqt_rust_regen
        FILES lib.rs
)

add_executable(madqt
    src/AppearanceDialog.cpp
    src/AppearanceDialog.h
    src/LinkPolicyWebPage.cpp
    src/LinkPolicyWebPage.h
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/ManageThemesDialog.cpp
    src/ManageThemesDialog.h
    src/MarkdownAppearance.cpp
    src/MarkdownAppearance.h
    src/ResourceSchemeHandler.cpp
    src/ResourceSchemeHandler.h
    src/RustyMarkdown.cpp
    src/RustyMarkdown.h
    src/Theme.h
    src/ThemeManager.cpp
    src/ThemeManager.h
    src/TocPanel.cpp
    src/TocPanel.h
    resources/fonts.qrc
    resources/icons.qrc
    resources/scripts.qrc
)

target_include_directories(madqt PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(madqt PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets
    madqt_rs
)

target_link_libraries(madqt PRIVATE dl pthread m)

target_compile_features(madqt PRIVATE cxx_std_20)

target_compile_options(madqt PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

include(GNUInstallDirs)

install(TARGETS madqt
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES packaging/org.xeyes.madqt.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES packaging/org.xeyes.madqt.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)

# Refresh the desktop database (mimeinfo.cache, for the text/markdown
# association) and the icon cache on real installs; packaging tools set
# DESTDIR and run these themselves.
install(CODE [[
    if("$ENV{DESTDIR}" STREQUAL "")
        find_program(UPDATE_DESKTOP_DATABASE update-desktop-database)
        if(UPDATE_DESKTOP_DATABASE)
            execute_process(COMMAND "${UPDATE_DESKTOP_DATABASE}" "${CMAKE_INSTALL_PREFIX}/share/applications")
        endif()
        find_program(GTK_UPDATE_ICON_CACHE gtk-update-icon-cache)
        if(GTK_UPDATE_ICON_CACHE)
            execute_process(COMMAND "${GTK_UPDATE_ICON_CACHE}" -q -t "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor" ERROR_QUIET)
        endif()
    endif()
]])

add_subdirectory(flatpak)
