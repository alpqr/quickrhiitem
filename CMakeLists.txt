cmake_minimum_required(VERSION 3.14)
project(testapp LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Qml)
find_package(Qt6 COMPONENTS Quick)

qt_add_executable(testapp
    main.cpp
    rhiitem.cpp rhiitem.h rhiitem_p.h
    customrhiitem.cpp customrhiitem.h
)
target_link_libraries(testapp PUBLIC
    Qt::Core
    Qt::Gui
    Qt::GuiPrivate
    Qt::Qml
    Qt::Quick
    Qt::QuickPrivate
)

qt_add_qml_module(testapp
    URI TestApp
    VERSION 1.0
    QML_FILES main.qml
    NO_RESOURCE_TARGET_PATH
)
