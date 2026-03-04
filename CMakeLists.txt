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
find_package(assimp REQUIRED)


set(SIMPLE_VIEWER_SOURCES
    src/app/main.cpp
    src/app/MainWindow.cpp
    src/render/GLViewport.cpp
    src/render/Shader.cpp
    src/render/Mesh.cpp
    src/render/PointCloud.cpp
    src/io/ModelLoader.cpp
    src/io/AssimpLoader.cpp
    src/camera/Camera.cpp
    src/camera/OrbitCamera.cpp
    src/camera/FlyCamera.cpp
    src/render/shaders.qrc
)

set(SIMPLE_VIEWER_HEADERS
    src/app/MainWindow.h
    src/render/GLViewport.h
    src/render/Shader.h
    src/render/Mesh.h
    src/render/PointCloud.h
    src/io/ModelLoader.h
    src/io/AssimpLoader.h
    src/io/ModelData.h
    src/camera/Camera.h
    src/camera/OrbitCamera.h
    src/camera/FlyCamera.h
)

set(IMGUIZMO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ImGuizmo")
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui")
set(SIMPLE_VIEWER_HAS_IMGUIZMO OFF)
if (EXISTS "${IMGUIZMO_DIR}/ImGuizmo.cpp" AND EXISTS "${IMGUIZMO_DIR}/ImGuizmo.h"
    AND EXISTS "${IMGUI_DIR}/imgui.cpp" AND EXISTS "${IMGUI_DIR}/imgui.h"
    AND EXISTS "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp")
    set(SIMPLE_VIEWER_HAS_IMGUIZMO ON)
    list(APPEND SIMPLE_VIEWER_SOURCES
        ${IMGUIZMO_DIR}/ImGuizmo.cpp
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )
endif()

add_executable(simple_viewer
    ${SIMPLE_VIEWER_SOURCES}
    ${SIMPLE_VIEWER_HEADERS}
)

target_include_directories(simple_viewer PRIVATE src)
if (SIMPLE_VIEWER_HAS_IMGUIZMO)
    target_include_directories(simple_viewer PRIVATE
        ${IMGUIZMO_DIR}
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
    )
    target_compile_definitions(simple_viewer PRIVATE SIMPLE_VIEWER_HAS_IMGUIZMO=1)
endif()


target_compile_definitions(simple_viewer PRIVATE
    $<$<PLATFORM_ID:Windows>:NOMINMAX>
)

target_link_libraries(simple_viewer PRIVATE
    Qt5::Widgets
    Qt5::OpenGLWidgets
)

if(TARGET assimp::assimp)
    target_link_libraries(simple_viewer PRIVATE assimp::assimp)
elseif(TARGET assimp)
    target_link_libraries(simple_viewer PRIVATE assimp)
elseif(TARGET Assimp::assimp)
    target_link_libraries(simple_viewer PRIVATE Assimp::assimp)
else()
    message(FATAL_ERROR "Assimp target not found. Expected assimp::assimp, assimp, or Assimp::assimp")
endif()


if(TARGET Qt5::Gui)
    target_link_libraries(simple_viewer PRIVATE Qt5::Gui)
endif()

if(TARGET Qt5::OpenGL)
    target_link_libraries(simple_viewer PRIVATE Qt5::OpenGL)
endif()
