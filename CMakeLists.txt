cmake_minimum_required(VERSION 3.16)

project(simple_viewer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets OpenGLWidgets)
find_package(Qt5 QUIET COMPONENTS Gui OpenGL)

set(SIMPLE_VIEWER_SOURCES
    src/app/main.cpp
    src/app/MainWindow.cpp
    src/render/GLViewport.cpp
    src/render/Shader.cpp
    src/render/Mesh.cpp
    src/render/PointCloud.cpp
    src/io/ModelLoader.cpp
    src/io/PLYLoader.cpp
    src/io/STLLoader.cpp
    src/camera/OrbitCamera.cpp
    src/camera/FlyCamera.cpp
)

set(SIMPLE_VIEWER_HEADERS
    src/app/MainWindow.h
    src/render/GLViewport.h
    src/render/Shader.h
    src/render/Mesh.h
    src/render/PointCloud.h
    src/io/ModelLoader.h
    src/io/PLYLoader.h
    src/io/STLLoader.h
    src/camera/OrbitCamera.h
    src/camera/FlyCamera.h
)

add_executable(simple_viewer
    ${SIMPLE_VIEWER_SOURCES}
    ${SIMPLE_VIEWER_HEADERS}
)

target_include_directories(simple_viewer PRIVATE src)

target_link_libraries(simple_viewer PRIVATE
    Qt5::Widgets
    Qt5::OpenGLWidgets
)

if(TARGET Qt5::Gui)
    target_link_libraries(simple_viewer PRIVATE Qt5::Gui)
endif()

if(TARGET Qt5::OpenGL)
    target_link_libraries(simple_viewer PRIVATE Qt5::OpenGL)
endif()
