cmake_minimum_required(VERSION 3.14)

set(CMAKE_BUILD_TYPE Debug)
project(image-viewer)

find_package(OPENGL REQUIRED)

add_subdirectory("thirdparty/fmt")

add_executable(image-viewer
    imgui/imgui.cpp
    imgui/imgui.h
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_impl_opengl3.cpp
    imgui/imgui_impl_opengl3.h
    imgui/imgui_impl_win32.cpp
    imgui/imgui_impl_win32.h
    imgui/imgui_internal.h
    imgui/imgui_widgets.cpp
    src/application.cpp
    src/glad.c
    src/image-viewer.rc
    src/main.cpp
    src/stb_image.cpp
    )

target_include_directories(image-viewer
    PUBLIC include
    PUBLIC imgui
    PUBLIC glm)

target_compile_features(image-viewer
    PRIVATE cxx_std_17
    )

target_link_libraries(image-viewer
    PRIVATE
        ${OPENGL_LIBRARIES}
        fmt::fmt)

target_compile_options(image-viewer
    PUBLIC 
        -DIMGUI_IMPL_WIN32_DISABLE_GAMEPAD)
#        -DUNICODE
#        -D_UNICODE

if(MINGW)
    target_link_options(image-viewer
        PUBLIC -static)
# -mwindows
# -municode
endif()
