cmake_minimum_required(VERSION 3.16)
project(rnux)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Core Gui Widgets Network REQUIRED)
find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(timelib REQUIRED)
pkg_check_modules(XCB REQUIRED xcb xcb-keysyms)

set(SOURCES
        src/main.cpp
        src/mainwindow.cpp
        src/windowui.cpp
        src/globalhotkey.cpp
        src/features/app_launcher.cpp
        src/features/calculator.cpp
        src/features/system_commands.cpp
        src/features/search.cpp
        src/features/time_conversion.cpp
        resources.qrc
)

set(HEADERS
        src/mainwindow.h
        src/windowui.h
        src/application.h
        src/globalhotkey.h
        src/features/feature_base.h
        src/features/app_launcher.h
        src/features/calculator.h
        src/features/system_commands.h
        src/features/search.h
        src/features/time_conversion.h
        src/third_party/exprtk.hpp
)

add_executable(rnux ${SOURCES} ${HEADERS})
target_include_directories(rnux PRIVATE src)
target_link_libraries(rnux
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        timelib::timelib
        ${X11_LIBRARIES}
        ${XCB_LIBRARIES}
)

target_compile_definitions(rnux PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x060000)
target_compile_options(rnux PRIVATE -Wall -Wextra -Wpedantic)
