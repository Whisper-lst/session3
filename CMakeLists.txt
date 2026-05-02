cmake_minimum_required(VERSION 3.20)
project(VideoEditor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W3 /WX- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui OpenGL)
find_package(Vulkan REQUIRED)

set(FFMPEG_ROOT "" CACHE PATH "Path to FFmpeg installation")
if(FFMPEG_ROOT)
    set(CMAKE_PREFIX_PATH "${FFMPEG_ROOT};${CMAKE_PREFIX_PATH}")
    link_directories("${FFMPEG_ROOT}/lib")
endif()

find_path(AVCODEC_INCLUDE_DIR NAMES libavcodec/avcodec.h)
find_path(AVFORMAT_INCLUDE_DIR NAMES libavformat/avformat.h)
find_path(AVUTIL_INCLUDE_DIR NAMES libavutil/avutil.h)
find_path(SWSCALE_INCLUDE_DIR NAMES libswscale/swscale.h)
find_path(SWRESAMPLE_INCLUDE_DIR NAMES libswresample/swresample.h)

find_library(AVCODEC_LIBRARY NAMES avcodec)
find_library(AVFORMAT_LIBRARY NAMES avformat)
find_library(AVUTIL_LIBRARY NAMES avutil)
find_library(SWSCALE_LIBRARY NAMES swscale)
find_library(SWRESAMPLE_LIBRARY NAMES swresample)

add_subdirectory(src/core)
add_subdirectory(src/video)
add_subdirectory(src/timeline)
add_subdirectory(src/renderer)
add_subdirectory(src/effects)
add_subdirectory(src/ui)

add_executable(VideoEditor 
    src/main.cpp
    resources/resources.qrc
)

target_link_libraries(VideoEditor PRIVATE
    Core
    Video
    Timeline
    Renderer
    Effects
    UI
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::OpenGL
    Vulkan::Vulkan
    ${AVCODEC_LIBRARY}
    ${AVFORMAT_LIBRARY}
    ${AVUTIL_LIBRARY}
    ${SWSCALE_LIBRARY}
    ${SWRESAMPLE_LIBRARY}
)

target_include_directories(VideoEditor PRIVATE
    ${AVCODEC_INCLUDE_DIR}
    ${AVFORMAT_INCLUDE_DIR}
    ${AVUTIL_INCLUDE_DIR}
    ${SWSCALE_INCLUDE_DIR}
    ${SWRESAMPLE_INCLUDE_DIR}
)

if(WIN32)
    set_target_properties(VideoEditor PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

install(TARGETS VideoEditor
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
