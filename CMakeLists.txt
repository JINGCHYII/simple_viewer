cmake_minimum_required(VERSION 3.21)
project(simple_viewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)

add_executable(simple_viewer
    src/main.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/render/GLViewport.h
    src/render/GLViewport.cpp
    src/scene/SceneObject.h
    src/scene/SceneObject.cpp
    src/scene/SceneManager.h
    src/scene/SceneManager.cpp
    src/io/ModelImporter.h
    src/io/ModelImporter.cpp
)

target_link_libraries(simple_viewer PRIVATE Qt6::Widgets Qt6::OpenGLWidgets)
