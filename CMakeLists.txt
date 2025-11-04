cmake_minimum_required(VERSION 3.16)

project(bool VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets OpenGLWidgets)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h

    inputpolygon.cpp
    inputpolygon.h

    canvas2d.cpp
    canvas2d.h

    geometrymodel.cpp
    geometrymodel.h
)

qt_add_executable(bool
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    booleanops.h
    booleanops.cpp
)

target_link_libraries(bool
    PRIVATE
        Qt6::Widgets
        Qt6::OpenGLWidgets
)

set_target_properties(bool PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS bool
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(bool)
